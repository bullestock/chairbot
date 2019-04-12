#!/bin/bash
#git clone https://github.com/nRF24/RF24.git
cd RF24
ln -s ../ESP_IDF .
cat<<EOF > utility/includes.h
#ifndef __RF24_INCLUDES_H__
#define __RF24_INCLUDES_H__
#define RF24_ESP_IDF
#include "ESP_IDF/RF24_arch_config.h"
#include "ESP_IDF/RF24_ESP_IDF.h"
#include "ESP_IDF/NRF24_spi.h"
#endif
EOF

cat<<EOF > component.mk
COMPONENT_ADD_INCLUDEDIRS=.
COMPONENT_SRCDIRS=. utility/ESP_IDF
EOF

cd ..
