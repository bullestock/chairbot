#include <stdio.h>
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

#include "motor.h"

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

    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}