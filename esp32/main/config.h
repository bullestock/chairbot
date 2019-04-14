#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

static const auto GPIO_INTERNAL_LED = GPIO_NUM_22;

static const int GPIO_PWM0A_OUT = 5;
static const int GPIO_PWM0B_OUT = 18;
static const int GPIO_PWM1A_OUT = 19;
static const int GPIO_PWM1B_OUT = 23;

static const auto max_radio_idle_time = 150/portTICK_PERIOD_MS;

