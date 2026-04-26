# libhelix-aac - Fixed-point HE-AAC decoder from RealNetworks Helix project
# Ported for ESP-IDF from: https://github.com/pschatzmann/arduino-libhelix
# License: RPSL (RealNetworks Public Source License)

set(HELIX_AAC_SRCS
    "aacdec.c"
    "aactabs.c"
    "bitstream.c"
    "buffers.c"
    "dct4.c"
    "decelmnt.c"
    "dequant.c"
    "fft.c"
    "filefmt.c"
    "helix_memory.c"
    "huffman.c"
    "hufftabs.c"
    "imdct.c"
    "noiseless.c"
    "pns.c"
    "sbr.c"
    "sbrfft.c"
    "sbrfreq.c"
    "sbrhfadj.c"
    "sbrhfgen.c"
    "sbrhuff.c"
    "sbrimdct.c"
    "sbrmath.c"
    "sbrqmf.c"
    "sbrside.c"
    "sbrtabs.c"
    "stproc.c"
    "tns.c"
    "trigtabs.c"
)

idf_component_register(
    SRCS ${HELIX_AAC_SRCS}
    INCLUDE_DIRS "."
)

# USE_DEFAULT_STDLIB: use standard <stdlib.h> instead of Helix-internal hlxclib/stdlib.h.
# This must be a compile definition (not just in aacdec.h) because buffers.c and sbr.c
# check it before including any header that would define it.
target_compile_definitions(${COMPONENT_LIB} PRIVATE USE_DEFAULT_STDLIB)

# Disable SBR on ESP32-C5 without PSRAM — not enough contiguous heap for PSInfoSBR (~50KB).
# When PSRAM is enabled (CONFIG_SPIRAM=y), there IS enough heap so SBR stays enabled
# and HE-AAC streams decode at full sample rate (44100 Hz).
# This is done as a compile definition (not just in ConfigHelix.h) to ensure it
# applies even with stale object files from a previous build.
if(CONFIG_IDF_TARGET_ESP32C5 AND NOT CONFIG_SPIRAM)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE HELIX_DISABLE_SBR)
endif()
