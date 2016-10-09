# Released by rdb under the Unlicense (unlicense.org)
# Based on information from:
# https://www.kernel.org/doc/Documentation/input/joystick-api.txt

import os, time, sys, getopt, smbus
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

MOTOR_ADDRESS = 4
ARM_ADDRESS = 5
                             
MOTOR_STATUS = 0x00
MOTOR_PWR = 0x01
MOTOR_VOLTAGE = 0x02

ARM_STATUS = 0x00
ARM_GOTO = 0x01

def set_servo(bus, axis, value):
    try:
        bus.write_i2c_block_data(ARM_ADDRESS, ARM_GOTO, [ axis, int(value) ])
        response = bus.read_byte_data(ARM_ADDRESS, ARM_STATUS)
        #print("ARM RESPONSE: %x" % response)
    except IOError:
        print("Arm IOError")

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

    bus = None
    if not no_serial:
        bus = smbus.SMBus(1)

    # Reopen LCD port
    lcd = LcdDriver(no_display)

    max_power = 100
    min_power = 5
    # Powerup levels
    powerup_map = [ 100, 150, 200, 255 ]
    powerup_duration = 30 # seconds

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
    gripper = 90
    gripper_min = 75
    gripper_max = 150

    set_servo(bus, 1, arm_rotation)
    set_servo(bus, 0, arm_lift1)
    set_servo(bus, 2, arm_lift2)

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
    pivot = 9000

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

    open_pressed = False
    close_pressed = False
    pivot_down_pressed = False
    pivot_up_pressed = False
    
    global status
    status = 'RUN'

    stop_mode_active = False
    powerup_level = 0
    powerup_level_max = 2
    powerup_time = time.time()
    
    # Main event loop
    while True:
        cur_time = time.time()

        # Check signal files from web server
        
        if stop_mode_active:
            print("In stop mode")
            if not os.path.exists('../html/files/STOP'):
                update_lcd(lcd, "Resuming")
                stop_mode_active = False
            update_lcd(lcd, "* STOPPED *")
            continue
        elif os.path.exists('../html/files/STOP'):
            update_lcd(lcd, "* STOPPED *")
            stop_mode_active = True
            print("Entering stop mode")

        if os.path.exists('../html/files/POWERUP'):
            update_lcd(lcd, "* POWERUP! *")
            print("Powerup")
            os.remove('../html/files/POWERUP')
            if powerup_level < len(powerup_map)-1:
                powerup_level = powerup_level + 1
                powerup_time = cur_time
                print("Powerup level: %d" % powerup_level)

        if powerup_level > 0:
            powerup_left = powerup_duration - (cur_time - powerup_time)
            if powerup_left > 0:
                update_lcd(lcd, "POWERUP: %d %d" % (powerup_level, int(powerup_left)))
            else:
                update_lcd(lcd, "POWERUP END")
                powerup_level = 0
            old_max = max_power
            max_power = powerup_map[powerup_level]
            if max_power != old_max:
                print("Max power now %d" % max_power)

        elapsed = cur_time - last_event_time
        limit = 0.5 if ry == 0 else 2.0
        if (elapsed >= limit) and ((rx != 0) or (ry != 0)):
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
                elif button_name == 'top2':
                    open_pressed = pressed
                elif button_name == 'base':
                    close_pressed = pressed
                elif button_name == 'base3':
                    pivot_down_pressed = pressed
                elif button_name == 'base4':
                    pivot_up_pressed = pressed

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
                        set_servo(bus, 2, arm_lift2)
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
                            print("Restricting arm 1: %d" % arm_lift1)
                            set_servo(bus, 0, arm_lift1)

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
                        set_servo(bus, 0, arm_lift1)

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
            fPivScale = 0.0 if abs(ry) > pivot else 1.0 - abs(ry)/float(pivot)

            # Calculate final mix of Drive and Pivot and convert to motor PWM range
            powerL = int(-((1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed))/float(max_range)*max_power)
            powerR = int(-((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*max_power)

        # Done every time   

        if (abs(powerL) < min_power) and (abs(powerR) < min_power):
            #if (abs(powerL) > 0) or (abs(powerR) > 0):
            #   print("Below minimum: %d/%d" % (powerL, powerR))
            powerL = 0
            powerR = 0
            
        elapsed = cur_time - last_motor_update_time
        if elapsed >= 0.2:
            try:
                signs = (1 if powerL < 0 else 0) | (2 if powerR < 0 else 0)
                data = [signs, abs(powerL), abs(powerR)]
                bus.write_i2c_block_data(MOTOR_ADDRESS, MOTOR_PWR, data)
                response = bus.read_byte_data(MOTOR_ADDRESS, MOTOR_STATUS)
                #print("MOTOR RESPONSE: %x" % response)
            except IOError:
                print("Exception: IOError")
            except:
                print("Exception: ")
                print(data)
                raise
            last_motor_update_time = cur_time
            if powerup_level == 0:
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
                set_servo(bus, 1, arm_rotation)

        if open_pressed or close_pressed:
            # Move gripper
            old_gripper = gripper
            step = 3 if open_pressed else -3
            gripper = gripper + step
            if gripper < gripper_min:
                gripper = gripper_min
            if gripper > gripper_max:
                gripper = gripper_max
            if gripper != old_gripper:
                set_servo(bus, 3, gripper)

        if False:
            if pivot_down_pressed or pivot_up_pressed:
                # Adjust pivot
                if pivot_down_pressed:
                    pivot = pivot - pivot_step
                    if pivot < pivot_min:
                        pivot = pivot_min
                if pivot_up_pressed:
                    pivot = pivot + pivot_step
                    if pivot > pivot_max:
                        pivot = pivot_max
                update_lcd(lcd, "PIVOT %d" % pivot)

        if not no_serial:
            since_last_voltage_update = cur_time - last_voltage_update_time
            if since_last_voltage_update >= 10:
                try:
                    v_data = bus.read_word_data(MOTOR_ADDRESS, MOTOR_VOLTAGE)
                    voltage = "%.2f" % (v_data/100.0)
                except IOError:
                    print "IOError reading battery voltage"
                last_voltage_update_time = cur_time

if __name__ == "__main__":
    main(sys.argv[1:])
