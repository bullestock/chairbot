// This code is stolen from https://github.com/FatherDivine/bt_audio-esp32 (license: Public Domain)

#include <algorithm>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_chip_info.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "i2s_audio.h"

/* MP3 decoder - minimp3 header-only library */
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD    /* Disable SIMD for ESP32 compatibility */
#define MINIMP3_ONLY_MP3   /* Strip MPEG Layer I/II code — reduces icache pressure on RISC-V */
#include "minimp3.h"

/* AAC decoder - libhelix-aac fixed-point HE-AAC decoder */
#include "aacdec.h"

/* Math functions for EQ coefficient computation (powf, sinf, cosf, sqrtf) */
#include <math.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <sys/stat.h>

/* ============================================================================
 * Configuration Section
 * ============================================================================ */

#ifdef CONFIG_STREAM_BUFFER_SIZE
#define STREAM_BUFFER_SIZE         CONFIG_STREAM_BUFFER_SIZE
#else
/* Without PSRAM, use a smaller stream buffer to leave more heap for
 * TLS/AES crypto operations during HTTPS streaming.  The HTTP client
 * reads in STREAM_BUFFER_SIZE chunks — 2KB is still efficient for network
 * I/O and saves 2KB of contiguous internal SRAM.
 * On S3/C6/ESP32 without PSRAM (512KB SRAM), 2KB saves heap for the AAC
 * decoder and mbedTLS dynamic buffers that otherwise cause OOM.
 * With PSRAM, 4KB gives better throughput since heap is plentiful. */
#if !CONFIG_SPIRAM
#define STREAM_BUFFER_SIZE         2048
#else
#define STREAM_BUFFER_SIZE         4096
#endif
#endif

#define I2S_BCLK_GPIO              GPIO_NUM_19
#define I2S_WS_GPIO                GPIO_NUM_27
#define I2S_DOUT_GPIO              GPIO_NUM_25

#define SD_MOSI_GPIO               GPIO_NUM_16
#define SD_MISO_GPIO               GPIO_NUM_17
#define SD_CLK_GPIO                GPIO_NUM_32
#define SD_CS_GPIO                 GPIO_NUM_4

#define AUDIO_SAMPLE_RATE          44100

/* ADTS header size — minimum bytes for a valid ADTS frame header */
#define ADTS_HEADER_SIZE           7

/* ---- Dual-buffer architecture (inspired by vladkorotnev/cd-player) ----
 *
 * The audio pipeline uses TWO ring buffers to decouple network jitter
 * from real-time audio playback:
 *
 *   HTTP stream ──► [Compressed buffer (large, PSRAM)] ──► Decode task (MP3 or AAC)
 *                                                        │
 *                                                        ▼
 *   I2S playback task ◄── [PCM buffer (small, internal SRAM)]
 *
 * The compressed buffer stores MP3/AAC data (~6-16 KB/s at 48-128kbps)
 * in PSRAM, giving seconds of network buffering at 256KB.  The PCM buffer
 * stores decoded samples (~176 KB/s at 44100Hz stereo 16-bit) in fast
 * internal SRAM for DMA-friendly I2S output.
 *
 * The decode task runs on a separate core and bridges the two buffers,
 * consuming compressed data at exactly the rate the I2S hardware clock
 * demands.  This naturally matches playback speed to the sample rate
 * without any flow-control hacks.
 */

/* MP3 compressed data ring buffer — large, in PSRAM if available.
 * At 128kbps (16 KB/s), 256KB = ~16 seconds of buffered stream data.
 * This absorbs network jitter and brief WiFi interruptions. */
#define MP3_RINGBUF_SIZE           (256 * 1024)  /* 256KB for compressed MP3 (needs PSRAM) */
#define MP3_RINGBUF_FALLBACK_SIZE  (24 * 1024)   /* 24KB fallback (~1.5s of 128kbps stream) */
#define MP3_RINGBUF_MIN_SIZE       (8 * 1024)    /* 8KB minimum (~0.5s of 128kbps stream) */

/* Minimum free heap to preserve after ring buffer allocation.
 * ESP-IDF 6.0 HTTP client + TLS (mbedTLS) + lwip + esp_vfs_select()
 * need significant contiguous heap for internal allocations.
 *
 * Three tiers based on available SRAM:
 *   Tier 1 — C5 without PSRAM (384KB SRAM, WiFi 6 dual-band uses ~200KB):
 *            70KB headroom, most aggressive.
 *   Tier 2 — Any other chip without PSRAM (S3/C6/ESP32, 512-520KB SRAM):
 *            65KB headroom, preserves heap for TLS + AAC SBR (~50KB).
 *   Tier 3 — Any chip with PSRAM: 60KB headroom, comfortable. */
#if CONFIG_IDF_TARGET_ESP32C5 && !CONFIG_SPIRAM
#define HEAP_HEADROOM_BYTES        (70 * 1024)
#elif !CONFIG_SPIRAM
#define HEAP_HEADROOM_BYTES        (65 * 1024)
#else
#define HEAP_HEADROOM_BYTES        (60 * 1024)
#endif

/* Ring buffer size tiers — tried in order until one fits with headroom */
static const size_t s_mp3_buf_sizes[] = {
    MP3_RINGBUF_SIZE, MP3_RINGBUF_FALLBACK_SIZE, MP3_RINGBUF_MIN_SIZE
};
#define MP3_BUF_SIZES_COUNT  (sizeof(s_mp3_buf_sizes) / sizeof(s_mp3_buf_sizes[0]))

/* PCM decoded audio ring buffer — must be in internal SRAM.
 * At 44100Hz stereo 16-bit (~176 KB/s):
 *   32KB = ~186ms of decoded audio (with PSRAM — heap is plentiful)
 *   16KB = ~93ms  (without PSRAM — S3/C6/ESP32, saves 16KB for TLS+AAC)
 *    8KB = ~46ms  (C5 without PSRAM — saves 24KB for WiFi 6 + TLS)
 *
 * Without PSRAM, 16KB is the sweet spot: enough for smooth playback while
 * leaving heap for TLS (~40KB) and AAC SBR (~50KB).  On C5 without PSRAM
 * (only 384KB SRAM), 8KB is needed to survive WiFi 6's higher memory use. */
#if CONFIG_IDF_TARGET_ESP32C5 && !CONFIG_SPIRAM
#define AUDIO_RINGBUF_SIZE         (8 * 1024)    /* 8KB — C5 no-PSRAM, save for WiFi 6 + TLS */
#define AUDIO_RINGBUF_FALLBACK_SIZE (4 * 1024)   /* 4KB — ~23ms, bare minimum */
#elif !CONFIG_SPIRAM
#define AUDIO_RINGBUF_SIZE         (16 * 1024)   /* 16KB — no-PSRAM (S3/C6/ESP32) */
#define AUDIO_RINGBUF_FALLBACK_SIZE (8 * 1024)   /* 8KB fallback */
#else
#define AUDIO_RINGBUF_SIZE         (32 * 1024)   /* 32KB — with PSRAM */
#define AUDIO_RINGBUF_FALLBACK_SIZE (16 * 1024)  /* 16KB fallback */
#endif

/* Pre-buffering: wait for MP3 buffer to fill before starting decode+playback.
 * 50% of 256KB = 128KB = ~8 seconds of compressed stream data. */
#define PREBUFFER_TIMEOUT_MS       15000  /* Max time to wait for MP3 buffer to fill */
#define PREBUFFER_THRESHOLD_PCT    50     /* Start decode when MP3 buffer is this % full */

/* PCM pre-buffer threshold — wait for PCM ring buffer to reach this level
 * before starting I2S playback.  Gives the decoder a head start. */
#define PCM_PREBUF_THRESHOLD_PCT   50     /* Start playback when PCM buffer reaches this % */

/* I2S write timeout */
#define I2S_WRITE_TIMEOUT_MS       500

/* I2S DMA buffer configuration.
 * Total DMA memory = dma_desc_num × dma_frame_num × 4 bytes (16-bit stereo).
 *
 * Three tiers:
 *   C5 no-PSRAM:     6 × 240 = ~5.6KB  (minimum for WiFi 6 + TLS headroom)
 *   Other no-PSRAM:  8 × 240 = ~7.5KB  (saves ~22KB vs full, smooth playback)
 *   With PSRAM:     16 × 480 = ~30KB   (generous DMA, heap is plentiful)
 *
 * Without PSRAM, 30KB for DMA is the single biggest heap consumer and the
 * primary cause of OOM in TLS/AAC on S3/C6 boards without PSRAM. */
#if CONFIG_IDF_TARGET_ESP32C5 && !CONFIG_SPIRAM
#define I2S_DMA_DESC_NUM           6
#define I2S_DMA_FRAME_NUM          240    /* ~5.6KB total DMA memory */
#elif !CONFIG_SPIRAM
#define I2S_DMA_DESC_NUM           8
#define I2S_DMA_FRAME_NUM          240    /* ~7.5KB total DMA memory */
#else
#define I2S_DMA_DESC_NUM           16
#define I2S_DMA_FRAME_NUM          480    /* ~30KB total DMA memory */
#endif

/* Playback chunk size — how many bytes to read from the PCM ring buffer
 * per iteration.  Larger chunks reduce ring buffer read overhead. */
#define PLAYBACK_CHUNK_SIZE        4096

/* Audio playback task stack size.
 * The playback task reads from the PCM ring buffer (zero-copy pointer from
 * xRingbufferReceiveUpTo) and calls i2s_channel_write — no large stack
 * allocations.  Local variables total ~40 bytes; i2s_channel_write passes
 * a pointer to the DMA engine (no copy); ESP_LOGx formatting uses ~200B.
 * Measured worst-case high-water mark is ~1.5KB, so 3KB provides ~100%
 * headroom while saving 1KB on memory-constrained chips without PSRAM.
 * With PSRAM, use 4KB for extra safety margin. */
#if !CONFIG_SPIRAM
#define PLAYBACK_TASK_STACK_SIZE   3072
#else
#define PLAYBACK_TASK_STACK_SIZE   4096
#endif

/* Decode task yield interval — yield every N frames to let IDLE feed the WDT.
 *
 * Single-core (C5/C6): yield after EVERY frame with a 2ms delay.
 *   At priority 5 (below playback=7 and stream=6), the decode task only
 *   runs when I/O tasks are blocked.  The explicit yield ensures IDLE and
 *   the menu task (priority 4) still get CPU time for WDT and UI.
 *
 * Dual-core (ESP32/S3): yield every 4 frames with 1ms delay.
 *   Decode runs on its own core at priority 8.  Infrequent yields are fine
 *   because IDLE runs on the other core. */
