#pragma once

bool motor_init(int& i2c_device);

bool motor_set(int i2c_device, int power_left, int power_right);
