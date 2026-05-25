#include <chrono>
#include <inttypes.h>
#include <random>
#include <stdio.h>
#include <string.h>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "battery.h"
#include "console.h"
#include "i2s_audio.h"
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

const int pwm_lights = 0;
bool is_flashing = false;
int flash_count = 0;
int volume = 10;

static void handle_pwm(const ForwardAirFrame& frame)
{
    peripherals_set_pwm(frame.data.pwm.index, frame.data.pwm.duty, frame.data.pwm.frequency);
}

unsigned long last_packet = 0;
int64_t total_packets = 0;
const int NOF_BATTERY_READINGS = 100;
float battery_readings[NOF_BATTERY_READINGS];
int battery_reading_index = 0;
float max_power = 0.1;

static void handle_battery(const Battery& battery, ReturnAirFrame& ret_frame)
{
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
    ret_frame.data.battery.mV = n ? sum/n*1000 : 0;
}

static void handle_sound(const ForwardAirFrame& frame,
                         ReturnAirFrame& ret_frame)
{
    switch (frame.data.sound.sound_command)
    {
    case SoundCommand::ListSounds:
        {
            ret_frame.command = Command::Sound;
            ret_frame.data.track.track_count = get_sd_track_count();
            const auto req_index = frame.data.sound.index;
            if (req_index >= ret_frame.data.track.track_count)
                printf("Invalid track requested: %u\n", req_index);
            else
            {
                ret_frame.data.track.index = req_index;
                strncpy(ret_frame.data.track.track, sd_get_tracks()[req_index].c_str(), ReturnAirFrame::TRACK_NAME_SIZE);
            }
        }
        break;
        
    case SoundCommand::PlaySound:
        {
            // TODO: Volume
            int index = frame.data.sound.index;
            if (index == ForwardAirFrame::SOUND_RANDOM)
            {
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                std::default_random_engine generator(seed);
                std::uniform_int_distribution<int> distribution(0, get_sd_track_count());
                index = distribution(generator);
                printf("Playing random track %d\n", index);
            }
            else
                printf("Playing track %d\n", index);
            start_sd_playback(frame.data.sound.index);
        }
        break;
        
    case SoundCommand::StopSound:
        stop_sd_playback();
        break;
        
    }
}

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
    ret_frame.command = Command::None;

    switch (frame.command)
    {
    case Command::None:
        break;
        
    case Command::Speed:
        max_power = std::min(1.0, static_cast<float>(frame.data.speed.speed)/4096.0);
        printf("Max power set to %.2f\n", max_power);
        break;

    case Command::Sound:
        handle_sound(frame, ret_frame);
        break;
        
    case Command::Pwm:
        if (peripherals_present())
            handle_pwm(frame);
        break;
        
    case Command::Battery:
        handle_battery(battery, ret_frame);
        break;
    }
    
    set_crc(ret_frame);
    send_frame(ret_frame);

    const float MIN_PIVOT = 0.2;
    const float MAX_PIVOT = 1.0;
    const float pivot = 0.5;
    float power_left = 0.0;
    float power_right = 0.0;
    compute_power(frame.right_x, frame.right_y, power_left, power_right,
                  pivot, max_power);
    static int count = 0;
    ++count;
    if (count > 100)
    {
        count = 0;
        // Max <maxpwr> L <left stick> R <right stick> (<right mapped>)
        printf("[%" PRId64 "] L %2.3f/%2.3f R %2.3f/%2.3f "
               "Power %2.2f/%2.2f Pivot %1.2f\n",
               total_packets,
               frame.left_x, frame.left_y, frame.right_x, frame.right_y,
               power_left, power_right, pivot);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    set_motors(power_left, power_right);
}

void main_loop(void* pvParameters)
{
    int loopcount = 0;

    last_packet = xTaskGetTickCount();
    printf("Start at %lu\n", last_packet);

    for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
        battery_readings[i] = 0.0;
    Battery battery;

    peripherals_blink_led(2);
    
    assert(init_radio());
    printf("Radio initialized\n");

    peripherals_blink_led(3);
    if (!i2s_init())
        peripherals_blink_led(10);
    
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
    vTaskDelay(100/portTICK_PERIOD_MS);
    init_nvs();
    vTaskDelay(100/portTICK_PERIOD_MS);

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
    {
        printf("I2S init: %d\n", i2s_init());

        run_console();        // never returns
    }
    printf("\nStarting application\n");

    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
