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

/*
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

*/

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
    //!!peripherals_play_sound(sound);
}

const int pwm_lights = 0;
bool is_flashing = false;
int flash_count = 0;
int volume = 10;

void handle_peripherals(const ForwardAirFrame& frame)
{
    int i = 0;
    for (const auto& e : frame.pwm)
        peripherals_set_pwm(i++, e.duty, e.frequency);

    /*
    if (frame.sound == ForwardAirFrame::SOUND_RANDOM)
        play_random_sound(0);
    else if (frame.sound == ForwardAirFrame::SOUND_ABORT)
        abort_sound();
    else if (frame.sound != 0)
        play_sound(frame.sound);
    */
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
        if (frame.magic)
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

    const float MIN_PIVOT = 0.2;
    const float MAX_PIVOT = 1.0;
    const float pivot = 0.5;
    float power_left = 0.0;
    float power_right = 0.0;
    compute_power(frame.right_x, frame.right_y, power_left, power_right,
                  pivot, 1.0);
    static int count = 0;
    ++count;
    if (count > 100)
    {
        count = 0;
        // Max <maxpwr> L <left stick> R <right stick> (<right mapped>) Pot <pots>
        printf("[%" PRId64 "] L %2.3f/%2.3f R %2.3f/%2.3f Pot %1.2f/%1.2f "
               "Power %2.2f/%2.2f Pivot %1.2f\n",
               total_packets,
               frame.left_x, frame.left_y, frame.right_x, frame.right_y,
               frame.volume, frame.analog,
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