#define DECODE_YIELD_INTERVAL      4
#define DECODE_YIELD_TICKS         1  /* 1 tick = 1ms at 1000Hz, 10ms at 100Hz */

/* Decode task priority — on single-core chips, run BELOW playback and stream
 * so that I/O-bound tasks (I2S DMA refill, HTTP reads) are serviced promptly.
 * The decode task runs whenever those tasks block on I/O, which is most of
 * the time.  On dual-core chips, decode is highest to maximize throughput. */
#define DECODE_TASK_PRIORITY       8

/* Decode task stack size.
 * C5 I2S recreate workaround (audio_reconfigure_sample_rate) puts
 * i2s_std_config_t + i2s_chan_config_t on the stack during first-frame.
 * With PSRAM: 16KB (stack lands in PSRAM via xTaskCreateWithCaps).
 * Without PSRAM: 12KB — tight but sufficient; the I2S config structs
 * add ~1.5KB, and the base AAC decode chain uses ~8.2KB measured.
 * Other targets: 12KB (no I2S recreate workaround). */
#if CONFIG_IDF_TARGET_ESP32C5 && CONFIG_SPIRAM
#define DECODE_TASK_STACK_SIZE     (16 * 1024)
#else
#define DECODE_TASK_STACK_SIZE     (12 * 1024)
#endif

/* Decode task self-delete macro.
 * When PSRAM is available, decode tasks are created with xTaskCreateWithCaps()
 * to allow task stacks to be placed in PSRAM (internal SRAM is too fragmented).
 * Tasks created with xTaskCreateWithCaps MUST be deleted with vTaskDeleteWithCaps
 * to properly free the caps-allocated stack memory. */
#if CONFIG_SPIRAM
#define DECODE_TASK_SELF_DELETE()  vTaskDeleteWithCaps(NULL)
#else
#define DECODE_TASK_SELF_DELETE()  vTaskDelete(NULL)
#endif

/* Other streaming constants */
#define STREAM_TIMEOUT_SEC         30
#define STREAM_REPORT_INTERVAL_MS  10000  /* 10 seconds between stream stats */
#define HTTP_CONNECT_MAX_RETRIES   3      /* Retry TLS/HTTP connect on transient failures */
#define HTTP_CONNECT_RETRY_DELAY_MS 2000  /* Delay between connection retries */
#define HTTP_MAX_REDIRECTS         5      /* Max 301/302/307/308 hops to follow */

#define SDCARD_ROOT_PATH "/sdcard/music"

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static const char* TAG = "audio";

/* Audio output */
static i2s_chan_handle_t s_i2s_tx_chan = NULL;
static RingbufHandle_t s_audio_ringbuf = NULL;       /* PCM decoded data (internal SRAM) */
static size_t s_audio_ringbuf_size = 0;              /* Actual allocated size */
/* PCM ring buffer must be entirely in internal SRAM.  When PSRAM is on
 * the heap, plain xRingbufferCreate() may place the control struct
 * (containing the portMUX spinlock) in PSRAM, causing corruption on
 * Xtensa.  We use xRingbufferCreateStatic() with explicit internal alloc. */
static uint8_t *s_pcm_ringbuf_storage = NULL;
static StaticRingbuffer_t *s_pcm_ringbuf_struct = NULL;

static RingbufHandle_t s_mp3_ringbuf = NULL;         /* Compressed MP3 data (PSRAM preferred) */
static size_t s_mp3_ringbuf_size = 0;                /* Actual allocated size */
/* When MP3 ring buffer is created via xRingbufferCreateStatic (PSRAM data +
 * internal SRAM control struct), we must track the separately-allocated
 * pointers for cleanup.  Set to non-NULL only for the static-creation path. */
static uint8_t *s_mp3_ringbuf_storage = NULL;        /* PSRAM data buffer (or NULL) */
static StaticRingbuffer_t *s_mp3_ringbuf_struct = NULL; /* Internal SRAM control (or NULL) */
static bool s_audio_output_enabled = false;
static bool s_audio_playing = false;
static uint32_t s_current_sample_rate = AUDIO_SAMPLE_RATE;

/* Set to true while stream_task (and its child decode/audio tasks) is running.
 * Cleared by stream_task after all child tasks have exited and resources are
 * freed.  start_streaming() polls this to avoid starting a new stream before
 * the previous one has fully released its heap allocations — critical on
 * memory-constrained chips (C6, 512KB SRAM, no PSRAM). */
static volatile bool s_stream_task_running = false;

/* Stream codec type - auto-detected from HTTP Content-Type or URL */
typedef enum {
    CODEC_UNKNOWN,
    CODEC_MP3,
    CODEC_AAC,
    CODEC_PCM
} stream_codec_t;

/* MP3 decoder state */
static mp3dec_t s_mp3_decoder;
static bool s_mp3_decoder_initialized = false;
static bool s_mp3_first_decode_logged = false;  /* For one-time MP3 info logging */

/* AAC decoder state */
static HAACDecoder s_aac_decoder = NULL;
static bool s_aac_decoder_initialized = false;
static bool s_aac_first_decode_logged = false;  /* For one-time AAC info logging */

/* ============================================================================
 * SD Card Playback State
 * ============================================================================ */

static volatile bool s_sd_playback_active = false;
static volatile bool s_sd_task_running = false;
/* Per-track "feeding" flag: true while the SD task is reading a file into the
 * compressed ring buffer.  Cleared when the file ends (or playback is stopped)
 * so that decode/playback child tasks see AUDIO_SOURCE_ACTIVE go false and
 * can drain remaining data then exit — preventing orphan tasks between tracks. */
static volatile bool s_sd_feeding_active = false;
static int s_sd_track_index = 0;
#define SD_MAX_TRACKS     64
#define SD_FILENAME_MAX   128

static std::vector<std::string> sd_tracks;

/* ============================================================================
 * 3-Band Software Equalizer
 *
 * Proper biquad IIR filters using Audio EQ Cookbook formulas:
 *   - Bass:   Low-shelf filter  (center ~300 Hz)
 *   - Mid:    Peaking/bell filter (center ~1 kHz)
 *   - Treble: High-shelf filter (center ~4 kHz)
 *
 * Each band only affects its frequency range — adjusting bass does NOT
 * muffle the highs, and vice versa.  Uses Q28 fixed-point coefficients
 * with 64-bit intermediate math for precision without floating-point
 * in the audio processing loop.
 *
 * Gain range: -12 dB to +12 dB in 1 dB steps.
 * Coefficients are computed using <math.h> (float) at init/change time,
 * then stored as Q28 integers for the real-time processing path.
 * ============================================================================ */

/* EQ gain in dB: -12 to +12, 0 = flat */
#define EQ_GAIN_MIN   (-12)
#define EQ_GAIN_MAX   (12)
#define EQ_GAIN_STEP  1
#define EQ_NUM_BANDS  3

/* Band indices */
#define EQ_BAND_BASS   0
#define EQ_BAND_MID    1
#define EQ_BAND_TREBLE 2

/* Fixed-point scale: Q28 gives us 4 bits of headroom above ±1.0 which is
 * enough for shelf/peak filter coefficients that can exceed 1.0. */
#define Q28_ONE  (1 << 28)

/* Filter center frequencies (Hz) and Q factors */
#define EQ_BASS_FREQ     300.0f
#define EQ_MID_FREQ     1000.0f
#define EQ_TREBLE_FREQ  4000.0f
#define EQ_BASS_Q        0.7f    /* Gentle shelf slope */
#define EQ_MID_Q         1.0f    /* Moderate peak width */
#define EQ_TREBLE_Q      0.7f    /* Gentle shelf slope */

/* EQ gain per band (dB) — 0 = flat */
static int s_eq_gains[EQ_NUM_BANDS] = {0, 0, 0};

/* EQ enabled flag */
static bool s_eq_enabled = false;

/* Software volume control (0–100%, default 100).
 * Applied as a linear 16-bit multiply in audio_write_to_buffer. */
#define VOLUME_MIN   0
#define VOLUME_MAX   100
#define VOLUME_STEP  5
static int s_volume = 100;

/* Biquad filter state per band, per channel (stereo).
 * Coefficients in Q28 fixed-point, state in Q28 for precision. */
typedef struct {
    int32_t b0, b1, b2;  /* Numerator coefficients (Q28) */
    int32_t a1, a2;       /* Denominator coefficients (Q28), a0 normalized to 1.0 */
    int64_t x1, x2;       /* Input history (full precision) */
    int64_t y1, y2;       /* Output history (full precision) */
} biquad_state_t;

#define EQ_MAX_CHANNELS 2
static biquad_state_t s_eq_filters[EQ_NUM_BANDS][EQ_MAX_CHANNELS];

/**
 * Compute proper biquad coefficients using Audio EQ Cookbook formulas.
 * Uses floating-point math (only called when gain changes, not in audio path).
 *
 * Bass:   Low-shelf  — boosts/cuts below center freq, passes highs untouched
 * Mid:    Peaking    — boosts/cuts around center freq, passes lows+highs
 * Treble: High-shelf — boosts/cuts above center freq, passes lows untouched
 *
 * Reference: Robert Bristow-Johnson "Audio EQ Cookbook"
 */
