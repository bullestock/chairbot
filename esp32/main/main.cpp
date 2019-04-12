#include <stdio.h>
#include <string.h>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <driver/adc.h>

#include "esp_wifi.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "RF24.h"

#include "motor.h"
#include "radio.h"


#define GPIO_INTERNAL_LED    GPIO_NUM_22

#define GPIO_PWM0A_OUT  5
#define GPIO_PWM0B_OUT 18
#define GPIO_PWM1A_OUT 19
#define GPIO_PWM1B_OUT 23

Motor* motor_a = nullptr;
Motor* motor_b = nullptr;

// -1 to +1
void set_motors(double m1, double m2)
{
    motor_a->set_speed(m1);
    motor_b->set_speed(-m2);
}

void main_loop(void* pvParameters)
{
    int loopcount = 0;
#define SHOW_DEBUG() 0 //((my_state == RUNNING) && (loopcount == 0))

#if 0
    // Motor test
    while (1)
    {
        printf("*** MOTOR TEST ***\n");
        set_motors(0, 0);
        for (int i = -100; i < 100; i++)
        {
            printf("%d\n", i);
            set_motors(0, i/100.0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(100/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        set_motors(0, 0);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        for (int i = -100; i < 100; i++)
        {
            printf("%d\n", i);
            set_motors(i/100.0, 0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(100/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
#endif

    
    while (1)
    {
        ++loopcount;
        if (loopcount >= 100)
            loopcount = 0;
        
        vTaskDelay(1/portTICK_PERIOD_MS);

        //xTaskNotifyWait(0, 0, NULL, 1);
    }
}

static TaskHandle_t xMainTask = nullptr;

static const uint8_t pipes[][6] = {"1BULL","2BULL"};

extern "C"
void app_main()
{
    motor_a = new Motor(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, MCPWM0B, GPIO_PWM0A_OUT, GPIO_PWM0B_OUT);
    motor_b = new Motor(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM1A, MCPWM1B, GPIO_PWM1A_OUT, GPIO_PWM1B_OUT);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << GPIO_INTERNAL_LED;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

#define SPI_SCLK     4
#define SPI_MISO    27
#define SPI_MOSI     2
#define SPI_CS      GPIO_NUM_16
#define SPI_CE      GPIO_NUM_17

    RF24 radio(SPI_CE, SPI_CS);
    assert(radio_init(radio));

	while (1)
	{
        // if there is data ready
        if (radio.available())
        {
            printf("received!\n");
#if 0
            last_packet = chrono::steady_clock::now();

            ForwardAirFrame frame;
            // Fetch the payload, and see if this was the last one.
            while (radio.available())
                radio.read(&frame, sizeof(frame));

            if (frame.magic != ForwardAirFrame::MAGIC_VALUE)
            {
                cerr << "Bad magic value; expected " << ForwardAirFrame::MAGIC_VALUE
                     << ", got " << frame.magic << endl;
                continue;
            }

            if (!check_crc(frame))
            {
                cerr << "Bad CRC" << endl;
                continue;
            }

            // Echo back tick value so we can compute round trip time
            radio.stopListening();

            ReturnAirFrame ret_frame;
            ret_frame.magic = ReturnAirFrame::MAGIC_VALUE;
            ret_frame.ticks = frame.ticks;
            battery_readings[battery_reading_index] = motor_get_battery(motor_device);
            ++battery_reading_index;
            if (battery_reading_index >= NOF_BATTERY_READINGS)
                battery_reading_index = 0;
            int n = 0;
            int32_t sum = 0;
            for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
                if (battery_readings[i])
                {
                    sum += battery_readings[i];
                    ++n;
                }
            // Round to nearest 0.1 V to prevent flickering
            ret_frame.battery = n ? 100*((sum/n+50)/100) : 0;
            set_crc(ret_frame);
            radio.write(&ret_frame, sizeof(ret_frame));

            radio.startListening();

            frame.right_x = 1023 - frame.right_x; // hack!

#define PUSH(bit)   (is_pushed(frame, bit) ? '1' : '0')
#define TOGGLE(bit) (is_toggle_down(frame, bit) ? 'D' : (is_toggle_up(frame, bit) ? 'U' : '-'))
                
            const int rx = frame.right_x - right_x_zero;
            const int ry = frame.right_y - right_y_zero;

            // Map right pot (0-255) to pivot value (20-51)
            const int pivot = frame.right_pot*2;
            // Map left pot (0-255) to max_power (20-255)
            max_power = static_cast<int>(20 + (256-20)/256.0*frame.left_pot);
            compute_power(rx, ry, power_left, power_right, pivot, max_power);
            ++count;
            if (count > 10)
            {
                count = 0;
                cerr << "L " << setw(4) << frame.left_x << "/" << setw(4) << frame.left_y
                     << " R " << setw(4) << frame.right_x << "/" << setw(4) << frame.right_y << " (" << rx << "/" << ry << ")"
                     << " P " << setw(3) << int(frame.left_pot) << "/" << setw(3) << int(frame.right_pot)
                     << " Push " << PUSH(0) << PUSH(1) << PUSH(2) << PUSH(3)
                     << " Toggle " << TOGGLE(0) << TOGGLE(1) << TOGGLE(2) << TOGGLE(3)
                     << " Power " << power_left << "/" << power_right << endl;
                if (is_toggle_up(frame, 3))
                    signal_control_lights(signal_device, true, true);
                else if (is_toggle_down(frame, 3))
                    signal_control_lights(signal_device, true, false);
                else
                    signal_control_lights(signal_device, false, true);
            }
            
            motor_set(motor_device, power_left, power_right);
            is_halted = false;
            
            if (is_pushed(frame, 0))
                signal_play_sound(signal_device, -1);
#endif
        }
        else
        {
#if 0
            // No data from radio
            const auto cur_time = chrono::steady_clock::now();
            if ((cur_time - last_packet > max_radio_idle_time) && !is_halted)
            {
                is_halted = true;
                cerr << "HALT: Last packet was seen at " << last_packet.time_since_epoch().count() << endl;
                first_reading = true;
                motor_set(motor_device, 0, 0);
            }
#endif
        }

#if 0
        // Change LED state every 3 seconds
        const auto cur_time = chrono::steady_clock::now();
        if (cur_time - last_led_flip > chrono::seconds(3))
        {
            last_led_flip = cur_time;
            led_state = !led_state;
            signal_control_led(signal_device, led_state);
        }
#endif
    }

    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}

// Local Variables:
// compile-command: "(cd ..; make)"
// End:
