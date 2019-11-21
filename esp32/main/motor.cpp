#include "motor.h"

#include "config.h"

#include <math.h>
#include <sys/time.h>

#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

Motor::Motor(mcpwm_unit_t _unit,
             mcpwm_timer_t _timer,
             mcpwm_io_signals_t _pwm_a,
             mcpwm_io_signals_t _pwm_b,
             int _gpio_a, int _gpio_b,
             int _frequency)
    : unit(_unit),
      timer(_timer)
{
    mcpwm_gpio_init(unit, _pwm_a, _gpio_a);
    mcpwm_gpio_init(unit, _pwm_b, _gpio_b);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = _frequency;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(unit, timer, &pwm_config);  
}

// Max 10 percent per second = 0.0001 per millisecond
const float MAX_DELTA = 0.002;

void Motor::set_speed(float speed)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    const uint32_t millis = tv.tv_sec + tv.tv_usec/1000;
    if (fabs(speed) > fabs(last_speed))
    {
        const auto elapsed = millis - last_millis;
        auto delta = fabs(speed - last_speed);
        const auto initial_delta = delta;
        bool clamped = false;
        while (delta/elapsed > MAX_DELTA && fabs(speed) > 0.01)
        {
            speed *= 0.9;
            delta = fabs(speed - last_speed);
            clamped = true;
        }
        if (clamped)
            printf("Delta %f T %d ms, clamp to %f\n", initial_delta, elapsed, speed);
    }
    last_speed = speed;
    last_millis = millis;

    if (speed >= 0)
    {
        mcpwm_set_signal_low(unit, timer, MCPWM_OPR_B);
        mcpwm_set_duty(unit, timer, MCPWM_OPR_A, speed*100);
        mcpwm_set_duty_type(unit, timer, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    }
    else
    {
        speed = -speed;
        mcpwm_set_signal_low(unit, timer, MCPWM_OPR_A);
        mcpwm_set_duty(unit, timer, MCPWM_OPR_B, speed*100);
        mcpwm_set_duty_type(unit, timer, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
    }
}

static const int max_range = 512;

int apply_s_curve(int x)
{
    if (x >= max_range)
        x = max_range-1;
    else if (x <= -max_range)
        x = -(max_range-1);

    const auto max_logit = 6.93;

    const auto scaled = 0.5 + abs(x)/static_cast<double>(2*max_range);
    // Logit function
    return -log(1.0/scaled - 1)*(x > 0 ? max_range/max_logit : -max_range/max_logit);
}

void compute_power(int rx, int ry, int& power_left, int& power_right, double pivot, int max_power)
{
    if (pivot < 0.1)
        pivot = 0.1;
    // rx = apply_s_curve(rx);
    // ry = apply_s_curve(ry);
    rx = -rx;
                
    int premix_l = 0;
    int premix_r = 0;
    // Calculate Drive Turn output due to Joystick X input
    if (ry >= 0)
    {
        // Forward
        premix_l = rx >= 0 ? max_range : max_range + rx;    // max_range
        premix_r = rx >= 0 ? max_range - rx : max_range;    // max_range
    }
    else
    {
        // Reverse
        premix_l = rx >= 0 ? max_range - rx : max_range;
        premix_r = rx >= 0 ? max_range : max_range + rx;
    }

    // Scale Drive output due to Joystick Y input (throttle)
    premix_l = premix_l * ry/(max_range+1.0);   // 0
    premix_r = premix_r * ry/(max_range+1.0);   // 0

    // Now calculate pivot amount
    // - Strength of pivot (nPivSpeed) based on Joystick X input
    // - Blending of pivot vs drive (fPivScale) based on Joystick Y input
    const auto piv_speed = rx;  // 0
    float piv_diff = abs(ry)/pivot;
    auto piv_scale = abs(ry) > pivot ? 0.0 : 1.0 - piv_diff;
    if (piv_scale > 1.0)
        piv_scale = 1.0;

    // Calculate final mix of Drive and Pivot and convert to motor PWM range
    power_left = int(-((1.0-piv_scale)*premix_l + piv_scale*(piv_speed))/float(max_range)*max_power);
    power_right = int(-((1.0-piv_scale)*premix_r + piv_scale*(-piv_speed))/float(max_range)*max_power);
}

void set_motors(double m1, double m2)
{
    motor_a->set_speed(m1);
    motor_b->set_speed(m2);
    gpio_set_level(GPIO_ENABLE, fabs(m1) > 0.001 || fabs(m2) > 0.001);
}
