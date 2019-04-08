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

#include "driver/spi_master.h"

#include "motor.h"
#include "nrf24l01p_lib.h"

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

#define pin_SCK      4
#define pin_MISO    27
#define pin_MOSI     2
#define pin_SS      16
#define pin_CE      GPIO_NUM_17

    printf("CE high\n");
    //vTaskDelay(1000/portTICK_PERIOD_MS);
    
    // Set CE high
    //gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << pin_CE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(pin_CE, 1);

    printf("SPI setup\n");
    //vTaskDelay(1000/portTICK_PERIOD_MS);

    // Set up SPI
    esp_err_t ret;
    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.miso_io_num = pin_MISO;
    buscfg.mosi_io_num = pin_MOSI;
    buscfg.sclk_io_num = pin_SCK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 32*8; //in bits (this is for 32 bytes. adjust as needed)
    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    //this is for 1MHz. adjust as needed
    devcfg.clock_speed_hz = 1*1000*1000;    // nRF24L01 allows up to 8 MHz
    devcfg.mode = 0;
    devcfg.spics_io_num =  pin_SS;
    devcfg.queue_size = 3;//how many transactions will be queued at once
    devcfg.command_bits = 8;
    devcfg.address_bits = 0;
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0); // no DMA
    printf("spi_bus_initialize: %d\n", ret);
    //vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(ret);
    spi_device_handle_t spi = 0;
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    printf("spi_bus_add_device: %d\n", ret);
    //vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(ret);
    
    printf("SPI setup done\n");
    vTaskDelay(1000/portTICK_PERIOD_MS);

    for (int n = 0; n < 32; ++n)
    {
        printf("read reg %d\n", n);
        //vTaskDelay(1000/portTICK_PERIOD_MS);

        uint8_t rx_data[10];
        memset(rx_data, 0, sizeof(rx_data));
        uint8_t tx_data[10];
        memset(tx_data, 0xFF, sizeof(tx_data));

        spi_transaction_t trans_desc;
        memset(&trans_desc, 0, sizeof(trans_desc));
        trans_desc.flags = 0;
        trans_desc.cmd = n; // 000A AAAA
        trans_desc.rx_buffer = rx_data;
        trans_desc.tx_buffer = tx_data;
        trans_desc.length = 5*8;
        trans_desc.rxlength = 0;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans_desc));

        for (int i = 0; i < 5; ++i)
        {
            printf("%02X ", rx_data[i]);
        }
        printf("\n");
    }

    
    xTaskCreate(&main_loop, "Main loop", 10240, NULL, 1, &xMainTask);
}

// Local Variables:
// compile-command: "(cd ..; make)"
// End:
