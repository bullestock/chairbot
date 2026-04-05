#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

// Motor control

static const auto GPIO_PWM0A_OUT = GPIO_NUM_5;
static const auto GPIO_PWM0B_OUT = GPIO_NUM_18;
static const auto GPIO_PWM1A_OUT = GPIO_NUM_33;
static const auto GPIO_PWM1B_OUT = GPIO_NUM_23;
static const auto GPIO_ENABLE = GPIO_NUM_26;

// Battery voltage measurement

static const auto GPIO_V_BAT = GPIO_NUM_35;

// I2C

static const auto GPIO_SDA = GPIO_NUM_21;
static const auto GPIO_SCL = GPIO_NUM_22;

// I2S

static const auto GPIO_I2S_BCK = GPIO_NUM_19;
static const auto GPIO_I2S_DIN = GPIO_NUM_25;
static const auto GPIO_I2S_SRCK = GPIO_NUM_27;

// SD card
static const auto GPIO_SD_CS = GPIO_NUM_4;
static const auto GPIO_SD_MOSI = GPIO_NUM_16;
static const auto GPIO_SD_SCK = GPIO_NUM_32;
static const auto GPIO_SD_MISO = GPIO_NUM_17;

static const auto max_radio_idle_time = 200/portTICK_PERIOD_MS;

// NVS keys

constexpr const char* MAC_KEY = "mac";
constexpr const char* MY_MAC_KEY = "mmac";

constexpr const char* TAG = "CHB";
