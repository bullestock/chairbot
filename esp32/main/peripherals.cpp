#include "peripherals.h"

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <I2SSpeaker.h>

#include "battery.h"
#include "config.h"

const int I2C_ADDRESS = 5;

adc_oneshot_unit_handle_t adc_handle = 0;
adc_cali_handle_t adc_cali_handle = 0;
bool adc_do_calibration = false;
static bool is_peripherals_present = false;

i2c_master_bus_handle_t i2c_bus_handle;

static i2c_master_dev_handle_t i2c_handle;

static QueueHandle_t queue;

struct Queue_item
{
    enum class Type
    {
        Pwm
    } type = Type::Pwm;
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
    // Outputs
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set
    io_conf.pin_bit_mask = 1ULL << GPIO_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_SDA,
        .scl_io_num = GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
    };
    i2c_mst_config.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    i2c_device_config_t i2c_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &i2c_cfg, &i2c_handle));

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

    // Check if peripherals board is present on the I2C bus
    uint8_t byte = 0; // 0: Noop
    const auto ret = i2c_master_transmit(i2c_handle, &byte, 1, 50);
    is_peripherals_present = ret == ESP_OK;
    printf("Peripherals present: %s\n", is_peripherals_present ? "yes" : "no");
}

bool peripherals_present()
{
    return is_peripherals_present;
}

void peripherals_set_pwm(int chan, uint8_t duty, uint8_t freq)
{
    static uint8_t last_duty[4];
    static uint8_t last_freq[4];
    if (duty == last_duty[chan] && freq == last_freq[chan])
        return;
    last_duty[chan] = duty;
    last_freq[chan] = freq;

    Queue_item i;
    i.type = Queue_item::Type::Pwm;
    i.param1 = chan;
    i.param2 = duty | (freq << 8);
    xQueueSend(queue, &i, 0);
}

void peripherals_do_set_pwm(int chan, uint8_t duty, uint8_t freq)
{
    uint8_t bytes[3];
    bytes[0] = 10 + chan; // 10-13: Set PWM output
    bytes[1] = duty;
    bytes[2] = freq;
    
    const auto ret = i2c_master_transmit(i2c_handle, bytes, sizeof(bytes), 50);
    if (ret == ESP_ERR_TIMEOUT)
        printf("Error [pwm]: Bus is busy\n");
    else if (ret != ESP_OK)
        printf("Error [pwm]: Write failed: %d\n", ret);
}

I2SSpeaker speaker(GPIO_I2S_DATA, GPIO_I2S_CLK, GPIO_I2S_WS);

void peripherals_loop(void*)
{
    const auto ret = speaker.init();
    if (ret != ESP_OK)
        printf("Error [i2s]: init failed: %d\n", ret);
    
    // Create a queue capable of containing 10 items.
    queue = xQueueCreate(10, sizeof(Queue_item));

    while (1)
    {
        Queue_item i;
        if (xQueueReceive(queue, &i, 0))
            switch (i.type)
            {
            case Queue_item::Type::Pwm:
                peripherals_do_set_pwm(i.param1,
                                       i.param2 & 0xFF,
                                       (i.param2 >> 8) & 0xFF);
                break;
            }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
