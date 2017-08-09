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
#include <unistd.h>
#include <RF24/RF24.h>

#include "../../remote/firmware/protocol.h"

using namespace std;

RF24 radio(22, 0);

bool radioNumber = 1;

/********************************/

// Radio pipe addresses for the 2 nodes to communicate.
const uint8_t pipes[][6] = {"1Node","2Node"};


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
	
	radio.startListening();
	
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
            }
        }
        delay(10);
    }
}
