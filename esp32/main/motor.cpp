#include "motor.h"

#include "config.h"

#include <math.h>
#include <sys/time.h>

#include <driver/gpio.h>
#include <driver/ledc.h>

Motor::Motor(ledc_timer_t _timer,
             ledc_channel_t _channel_a,
             ledc_channel_t _channel_b,
             gpio_num_t _pwm_a,
             gpio_num_t _pwm_b,
             unsigned _frequency)
    : timer(_timer),
      channel_a(_channel_a),
      channel_b(_channel_b)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << _pwm_a) | (1ULL << _pwm_b),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = _timer,
        .freq_hz = _frequency,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t pwm_channel = {
        .gpio_num = _pwm_a,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = _channel_a,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = _timer,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&pwm_channel);

    pwm_channel.gpio_num = _pwm_b;
    pwm_channel.channel = _channel_b;
    ledc_channel_config(&pwm_channel);
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
#if DEBUG_CLAMPING
        const auto initial_delta = delta;
        bool clamped = false;
#endif
        while (delta/elapsed > MAX_DELTA && fabs(speed) > 0.01)
        {
            speed *= 0.9;
            delta = fabs(speed - last_speed);
#if DEBUG_CLAMPING
            clamped = true;
#endif
        }
#if DEBUG_CLAMPING
        if (clamped)
            printf("Delta %f T %d ms, clamp to %f\n",
                   initial_delta, (int) elapsed, speed);
#endif
    }
    last_speed = speed;
    last_millis = millis;

    ledc_channel_t c1 = channel_a;
    ledc_channel_t c2 = channel_b;
    if (speed < 0)
    {
        std::swap(c1, c2);
        speed = -speed;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, c2, speed*255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, c2);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, c1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, c1);
}

static const int max_range = 4096;

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

void compute_power(int rx, int ry, int& power_left, int& power_right, float pivot, int max_power)
{
    // Map relative X/Y to (-1, 1)
    const float x = -rx/static_cast<float>(max_range/2.0) * pivot/100;
    const float y = ry/static_cast<float>(max_range/2.0);

    // Convert from Cartesian to polar
    const float r = hypot(x, y);
    float t = atan2(y, x);

    // Rotate by 45 degrees to change coordinate system,
    // then by 90 degrees to swap axes
    t += M_PI / 4 + M_PI / 2;

    // Convert back to cartesian
    float left = r * cos(t);
    float right = r * sin(t);

    // rescale the new coords
    left = left * sqrt(2);
    right = right * sqrt(2);

    // clamp to (-1, 1)
    left = std::max<float>(-1.0, std::min<float>(left, 1.0));
    right = std::max<float>(-1.0, std::min<float>(right, 1.0));

    // Convert to percent
    power_left = static_cast<int>(left*100);
    power_right = static_cast<int>(right*100);
    
    const int min_power = 7;
    if (abs(power_left) < min_power)
        power_left = 0;
    if (abs(power_right) < min_power)
        power_right = 0;
}

void set_motors(double m1, double m2)
{
    motor_a->set_speed(m1);
    motor_b->set_speed(m2);
    gpio_set_level(GPIO_ENABLE, fabs(m1) > 0.001 || fabs(m2) > 0.001);
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