static void eq_compute_coefficients(int band, int gain_db)
{
    /* At 0 dB, set identity (pass-through) filter */
    if (gain_db == 0) {
        for (int ch = 0; ch < EQ_MAX_CHANNELS; ch++) {
            biquad_state_t *f = &s_eq_filters[band][ch];
            f->b0 = Q28_ONE;
            f->b1 = 0;
            f->b2 = 0;
            f->a1 = 0;
            f->a2 = 0;
        }
        return;
    }

    /* Get filter parameters for this band */
    float freq, Q;
    switch (band) {
        case EQ_BAND_BASS:   freq = EQ_BASS_FREQ;   Q = EQ_BASS_Q;   break;
        case EQ_BAND_MID:    freq = EQ_MID_FREQ;     Q = EQ_MID_Q;    break;
        case EQ_BAND_TREBLE: freq = EQ_TREBLE_FREQ;  Q = EQ_TREBLE_Q; break;
        default:             freq = EQ_MID_FREQ;      Q = EQ_MID_Q;    break;
    }

    /* Use actual sample rate (updated when stream starts playing).
     * s_current_sample_rate is initialized to CONFIG_AUDIO_SAMPLE_RATE (default 44100)
     * and updated by audio_reconfigure_sample_rate() when the decoder detects the
     * actual stream sample rate. */
    float Fs = (float)s_current_sample_rate;
    if (Fs < 8000.0f) Fs = 44100.0f;  /* Safety fallback */
    float A  = powf(10.0f, (float)gain_db / 40.0f);  /* sqrt(10^(dB/20)) for shelves */
    float w0 = 2.0f * (float)M_PI * freq / Fs;
    float cosw0 = cosf(w0);
    float sinw0 = sinf(w0);

    float fb0, fb1, fb2, fa0, fa1, fa2;

    switch (band) {
        case EQ_BAND_BASS: {
            /* Low-shelf filter */
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = sqrtf(A);
            float twoSqrtAalpha = 2.0f * sqrtA * alpha;
            fb0 =        A * ((A + 1.0f) - (A - 1.0f) * cosw0 + twoSqrtAalpha);
            fb1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
            fb2 =        A * ((A + 1.0f) - (A - 1.0f) * cosw0 - twoSqrtAalpha);
            fa0 =             (A + 1.0f) + (A - 1.0f) * cosw0 + twoSqrtAalpha;
            fa1 =    -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
            fa2 =             (A + 1.0f) + (A - 1.0f) * cosw0 - twoSqrtAalpha;
            break;
        }
        case EQ_BAND_MID: {
            /* Peaking EQ filter */
            float alpha = sinw0 / (2.0f * Q);
            fb0 =  1.0f + alpha * A;
            fb1 = -2.0f * cosw0;
            fb2 =  1.0f - alpha * A;
            fa0 =  1.0f + alpha / A;
            fa1 = -2.0f * cosw0;
            fa2 =  1.0f - alpha / A;
            break;
        }
        case EQ_BAND_TREBLE: {
            /* High-shelf filter */
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = sqrtf(A);
            float twoSqrtAalpha = 2.0f * sqrtA * alpha;
            fb0 =        A * ((A + 1.0f) + (A - 1.0f) * cosw0 + twoSqrtAalpha);
            fb1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
            fb2 =        A * ((A + 1.0f) + (A - 1.0f) * cosw0 - twoSqrtAalpha);
            fa0 =              (A + 1.0f) - (A - 1.0f) * cosw0 + twoSqrtAalpha;
            fa1 =       2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
            fa2 =              (A + 1.0f) - (A - 1.0f) * cosw0 - twoSqrtAalpha;
            break;
        }
        default:
            /* Identity */
            fb0 = 1.0f; fb1 = 0.0f; fb2 = 0.0f;
            fa0 = 1.0f; fa1 = 0.0f; fa2 = 0.0f;
            break;
    }

    /* Normalize by a0 and convert to Q28 fixed-point */
    float inv_a0 = 1.0f / fa0;
    int32_t ib0 = (int32_t)(fb0 * inv_a0 * (float)Q28_ONE);
    int32_t ib1 = (int32_t)(fb1 * inv_a0 * (float)Q28_ONE);
    int32_t ib2 = (int32_t)(fb2 * inv_a0 * (float)Q28_ONE);
    int32_t ia1 = (int32_t)(fa1 * inv_a0 * (float)Q28_ONE);
    int32_t ia2 = (int32_t)(fa2 * inv_a0 * (float)Q28_ONE);

    for (int ch = 0; ch < EQ_MAX_CHANNELS; ch++) {
        biquad_state_t *f = &s_eq_filters[band][ch];
        f->b0 = ib0;
        f->b1 = ib1;
        f->b2 = ib2;
        f->a1 = ia1;
        f->a2 = ia2;
        /* Don't reset history — avoids clicks when changing EQ on the fly */
    }
}

/**
 * Initialize the EQ with flat response (all gains = 0 dB).
 */
static void eq_init()
{
    memset(s_eq_filters, 0, sizeof(s_eq_filters));
    for (int b = 0; b < EQ_NUM_BANDS; b++) {
        s_eq_gains[b] = 0;
        eq_compute_coefficients(b, 0);
    }
    s_eq_enabled = false;
}

#if 0
/**
 * Set EQ gain for a specific band and recompute coefficients.
 */
static void eq_set_gain(int band, int gain_db)
{
    if (band < 0 || band >= EQ_NUM_BANDS) return;
    if (gain_db < EQ_GAIN_MIN) gain_db = EQ_GAIN_MIN;
    if (gain_db > EQ_GAIN_MAX) gain_db = EQ_GAIN_MAX;

    s_eq_gains[band] = gain_db;
    eq_compute_coefficients(band, gain_db);

    /* Auto-enable EQ when any band is non-zero */
    s_eq_enabled = (s_eq_gains[0] != 0 || s_eq_gains[1] != 0 || s_eq_gains[2] != 0);
}
#endif

/**
 * Apply one biquad filter to a single sample.
 * Uses Q28 coefficients with 64-bit intermediate math for precision.
 * Input and output are 16-bit PCM values.
 */
static inline int16_t biquad_process(biquad_state_t *f, int16_t input)
{
    int64_t x0 = (int64_t)input;
    int64_t y0 = (int64_t)f->b0 * x0
               + (int64_t)f->b1 * f->x1
               + (int64_t)f->b2 * f->x2
               - (int64_t)f->a1 * f->y1
               - (int64_t)f->a2 * f->y2;

    /* Shift down by Q28 to get the output sample */
    y0 >>= 28;

    f->x2 = f->x1;
    f->x1 = x0;
    f->y2 = f->y1;
    f->y1 = y0;

    /* Clamp to 16-bit range */
    if (y0 > 32767) y0 = 32767;
    if (y0 < -32768) y0 = -32768;

    return (int16_t)y0;
}

/**
 * Apply all 3 EQ bands to a buffer of interleaved 16-bit PCM samples.
 * Processes in-place for zero-copy efficiency.
 *
 * Each band is a proper biquad filter that only affects its frequency range:
 * - Bass (low-shelf) adjusts lows without touching highs
 * - Mid (peaking) adjusts midrange without touching lows or highs
 * - Treble (high-shelf) adjusts highs without touching lows
 *
 * @param samples  Pointer to interleaved 16-bit PCM samples
 * @param num_samples  Total number of samples (frames * channels)
 * @param channels  Number of channels (1=mono, 2=stereo)
 */
static void eq_process(int16_t *samples, size_t num_samples, int channels)
{
    if (!s_eq_enabled || channels < 1 || channels > EQ_MAX_CHANNELS) return;

    for (size_t i = 0; i < num_samples; i++) {
        int ch = i % channels;
        int16_t s = samples[i];

        /* Apply each enabled band in series */
        for (int b = 0; b < EQ_NUM_BANDS; b++) {
            if (s_eq_gains[b] != 0) {
                s = biquad_process(&s_eq_filters[b][ch], s);
            }
        }

        samples[i] = s;
    }
}


/* ============================================================================
 * I2S Audio Output Functions
 * ============================================================================ */

/**
 * Initialize I2S interface for audio output to external DAC
 * Supports common I2S DACs like MAX98357A, PCM5102, UDA1334A
 */
