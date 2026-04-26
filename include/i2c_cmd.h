#pragma once

enum class I2c_cmd
{
    Pwm1 = 10,
    Pwm2,
    Pwm3,
    Pwm4,
    Uart1_tx = 20,
};

static const int I2C_BUF_SIZE = 64;
