#include "peripherals.h"

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/i2c.h>
#include <freertos/queue.h>

#include "battery.h"
#include "config.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate

adc_oneshot_unit_handle_t adc_handle = 0;
adc_cali_handle_t adc_cali_handle = 0;
bool adc_do_calibration = false;

static QueueHandle_t sound_queue;

struct Queue_item
{
    enum class Type
    {
        Sound,
        Volume,
        Pwm
    } type = Type::Sound;
    int param1 = 0;
    int param2 = 0;
};

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                 adc_atten_t atten, adc_cali_handle_t& out_handle)
{
    adc_cali_handle_t handle = 0;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        printf("calibration scheme version is Curve Fitting\n");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
            calibrated = true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        printf("calibration scheme version is Line Fitting\n");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
            calibrated = true;
    }
#endif

    out_handle = handle;
    if (ret == ESP_OK)
        printf("Calibration Success\n");
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
        printf("eFuse not burnt, skip software calibration\n");
    else
        printf("Invalid arg or no memory\n");

    return calibrated;
}

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

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_7, &config));
    
    adc_do_calibration = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_7, ADC_ATTEN_DB_12,
                                              adc_cali_handle);
}

const int i2c_address = 5;

void peripherals_play_sound(int sound)
{
    Queue_item i;
    i.type = Queue_item::Type::Sound;
    i.param1 = sound;
    xQueueSend(sound_queue, &i, 0);
}

void peripherals_do_play_sound(int sound)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_address << 1 | I2C_MASTER_WRITE, 1);
    const uint8_t data = (uint8_t) sound;
    i2c_master_write_byte(cmd, 1, 1); // 1: Play sound
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error [sound]: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error [sound]: Write failed: %d", ret);
    i2c_cmd_link_delete(cmd);        
}

void peripherals_set_volume(int volume)
{
    Queue_item i;
    i.type = Queue_item::Type::Volume;
    i.param1 = volume;
    xQueueSend(sound_queue, &i, 0);
}

void peripherals_do_set_volume(int volume)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_address << 1 | I2C_MASTER_WRITE, 1);
    const uint8_t data = (uint8_t) volume;
    i2c_master_write_byte(cmd, 4, 1); // 4: Set volume
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error [volume]: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error [volume]: Write failed: %d", ret);
    i2c_cmd_link_delete(cmd);        
}

void peripherals_set_pwm(int chan, int value)
{
    Queue_item i;
    i.type = Queue_item::Type::Pwm;
    i.param1 = chan;
    i.param2 = value;
    xQueueSend(sound_queue, &i, 0);
}

void peripherals_do_set_pwm(int chan, int value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, i2c_address << 1 | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, 10 + chan, 1); // 10-13: Set PWM output
    i2c_master_write_byte(cmd, (uint8_t) value, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error [pwm]: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error [pwm]: Write failed: %d", ret);
    i2c_cmd_link_delete(cmd);        
}

void sound_loop(void*)
{
    // Create a queue capable of containing 10 items.
    sound_queue = xQueueCreate(10, sizeof(Queue_item));

    while (1)
    {
        Queue_item i;
        if (xQueueReceive(sound_queue, &i, 0))
            switch (i.type)
            {
            case Queue_item::Type::Sound:
                peripherals_do_play_sound(i.param1);
                break;

            case Queue_item::Type::Volume:
                peripherals_do_set_volume(i.param1);
                break;

            case Queue_item::Type::Pwm:
                peripherals_do_set_pwm(i.param1, i.param2);
                break;
            }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
