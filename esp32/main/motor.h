#pragma once

#include <driver/mcpwm.h>

class Motor
{
public:
    Motor(mcpwm_unit_t unit,
          mcpwm_timer_t timer,
          mcpwm_io_signals_t pwm_a, mcpwm_io_signals_t pwm_b,
          int gpio_a, int gpio_b);

    // -1.0 to 1.0
    void set_speed(float speed);

private:
    mcpwm_unit_t unit = (mcpwm_unit_t) 0;
    mcpwm_timer_t timer = (mcpwm_timer_t) 0;
};

// The threshold at which the pivot action starts
// This threshold is measured in units on the Y-axis
// away from the X-axis (Y=0). A greater value will assign
// more of the joystick's range to pivot actions.
const int PIVOT = 35;

void compute_power(int rx, int ry, int& power_left, int& power_right, int pivot = PIVOT, int max_power = 100);
