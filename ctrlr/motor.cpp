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
