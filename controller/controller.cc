// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>

#include "joystick.hh"
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

const char* UPDATELCD = "/home/pi/chairbot/updatelcd";

void updatelcd(const string& msg)
{
    ostringstream s;
    s << UPDATELCD << " \"" << msg << '"';
    system(s.str().c_str());
}

void show_voltage(const string& v)
{
   updatelcd(v);
}

int main(int argc, char** argv)
{
    // Create an instance of Joystick
    Joystick* js = nullptr;

    while (!js)
    {
        js = new Joystick("/dev/input/js0");

        // Ensure that it was found and that we can use it
        if (!js->isFound())
        {
            delete js;
            js = nullptr;
            cerr << "Failed to open joystick device" << endl;
            sleep(2);
        }
    }

    const char* portname = "/dev/ardumotor";
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        updatelcd("No motor driver");
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    set_interface_attribs(fd, B57600);
    char buf[30] = { 0 };
    const auto nbytes = read(fd, buf, sizeof(buf)-1);
    buf[nbytes] = 0;
    cout << "BANNER: " << buf << endl;
  
    int x = 0;
    int y = 0;
    int powerL = 0;
    int powerR = 0;
    struct timespec last_tick, last_voltage_update_tick;
    if (clock_gettime(CLOCK_MONOTONIC, &last_tick) ||
        clock_gettime(CLOCK_MONOTONIC, &last_voltage_update_tick))
    {
        cout << "clock_gettime() failed: " << errno << endl;
        return 1;
    }
    
    while (true)
    {
        // Restrict rate
        usleep(1000);

        // Attempt to sample an event from the joystick
        JoystickEvent event;
        if (js->sample(&event))
        {
            if (event.isButton())
            {
                printf("Button %u is %s\n",
                       event.number,
                       event.value == 0 ? "up" : "down");
            }
            else if (event.isAxis() && ((event.number < 4) ||
                                        ((event.number >= 8) && (event.number < 20))))
            {
                //printf("Axis %u is at position %d\n", event.number, event.value);
            }
            if (event.isAxis())
            {
                const int maxRange = 32767;
                switch (event.number)
                {
                case 2:
                    // X
                    x = event.value;
                    break;
                case 3:
                    // Y
                    y = event.value;
                    break;
                }

                // CONFIG
                // - fPivYLimt  : The threshold at which the pivot action starts
                //                This threshold is measured in units on the Y-axis
                //                away from the X-axis (Y=0). A greater value will assign
                //                more of the joystick's range to pivot actions.
                const double fPivYLimit = 8192.0;
			
                // TEMP VARIABLES
                float   nMotPremixL;    // Motor (left)  premixed output        (-128..+127)
                float   nMotPremixR;    // Motor (right) premixed output        (-128..+127)
                int     nPivSpeed;      // Pivot Speed                          (-128..+127)
                float   fPivScale;      // Balance scale b/w drive and pivot    (   0..1   )


                // Calculate Drive Turn output due to Joystick X input
                if (y >= 0)
                {
                    // Forward
                    nMotPremixL = (x >= 0) ? maxRange : (maxRange + x);
                    nMotPremixR = (x >= 0) ? (maxRange - x) : maxRange;
                }
                else
                {
                    // Reverse
                    nMotPremixL = (x>=0)? (maxRange - x) : maxRange;
                    nMotPremixR = (x>=0)? maxRange : (maxRange + x);
                }

                // Scale Drive output due to Joystick Y input (throttle)
                nMotPremixL = nMotPremixL * y/(maxRange+1.0);
                nMotPremixR = nMotPremixR * y/(maxRange+1.0);

                // Now calculate pivot amount
                // - Strength of pivot (nPivSpeed) based on Joystick X input
                // - Blending of pivot vs drive (fPivScale) based on Joystick Y input
                nPivSpeed = x;
                fPivScale = (abs(y) > fPivYLimit) ? 0.0 : (1.0 - abs(y)/fPivYLimit);

                // Calculate final mix of Drive and Pivot and convert to motor PWM range
                powerL = -((1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed))/static_cast<double>(maxRange)*255;
                powerR = -((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/static_cast<double>(maxRange)*255;
            }
        }

        struct timespec cur_tick;
        if (clock_gettime(CLOCK_MONOTONIC, &cur_tick))
        {
            cout << "clock_gettime() failed: " << errno << endl;
            return 1;
        }
        const auto elapsed = (cur_tick.tv_sec - last_tick.tv_sec) + (cur_tick.tv_nsec - last_tick.tv_nsec)/1000000000.0;
        if (elapsed >= 0.2)
        {
            cout << "X " << setw(3) << x << " Y " << setw(3) << y << " L " << setw(3) << powerL << " R " << setw(3) << powerR << endl;
            ostringstream s;
            s << "M " << powerL << " " << powerR << endl;
            write(fd, s.str().c_str(), s.str().size());
            char buf[30] = { 0 };
            const auto nbytes = read(fd, buf, sizeof(buf)-1);
            buf[nbytes] = 0;
            cout << "RESPONSE: " << buf << endl;
            last_tick = cur_tick;
        }
        const auto since_last_voltage_update = (cur_tick.tv_sec - last_voltage_update_tick.tv_sec) + (cur_tick.tv_nsec - last_voltage_update_tick.tv_nsec)/1000000000.0;
        if (since_last_voltage_update >= 60)
        {
            ostringstream s;
            s << "V" << endl;
            tcflush(fd, TCIOFLUSH);
            write(fd, s.str().c_str(), s.str().size());
            char buf[30] = { 0 };
            const auto nbytes = read(fd, buf, sizeof(buf)-1);
            if (nbytes < 1)
            {
                cout << "Error reading battery voltage" << endl;
            }
            else
            {
                buf[nbytes] = 0;
                cout << "Battery voltage " << buf << endl;
                show_voltage(buf);
            }
            last_voltage_update_tick = cur_tick;
        }
    }
}
