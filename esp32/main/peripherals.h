#pragma once

#include <driver/i2c_master.h>

void sound_loop(void*);

void init_peripherals();

bool peripherals_present();

void peripherals_play_sound(int bank);

void peripherals_set_volume(int bank);

void peripherals_set_pwm(int chan, int brightness);
 
extern i2c_master_bus_handle_t i2c_bus_handle;
