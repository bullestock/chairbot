#pragma once

#include <stdint.h>

bool signal_init(int& i2c_device);

bool signal_play_sound(int i2c_device, int sound_index);

bool signal_control_lights(int i2c_device, bool on, bool steady);

bool signal_control_led(int i2c_device, bool on);
