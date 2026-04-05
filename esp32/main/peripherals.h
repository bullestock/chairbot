#pragma once

#include <driver/i2c_master.h>

enum class I2c_cmd
{
    Pwm1 = 10,
    Pwm2,
    Pwm3,
    Pwm4,
    Uart1 = 20,
};
    
void peripherals_loop(void*);

void init_peripherals();

bool peripherals_present();

void peripherals_set_pwm(int chan, uint8_t duty, uint8_t freq);

void peripherals_send_uart(const char* data);

extern i2c_master_bus_handle_t i2c_bus_handle;
