#pragma once

// The threshold at which the pivot action starts
// This threshold is measured in units on the Y-axis
// away from the X-axis (Y=0). A greater value will assign
// more of the joystick's range to pivot actions.
const int PIVOT = 35;

bool motor_init(int& i2c_device);

bool motor_set(int i2c_device, int power_left, int power_right);

void compute_power(int rx, int ry, int& power_left, int& power_right, int pivot = PIVOT);
