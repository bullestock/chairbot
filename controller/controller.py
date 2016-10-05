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
    update_lcd(lcd, "Starting!")

    motor = None
    arm = None
    if not no_serial:
        try:
            port0 = serial.Serial("/dev/ttyUSB0", 57600,
                                  serial.EIGHTBITS,
                                  serial.PARITY_NONE,
                                  serial.STOPBITS_ONE,
                                  timeout = 2,
                                  rtscts = False,
                                  dsrdtr = True)
        except serial.serialutil.SerialException:
            print("Could not open ttyUSB0")
            update_lcd(lcd, "No ttyUSB0")
            sys.exit()

        banner0 = port0.readline()
        print("ttyUSB0 banner: %s" % banner0)
        
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

        if banner0.find("MOTOR") >= 0:
            print "ttyUSB0 is motor controller, ttyUSB2 is arm"
            motor = port0
            arm = port2
            port1.close
        elif banner1.find("MOTOR") >= 0:
            print "ttyUSB1 is motor controller, ttyUSB2 is arm"
            motor = port1
            arm = port2
            port0.close
        elif banner2.find("MOTOR") >= 0:
            print "ttyUSB1 is motor controller, ttyUSB1 is arm"
            motor = port2
            arm = port1
            port0.close
        else:
            print "No banner found"
            update_lcd(lcd, "No motor")
            sys.exit()

    # Reopen LCD port
    lcd = LcdDriver(no_display)

    max_power = 64
    min_power = 5

    # Arm state
    arm_rotation_min = 0
    arm_rotation_max = 160
    arm_rotation = 87
    arm_lift1_min = 30
    arm_lift1_max = 180
    arm_lift1 = (arm_lift1_min+arm_lift1_max)/2
    arm_lift2_min = 20
    arm_lift2_max = 120
    arm_lift2 = (arm_lift2_min+arm_lift2_max)/2

    if not no_serial:
        cmd = "G 0 %d" % arm_rotation
        arm.write("%s\n" % cmd)
        response = arm.readline()
        print("RESPONSE: %s" % response)
        cmd = "G 1 %d" % arm_lift1
        arm.write("%s\n" % cmd)
        response = arm.readline()
        print("RESPONSE: %s" % response)
        cmd = "G 2 %d" % arm_lift2
        arm.write("%s\n" % cmd)
        response = arm.readline()
        print("RESPONSE: %s" % response)

    # Right X/Y (motors)
    rx = 0
    ry = 0
    last_rx = 0
    last_ry = 0
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

    turn_left_pressed = False
    turn_right_pressed = False
    turn_pressed_time = None

    global status
    status = 'RUN'

    # Main event loop
    while True:
        cur_time = time.time()

        elapsed = cur_time - last_event_time
        if (elapsed > 2) and ((rx != 0) or (ry != 0)):
            status = 'STOP'
            rx = 0
            ry = 0
            print('STOP')
            
        event = js.get_event()
        if event and event.is_valid():
            status = 'RUN'
            last_event_time = cur_time

            # -- BUTTONS --
            
            is_button, button_name, pressed = event.get_button()
            if is_button:
                if button_name == 'base2':
                    turn_left_pressed = pressed
                    if pressed:
                        turn_pressed_time = time.time()
                elif button_name == 'pinkie':
                    turn_right_pressed = pressed
                    if pressed:
                        turn_pressed_time = time.time()

            # -- AXES --
            
            is_axis, axis_name, value = event.get_axis()
            if is_axis:
                if axis_name == 'z':
                    #print("X: %d" % value)
                    rx = value
                elif axis_name == 'rz':
                    #print("Y: %d" % value)
                    ry = value
                elif axis_name == 'x':
                    # Lift arm
                    # Lift or raise arm
                    old_lift = arm_lift2
                    step = value/(max_range*0.2)
                    if value > 0:
                        arm_lift2 = arm_lift2 - step
                        if arm_lift2 < arm_lift2_min:
                            arm_lift2 = arm_lift2_min
                    elif value < 0:
                        arm_lift2 = arm_lift2 - step
                        if arm_lift2 > arm_lift2_max:
                            arm_lift2 = arm_lift2_max
                    if arm_lift2 != old_lift:
                        cmd = "G 2 %d" % arm_lift2
                        #print("ARM 2: %d" % arm_lift2)
                        if no_serial:
                            print("ARM: %s" % cmd)
                        else:
                            arm.write("%s\n" % cmd)
                            response = arm.readline()
                            #print("RESPONSE]: %s" % response)
                        if arm_lift2 <= 35:
                            arm_lift1_min = 90
                            arm_lift1_max = 180
                        elif arm_lift2 <= 70:
                            arm_lift1_min = 50
                            arm_lift1_max = 180
                        else:
                            arm_lift1_min = 30
                            arm_lift1_max = 150
                        old_lift = arm_lift1
                        if arm_lift1 < arm_lift1_min:
                            arm_lift1 = arm_lift1_min
                        if arm_lift1 > arm_lift1_max:
                            arm_lift1 = arm_lift1_max
                        if arm_lift1 != old_lift:
                            cmd = "G 1 %d" % arm_lift1
                            print("Restricting arm 1: %d" % arm_lift1)
                            if not no_serial:
                                arm.write("%s\n" % cmd)
                                response = arm.readline()
                                #print("RESPONSE: %s" % response)

                elif axis_name == 'y':
                    # Lift or raise arm
                    old_lift = arm_lift1
                    step = value/(max_range*0.2)
                    if value > 0:
                        arm_lift1 = arm_lift1 - step
                        if arm_lift1 < arm_lift1_min:
                            arm_lift1 = arm_lift1_min
                    elif value < 0:
                        arm_lift1 = arm_lift1 - step
                        if arm_lift1 > arm_lift1_max:
                            arm_lift1 = arm_lift1_max
                    if arm_lift1 != old_lift:
                        cmd = "G 1 %d" % arm_lift1
                        #print("ARM 1: %d" % arm_lift1)
                        if no_serial:
                            print("ARM: %s" % cmd)
                        else:
                            arm.write("%s\n" % cmd)
                            response = arm.readline()
                            #print("RESPONSE: %s" % response)

                else:
                    print("Axis: %s" % axis_name)

        if (last_rx != rx) or (last_ry != ry):
            last_rx = rx
            last_ry = ry
            
            # Calculate Drive Turn output due to Joystick X input
            if ry >= 0:
                # Forward
                nMotPremixL = max_range if rx >= 0 else max_range + rx
                nMotPremixR = max_range - rx if rx >= 0 else max_range
            else:
                # Reverse
                nMotPremixL = max_range - rx if rx >= 0 else max_range
                nMotPremixR = max_range if rx >= 0 else max_range + rx

            # Scale Drive output due to Joystick Y input (throttle)
            nMotPremixL = nMotPremixL * ry/(max_range+1.0)
            nMotPremixR = nMotPremixR * ry/(max_range+1.0)

            # Now calculate pivot amount
            # - Strength of pivot (nPivSpeed) based on Joystick X input
            # - Blending of pivot vs drive (fPivScale) based on Joystick Y input
            nPivSpeed = rx
            fPivScale = 0.0 if abs(ry) > pivot else 1.0 - abs(ry)/pivot

            # Calculate final mix of Drive and Pivot and convert to motor PWM range
            powerL = -((1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed))/float(max_range)*max_power
            powerR = -((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power

        # Done every time   

        if (abs(powerL) < min_power) and (abs(powerR) < min_power):
            #if (abs(powerL) > 0) or (abs(powerR) > 0):
            #   print("Below minimum: %d/%d" % (powerL, powerR))
            powerL = 0
            powerR = 0
            
        elapsed = cur_time - last_motor_update_time
        if elapsed >= 0.2:
            #print("X %3d Y %3d L %3d R %3d" % (rx, ry, powerL, powerR))
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

        if turn_left_pressed or turn_right_pressed:
            # Turn arm
            old_rot = arm_rotation
            step = 1 if turn_left_pressed else -1
            elapsed = cur_time - turn_pressed_time
            if elapsed > 1:
                step = step * 3
            arm_rotation = arm_rotation + step
            if arm_rotation < arm_rotation_min:
                arm_rotation = arm_rotation_min
            if arm_rotation > arm_rotation_max:
                arm_rotation = arm_rotation_max
            if arm_rotation != old_rot:
                cmd = "G 0 %d" % arm_rotation
                if no_serial:
                    print("ARM: %s" % cmd)
                else:
                    arm.write("%s\n" % cmd)
                    response = arm.readline()
                    #print("RESPONSE: %s" % response)

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
