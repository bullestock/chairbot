#pragma once

#include "mirf.h"

class ReturnAirFrame;

bool init_radio(NRF24_t& dev);

bool send_frame(NRF24_t& dev,
                ReturnAirFrame& frame);

bool data_ready(NRF24_t& dev);
