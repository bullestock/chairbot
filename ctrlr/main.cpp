/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library
TMRh20 2014 - Updated to work with optimized RF24 Arduino library

 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class on RPi, communicating to an Arduino running
 * the GettingStarted sketch.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <RF24/RF24.h>

#include <linux/i2c-dev.h>

#include "../../remote/firmware/protocol.h"

using namespace std;

RF24 radio(22, 0);

// Radio pipe addresses for the 2 nodes to communicate.
const uint8_t pipes[][6] = {"1Node","2Node"};

const int MOTOR_STATUS = 0x00;
const int MOTOR_PWR = 0x01;
const int MOTOR_VOLTAGE = 0x02;

int main(int argc, char** argv)
{
    cout << "Chairbot nRF24 controller\n";

    // Setup and configure rf radio
    if (!radio.begin())
    {
        cout << "No radio found" << endl;
        exit(1);
    }
    radio.setChannel(108);

    radio.setRetries(15, 15);

    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);

    int i2c_device = open("/dev/i2c-1", O_RDWR);
    if (i2c_device < 0)
    {
        cout << "No I2C device found" << endl;
        exit(1);
    }

    const int motor_addr = 0x04;

    if (ioctl(i2c_device, I2C_SLAVE, motor_addr) < 0)
    {
        cout << "Error setting up I2C: " << strerror(errno) << endl;
        exit(1);
    }

	radio.startListening();

    int powerL = 0;
    int powerR = 0;

    // The threshold at which the pivot action starts
    // This threshold is measured in units on the Y-axis
    // away from the X-axis (Y=0). A greater value will assign
    // more of the joystick's range to pivot actions.
    const int pivot = 35;

	while (1)
	{
        // if there is data ready
        if (radio.available())
        {
            AirFrame frame;
            // Fetch the payload, and see if this was the last one.
            while (radio.available())
                radio.read(&frame, sizeof(frame));

            if (frame.magic != AirFrame::MAGIC_VALUE)
            {
                cerr << "Bad magic value; expected " << AirFrame::MAGIC_VALUE << ", got " << frame.magic << endl;
            }
            else
            {
                // Echo back tick value so we can compute round trip time
                radio.stopListening();
				
                radio.write(&frame.ticks, sizeof(frame.ticks));

                radio.startListening();

                cerr << "Sticks " << frame.left_x << " " << frame.left_y
                     << " " << frame.right_x << " " << frame.right_y << endl;
                
                const int max_power = 100;

                const int max_range = 511;
                const int rx = frame.right_x - max_range;
                const int ry = frame.right_y - max_range;

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
                const auto fPivScale = abs(ry) > pivot ? 0.0 : 1.0 - abs(ry)/float(pivot);

                // Calculate final mix of Drive and Pivot and convert to motor PWM range
                powerL = int(-((1.0-fPivScale)*nMotPremixL + fPivScale*(nPivSpeed))/float(max_range)*max_power);
                powerR = int(-((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power);
            }
        }
        delay(10);

        const uint8_t data[4] = {
            MOTOR_PWR,
            static_cast<uint8_t>((powerL < 0 ? 0x01 : 0) | (powerL < 0 ? 0x02 : 0)),
            static_cast<uint8_t>(abs(powerL)),
            static_cast<uint8_t>(abs(powerR))
        };
        //cout << powerL << "/" << powerR << endl;
        if (write(i2c_device, data, sizeof(data)) != sizeof(data))
            cout << "Error writing I2C: " << strerror(errno) << endl;
    }
}
