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

#include <RF24/RF24.h>

#include "../../remote/firmware/protocol.h"

#include "radio.h"
#include "motor.h"

using namespace std;

RF24 radio(22, 0);

int main(int argc, char** argv)
{
    cout << "Chairbot nRF24 controller\n";

    // Setup and configure rf radio
    if (!radio_init(radio))
    {
        cout << "Could not initialize radio" << endl;
        exit(1);
    }

    int motor_device = 0;
    if (!motor_init(motor_device))
        exit(1);

    int power_left = 0;
    int power_right = 0;

    int left_x_zero = 0;
    int left_y_zero = 0;
    int right_x_zero = 0;
    int right_y_zero = 0;
    bool first_reading = true;
    
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
                continue;
            }

            // Echo back tick value so we can compute round trip time
            radio.stopListening();
				
            radio.write(&frame.ticks, sizeof(frame.ticks));

            radio.startListening();

            if (first_reading)
            {
                // Zero sticks
                left_x_zero = frame.left_x;
                left_y_zero = frame.left_y;
                right_x_zero = frame.right_x;
                right_y_zero = frame.right_y;
                first_reading = false;
            }

#define PUSH(bit)   ((frame.switches & (1 << bit)) ? '1' : '0')
#define TOGGLE(bit)   ((frame.switches & (1 << 8+2*bit)) ? 'D' : ((frame.switches & (2 << 8+2*bit)) ? 'U' : '-'))
                
            cerr << "Left " << frame.left_x << "/" << frame.left_y
                 << " Right " << frame.right_x << " " << frame.right_y
                 << " Pot " << int(frame.left_pot) << "/" << int(frame.right_pot)
                 << " Push " << PUSH(0) << PUSH(1) << PUSH(2) << PUSH(3)
                 << " Toggle " << TOGGLE(0) << TOGGLE(1) << TOGGLE(2) << TOGGLE(3)
                 << endl;
                
            const int rx = frame.right_x - right_x_zero;
            const int ry = frame.right_y - right_y_zero;

            compute_power(rx, ry, power_left, power_right);
            motor_set(motor_device, power_left, power_right);
        }
    }
}
