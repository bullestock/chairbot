/* helix_memory.c — ESP-IDF memory allocator for libhelix-aac
 *
 * On chips WITHOUT PSRAM, uses heap_caps_malloc to ensure allocations come
 * from internal SRAM (MALLOC_CAP_INTERNAL).
 *
 * On chips WITH PSRAM (CONFIG_SPIRAM), uses MALLOC_CAP_DEFAULT so allocations
 * can fall back to PSRAM when internal SRAM is fragmented. This is critical
 * on ESP32-C5 where WiFi 6 dual-band leaves only ~12KB largest contiguous
 * internal block — not enough for PSInfoBase (~24KB).
 *
 * Memory budget (approximate):
 *   Without SBR: AACDecInfo (~0.5KB) + PSInfoBase (~24KB) ≈ ~25KB
 *   With SBR:    + sbrWorkBuf (~8KB in PSInfoBase) + PSInfoSBR (~50KB) ≈ ~75KB
 */

#include "utils/helix_memory.h"
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "sdkconfig.h"
void* helix_malloc(int size) {
#if CONFIG_SPIRAM
    /* PSRAM available — use MALLOC_CAP_DEFAULT so large allocations (PSInfoBase,
     * PSInfoSBR) can go to PSRAM when internal SRAM is fragmented. */
    return heap_caps_malloc((size_t)size, MALLOC_CAP_DEFAULT);
#else
    return heap_caps_malloc((size_t)size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
}
void helix_free(void *ptr) {
    heap_caps_free(ptr);
}
#else
void* helix_malloc(int size) {
    return malloc((size_t)size);
}
void helix_free(void *ptr) {
    free(ptr);
}
#endif
