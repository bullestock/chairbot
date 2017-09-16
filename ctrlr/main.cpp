/*
 Copyright (C) 2017 Torsten Martinsen

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <chrono>
#include <cstdlib>
#include <iomanip>
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

const auto max_radio_idle_time = chrono::milliseconds(100);
                                            
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

    auto last_packet = chrono::steady_clock::now();

    int count = 0;
	while (1)
	{
        // if there is data ready
        if (radio.available())
        {
            last_packet = chrono::steady_clock::now();

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
                
            const int rx = frame.right_x - right_x_zero;
            const int ry = frame.right_y - right_y_zero;

            // Map right pot (0-255) to pivot value (20-51)
            const int pivot = 20 + frame.right_pot/8;
            compute_power(rx, ry, power_left, power_right, pivot);
            ++count;
            if (count > 10)
            {
                count = 0;
                cerr << "L " << setw(4) << frame.left_x << "/" << setw(4) << frame.left_y
                     << " R " << setw(4) << frame.right_x << "/" << setw(4) << frame.right_y << " (" << rx << "/" << ry << ")"
                     << " P " << setw(3) << int(frame.left_pot) << "/" << setw(3) << int(frame.right_pot)
                     << " Push " << PUSH(0) << PUSH(1) << PUSH(2) << PUSH(3)
                     << " Toggle " << TOGGLE(0) << TOGGLE(1) << TOGGLE(2) << TOGGLE(3)
                     << " Power " << power_left << "/" << power_right << endl;
            }
            
            motor_set(motor_device, power_left, power_right);
        }
        else
        {
            // No data from radio
            const auto cur_time = chrono::steady_clock::now();
            if (cur_time - last_packet > max_radio_idle_time)
            {
                motor_set(motor_device, 0, 0);
                cerr << "HALT: Last packet was seen at " << last_packet.time_since_epoch().count() << endl;
                first_reading = true;
            }
        }
    }
}
