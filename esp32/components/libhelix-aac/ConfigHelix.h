#pragma once

/* Minimal ConfigHelix.h for ESP-IDF (pure C, no Arduino)
 * Only the AAC-related defines are needed for libhelix-aac. */

#ifndef AAC_MAX_OUTPUT_SIZE
#  define AAC_MAX_OUTPUT_SIZE (1024 * 8)
#endif
#ifndef AAC_MAX_FRAME_SIZE
#  define AAC_MAX_FRAME_SIZE 2100
#endif

/* Enable SBR (Spectral Band Replication) for HE-AAC support.
 * SBR doubles the frequency bandwidth of low-bitrate AAC streams
 * (HE-AACv1 = AAC-LC + SBR), but adds ~8KB to the PSInfoBase struct
 * and requires an additional ~50KB PSInfoSBR allocation at decode time.
 *
 * On ESP32-C5 without PSRAM (384KB SRAM, WiFi 6 dual-band uses ~200KB),
 * there is insufficient contiguous heap for PSInfoSBR (~50KB).
 * With PSRAM enabled, SBR stays enabled and HE-AAC streams decode
 * at full sample rate (44100 Hz).
 * Without SBR, AAC-LC decoding still works — audio plays at the base
 * sample rate (e.g., 22050 Hz instead of 44100 Hz for HE-AAC streams).
 *
 * HELIX_DISABLE_SBR is set by CMakeLists.txt for ESP32-C5 builds without PSRAM.
 * The CONFIG_IDF_TARGET_ESP32C5 guard is a fallback in case the CMake
 * definition is missed. */
#if defined(HELIX_DISABLE_SBR) || (defined(CONFIG_IDF_TARGET_ESP32C5) && !defined(CONFIG_SPIRAM))
/* SBR disabled — not enough contiguous heap for PSInfoSBR (~50KB) */
#else
#  ifndef HELIX_FEATURE_AUDIO_CODEC_AAC_SBR
#    define HELIX_FEATURE_AUDIO_CODEC_AAC_SBR
#  endif
#endif
