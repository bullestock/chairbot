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
#include <thread>

#include <boost/program_options.hpp>

#include <errno.h>
#include <unistd.h>

#include <RF24/RF24.h>

#include "../../remote/firmware/protocol.h"

#include "radio.h"
#include "motor.h"
#include "signal.h"

using namespace std;
namespace po = boost::program_options;

const auto max_radio_idle_time = chrono::milliseconds(150);
                                            
RF24 radio(22, 0);

bool is_pushed(const ForwardAirFrame& frame, int button)
{
    return frame.switches & (1 << button);
}

bool is_toggle_up(const ForwardAirFrame& frame, int sw)
{
    return frame.switches & (2 << 8+2*sw);
}

bool is_toggle_down(const ForwardAirFrame& frame, int sw)
{
    return frame.switches & (1 << 8+2*sw);
}

int main(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
       ("help,h", "produce help message")
       ("motortest,m", "exercise motors")
       ("soundtest,s", "play a sound")
       ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << "\n";
        return 1;
    }

    int motor_device = 0;
    if (!motor_init(motor_device))
        exit(1);

    if (vm.count("motortest"))
    {
        cout << "Left" << endl;
        for (int i = 0; i < 256; ++i)
        {
            motor_set(motor_device, i, 0);
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        cout << "Right" << endl;
        for (int i = 0; i < 256; ++i)
        {
            motor_set(motor_device, 0, i);
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        return 0;
    }

    int signal_device = 0;
    if (!signal_init(signal_device))
        exit(1);

    if (vm.count("soundtest"))
    {
        cout << "Play random sound" << endl;
        signal_play_sound(signal_device, -1);
        return 0;
    }
    
    cout << "Chairbot nRF24 controller\n";

    // Setup and configure radio
    if (!radio_init(radio))
    {
        cout << "Could not initialize radio" << endl;
        exit(1);
    }

    int power_left = 0;
    int power_right = 0;

    int left_x_zero = 512;
    int left_y_zero = 512;
    int right_x_zero = 512;
    int right_y_zero = 512;
    bool first_reading = true;

    auto last_packet = chrono::steady_clock::now();

    const int NOF_BATTERY_READINGS = 100;
    int32_t battery_readings[NOF_BATTERY_READINGS];
    int battery_reading_index = 0;
    for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
        battery_readings[i] = 0;

    const int max_power = static_cast<int>(0.35*255);
    
    int count = 0;
    bool led_state = false;
    auto last_led_flip = chrono::steady_clock::now();
    bool is_halted = false;
	while (1)
	{
        // if there is data ready
        if (radio.available())
        {
            last_packet = chrono::steady_clock::now();

            ForwardAirFrame frame;
            // Fetch the payload, and see if this was the last one.
            while (radio.available())
                radio.read(&frame, sizeof(frame));

            if (frame.magic != ForwardAirFrame::MAGIC_VALUE)
            {
                cerr << "Bad magic value; expected " << ForwardAirFrame::MAGIC_VALUE
                     << ", got " << frame.magic << endl;
                continue;
            }

            if (!check_crc(frame))
            {
                cerr << "Bad CRC" << endl;
                continue;
            }

            // Echo back tick value so we can compute round trip time
            radio.stopListening();

            ReturnAirFrame ret_frame;
            ret_frame.magic = ReturnAirFrame::MAGIC_VALUE;
            ret_frame.ticks = frame.ticks;
            battery_readings[battery_reading_index] = motor_get_battery(motor_device);
            ++battery_reading_index;
            if (battery_reading_index >= NOF_BATTERY_READINGS)
                battery_reading_index = 0;
            int n = 0;
            int32_t sum = 0;
            for (int i = 0; i < NOF_BATTERY_READINGS; ++i)
                if (battery_readings[i])
                {
                    sum += battery_readings[i];
                    ++n;
                }
            // Round to nearest 0.1 V to prevent flickering
            ret_frame.battery = n ? 100*((sum/n+50)/100) : 0;
            set_crc(ret_frame);
            radio.write(&ret_frame, sizeof(ret_frame));

            radio.startListening();

            frame.right_x = 1023 - frame.right_x; // hack!

#if 0
            if (first_reading)
            {
                // Zero sticks
                left_x_zero = frame.left_x;
                left_y_zero = frame.left_y;
                right_x_zero = frame.right_x;
                right_y_zero = frame.right_y;
                first_reading = false;
            }
#endif
	   
#define PUSH(bit)   (is_pushed(frame, bit) ? '1' : '0')
#define TOGGLE(bit) (is_toggle_down(frame, bit) ? 'D' : (is_toggle_up(frame, bit) ? 'U' : '-'))
                
            const int rx = frame.right_x - right_x_zero;
            const int ry = frame.right_y - right_y_zero;

            // Map right pot (0-255) to pivot value (20-51)
            const int pivot = 10 + frame.right_pot/2;
            compute_power(rx, ry, power_left, power_right, pivot, max_power);
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
                if (is_toggle_up(frame, 3))
                    signal_control_lights(signal_device, true, true);
                else if (is_toggle_down(frame, 3))
                    signal_control_lights(signal_device, true, false);
                else
                    signal_control_lights(signal_device, false, true);
            }
            
            motor_set(motor_device, power_left, power_right);
            is_halted = false;
            
            if (is_pushed(frame, 0))
                signal_play_sound(signal_device, -1);
        }
        else
        {
            // No data from radio
            const auto cur_time = chrono::steady_clock::now();
            if ((cur_time - last_packet > max_radio_idle_time) && !is_halted)
            {
                is_halted = true;
                cerr << "HALT: Last packet was seen at " << last_packet.time_since_epoch().count() << endl;
                first_reading = true;
                motor_set(motor_device, 0, 0);
            }
        }

        // Change LED state every 3 seconds
        const auto cur_time = chrono::steady_clock::now();
        if (cur_time - last_led_flip > chrono::seconds(3))
        {
            last_led_flip = cur_time;
            led_state = !led_state;
            signal_control_led(signal_device, led_state);
        }
    }
}
