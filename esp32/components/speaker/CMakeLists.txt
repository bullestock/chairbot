
idf_component_register(SRCS I2SSpeaker.cpp MP3Decoder.cpp MP3Player.cpp
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_timer driver esp-libhelix-mp3)
