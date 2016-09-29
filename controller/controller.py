# Released by rdb under the Unlicense (unlicense.org)
# Based on information from:
# https://www.kernel.org/doc/Documentation/input/joystick-api.txt

import os, serial, time, sys, getopt
from lcddriver import LcdDriver
from sixaxis import Joystick

global status
status = '---'

def update_lcd(lcd, s):
    global status
    lcd.update(status, s)
    
def show_voltage(lcd, v):
    update_lcd(lcd, v)

global no_serial
no_serial = False

def main(argv):
    no_display = False
    voltage = ''
    try:                                
        opts, args = getopt.getopt(argv, "hdm", ["help", "nodisplay", "nomotor"])
    except getopt.GetoptError:
        usage() 
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage() 
            sys.exit() 
        elif opt == '-d':
            no_display = True
        elif opt == '-m':
            global no_serial
            no_serial = True

    js = Joystick()

    lcd = LcdDriver(no_display)
    update_lcd(lcd, "Starting")

    motor = None
    arm = None
    if not no_serial:
        try:
            port1 = serial.Serial("/dev/ttyUSB1", 57600,
                                  serial.EIGHTBITS,
                                  serial.PARITY_NONE,
                                  serial.STOPBITS_ONE,
                                  timeout = 2,
                                  rtscts = False,
                                  dsrdtr = True)
        except serial.serialutil.SerialException:
            print("Could not open ttyUSB1")
            update_lcd(lcd, "No ttyUSB1")
            sys.exit()

        banner1 = port1.readline()
        print("ttyUSB1 banner: %s" % banner1)
        
        try:
            port2 = serial.Serial("/dev/ttyUSB2", 57600,
                                  serial.EIGHTBITS,
                                  serial.PARITY_NONE,
                                  serial.STOPBITS_ONE,
                                  timeout = 2,
                                  rtscts = False)
        except serial.serialutil.SerialException:
            print("Could not open ttyUSB2")
            update_lcd(lcd, "No ttyUSB2")
            sys.exit()

        banner2 = port2.readline()
        print("ttyUSB2 banner: %s" % banner2)

        if banner1.find("MOTOR") >= 0:
            print "ttyUSB1 is motor controller"
            motor = port1
            arm = port2
        elif banner2.find("MOTOR") >= 0:
            print "ttyUSB1 is motor controller"
            motor = port2
            arm = port1
        else:
            print "No banner found"
            update_lcd(lcd, "No motor")
            sys.exit()

    max_power = 64
    min_power = 5
    x = 0
    y = 0
    last_x = 0
    last_y = 0
    powerL = 0
    powerR = 0

    # The threshold at which the pivot action starts
    # This threshold is measured in units on the Y-axis
    # away from the X-axis (Y=0). A greater value will assign
    # more of the joystick's range to pivot actions.
    pivot = 8192.0

    pivot_min = 100
    pivot_max = 32767-100
    pivot_step = 128

    first = True
    max_loop_time = 0

    max_range = 32767

    last_motor_update_time = time.time()
    last_voltage_update_time = time.time()
    last_event_time = time.time()

    global status
    status = 'RUN'

    # Main event loop
    while True:
        cur_time = time.time()

        elapsed = cur_time - last_event_time
        if (elapsed > 2) and ((x != 0) or (y != 0)):
            status = 'STOP'
            x = 0
            y = 0
            print('STOP')
            
        event = js.get_event()
        if event and event.is_valid():
            status = 'RUN'
            last_event_time = cur_time
            is_button, button_name, pressed = event.get_button()
            # if is_button:
            #     if pressed:
            #         print("%s pressed" % (button_name))
            #     else:
            #         print("%s released" % (button_name))
            
            is_axis, axis_name, value = event.get_axis()
            if is_axis:
                if axis_name == 'z':
                    #print("X: %d" % value)
                    x = value
                if axis_name == 'rz':
                    #print("Y: %d" % value)
                    y = value

        if (last_x != x) or (last_y != y):
            last_x = x
            last_y = y
            
            # Calculate Drive Turn output due to Joystick X input
            if y >= 0:
                # Forward
                nMotPremixL = max_range if x >= 0 else max_range + x
                nMotPremixR = max_range - x if x >= 0 else max_range
            else:
                # Reverse
                nMotPremixL = max_range - x if x >= 0 else max_range
                nMotPremixR = max_range if x >= 0 else max_range + x

            # Scale Drive output due to Joystick Y input (throttle)
            nMotPremixL = nMotPremixL * y/(max_range+1.0)
            nMotPremixR = nMotPremixR * y/(max_range+1.0)

            # Now calculate pivot amount
            # - Strength of pivot (nPivSpeed) based on Joystick X input
            # - Blending of pivot vs drive (fPivScale) based on Joystick Y input
            nPivSpeed = x
            fPivScale = 0.0 if abs(y) > pivot else 1.0 - abs(y)/pivot

            # Calculate final mix of Drive and Pivot and convert to motor PWM range
            powerL = -((1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed))/float(max_range)*max_power
            powerR = -((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power

        # Done every time   

        if (abs(powerL) < min_power) and (abs(powerR) < min_power):
            if (abs(powerL) > 0) or (abs(powerR) > 0):
                print("Below minimum: %d/%d" % (powerL, powerR))
            powerL = 0
            powerR = 0
            
        elapsed = cur_time - last_motor_update_time
        if elapsed >= 0.2:
            print("X %3d Y %3d L %3d R %3d" % (x, y, powerL, powerR))
            cmd = "M %d %d" % (powerL, powerR)
            if no_serial:
                print("MOTOR: %s" % cmd)
                update_lcd(lcd, "MOTOR: %d %d" % (powerL, powerR))
            else:
                motor.write("%s\n" % cmd)
                response = motor.readline()
                #print("RESPONSE: %s" % response)
            last_motor_update_time = cur_time
            show_voltage(lcd, voltage)

        if not no_serial:
            since_last_voltage_update = cur_time - last_voltage_update_time
            if since_last_voltage_update >= 60:
                motor.write("V\n")
                v_reply = motor.readline().rstrip()
                if not v_reply:
                    print("Error reading battery voltage")
                else:
                    print("Battery voltage %s\n" % v_reply)
                    voltage = v_reply
                last_voltage_update_time = cur_time

if __name__ == "__main__":
    main(sys.argv[1:])
