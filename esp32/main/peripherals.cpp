#include "peripherals.h"

#include <driver/i2c.h>
#include <driver/adc.h>

#include "battery.h"
#include "config.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate

void init_peripherals()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << GPIO_INTERNAL_LED;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << GPIO_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    ESP_ERROR_CHECK(gpio_set_level(GPIO_ENABLE, 0));

    i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = GPIO_SDA;
	conf.scl_io_num = GPIO_SCL;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));

	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t) ADC_CHANNEL_7, ADC_ATTEN_DB_0);
    adc_chars = (esp_adc_cal_characteristics_t*) calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

const int i2c_address = 5;

const int SOUND_RANDOM = 0x00;

void peripherals_play_sound()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_address << 1 | I2C_MASTER_WRITE, 1);
    const uint8_t data = (uint8_t) SOUND_RANDOM;
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error: Write failed: %d", ret);
    i2c_cmd_link_delete(cmd);        
}

void peripherals_set_pwm(int chan, int value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_address << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, chan == 1 ? 2 : 3, 1);
    i2c_master_write_byte(cmd, (uint8_t) value, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_RATE_MS);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error: Write failed: %d", ret);
    i2c_cmd_link_delete(cmd);        
}