static esp_err_t audio_i2s_init()
{
    ESP_LOGI(TAG, "Initializing I2S audio output...");
    ESP_LOGI(TAG, "I2S Config: BCLK=%d, WS=%d, DOUT=%d, Rate=%d Hz",
             I2S_BCLK_GPIO, I2S_WS_GPIO, I2S_DOUT_GPIO, AUDIO_SAMPLE_RATE);
    ESP_LOGI(TAG, "I2S Format: PHILIPS (I2S), 16-bit stereo");
    ESP_LOGI(TAG, "Supported DACs: MAX98357A, PCM5102, UDA1334A");
    ESP_LOGI(TAG, "NOTE: PCM5102 requires FMT pin set to I2S mode (GND)");

    /* Create I2S TX channel — see I2S_DMA_DESC_NUM/I2S_DMA_FRAME_NUM defines
     * for per-target DMA buffer sizing rationale. */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = I2S_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = I2S_DMA_FRAME_NUM;

    esp_err_t ret = i2s_new_channel(&chan_cfg, &s_i2s_tx_chan, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure I2S standard mode */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(s_i2s_tx_chan, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S STD mode: %s", esp_err_to_name(ret));
        i2s_del_channel(s_i2s_tx_chan);
        s_i2s_tx_chan = NULL;
        return ret;
    }

    /* Create PCM ring buffer (small, MUST be in internal SRAM for I2S DMA).
     * When PSRAM is on the heap, plain xRingbufferCreate() may place the
     * control struct (with portMUX spinlock) in PSRAM, causing corruption
     * on Xtensa.  Use xRingbufferCreateStatic() with explicit internal alloc. */
    if (!s_audio_ringbuf)
    {
        ESP_LOGI(TAG, "Free heap before PCM buffer: %d KB internal",
                 (int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
        size_t buf_size = AUDIO_RINGBUF_SIZE;
        /* Allocate storage + control struct both in internal SRAM */
        s_pcm_ringbuf_storage = (uint8_t*) heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        s_pcm_ringbuf_struct = (StaticRingbuffer_t*) heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!s_pcm_ringbuf_storage || !s_pcm_ringbuf_struct) {
            heap_caps_free(s_pcm_ringbuf_storage);
            heap_caps_free(s_pcm_ringbuf_struct);
            s_pcm_ringbuf_storage = NULL;
            s_pcm_ringbuf_struct = NULL;
            buf_size = AUDIO_RINGBUF_FALLBACK_SIZE;
            ESP_LOGW(TAG, "PCM buffer %dKB failed, trying %dKB",
                     AUDIO_RINGBUF_SIZE / 1024, buf_size / 1024);
            s_pcm_ringbuf_storage = (uint8_t*) heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            s_pcm_ringbuf_struct = (StaticRingbuffer_t*) heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        }
        if (s_pcm_ringbuf_storage && s_pcm_ringbuf_struct) {
            s_audio_ringbuf = xRingbufferCreateStatic(buf_size, RINGBUF_TYPE_BYTEBUF,
                                                       s_pcm_ringbuf_storage, s_pcm_ringbuf_struct);
        }
        if (!s_audio_ringbuf)
        {
            ESP_LOGE(TAG, "Failed to create PCM ring buffer");
            heap_caps_free(s_pcm_ringbuf_storage);
            heap_caps_free(s_pcm_ringbuf_struct);
            s_pcm_ringbuf_storage = NULL;
            s_pcm_ringbuf_struct = NULL;
            i2s_del_channel(s_i2s_tx_chan);
            s_i2s_tx_chan = NULL;
            return ESP_ERR_NO_MEM;
        }
        s_audio_ringbuf_size = buf_size;
        ESP_LOGI(TAG, "PCM ring buffer: %d KB (internal SRAM, static alloc)", buf_size / 1024);
    }

    /* Create MP3 compressed data ring buffer (large).
     * Strategy: First try PSRAM (if available) for the full 256KB buffer.
     * If no PSRAM or PSRAM alloc fails, fall back to smaller internal SRAM
     * sizes with headroom checks to ensure HTTP/TLS have enough heap. */
    if (!s_mp3_ringbuf) {
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Free heap - internal: %d KB, PSRAM: %d KB",
                 (int)(free_internal / 1024), (int)(free_spiram / 1024));

        size_t mp3_buf_size = 0;

        /* Try PSRAM first (no headroom concern — PSRAM is separate from internal heap).
         * IMPORTANT: We must NOT use xRingbufferCreateWithCaps(MALLOC_CAP_SPIRAM)
         * because it puts the ring buffer's control structure (including the
         * portMUX spinlock) in PSRAM.  On Xtensa, spinlocks require internal
         * SRAM for atomic compare-and-swap.  A spinlock in PSRAM causes silent
         * corruption when multiple tasks access the ring buffer concurrently,
         * leading to crashes in unrelated code (WiFi, heap allocator, etc.).
         *
         * Instead: allocate the DATA buffer in PSRAM, the control struct in
         * internal SRAM, and use xRingbufferCreateStatic() to combine them. */
        if (free_spiram >= MP3_RINGBUF_SIZE + 1024) {
            s_mp3_ringbuf_storage = (uint8_t*) heap_caps_malloc(MP3_RINGBUF_SIZE, MALLOC_CAP_SPIRAM);
            s_mp3_ringbuf_struct = (StaticRingbuffer_t*) heap_caps_malloc(sizeof(StaticRingbuffer_t), MALLOC_CAP_INTERNAL);
            if (s_mp3_ringbuf_storage && s_mp3_ringbuf_struct) {
                s_mp3_ringbuf = xRingbufferCreateStatic(MP3_RINGBUF_SIZE,
                                                         RINGBUF_TYPE_BYTEBUF,
                                                         s_mp3_ringbuf_storage,
                                                         s_mp3_ringbuf_struct);
            }
            if (s_mp3_ringbuf) {
                mp3_buf_size = MP3_RINGBUF_SIZE;
                ESP_LOGI(TAG, "MP3 buffer: %dKB in PSRAM (control struct in internal SRAM)",
                         (int)(mp3_buf_size / 1024));
            } else {
                /* Clean up partial allocations */
                heap_caps_free(s_mp3_ringbuf_storage);
                heap_caps_free(s_mp3_ringbuf_struct);
                s_mp3_ringbuf_storage = NULL;
                s_mp3_ringbuf_struct = NULL;
            }
        }

        /* Fall back to internal SRAM with headroom checks.
         * Start at index 1 to skip the 256KB PSRAM-only size — it would never fit
         * in internal SRAM (~326KB total) with 80KB headroom. */
        if (!s_mp3_ringbuf) {
            for (int i = 1; i < (int)MP3_BUF_SIZES_COUNT; i++) {
                mp3_buf_size = s_mp3_buf_sizes[i];
                if (free_internal < mp3_buf_size + HEAP_HEADROOM_BYTES) {
                    ESP_LOGW(TAG, "Skipping MP3 buffer %dKB (only %dKB internal free, need %dKB headroom)",
                             (int)(mp3_buf_size / 1024), (int)(free_internal / 1024),
                             (int)(HEAP_HEADROOM_BYTES / 1024));
                    continue;
                }
                s_mp3_ringbuf = xRingbufferCreate(mp3_buf_size, RINGBUF_TYPE_BYTEBUF);
                if (s_mp3_ringbuf) {
                    ESP_LOGI(TAG, "MP3 buffer: %dKB allocated in internal SRAM", (int)(mp3_buf_size / 1024));
                    break;
                }
                ESP_LOGW(TAG, "MP3 buffer %dKB allocation failed", (int)(mp3_buf_size / 1024));
            }
        }

        if (!s_mp3_ringbuf) {
            ESP_LOGE(TAG, "Failed to create MP3 ring buffer (free: %d KB internal, %d KB PSRAM)",
                     (int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                     (int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
            i2s_del_channel(s_i2s_tx_chan);
            s_i2s_tx_chan = NULL;
            return ESP_ERR_NO_MEM;
        }
        s_mp3_ringbuf_size = mp3_buf_size;
        ESP_LOGI(TAG, "MP3 ring buffer: %d KB (~%d sec of 128kbps stream)",
                 (int)(mp3_buf_size / 1024), (int)(mp3_buf_size / (128 * 1024 / 8)));
        ESP_LOGI(TAG, "Free internal heap after buffers: %d KB (largest block: %d KB)",
                 (int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                 (int)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024));
    }

    s_audio_output_enabled = true;
    ESP_LOGI(TAG, "I2S audio output initialized successfully");
    ESP_LOGI(TAG, "Expected bit clock: ~%.2f MHz", (AUDIO_SAMPLE_RATE * 16 * 2) / 1000000.0);

    return ESP_OK;
}

/**
 * Start I2S audio playback
 */
static esp_err_t audio_start()
{
    if (!s_audio_output_enabled || !s_i2s_tx_chan) {
        ESP_LOGW(TAG, "Audio output not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = i2s_channel_enable(s_i2s_tx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    s_audio_playing = true;
    ESP_LOGI(TAG, "Audio playback started");
    return ESP_OK;
}

/**
 * Stop I2S audio playback
 */
static esp_err_t audio_stop()
{
    if (!s_audio_output_enabled || !s_i2s_tx_chan) {
        return ESP_OK;
    }

    s_audio_playing = false;
    esp_err_t ret = i2s_channel_disable(s_i2s_tx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Audio playback stopped");
    return ESP_OK;
}

/**
 * Reconfigure I2S sample rate to match decoded audio
 * Must be called when I2S channel is disabled (between stop/start).
 *
 * On ESP32-C5, i2s_channel_reconfig_std_clock() can crash with a corrupt
 * DMA channel pointer (gdma_reset called with dma_chan=0x1).  As a
 * workaround, delete and recreate the I2S channel with the new rate.
 * This is safe because the channel is always in READY state (not enabled)
 * when reconfiguration happens — the decode task reconfigures before
 * the playback task calls audio_start().
 */
static esp_err_t audio_reconfigure_sample_rate(uint32_t new_rate)
{
    if (!s_i2s_tx_chan || new_rate == s_current_sample_rate) {
        if (new_rate == s_current_sample_rate) {
            ESP_LOGD(TAG, "I2S already at %lu Hz, no reconfiguration needed", (unsigned long)new_rate);
        }
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Reconfiguring I2S: %lu Hz -> %lu Hz",
             (unsigned long)s_current_sample_rate, (unsigned long)new_rate);

    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(new_rate);
    esp_err_t ret = i2s_channel_reconfig_std_clock(s_i2s_tx_chan, &clk_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reconfigure I2S clock: %s", esp_err_to_name(ret));
        return ret;
    }

    s_current_sample_rate = new_rate;
    ESP_LOGI(TAG, "I2S sample rate set to %lu Hz", (unsigned long)new_rate);

    /* Recalculate EQ filter coefficients for the new sample rate */
    if (s_eq_enabled) {
        for (int b = 0; b < EQ_NUM_BANDS; b++) {
            if (s_eq_gains[b] != 0) {
                eq_compute_coefficients(b, s_eq_gains[b]);
            }
        }
        ESP_LOGD(TAG, "EQ coefficients recalculated for %lu Hz", (unsigned long)new_rate);
    }

    return ESP_OK;
}

/* Forward declaration - defined later in audio playback section */
static int audio_get_buffer_level();

/* Forward declaration for audio_write_to_buffer (used by MP3 decoder) */
static size_t audio_write_to_buffer(const uint8_t* data, size_t len);

/* ============================================================================
 * MP3 Decoder Functions
 * ============================================================================ */

/**
 * Initialize the MP3 decoder
 */
static void mp3_decoder_init()
{
    if (!s_mp3_decoder_initialized)
    {
        mp3dec_init(&s_mp3_decoder);
        s_mp3_decoder_initialized = true;
        ESP_LOGI(TAG, "MP3 decoder initialized");
    }
}

/* Static PCM output buffer to avoid stack allocation (saves ~4.5KB stack per decode call) */
static mp3d_sample_t s_mp3_pcm_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];

/**
 * MP3 decode task — runs on its own FreeRTOS task, bridges MP3 and PCM ring buffers.
 *
 * Reads compressed MP3 data from s_mp3_ringbuf, decodes it frame by frame
 * using minimp3, and writes decoded PCM to s_audio_ringbuf for the I2S
 * playback task.
 *
 * Natural backpressure: when the PCM ring buffer is full, audio_write_to_buffer
 * blocks (up to 500ms), which slows the decode rate to exactly match the
 * I2S hardware clock.  No flow-control hacks needed.
 */
static void mp3_decode_task(void *pvParameters)
{
    ESP_LOGI(TAG, "MP3 decode task started");

    mp3_decoder_init();

    /* Local accumulation buffer for MP3 frame assembly.
     * MP3 frames can be up to ~1728 bytes (MPEG1 Layer3 at 320kbps/32kHz).
     * We need enough data to always contain at least one complete frame. */
    const size_t MP3_ACCUM_SIZE = 4 * 1024;
    /* Allocate from INTERNAL SRAM — with PSRAM in the heap, plain malloc()
     * may return a PSRAM pointer.  MP3 decoding is CPU-intensive and accesses
     * this buffer frequently; PSRAM latency would slow decoding and the cache
     * miss handling can interfere with ISR timing. */
    uint8_t* accum = (uint8_t*) heap_caps_malloc(MP3_ACCUM_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!accum) {
        ESP_LOGE(TAG, "Failed to allocate MP3 accumulation buffer");
        DECODE_TASK_SELF_DELETE();
        return;
    }
    size_t accum_fill = 0;

    mp3dec_frame_info_t frame_info;
    int frames_since_yield = 0;

    while (s_sd_feeding_active || (s_audio_playing && accum_fill > 0))
    {
        /* Try to top up the accumulation buffer from the MP3 ring buffer */
        if (accum_fill < MP3_ACCUM_SIZE)
        {
            size_t want = MP3_ACCUM_SIZE - accum_fill;
            size_t got = 0;
            uint8_t* chunk = (uint8_t*) xRingbufferReceiveUpTo(s_mp3_ringbuf, &got, pdMS_TO_TICKS(100), want);
            if (chunk && got > 0)
            {
                memcpy(accum + accum_fill, chunk, got);
                accum_fill += got;
                vRingbufferReturnItem(s_mp3_ringbuf, chunk);
            }
        }

        if (accum_fill < 4)
        {
            /* Not enough data for a frame header — wait for more */
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Decode one frame */
        int samples = mp3dec_decode_frame(&s_mp3_decoder,
                                          accum, accum_fill,
                                          s_mp3_pcm_buffer, &frame_info);

        if (frame_info.frame_bytes == 0)
        {
            /* Not enough data for a complete frame — need more from ring buffer */
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Remove consumed bytes from accumulation buffer */
        size_t consumed = frame_info.frame_bytes;
        if (consumed < accum_fill)
            memmove(accum, accum + consumed, accum_fill - consumed);
        accum_fill -= consumed;

        if (samples > 0)
        {
            /* Log first successful decode and reconfigure I2S BEFORE writing PCM */
            if (!s_mp3_first_decode_logged)
            {
                ESP_LOGI(TAG, "MP3: %d Hz, %d ch, %d kbps",
                    frame_info.hz, frame_info.channels, frame_info.bitrate_kbps);
                s_mp3_first_decode_logged = true;

                /* Reconfigure I2S if MP3 sample rate differs from configured rate */
                if (frame_info.hz > 0 && (uint32_t)frame_info.hz != s_current_sample_rate)
                {
                    ESP_LOGI(TAG, "MP3: Reconfigure to %d", frame_info.hz);
                    esp_err_t rc = audio_reconfigure_sample_rate((uint32_t) frame_info.hz);
                    if (rc != ESP_OK)
                        ESP_LOGW(TAG, "I2S reconfigure failed, playback may be distorted");
                }
            }

            size_t pcm_bytes = samples * frame_info.channels * sizeof(mp3d_sample_t);

            /* If the stream is mono but I2S is stereo, duplicate each sample
             * to both L and R channels.  Work backwards to do it in-place. */
            if (frame_info.channels == 1)
            {
                for (int i = samples - 1; i >= 0; i--)
                {
                    mp3d_sample_t sample = s_mp3_pcm_buffer[i];
                    s_mp3_pcm_buffer[i * 2 + 1] = sample;
                    s_mp3_pcm_buffer[i * 2]     = sample;
                }
                pcm_bytes *= 2;  /* stereo = double the bytes */
            }

            /* Write decoded PCM to audio ring buffer.
             * This blocks when buffer is full — natural backpressure. */
            size_t written = audio_write_to_buffer((uint8_t*)  s_mp3_pcm_buffer, pcm_bytes);
            //ESP_LOGI(TAG, "MP3: Wrote %d", (int) written);
            if (written == 0 && s_sd_feeding_active)
            {
                /* Buffer full after timeout — try once more after a short wait */
                vTaskDelay(pdMS_TO_TICKS(50));
                audio_write_to_buffer((uint8_t*) s_mp3_pcm_buffer, pcm_bytes);
            }
        }

        /* Yield periodically to prevent watchdog timeout on chips where
         * FreeRTOS runs on one application core (C5/C6/C3/H2).  The IDLE
         * task must run to feed the WDT, and the menu task (priority 4) must
         * run for UI responsiveness.
         * On single-core: yield every frame for 2ms — gives IDLE, menu, and
         * other tasks CPU time.  Decode priority is 5 (below I/O tasks), so
         * playback and stream run whenever they need to.
         * On dual-core: yield every 4 frames for 1 tick — IDLE runs on the
         * other core, so minimal yielding is sufficient. */
        if (++frames_since_yield >= DECODE_YIELD_INTERVAL) {
            frames_since_yield = 0;
            vTaskDelay(DECODE_YIELD_TICKS);
        }
    }

    heap_caps_free(accum);
    ESP_LOGI(TAG, "MP3 decode task finished");
    DECODE_TASK_SELF_DELETE();
}

/* ============================================================================
 * AAC Decoder Functions
 * ============================================================================ */

/**
 * Initialize the AAC decoder (libhelix-aac)
 */
static void aac_decoder_init()
{
    if (!s_aac_decoder_initialized) {
        /* Log free heap before AAC init for diagnostics */
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        ESP_LOGI(TAG, "Free internal heap before AAC init: %u KB", (unsigned)(free_internal / 1024));

        s_aac_decoder = AACInitDecoder();
        if (s_aac_decoder) {
            s_aac_decoder_initialized = true;
            ESP_LOGI(TAG, "AAC decoder initialized (libhelix-aac)");
            /* Note: If "OOM in SBR" was printed above, the decoder will still work
             * for AAC-LC content.  HE-AAC streams will decode at base sample rate
             * (e.g., 22050 Hz instead of 44100 Hz) — still audible, just lower quality.
             * Adding PSRAM will allow full HE-AAC (SBR) decoding. */
        } else {
            ESP_LOGE(TAG, "Failed to initialize AAC decoder");
        }
    }
}

/**
 * Free the AAC decoder
 */
static void aac_decoder_free()
{
    if (s_aac_decoder) {
        AACFreeDecoder(s_aac_decoder);
        s_aac_decoder = NULL;
        s_aac_decoder_initialized = false;
        s_aac_first_decode_logged = false;
    }
}

/* AAC output: up to 2048 samples per channel (1024 AAC + SBR doubles it),
 * stereo = 4096 samples total.  16-bit = 8KB static buffer. */
#define AAC_MAX_PCM_SAMPLES  (AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2)  /* *2 for SBR */
static int16_t s_aac_pcm_buffer[AAC_MAX_PCM_SAMPLES];

/**
 * AAC decode task — runs on its own FreeRTOS task, bridges compressed and PCM ring buffers.
 *
 * Reads compressed AAC data from s_mp3_ringbuf (reused for any compressed codec),
 * decodes it frame by frame using libhelix-aac, and writes decoded PCM to
 * s_audio_ringbuf for the I2S playback task.
 */
static void aac_decode_task(void *pvParameters)
{
    ESP_LOGI(TAG, "AAC decode task started");

    if (!s_aac_decoder_initialized) {
        aac_decoder_init();
    }
    if (!s_aac_decoder) {
        ESP_LOGE(TAG, "AAC decoder not available, task exiting");
        DECODE_TASK_SELF_DELETE();
        return;
    }

    /* Local accumulation buffer for AAC frame assembly.
     * AAC frames can be up to ~1536 bytes (768 * 2 channels).
     * With ADTS header overhead, 6KB is generous.
     * Fall back to 3KB if 6KB is not available (memory-constrained chips like C6). */
    const size_t AAC_ACCUM_PREFERRED = 6 * 1024;
    const size_t AAC_ACCUM_FALLBACK  = 3 * 1024;
    size_t AAC_ACCUM_SIZE = AAC_ACCUM_PREFERRED;
    uint8_t *accum = (uint8_t*) heap_caps_malloc(AAC_ACCUM_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!accum) {
        AAC_ACCUM_SIZE = AAC_ACCUM_FALLBACK;
        accum = (uint8_t*) heap_caps_malloc(AAC_ACCUM_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!accum) {
            ESP_LOGE(TAG, "Failed to allocate AAC accumulation buffer");
            DECODE_TASK_SELF_DELETE();
            return;
        }
        ESP_LOGW(TAG, "AAC accumulation buffer reduced to %u bytes", (unsigned)AAC_ACCUM_SIZE);
    }
    size_t accum_fill = 0;
    int consecutive_errors = 0;
    int frames_since_yield = 0;

    while (s_sd_feeding_active || (s_audio_playing && accum_fill > 0)) {
        /* Try to top up the accumulation buffer from the compressed ring buffer */
        if (accum_fill < AAC_ACCUM_SIZE) {
            size_t want = AAC_ACCUM_SIZE - accum_fill;
            size_t got = 0;
            uint8_t *chunk = (uint8_t*) xRingbufferReceiveUpTo(
                s_mp3_ringbuf, &got, pdMS_TO_TICKS(100), want);
            if (chunk && got > 0) {
                memcpy(accum + accum_fill, chunk, got);
                accum_fill += got;
                vRingbufferReturnItem(s_mp3_ringbuf, chunk);
            }
        }

        if (accum_fill < ADTS_HEADER_SIZE) {
            /* Not enough data for an ADTS header */
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Find ADTS sync word (0xFFF) */
        int sync_offset = AACFindSyncWord(accum, (int)accum_fill);
        if (sync_offset < 0) {
            /* No sync word found — discard buffer and refill */
            accum_fill = 0;
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Skip bytes before sync word */
        if (sync_offset > 0) {
            memmove(accum, accum + sync_offset, accum_fill - sync_offset);
            accum_fill -= sync_offset;
        }

        /* Need at least 7 bytes for a complete ADTS header after sync adjustment */
        if (accum_fill < ADTS_HEADER_SIZE) {
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Validate ADTS frame length before decoding.
         * ADTS header bytes 3-5 contain the 13-bit frame length.
         * If the frame claims to be larger than what we have, wait for more data.
         * If the frame length is impossibly small or zero, skip this sync word.
         * If the frame length exceeds our buffer capacity, the header is corrupt
         * — skip the sync word to avoid a deadlock (buffer can never fill enough). */
        {
            unsigned int adts_frame_len = ((unsigned int)(accum[3] & 0x03) << 11) |
                                          ((unsigned int)accum[4] << 3) |
                                          ((unsigned int)(accum[5] >> 5) & 0x07);
            if (adts_frame_len < ADTS_HEADER_SIZE || adts_frame_len > AAC_ACCUM_SIZE) {
                /* Invalid frame length — skip past this sync word */
                memmove(accum, accum + 2, accum_fill - 2);
                accum_fill -= 2;
                vTaskDelay(1);
                continue;
            }
            if (adts_frame_len > accum_fill) {
                /* Incomplete frame — wait for more data */
                if (!s_sd_feeding_active)
                    break;
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
        }

        /* Decode one frame */
        unsigned char *inbuf_ptr = accum;
        int bytes_left = (int)accum_fill;
        int err = AACDecode(s_aac_decoder, &inbuf_ptr, &bytes_left, s_aac_pcm_buffer);

        if (err == ERR_AAC_NONE) {
            consecutive_errors = 0;

            /* Get frame info for sample rate, channels, etc. */
            AACFrameInfo frame_info;
            AACGetLastFrameInfo(s_aac_decoder, &frame_info);

            /* Remove consumed bytes from accumulation buffer */
            size_t consumed = accum_fill - (size_t)bytes_left;
            if ((size_t)bytes_left > 0) {
                memmove(accum, accum + consumed, (size_t)bytes_left);
            }
            accum_fill = (size_t)bytes_left;

            if (frame_info.outputSamps > 0) {
                /* Log first successful decode and reconfigure I2S BEFORE writing PCM */
                if (!s_aac_first_decode_logged) {
                    ESP_LOGI(TAG, "AAC: %d Hz (out: %d Hz), %d ch, %d kbps, profile %d",
                             frame_info.sampRateCore, frame_info.sampRateOut,
                             frame_info.nChans, frame_info.bitRate / 1000,
                             frame_info.profile);
                    s_aac_first_decode_logged = true;

                    /* Reconfigure I2S if AAC sample rate differs from configured rate.
                     * Use sampRateOut which accounts for SBR (HE-AAC doubles the rate). */
                    int out_rate = frame_info.sampRateOut > 0 ? frame_info.sampRateOut : frame_info.sampRateCore;
                    if (out_rate > 0 && (uint32_t)out_rate != s_current_sample_rate) {
                        esp_err_t rc = audio_reconfigure_sample_rate((uint32_t)out_rate);
                        if (rc != ESP_OK) {
                            ESP_LOGW(TAG, "I2S reconfigure failed, playback may be distorted");
                        }
                    }
                }

                size_t pcm_bytes = (size_t)frame_info.outputSamps * sizeof(int16_t);

                /* If the stream is mono but I2S is stereo, duplicate each sample
                 * to both L and R channels.  Work backwards to do it in-place. */
                if (frame_info.nChans == 1) {
                    int mono_samples = frame_info.outputSamps;
                    for (int i = mono_samples - 1; i >= 0; i--) {
                        int16_t sample = s_aac_pcm_buffer[i];
                        s_aac_pcm_buffer[i * 2 + 1] = sample;
                        s_aac_pcm_buffer[i * 2]     = sample;
                    }
                    pcm_bytes *= 2;  /* stereo = double the bytes */
                }

                /* Write decoded PCM to audio ring buffer */
                size_t written = audio_write_to_buffer((uint8_t*) s_aac_pcm_buffer, pcm_bytes);
                if (written == 0 && s_sd_feeding_active)
                {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    audio_write_to_buffer((uint8_t*) s_aac_pcm_buffer, pcm_bytes);
                }
            }
        } else if (err == ERR_AAC_INDATA_UNDERFLOW) {
            /* Need more data — wait for stream to provide more */
            if (!s_sd_feeding_active)
                break;
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        } else {
            /* Decode error — skip past the current sync word to find the next frame */
            consecutive_errors++;
            if (!s_audio_playing) {
                ESP_LOGI(TAG, "AAC: playback stopped, exiting decoder");
                break;
            }
            if (consecutive_errors > 20) {
                ESP_LOGE(TAG, "AAC: too many consecutive errors (%d), giving up", consecutive_errors);
                break;
            }
            ESP_LOGD(TAG, "AAC decode error %d, skipping to next sync", err);

            /* Use decoder's consumed byte count if it advanced, otherwise skip 2 bytes */
            size_t consumed = (size_t)(inbuf_ptr - accum);
            if (consumed == 0) {
                consumed = 2;  /* Skip past current sync word */
            }
            if (consumed < accum_fill) {
                memmove(accum, accum + consumed, accum_fill - consumed);
                accum_fill -= consumed;
            } else {
                accum_fill = 0;
            }
            /* Yield after errors to prevent WDT on single-core chips (C6) */
            vTaskDelay(1);
        }

        /* Yield periodically — see mp3_decode_task comment for rationale. */
        if (++frames_since_yield >= DECODE_YIELD_INTERVAL) {
            frames_since_yield = 0;
            vTaskDelay(DECODE_YIELD_TICKS);
        }
    }

    heap_caps_free(accum);
    aac_decoder_free();
    ESP_LOGI(TAG, "AAC decode task finished");
    DECODE_TASK_SELF_DELETE();
}

/**
 * Write raw MP3 data to the MP3 compressed ring buffer (called from stream task).
 * Returns number of bytes written (0 if buffer is full after timeout).
 */
static size_t mp3_write_to_buffer(const uint8_t *data, size_t len)
{
    if (!s_mp3_ringbuf) {
        return 0;
    }
    BaseType_t ret = xRingbufferSend(s_mp3_ringbuf, data, len,
                                      s_sd_feeding_active ? pdMS_TO_TICKS(500) : pdMS_TO_TICKS(50));
    return (ret == pdTRUE) ? len : 0;
}

/**
 * Write audio data to the ring buffer (called from decode tasks)
 * Applies EQ processing if enabled before writing to the buffer.
 * Returns number of bytes written
 */
static size_t audio_write_to_buffer(const uint8_t* data, size_t len)
{
    if (!s_audio_output_enabled || !s_audio_ringbuf)
        return 0;

    const uint8_t* send_data = data;
    TickType_t timeout = s_sd_feeding_active ? pdMS_TO_TICKS(500) : pdMS_TO_TICKS(50);

    /* Apply 3-band EQ to PCM data ONLY when enabled.
     * When disabled, skip the processing and memcpy entirely
     * to maximize decode throughput on slower chips (C6 RISC-V @ 160MHz).
     * The eq_buf is static to avoid blowing the decode task stack —
     * a local 8KB VLA would overflow the 12KB stack when combined with
     * the deep AACDecode/IMDCT call chain. */
    #define EQ_BUF_SAMPLES 4096  /* Max samples for EQ processing (8KB at 16-bit) */

    bool need_buf = (s_eq_enabled || s_volume < VOLUME_MAX) &&
                    len >= 4 && len <= (EQ_BUF_SAMPLES * sizeof(int16_t));

    if (need_buf)
    {
        /* Static buffer avoids 8KB stack spike.  Thread-safe because
         * audio_write_to_buffer is only called from the single active
         * decode task (MP3 or AAC, never both concurrently). */
        static int16_t eq_buf[EQ_BUF_SAMPLES];
        size_t num_samples = len / sizeof(int16_t);
        memcpy(eq_buf, data, len);

        if (s_eq_enabled)
            eq_process(eq_buf, num_samples, 2 /* stereo */);

        /* Apply software volume scaling (linear). */
        if (s_volume < VOLUME_MAX)
            for (size_t i = 0; i < num_samples; i++)
                eq_buf[i] = (int16_t)(((int32_t) eq_buf[i] * s_volume) / VOLUME_MAX);

        BaseType_t ret = xRingbufferSend(s_audio_ringbuf, eq_buf, len, timeout);
        return (ret == pdTRUE) ? len : 0;
    }

    /* No EQ and full volume — send raw PCM directly to ring buffer */
    BaseType_t ret = xRingbufferSend(s_audio_ringbuf, send_data, len, timeout);
    return (ret == pdTRUE) ? len : 0;
}

/**
 * Audio playback task - reads from ring buffer and writes to I2S
 * Note: This task is only used for I2S mode. USB mode uses callbacks.
 */
static void audio_playback_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Audio playback task started");

    const size_t chunk_size = PLAYBACK_CHUNK_SIZE;
    size_t bytes_written = 0;

    /* Wait for PCM ring buffer to get some data from the decode task.
     * A higher pre-buffer level gives the decoder a head start, which is
     * critical on chips like the C6 where all FreeRTOS tasks share one
     * application core (the LP core is a coprocessor, not used by FreeRTOS). */
    int prebuf_ms = 0;
    const int PCM_PREBUF_PCT = PCM_PREBUF_THRESHOLD_PCT;
    while (prebuf_ms < 5000 && s_sd_feeding_active)
    {
        int level = audio_get_buffer_level();
        if (level >= PCM_PREBUF_PCT)
            break;
        vTaskDelay(pdMS_TO_TICKS(50));
        prebuf_ms += 50;
    }
    ESP_LOGI(TAG, "PCM pre-buffered for %d ms, level: %d%%",
        prebuf_ms, audio_get_buffer_level());

    /* Start I2S playback */
    if (audio_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start audio, task exiting");
        vTaskDelete(NULL);
        return;
    }

    size_t total_bytes = 0;
    while (s_sd_feeding_active || s_audio_playing)
    {
        size_t item_size = 0;
        uint8_t* data = (uint8_t*) xRingbufferReceiveUpTo(s_audio_ringbuf, &item_size,
                                                          pdMS_TO_TICKS(100), chunk_size);

        if (data && item_size > 0)
        {
            total_bytes += item_size;
            /* Write ALL data to I2S — partial writes lose audio and cause
             * playback to speed up because skipped PCM data shortens the
             * effective audio duration.  Loop until every byte is sent. */
            size_t total_written = 0;
            while (total_written < item_size && s_audio_playing)
            {
                esp_err_t ret = i2s_channel_write(s_i2s_tx_chan,
                                                  data + total_written,
                                                  item_size - total_written,
                                                  &bytes_written,
                                                  pdMS_TO_TICKS(I2S_WRITE_TIMEOUT_MS));
                if (ret != ESP_OK || bytes_written == 0)
                {
                    ESP_LOGW(TAG, "I2S write error: %s (wrote %u/%u)",
                             esp_err_to_name(ret), (unsigned)total_written, (unsigned)item_size);
                    break;
                }
                total_written += bytes_written;
            }
            /* Return the buffer space */
            vRingbufferReturnItem(s_audio_ringbuf, data);
        }
        else
        {
            ESP_LOGI(TAG, "No data in ring buffer");
            /* No data available — let I2S DMA continue playing its internal
             * buffer while we wait for the decode task to produce more PCM.
             * On chips with one FreeRTOS core, stopping/restarting I2S during
             * brief underruns causes worse glitches than letting DMA drain. */
            if (!s_sd_feeding_active)
                /* Stream ended, exit playback */
                break;

            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

#if 0
    auto data = (uint8_t*) heap_caps_malloc(PLAYBACK_CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    memset(data, 0, PLAYBACK_CHUNK_SIZE);
    for (int i = 0; i < 10; ++i)
    {
        esp_err_t ret = i2s_channel_write(s_i2s_tx_chan,
                                          data, PLAYBACK_CHUNK_SIZE, &bytes_written,
                                          pdMS_TO_TICKS(I2S_WRITE_TIMEOUT_MS));
        if (ret != ESP_OK || bytes_written == 0)
        {
            ESP_LOGW(TAG, "I2S write error: %s (wrote %u)",
                     esp_err_to_name(ret), (unsigned) bytes_written);
            break;
        }
        printf("Wrote %d zeroes\n", (int) bytes_written);
    }
    //vTaskDelay(pdMS_TO_TICKS(1000));
#endif
    audio_stop();
    ESP_LOGI(TAG, "Audio playback task finished");
    vTaskDelete(NULL);
}

/**
 * Get audio buffer fill level in percentage
 */
static int audio_get_buffer_level()
{
    if (!s_audio_ringbuf || s_audio_ringbuf_size == 0)
        return 0;

    UBaseType_t free_size = xRingbufferGetCurFreeSize(s_audio_ringbuf);
    int used = (int) (s_audio_ringbuf_size - free_size);
    if (used < 0)
        used = 0;
    return (used * 100) / (int) s_audio_ringbuf_size;
}

/**
 * Flush all data from a ring buffer by draining and returning all items.
 * Called before starting a new stream to prevent stale audio data from
 * the previous stream from being played.
 */
static void flush_ringbuffer(RingbufHandle_t rbuf, const char *name)
{
    if (!rbuf)
        return;
    size_t item_size;
    int flushed = 0;
    while (true) {
        void *item = xRingbufferReceiveUpTo(rbuf, &item_size, 0, 4096);
        if (!item)
            break;
        vRingbufferReturnItem(rbuf, item);
        flushed += (int)item_size;
    }
    if (flushed > 0)
        ESP_LOGI(TAG, "Flushed %d bytes from %s ring buffer", flushed, name);
}

/**
 * Reset decoder state flags so the next stream gets a fresh decoder.
 * Called during stream switching and before starting a new stream to prevent
 * stale codec state from the previous stream (e.g. AAC decoder left in error
 * state) from contaminating the new stream.
 * Note: the AAC decoder object itself is freed by aac_decode_task on exit
 * (aac_decoder_free), so we only reset flags here.
 */
static void reset_decoder_state(void)
{
    s_mp3_decoder_initialized = false;
    s_mp3_first_decode_logged = false;
    s_aac_first_decode_logged = false;
}

/* ============================================================================
 * SD Card Playback
 * ============================================================================ */

/**
 * Scan /sdcard/music/ (and subdirectories) for supported audio and playlist files.
 * Returns the number of tracks found.
 *
 * Supported extensions:
 *   - .mp3       — MP3 audio (libhelix-mp3 decoder)
 *   - .aac       — AAC audio (libhelix-aac decoder)
 *   - .wav       — WAV/PCM audio (raw PCM, bypasses decode task)
 *   - .m3u       — M3U playlist file
 *
 * TODO: Add support for additional audio formats:
 *   - FLAC (.flac)    — Needs dr_flac or libFLAC library. FLAC decoding is
 *                        CPU-intensive; may require PSRAM for buffers on
 *                        single-core chips. Add as ESP-IDF component.
 *   - OGG Vorbis (.ogg) — Needs stb_vorbis (public domain, ~350KB flash) or
 *                        Tremor (integer-only, ~100KB flash). Add as ESP-IDF
 *                        component, create ogg_decode_task similar to mp3/aac.
 *   - OPUS (.opus)    — Needs libopus. High CPU cost, likely ESP32-S3/PSRAM only.
 * When adding new formats, extend sd_is_audio_ext() and add codec
 * detection in sd_playback_task() to select the appropriate decoder.
 */
/**
 * Helper: check if a file extension is a supported audio type (for playback).
 * Note: .m3u is NOT included here — playlists are metadata, not audio files.
 * This keeps the playback track list free of non-audio entries, so track
 * counts, NEXT/PREV, repeat logic and STATUS output stay consistent.
 */
static bool sd_is_audio_ext(const char *ext)
{
    return (strcasecmp(ext, ".mp3") == 0 ||
            strcasecmp(ext, ".aac") == 0 ||
            strcasecmp(ext, ".wav") == 0);
}

static void sd_scan_dir_recursive(const char* dirpath, int max_depth)
{
    if (max_depth <= 0)
        return;

    DIR *dir = opendir(dirpath);
    if (!dir)
    {
        ESP_LOGW(TAG, "Cannot open directory: %s", dirpath);
        return;
    }

    while (struct dirent* entry = readdir(dir))
    {
        const char* name = entry->d_name;
        /* Skip hidden files and . / .. */
        if (name[0] == '.')
            continue;

        /* Build full path — skip entries whose combined path exceeds our
         * storage limit (avoids truncation and the associated compiler warning
         * from -Wformat-truncation with snprintf). */
        size_t dlen = strlen(dirpath);
        size_t nlen_d = strlen(name);
        if (dlen + 1 + nlen_d >= SD_FILENAME_MAX) continue;
        char fullpath[SD_FILENAME_MAX];
        memcpy(fullpath, dirpath, dlen);
        fullpath[dlen] = '/';
        memcpy(fullpath + dlen + 1, name, nlen_d + 1); /* includes '\0' */

        /* Check if it's a directory — recurse into it.
         * d_type may be DT_UNKNOWN on some VFS/filesystem implementations,
         * so fall back to stat() when needed. */
        bool is_dir = (entry->d_type == DT_DIR);
        if (!is_dir && entry->d_type == DT_UNKNOWN) {
            struct stat st;
            if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = true;
            }
        }
        if (is_dir) {
            sd_scan_dir_recursive(fullpath, max_depth - 1);
            continue;
        }

        /* Only accept files with supported audio extensions (not playlists). */
        if (nlen_d < 5)
            continue;
        /* Find the last '.' to get the extension (handles variable-length names) */
        const char *dot = strrchr(name, '.');
        if (dot && sd_is_audio_ext(dot))
            sd_tracks.push_back(fullpath + strlen(SDCARD_ROOT_PATH) + 1);
    }
    closedir(dir);
}

static int sd_scan_music_files()
{
    sd_scan_dir_recursive(SDCARD_ROOT_PATH, 4);
    ESP_LOGI(TAG, "SD card: found %d file(s) in " SDCARD_ROOT_PATH, (int) sd_tracks.size());
    std::sort(sd_tracks.begin(), sd_tracks.end());
    return (int) sd_tracks.size();
}

const std::vector<std::string>& sd_get_tracks()
{
    return sd_tracks;
}

#define SD_READ_BUF_SIZE  2048

static void abort_sd_playback()
{
    s_sd_playback_active = false;
    s_sd_task_running = false;
    vTaskDelete(NULL);
}

/**
 * SD card playback task — reads MP3 files from SD card and feeds them through
 * the decode → playback pipeline (same as WiFi streaming but from local files).
 */
static void sd_playback_task(void *pvParameters)
{
    ESP_LOGI(TAG, "SD playback task started");

    if (s_sd_track_index < 0 || s_sd_track_index >= sd_tracks.size())
    {
        ESP_LOGI(TAG, "SD: Invalid track %d", s_sd_track_index);
        abort_sd_playback();
        return;
    }
            
    const std::string filepath = std::string(SDCARD_ROOT_PATH) +
        std::string("/") +
        sd_tracks[s_sd_track_index];

    /* Skip playlist files — they're metadata, not audio */
    {
        size_t plen = filepath.size();
        if (plen >= 4 && strcasecmp(filepath.c_str() + plen - 4, ".m3u") == 0)
        {
            ESP_LOGI(TAG, "SD: Track %d is a playlist", s_sd_track_index);
            abort_sd_playback();
            return;
        }
    }

    ESP_LOGI(TAG, "SD: Playing track %d/%d: %s",
        s_sd_track_index + 1, (int) sd_tracks.size(),
        sd_tracks[s_sd_track_index].c_str());
    
    FILE* f = fopen(filepath.c_str(), "rb");
    if (!f)
    {
        ESP_LOGW(TAG, "Cannot open file: %s", filepath.c_str());
        abort_sd_playback();
        return;
    }

    /* Flush stale data from previous track/source */
    flush_ringbuffer(s_mp3_ringbuf, "compressed");
    flush_ringbuffer(s_audio_ringbuf, "PCM");
    reset_decoder_state();

    /* Detect codec from file extension */
    stream_codec_t track_codec = CODEC_MP3;
    bool is_wav = false;
    {
        size_t plen = filepath.size();
        if (plen >= 4)
        {
            const char* ext = filepath.c_str() + plen - 4;
            if (strcasecmp(ext, ".aac") == 0)
                track_codec = CODEC_AAC;
            else if (strcasecmp(ext, ".wav") == 0)
            {
                is_wav = true;
                track_codec = CODEC_PCM;
            }
        }
    }

    /* For WAV files: parse header, configure I2S, and feed PCM directly.
     * For MP3/AAC: use the normal decode→playback pipeline. */
    if (is_wav)
    {
        /* === WAV playback path (no decode task needed) === */
        /* Read and validate the WAV header */
        uint8_t hdr[44];
        size_t hdr_read = fread(hdr, 1, 44, f);
        if (hdr_read < 44 ||
            memcmp(hdr, "RIFF", 4) != 0 ||
            memcmp(hdr + 8, "WAVE", 4) != 0)
        {
            ESP_LOGW(TAG, "Invalid WAV header: %s", filepath.c_str());
            fclose(f);
            abort_sd_playback();
            return;
        }
        /* Parse format fields from the standard 44-byte header */
        uint16_t audio_fmt   = hdr[20] | (hdr[21] << 8);
        uint16_t num_ch      = hdr[22] | (hdr[23] << 8);
        uint32_t sample_rate = hdr[24] | (hdr[25] << 8) | (hdr[26] << 16) | (hdr[27] << 24);
        uint16_t bits_per    = hdr[34] | (hdr[35] << 8);

        if (audio_fmt != 1)
        {
            /* 1 = PCM */
            ESP_LOGW(TAG, "WAV: unsupported format %d (only PCM=1)", audio_fmt);
            fclose(f);
            abort_sd_playback();
            return;
        }
        if (bits_per != 16)
        {
            ESP_LOGW(TAG, "WAV: unsupported %d-bit (only 16-bit)", bits_per);
            fclose(f);
            abort_sd_playback();
            return;
        }
        if (num_ch != 2)
        {
            ESP_LOGW(TAG, "WAV: unsupported %d-channel (only stereo/2ch)", num_ch);
            fclose(f);
            abort_sd_playback();
            return;
        }
        printf("WAV: %lu Hz, %d ch, %d-bit PCM\n",
               (unsigned long) sample_rate, num_ch, bits_per);

        /* Reconfigure I2S to the WAV sample rate */
        audio_reconfigure_sample_rate(sample_rate);

        s_sd_feeding_active = true;

        /* Create only the playback task (no decode task for raw PCM) */
        TaskHandle_t audio_task_handle = NULL;
        if (s_audio_output_enabled)
        {
            BaseType_t tret = xTaskCreate(audio_playback_task, "audio_playback",
                                          PLAYBACK_TASK_STACK_SIZE, NULL, 7, &audio_task_handle);
            if (tret != pdPASS)
            {
                ESP_LOGW(TAG, "Failed to create audio playback task for WAV");
                s_sd_feeding_active = false;
                fclose(f);
                abort_sd_playback();
                return;
            }
        }
        
        /* Read WAV data and feed directly to PCM ring buffer */
        char* read_buf = (char*) heap_caps_malloc(SD_READ_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!read_buf)
        {
            ESP_LOGE(TAG, "Failed to allocate SD read buffer");
            s_sd_feeding_active = false;
            fclose(f);
            abort_sd_playback();
            return;
        }

        while (s_sd_playback_active)
        {
            size_t bytes_read = fread(read_buf, 1, SD_READ_BUF_SIZE, f);
            if (bytes_read == 0)
                break; /* EOF */
            /* Write raw PCM samples directly to the audio ring buffer */
            audio_write_to_buffer((uint8_t*) read_buf, bytes_read);
        }

        fclose(f);
        heap_caps_free(read_buf);
        ESP_LOGI(TAG, "File EOF");
        s_sd_feeding_active = false;

        /* Wait for playback task to finish */
        if (audio_task_handle)
        {
            for (int w = 0; w < 80; w++)
            {
                if (eTaskGetState(audio_task_handle) == eDeleted)
                    break;
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        ESP_LOGI(TAG, "WAV playback complete");
        vTaskDelay(pdMS_TO_TICKS(100));
        s_sd_task_running = false;
        s_sd_playback_active = false;
        vTaskDelete(NULL);
        return;
    } // is_wav

    /* === MP3/AAC playback path (decode → playback pipeline) === */

    /* Initialize the appropriate decoder */
    if (track_codec == CODEC_AAC)
        aac_decoder_init();
    else
        mp3_decoder_init();

    /* Mark feeding active BEFORE creating decode/playback tasks.
     * On dual-core chips (ESP32/S3), a higher-priority task created by
     * xTaskCreate can run immediately on the other core.  If the decode
     * task (priority 8) starts before s_sd_feeding_active is true, it sees
     * s_sd_feeding_active == false and exits its loop immediately — no
     * audio output.  Setting the flag first prevents this race. */
    s_sd_feeding_active = true;

    /* Create decode task — select MP3 or AAC based on file extension */
    TaskHandle_t decode_task_handle = NULL;
    if (s_audio_output_enabled && s_mp3_ringbuf) {
        const char *task_name = (track_codec == CODEC_AAC) ? "aac_decode" : "mp3_decode";
        TaskFunction_t decode_fn = (track_codec == CODEC_AAC)
            ? aac_decode_task : mp3_decode_task;
#if CONFIG_SPIRAM
        BaseType_t tret = xTaskCreateWithCaps(decode_fn, task_name,
                                              DECODE_TASK_STACK_SIZE, NULL, DECODE_TASK_PRIORITY,
                                              &decode_task_handle, MALLOC_CAP_DEFAULT);
#else
        BaseType_t tret = xTaskCreate(decode_fn, task_name,
                                      DECODE_TASK_STACK_SIZE, NULL, DECODE_TASK_PRIORITY, &decode_task_handle);
#endif
        if (tret != pdPASS) {
            ESP_LOGW(TAG, "Failed to create %s decode task for SD playback", task_name);
            s_sd_feeding_active = false;
            fclose(f);
            abort_sd_playback();
            return;
        }
    }

    /* Create playback task */
    TaskHandle_t audio_task_handle = NULL;
    if (s_audio_output_enabled) {
        BaseType_t tret = xTaskCreate(audio_playback_task, "audio_playback",
                                      PLAYBACK_TASK_STACK_SIZE, NULL, 7, &audio_task_handle);
        if (tret != pdPASS) {
            ESP_LOGW(TAG, "Failed to create audio playback task for SD");
        }
    }

    /* Read file and feed to compressed ring buffer */
    char* read_buf = (char*) heap_caps_malloc(SD_READ_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!read_buf)
    {
        ESP_LOGE(TAG, "Failed to allocate SD read buffer");
        s_sd_feeding_active = false;
        fclose(f);
        abort_sd_playback();
        return;
    }

    while (s_sd_playback_active)
    {
        size_t bytes_read = fread(read_buf, 1, SD_READ_BUF_SIZE, f);
        if (bytes_read == 0)
            /* End of file */
            break;
        mp3_write_to_buffer((uint8_t*) read_buf, bytes_read);
    }

    fclose(f);
    ESP_LOGI(TAG, "MP3 file read complete");
    heap_caps_free(read_buf);

    /* Clear feeding flag so s_sd_feeding_active goes false, allowing
     * decode/playback child tasks to drain remaining data and exit.
     * s_sd_playback_active stays true to keep the outer track loop going. */
    s_sd_feeding_active = false;

    /* Wait for child tasks to finish */
    if (decode_task_handle)
    {
        ESP_LOGI(TAG, "Wait for decode task completion");
        for (int w = 0; w < 80; w++)
        {
            if (eTaskGetState(decode_task_handle) == eDeleted)
                break;
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    if (audio_task_handle)
    {
        ESP_LOGI(TAG, "Wait for audio task completion");
        for (int w = 0; w < 80; w++)
        {
            if (eTaskGetState(audio_task_handle) == eDeleted)
                break;
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    /* Let IDLE task free deleted task stacks */
    vTaskDelay(pdMS_TO_TICKS(100));

    s_sd_playback_active = false;
    s_sd_task_running = false;

    ESP_LOGI(TAG, "SD playback task finished");

    vTaskDelete(NULL);
}

/**
 * Stop SD card playback if active.
 * Sets the stop flag and waits for the SD task to finish cleanup.
 */
void stop_sd_playback()
{
    if (!s_sd_playback_active && !s_sd_task_running)
        return;

    ESP_LOGI(TAG, "Stopping SD card playback...");
    s_sd_playback_active = false;
    s_sd_feeding_active = false;
    s_audio_playing = false;

    /* Wait for SD task to finish */
    for (int w = 0; w < 120 && s_sd_task_running; w++)
        vTaskDelay(pdMS_TO_TICKS(50));
    if (s_sd_task_running)
        ESP_LOGW(TAG, "SD playback cleanup timed out");
    vTaskDelay(pdMS_TO_TICKS(100));
}

/**
 * Start SD card playback.
 * Stops any active WiFi stream first, then mounts SD and starts playing.
 */
bool start_sd_playback(int track_index)
{
    if (s_sd_playback_active)
    {
        ESP_LOGW(TAG, "SD playback already active");
        return false;
    }

    s_sd_track_index = track_index;

    /* Flush buffers between sources — critical on C5 without PSRAM */
    flush_ringbuffer(s_mp3_ringbuf, "compressed");
    flush_ringbuffer(s_audio_ringbuf, "PCM");
    reset_decoder_state();

    s_sd_playback_active = true;
    s_sd_task_running = true;

#if CONFIG_IDF_TARGET_ESP32C5 && !CONFIG_SPIRAM
    BaseType_t ret = xTaskCreate(sd_playback_task, "sd_play", 8192, NULL, 6, NULL);
#elif !CONFIG_SPIRAM
    BaseType_t ret = xTaskCreate(sd_playback_task, "sd_play", 10240, NULL, 6, NULL);
#else
    BaseType_t ret = xTaskCreate(sd_playback_task, "sd_play", 12288, NULL, 6, NULL);
#endif
    if (ret != pdPASS)
    {
        s_sd_playback_active = false;
        s_sd_task_running = false;
        ESP_LOGE(TAG, "Failed to create SD playback task");
        return false;
    }
    return true;
}

bool i2s_init()
{
    /* Initialize audio output (I2S or USB based on configuration) */
    ESP_LOGI(TAG, "Initializing audio subsystem...");

    if (audio_i2s_init() != ESP_OK)
        return false;

    /* Initialize 3-band equalizer */
    eq_init();
    ESP_LOGI(TAG, "3-band EQ initialized (bass/mid/treble)");

    ESP_LOGI(TAG, "I2S GPIO: BCLK=%d, WS=%d, DOUT=%d",
             I2S_BCLK_GPIO, I2S_WS_GPIO, I2S_DOUT_GPIO);

    /* Mount the SD card filesystem */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_GPIO;
    slot_config.host_id = (spi_host_device_t) host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 0,
    };
    sdmmc_card_t *card = NULL;

    /* SPI bus may already be initialized from menu_init detection probe */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_GPIO,
        .miso_io_num = SD_MISO_GPIO,
        .sclk_io_num = SD_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t bus_ret = spi_bus_initialize((spi_host_device_t) host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (bus_ret != ESP_OK && bus_ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(bus_ret));
        return false;
    }

    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_cfg, &card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    sd_scan_music_files();

    if (sd_tracks.empty())
    {
        ESP_LOGW(TAG, "No audio files found in " SDCARD_ROOT_PATH);
        esp_vfs_fat_sdcard_unmount("/sdcard", card);
        return false;
    }

    return true;
}

int get_sd_track_count()
{
    return (int) sd_tracks.size();
}

bool check_sd_track(int track_index,
                    std::string& error_msg)
{
    if (track_index < 0 || track_index >= sd_tracks.size())
    {
        error_msg = "Invalid track index";
        return false;
    }
            
    const std::string filepath = std::string(SDCARD_ROOT_PATH) +
        std::string("/") +
        sd_tracks[track_index];
    FILE* f = fopen(filepath.c_str(), "rb");
    if (!f)
    {
        error_msg = "Cannot open file";
        return false;
    }

    bool is_wav = false;
    const size_t plen = filepath.size();
    if (plen >= 4)
    {
        const char* ext = filepath.c_str() + plen - 4;
        if (strcasecmp(ext, ".wav") == 0)
            is_wav = true;
    }
    if (!is_wav)
    {
        fclose(f);
        return true;
    }

    uint8_t hdr[44];
    size_t hdr_read = fread(hdr, 1, 44, f);
    fclose(f);
    if (hdr_read < 44 ||
        memcmp(hdr, "RIFF", 4) != 0 ||
        memcmp(hdr + 8, "WAVE", 4) != 0)
    {
        error_msg = "Invalid WAV header: " + std::string((const char*) hdr, 4);
        return false;
    }
    uint16_t audio_fmt   = hdr[20] | (hdr[21] << 8);
    uint16_t num_ch      = hdr[22] | (hdr[23] << 8);
    uint16_t bits_per    = hdr[34] | (hdr[35] << 8);

    if (audio_fmt != 1)
    {
        /* 1 = PCM */
        error_msg = "WAV: unsupported format " + std::to_string(audio_fmt);
        return false;
    }
    if (bits_per != 16)
    {
        error_msg = "WAV: unsupported bit depth " + std::to_string(bits_per);
        return false;
    }
    if (num_ch != 2)
    {
        error_msg = "WAV: unsupported channels: " + std::to_string(num_ch);
        return false;
    }
    return true;
}

void set_sd_volume(int volume)
{
    s_volume = volume;
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
