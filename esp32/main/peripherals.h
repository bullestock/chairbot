#pragma once

#include <string>

#include <driver/i2c_master.h>

void peripherals_loop(void*);

void init_peripherals();

bool peripherals_present();

void peripherals_set_pwm(int chan, uint8_t duty, uint8_t freq);

void peripherals_write_uart(const char* data);

std::string peripherals_read_uart();

extern i2c_master_bus_handle_t i2c_bus_handle;
