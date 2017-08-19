#include "motor.h"

#include <linux/i2c-dev.h>

#include <cstdlib>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const int MOTOR_STATUS = 0x00;
const int MOTOR_PWR = 0x01;
const int MOTOR_VOLTAGE = 0x02;

// The threshold at which the pivot action starts
// This threshold is measured in units on the Y-axis
// away from the X-axis (Y=0). A greater value will assign
// more of the joystick's range to pivot actions.
const int PIVOT = 35;

bool motor_init(int& i2c_device)
{
    i2c_device = open("/dev/i2c-1", O_RDWR);
    if (i2c_device < 0)
    {
        cout << "No I2C device found" << endl;
        return false;
    }

    const int motor_addr = 0x04;

    if (ioctl(i2c_device, I2C_SLAVE, motor_addr) < 0)
    {
        cout << "Error setting up I2C: " << strerror(errno) << endl;
        return false;
    }        
    return true;
}

bool motor_set(int i2c_device, int power_left, int power_right)
{
    const uint8_t data[4] = {
        MOTOR_PWR,
        static_cast<uint8_t>((power_left < 0 ? 0x01 : 0) | (power_right < 0 ? 0x02 : 0)),
        static_cast<uint8_t>(abs(power_left)),
        static_cast<uint8_t>(abs(power_right))
    };
    if (write(i2c_device, data, sizeof(data)) != sizeof(data))
    {
        cout << "Error writing I2C: " << strerror(errno) << endl;
        return false;
    }
    return true;
}

void compute_power(int rx, int ry, int& power_left, int& power_right)
{
    const int max_power = 100;

    const int max_range = 511;
                
    cout << "RX " << rx << " RY " << ry << endl;

    int nMotPremixL = 0;
    int nMotPremixR = 0;
    // Calculate Drive Turn output due to Joystick X input
    if (ry >= 0)
    {
        // Forward
        nMotPremixL = rx >= 0 ? max_range : max_range + rx;
        nMotPremixR = rx >= 0 ? max_range - rx : max_range;
    }
    else
    {
        // Reverse
        nMotPremixL = rx >= 0 ? max_range - rx : max_range;
        nMotPremixR = rx >= 0 ? max_range : max_range + rx;
    }

    // Scale Drive output due to Joystick Y input (throttle)
    nMotPremixL = nMotPremixL * ry/(max_range+1.0);
    nMotPremixR = nMotPremixR * ry/(max_range+1.0);

    // Now calculate pivot amount
    // - Strength of pivot (nPivSpeed) based on Joystick X input
    // - Blending of pivot vs drive (fPivScale) based on Joystick Y input
    const auto nPivSpeed = rx;
    const auto fPivScale = abs(ry) > PIVOT ? 0.0 : 1.0 - abs(ry)/float(PIVOT);

    // Calculate final mix of Drive and Pivot and convert to motor PWM range
    power_left = int(-((1.0-fPivScale)*nMotPremixL + fPivScale*(nPivSpeed))/float(max_range)*max_power);
    power_right = int(-((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power);
}