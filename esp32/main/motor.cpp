#include "motor.h"

#include <math.h>

#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

Motor::Motor(mcpwm_unit_t _unit,
             mcpwm_timer_t _timer,
             mcpwm_io_signals_t _pwm_a,
             mcpwm_io_signals_t _pwm_b,
             int _gpio_a, int _gpio_b)
    : unit(_unit),
      timer(_timer)
{
    mcpwm_gpio_init(unit, _pwm_a, _gpio_a);
    mcpwm_gpio_init(unit, _pwm_b, _gpio_b);
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;
    pwm_config.cmpr_a = 0;
    pwm_config.cmpr_b = 0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(unit, timer, &pwm_config);  
}

void Motor::set_speed(float speed)
{
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

void compute_power(int rx, int ry, int& power_left, int& power_right, int pivot, int max_power)
{
    rx = apply_s_curve(rx);
    ry = apply_s_curve(ry);
                
    int nMotPremixL = 0;
    int nMotPremixR = 0;
    // Calculate Drive Turn output due to Joystick X input
    if (ry >= 0)
    {
        // Forward
        nMotPremixL = rx >= 0 ? max_range : max_range + rx;
        nMotPremixR = rx >= 0 ? max_range - rx : max_range;
    }
    else
    {
        // Reverse
        nMotPremixL = rx >= 0 ? max_range - rx : max_range;
        nMotPremixR = rx >= 0 ? max_range : max_range + rx;
    }

    // Scale Drive output due to Joystick Y input (throttle)
    nMotPremixL = nMotPremixL * ry/(max_range+1.0);
    nMotPremixR = nMotPremixR * ry/(max_range+1.0);

    // Now calculate pivot amount
    // - Strength of pivot (nPivSpeed) based on Joystick X input
    // - Blending of pivot vs drive (fPivScale) based on Joystick Y input
    const auto nPivSpeed = rx;
    const auto fPivScale = abs(ry) > pivot ? 0.0 : 1.0 - abs(ry)/float(pivot);

    // Calculate final mix of Drive and Pivot and convert to motor PWM range
    power_left = int(-((1.0-fPivScale)*nMotPremixL + fPivScale*(nPivSpeed))/float(max_range)*max_power);
    power_right = int(-((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power);
}
