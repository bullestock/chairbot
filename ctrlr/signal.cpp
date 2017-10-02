#include "signal.h"

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

const int SIGNAL_SOUND_RANDOM = 0x00;
const int SIGNAL_SOUND_SPECIFIC = 0x01;
const int SIGNAL_LIGHTS_STEADY = 0x02;
const int SIGNAL_LIGHTS_FLASHING = 0x03;
const int SIGNAL_LED = 0x04;

bool signal_init(int& i2c_device)
{
    i2c_device = open("/dev/i2c-1", O_RDWR);
    if (i2c_device < 0)
    {
        cout << "No I2C device found" << endl;
        return false;
    }

    const int signal_addr = 0x05;

    if (ioctl(i2c_device, I2C_SLAVE, signal_addr) < 0)
    {
        cout << "Error setting up I2C: " << strerror(errno) << endl;
        return false;
    }        
    return true;
}

bool signal_play_sound(int i2c_device, int sound_index)
{
    if (sound_index >= 0)
    {
        const uint8_t data[2] = {
            SIGNAL_SOUND_SPECIFIC,
            static_cast<uint8_t>(sound_index)
        };
        if (write(i2c_device, data, sizeof(data)) != sizeof(data))
        {
            cout << "Error writing I2C (sound): " << strerror(errno) << endl;
            return false;
        }
    }
    else
    {
        const uint8_t data = SIGNAL_SOUND_RANDOM;
        if (write(i2c_device, &data, sizeof(data)) != sizeof(data))
        {
            cout << "Error writing I2C (sound): " << strerror(errno) << endl;
            return false;

        }
    }
    return true;
}

bool signal_control_lights(int i2c_device, bool on, bool steady)
{
    const uint8_t data[2] = {
        static_cast<uint8_t>(steady ? SIGNAL_LIGHTS_STEADY : SIGNAL_LIGHTS_FLASHING),
        on
    };
    if (write(i2c_device, data, sizeof(data)) != sizeof(data))
    {
        cout << "Error writing I2C (lights): " << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool signal_control_led(int i2c_device, bool on)
{
    const uint8_t data[2] = {
        static_cast<uint8_t>(SIGNAL_LED),
        on
    };
    if (write(i2c_device, data, sizeof(data)) != sizeof(data))
    {
        cout << "Error writing I2C (LED): " << strerror(errno) << endl;
        return false;
    }
    return true;
}
