#include "battery.h"

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>

extern adc_oneshot_unit_handle_t adc_handle;
extern adc_cali_handle_t adc_cali_handle;
extern bool adc_do_calibration;

Battery::Battery()
{
}

float Battery::read_voltage() const
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_7, &raw));
    int voltage = raw;
    if (adc_do_calibration)
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage));
    return voltage * 0.010968279735 * 1.00413736036;
}
