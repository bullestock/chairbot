#pragma once

void sound_loop(void*);

void init_peripherals();

void peripherals_play_sound(int bank);

void peripherals_set_volume(int bank);

void peripherals_set_pwm(int chan, int brightness);
 
