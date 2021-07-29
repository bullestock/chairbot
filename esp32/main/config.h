#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

static const auto GPIO_INTERNAL_LED = GPIO_NUM_22;

// Motor control

static const int GPIO_PWM0A_OUT = 5;
static const int GPIO_PWM0B_OUT = 18;
static const int GPIO_PWM1A_OUT = 19;
static const int GPIO_PWM1B_OUT = 23;
static const auto GPIO_ENABLE = GPIO_NUM_25;

// Battery voltage measurement

static const auto GPIO_V_BAT = GPIO_NUM_35;

// nRF24L01

static const int SPI_SCLK = 4;
static const int SPI_MISO = 27;
static const int SPI_MOSI = 2;
static const auto SPI_CS = GPIO_NUM_16;
static const auto SPI_CE = GPIO_NUM_17;

// I2C

static const auto GPIO_SDA = GPIO_NUM_21;
static const auto GPIO_SCL = GPIO_NUM_22;

static const auto max_radio_idle_time = 200/portTICK_PERIOD_MS;

