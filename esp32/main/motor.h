#pragma once

#include <driver/ledc.h>

#include <memory>

class Motor
{
public:
    Motor(ledc_timer_t _timer,
          ledc_channel_t _channel_a,
          ledc_channel_t _channel_b,
          gpio_num_t _pwm_a,
          gpio_num_t _pwm_b,
          unsigned _frequency = 1000);

    // -1.0 to 1.0
    void set_speed(float speed);

private:
    ledc_timer_t timer = (ledc_timer_t) 0;
    ledc_channel_t channel_a = (ledc_channel_t) 0;
    ledc_channel_t channel_b = (ledc_channel_t) 0;
    float last_speed = 0.0;
    uint32_t last_millis = 0;
};

// The threshold at which the pivot action starts
// This threshold is measured in units on the Y-axis
// away from the X-axis (Y=0). A greater value will assign
// more of the joystick's range to pivot actions.
const int PIVOT = 35;

void compute_power(int rx, int ry, int& power_left, int& power_right, double pivot = PIVOT, int max_power = 100);

// -1 to +1
void set_motors(double m1, double m2);

extern std::unique_ptr<Motor> motor_a;
extern std::unique_ptr<Motor> motor_b;
