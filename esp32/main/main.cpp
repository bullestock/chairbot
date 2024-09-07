#include <stdio.h>
#include <string.h>
#include <chrono>
#include <random>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "sdkconfig.h"
#include "nvs_flash.h"

#include "battery.h"
#include "console.h"
#include "motor.h"
#include "peripherals.h"
#include "radio.h"

#include "protocol.h"

#include "config.h"

static const unsigned pwm_freq = 200;

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

void main_loop(void* pvParameters)
{
    int loopcount = 0;
#define SHOW_DEBUG() 0 //((my_state == RUNNING) && (loopcount == 0))


    int power_left = 0;
    int power_right = 0;

#if 0
    int left_x_zero = 512;
    int left_y_zero = 512;
#endif
    int right_x_zero = 512;
    int right_y_zero = 512;
    
    auto last_packet = xTaskGetTickCount();
    printf("Start at %lu\n", last_packet);

    const int NOF_BATTERY_READINGS = 100;
    float battery_readings[NOF_BATTERY_READINGS];
    int battery_reading_index = 0;
    for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
        battery_readings[i] = 0.0;
    Battery battery;

    int max_power = 255;
    
    int count = 0;
    bool led_state = false;
    auto last_led_flip = xTaskGetTickCount();
    bool is_halted = false;

    NRF24_t radio;
    assert(init_radio(radio));
    printf("Radio initialized\n");

    bool is_zeroed = false;
    
    while (1)
    {
        ++loopcount;
        if (loopcount >= 100)
            loopcount = 0;

        ForwardAirFrame frame;
        Nrf24_getData(&radio, reinterpret_cast<uint8_t*>(&frame));
        printf("Payload: %02X  Magic: %04X\n", radio.payload, frame.magic);
        if (frame.magic == ForwardAirFrame::MAGIC_VALUE)
        {
            last_packet = xTaskGetTickCount();

            if (!check_crc(frame))
            {
                printf("Bad CRC\n");
                continue;
            }

            // Echo back tick value so we can compute round trip time

            ReturnAirFrame ret_frame;
            ret_frame.magic = ReturnAirFrame::MAGIC_VALUE;
            ret_frame.ticks = frame.ticks;
#if 1
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
            // Round to nearest 0.1 V to prevent flickering
            //ret_frame.battery = n ? 100*((sum/n+50)/100) : 0;
            ret_frame.battery = n ? sum/n*1000 : 0;
#endif
            set_crc(ret_frame);
            //!!radio.write(&ret_frame, sizeof(ret_frame));

            //!!radio.startListening();

            if (!is_zeroed)
            {
                is_zeroed = true;
                right_x_zero = frame.right_x;
                right_y_zero = frame.right_y;
            }

#define PUSH(bit)   (is_pushed(frame, bit) ? '1' : '0')
#define TOGGLE(bit) (is_toggle_down(frame, bit) ? 'D' : (is_toggle_up(frame, bit) ? 'U' : '-'))

            const int MIN_DELTA = 7;
            int rx = frame.right_x - right_x_zero;
            if (abs(rx) < MIN_DELTA)
                rx = 0;
            int ry = frame.right_y - right_y_zero;
            if (abs(ry) < MIN_DELTA)
                ry = 0;

            // Map right pot (0-255) to pivot value
            const int pivot = 5 + frame.right_pot/4.0;
            // Map left pot (0-255) to max_power (20-255)
            max_power = static_cast<int>(20 + (256-20)/256.0*frame.left_pot);
            compute_power(rx, ry, power_left, power_right, pivot, max_power);
            ++count;
            if (count > 10)
            {
                count = 0;
                // L <left stick> R <right stick> (<right mapped>) Pot <pots>
                printf("L %4d/%4d R %4d/%4d (%d/%d) Pot %3d/%3d Push %c%c%c%c%c%c"
                       " Toggle %c%c%c%c"
                       " Power %d/%d Pivot %d\n",
                       (int) frame.left_x, (int) frame.left_y, (int) frame.right_x, (int) frame.right_y, rx, ry,
                       (int) frame.left_pot, (int) frame.right_pot,
                       PUSH(0), PUSH(1), PUSH(2), PUSH(3), PUSH(4), PUSH(5),
                       TOGGLE(0), TOGGLE(1), TOGGLE(2), TOGGLE(3),
                       power_left, power_right, pivot);
            }
            set_motors(power_left/255.0, power_right/255.0);
            is_halted = false;
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
        else
        {
            // No data from radio
            const auto cur_time = xTaskGetTickCount();
            if ((cur_time - last_packet > max_radio_idle_time) && !is_halted)
            {
                is_halted = true;
                printf("HALT: Last packet was seen at %lu\n", last_packet);
                set_motors(0, 0);
            }
        }

        // Change LED state every 3 seconds
        const auto cur_time = xTaskGetTickCount();
        if (cur_time - last_led_flip > 3000/portTICK_PERIOD_MS)
        {
            printf("flip %d\n", led_state);
            last_led_flip = cur_time;
            led_state = !led_state;
            gpio_set_level(GPIO_INTERNAL_LED, led_state);
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
    xTaskCreate(&sound_loop, "Sound loop", 10240, NULL, 1, &xSoundTask);
    if (debug)
        run_console();        // never returns
    printf("\nStarting application\n");

    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
