#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <random>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "battery.h"
#include "console.h"
#include "motor.h"
#include "nvs.h"
#include "peripherals.h"
#include "radio.h"

#include "protocol.h"

#include "config.h"

static const unsigned pwm_freq = 200;
static const int POT_MAX = 3950;

std::unique_ptr<Motor> motor_a;
std::unique_ptr<Motor> motor_b;

bool is_pushed(const ForwardAirFrame& frame, int button)
{
    return frame.pushbuttons & (1 << button);
}

bool is_toggle_up(const ForwardAirFrame& frame, int sw)
{
    return frame.toggles & (1 << 2*sw);
}

bool is_toggle_down(const ForwardAirFrame& frame, int sw)
{
    return frame.toggles & (2 << 2*sw);
}

const int NOF_SOUND_BANKS = 3;
int nof_sounds_per_bank[NOF_SOUND_BANKS] = {
    81,
    3,
    25
};

void play_random_sound(int bank)
{
    if (bank >= NOF_SOUND_BANKS)
        return;
    int lower_bound = 0;
    int nof_sounds = nof_sounds_per_bank[bank];
    int index = 0;
    int count = bank;
    while (count > 0)
    {
        lower_bound += nof_sounds_per_bank[index];
        ++index;
        --count;
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(lower_bound, lower_bound + nof_sounds - 1);
    int sound = distribution(generator);
    printf("Bank %d: [%d %d] -> Sound %d\n", bank,
           lower_bound, lower_bound + nof_sounds - 1,
           sound);
    peripherals_play_sound(sound);
}

const int pwm_lights = 0;
bool is_flashing = false;
int flash_count = 0;
int volume = 10;

void handle_peripherals(const ForwardAirFrame& frame)
{
    if (is_flashing)
    {
        if (++flash_count > 10)
        {
            flash_count = 0;
            is_flashing = false;
        }
        else
        {
            peripherals_set_pwm(pwm_lights, flash_count % 2 ? 255 : 0);
        }
    }
    else
    {
        // Toggle 3: Headlights
        if (is_toggle_up(frame, 3))
            peripherals_set_pwm(pwm_lights, 255);
        else if (is_toggle_down(frame, 3))
            peripherals_set_pwm(pwm_lights, 0);
        else
            peripherals_set_pwm(pwm_lights, 64);
    }
    // Toggle 0: Amp power
    if (is_toggle_up(frame, 0))
        peripherals_set_pwm(3, 255);
    else
        peripherals_set_pwm(3, 0);

    // Push 0, 1, 2: Random sounds
    if (is_pushed(frame, 0))
        play_random_sound(0);
    else if (is_pushed(frame, 1))
        play_random_sound(1);
    else if (is_pushed(frame, 2))
        play_random_sound(2);
    // Push 3: Flash headlights
    else if (is_pushed(frame, 3) && !is_flashing)
        is_flashing = true;
    // Push 4: Volume up
    else if (is_pushed(frame, 4))
    {
        if (volume < 20)
            ++volume;
        peripherals_set_volume(volume);
    }
    // Push 5: Volume down
    else if (is_pushed(frame, 5))
    {
        if (volume > 1)
            --volume;
        peripherals_set_volume(volume);
    }
}

unsigned long last_packet = 0;
int64_t total_packets = 0;
const int NOF_BATTERY_READINGS = 100;
float battery_readings[NOF_BATTERY_READINGS];
int battery_reading_index = 0;

void handle_frame(const ForwardAirFrame& frame,
                  const Battery& battery)
{
    if (frame.magic != ForwardAirFrame::MAGIC_VALUE)
    {
        printf("Bad magic: %04X\n", frame.magic);
        return;
    }

    ++total_packets;

    last_packet = xTaskGetTickCount();

    // Echo back tick value so we can compute round trip time

    ReturnAirFrame ret_frame;
    ret_frame.magic = ReturnAirFrame::MAGIC_VALUE;
    ret_frame.ticks = frame.ticks;

    battery_readings[battery_reading_index] = battery.read_voltage();
    ++battery_reading_index;
    if (battery_reading_index >= NOF_BATTERY_READINGS)
        battery_reading_index = 0;
    int n = 0;
    float sum = 0;
    for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
        if (battery_readings[i])
        {
            sum += battery_readings[i];
            ++n;
        }
    ret_frame.battery = n ? sum/n*1000 : 0;
    
    set_crc(ret_frame);
    send_frame(ret_frame);

#define PUSH(bit)   (is_pushed(frame, bit) ? '1' : '0')
#define TOGGLE(bit) (is_toggle_down(frame, bit) ? 'D' : (is_toggle_up(frame, bit) ? 'U' : '-'))

    // Map right pot (0-1) to pivot value (0.2-1.0)
    const float MIN_PIVOT = 0.2;
    const float MAX_PIVOT = 1.0;
    const float pivot = std::min(MAX_PIVOT,
                                 MIN_PIVOT + (MAX_PIVOT - MIN_PIVOT)*frame.right_pot);
    // Map left pot (0-1) to max_power (0.2-1.0)
    const float MIN_POWER = 0.2;
    const float MAX_POWER = 1.0;
    const float max_power = std::min(MAX_POWER,
                                     MIN_POWER + (MAX_POWER - MIN_POWER)*frame.left_pot);
    float power_left = 0.0;
    float power_right = 0.0;
    compute_power(frame.right_x, frame.right_y, power_left, power_right, pivot, max_power);
    static int count = 0;
    ++count;
    if (count > 100)
    {
        count = 0;
        // Max <maxpwr> L <left stick> R <right stick> (<right mapped>) Pot <pots>
        printf("[%" PRId64 "] Max %1.2f L %2.3f/%2.3f R %2.3f/%2.3f Pot %1.2f/%1.2f "
               "Push %c%c%c%c%c%c "
               "Toggle %c%c%c%c "
               "Power %2.2f/%2.2f Pivot %1.2f\n",
               total_packets,
               max_power,
               frame.left_x, frame.left_y, frame.right_x, frame.right_y,
               frame.left_pot, frame.right_pot,
               PUSH(0), PUSH(1), PUSH(2), PUSH(3), PUSH(4), PUSH(5),
               TOGGLE(0), TOGGLE(1), TOGGLE(2), TOGGLE(3),
               power_left, power_right, pivot);
    }
    set_motors(power_left, power_right);

#if 0
    if (peripherals_present())
#endif
        handle_peripherals(frame);
}

void main_loop(void* pvParameters)
{
    int loopcount = 0;

    last_packet = xTaskGetTickCount();
    printf("Start at %lu\n", last_packet);

    for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
        battery_readings[i] = 0.0;
    Battery battery;

    assert(init_radio());
    printf("Radio initialized\n");

    while (1)
    {
        ++loopcount;
        if (loopcount >= 100)
            loopcount = 0;

#if 0
        if (is_halted())
            set_motors(0, 0);
        else
#endif
        {
            const auto frame = get_received_frame();
            handle_frame(frame, battery);
        }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

static TaskHandle_t xMainTask = nullptr;
static TaskHandle_t xSoundTask = nullptr;

extern "C"
void app_main()
{
    init_peripherals();
    init_nvs();

    motor_a = std::make_unique<Motor>(LEDC_TIMER_0, LEDC_CHANNEL_0, LEDC_CHANNEL_1, GPIO_PWM0A_OUT, GPIO_PWM0B_OUT, pwm_freq);
    motor_b = std::make_unique<Motor>(LEDC_TIMER_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3, GPIO_PWM1A_OUT, GPIO_PWM1B_OUT, pwm_freq);

    printf("Press a key to enter console\n");
    bool debug = false;
    for (int i = 0; i < 20; ++i)
    {
        if (getchar() != EOF)
        {
            debug = true;
            break;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    xTaskCreate(&peripherals_loop, "Sound loop", 10240, NULL, 1, &xSoundTask);
    if (debug)
        run_console();        // never returns
    printf("\nStarting application\n");

    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
