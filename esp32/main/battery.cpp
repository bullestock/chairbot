#include "battery.h"

#include <driver/adc.h>

esp_adc_cal_characteristics_t* adc_chars = nullptr;

Battery::Battery()
{
}

float Battery::read_voltage() const
{
    auto adc_reading = adc1_get_raw((adc1_channel_t) ADC_CHANNEL_7);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    return voltage * 0.010968279735 * 1.00413736036;
}
