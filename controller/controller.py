# Released by rdb under the Unlicense (unlicense.org)
# Based on information from:
# https://www.kernel.org/doc/Documentation/input/joystick-api.txt

import os, struct, array, serial, time, sys, getopt
from fcntl import ioctl
from lcddriver import LcdDriver

# Iterate over the joystick devices.
print('Available devices:')

for fn in os.listdir('/dev/input'):
    if fn.startswith('js'):
        print('  /dev/input/%s' % (fn))

# These constants were borrowed from linux/input.h
axis_names = {
    0x00 : 'x',
    0x01 : 'y',
    0x02 : 'z',
    0x03 : 'rx',
    0x04 : 'ry',
    0x05 : 'rz',
    0x06 : 'trottle',
    0x07 : 'rudder',
    0x08 : 'wheel',
    0x09 : 'gas',
    0x0a : 'brake',
    0x10 : 'hat0x',
    0x11 : 'hat0y',
    0x12 : 'hat1x',
    0x13 : 'hat1y',
    0x14 : 'hat2x',
    0x15 : 'hat2y',
    0x16 : 'hat3x',
    0x17 : 'hat3y',
    0x18 : 'pressure',
    0x19 : 'distance',
    0x1a : 'tilt_x',
    0x1b : 'tilt_y',
    0x1c : 'tool_width',
    0x20 : 'volume',
    0x28 : 'misc',
}

button_names = {
    0x120 : 'trigger',
    0x121 : 'thumb',
    0x122 : 'thumb2',
    0x123 : 'top',
    0x124 : 'top2',
    0x125 : 'pinkie',
    0x126 : 'base',
    0x127 : 'base2',
    0x128 : 'base3',
    0x129 : 'base4',
    0x12a : 'base5',
    0x12b : 'base6',
    0x12f : 'dead',
    0x130 : 'a',
    0x131 : 'b',
    0x132 : 'c',
    0x133 : 'x',
    0x134 : 'y',
    0x135 : 'z',
    0x136 : 'tl',
    0x137 : 'tr',
    0x138 : 'tl2',
    0x139 : 'tr2',
    0x13a : 'select',
    0x13b : 'start',
    0x13c : 'mode',
    0x13d : 'thumbl',
    0x13e : 'thumbr',

    0x220 : 'dpad_up',
    0x221 : 'dpad_down',
    0x222 : 'dpad_left',
    0x223 : 'dpad_right',

    # XBox 360 controller uses these codes.
    0x2c0 : 'dpad_left',
    0x2c1 : 'dpad_right',
    0x2c2 : 'dpad_up',
    0x2c3 : 'dpad_down',
}

axis_map = []
button_map = []

#!! handle failure
# Open the joystick device.
fn = '/dev/input/js0'
print('Opening %s...' % fn)
jsdev = open(fn, 'rb')

# Get the device name.
buf = array.array('c', ['\0'] * 64)
ioctl(jsdev, 0x80006a13 + (0x10000 * len(buf)), buf) # JSIOCGNAME(len)
js_name = buf.tostring()
print('Device name: %s' % js_name)

# Get number of axes and buttons.
buf = array.array('B', [0])
ioctl(jsdev, 0x80016a11, buf) # JSIOCGAXES
num_axes = buf[0]

buf = array.array('B', [0])
ioctl(jsdev, 0x80016a12, buf) # JSIOCGBUTTONS
num_buttons = buf[0]

# Get the axis map.
buf = array.array('B', [0] * 0x40)
ioctl(jsdev, 0x80406a32, buf) # JSIOCGAXMAP

for axis in buf[:num_axes]:
    axis_name = axis_names.get(axis, 'unknown(0x%02x)' % axis)
    axis_map.append(axis_name)

# Get the button map.
buf = array.array('H', [0] * 200)
ioctl(jsdev, 0x80406a34, buf) # JSIOCGBTNMAP

for btn in buf[:num_buttons]:
    btn_name = button_names.get(btn, 'unknown(0x%03x)' % btn)
    button_map.append(btn_name)

print('%d axes found' % num_axes)
print('%d buttons found' % num_buttons)


def update_lcd(lcd, s):
    lcd.write_line(s, 2)
    
def show_voltage(lcd, v):
    update_lcd(v)

global no_motor
no_motor = False

def main(argv):
    no_display = False
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
            global no_motor
            no_motor = True

    lcd = LcdDriver(no_display)
    update_lcd(lcd, "Starting")

    if not no_motor:
        try:
            motor = serial.Serial("/dev/ardumotor", 57600,
                                  serial.EIGHTBITS,
                                  serial.PARITY_NONE,
                                  serial.STOPBITS_ONE,
                                  timeout = 5,
                                  rtscts = False)
        except serial.serialutil.SerialException:
            print("Could not open motor driver")
            update_lcd(lcd, "No motor driver")
            sys.exit()

        banner = motor.readline()
        print("Banner: %s" % banner)

    max_power = 255
    x = 0
    y = 0
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

    #struct timespec last_loop
    first = True
    max_loop_time = 0

    max_range = 32767

    last_motor_update_time = time.time()
    last_voltage_update_time = time.time()

    # Main event loop
    while True:
        time.sleep(0.01)

        evbuf = jsdev.read(8)
        if evbuf:
            time_stamp, value, type, number = struct.unpack('IhBB', evbuf)

            is_initial = type & 0x80

            if type & 0x01:
                button = button_map[number]
                # if button:
                #     if value:
                #         print("%s pressed" % (button))
                #     else:
                #         print("%s released" % (button))

            if type & 0x02:
                # Axis event
                axis = axis_map[number]
                if axis == 'z':
                    print("X: %d" % value)
                    x = value
                if axis == 'rz':
                    print("Y: %d" % value)
                    y = value

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
                powerL = -((1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed))/float(max_range)*255
                powerR = -((1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed))/float(max_range)*255

        # Done every time   

        # Limit power
        powerL = max(-max_power, min(powerL, max_power))
        powerR = max(-max_power, min(powerR, max_power))

        cur_time = time.time()
        elapsed = cur_time - last_motor_update_time
        if elapsed >= 0.1:
            print("X %3d Y %3d L %3d R %3d" % (x, y, powerL, powerR))
            cmd = "M %d %d\n" % (powerL, powerR)
            if no_motor:
                print("MOTOR: %s" % cmd)
            else:
                motor.write(cmd)
                response = motor.readline()
                #print("RESPONSE: %s" % response)
            last_motor_update_time = cur_time

        if not no_motor:
            since_last_voltage_update = cur_time - last_voltage_update_time
            if since_last_voltage_update >= 60:
                motor.write("V\n")
                v_reply = motor.readline().rstrip()
                if not v_reply:
                    print("Error reading battery voltage")
                else:
                    print("Battery voltage %s\n" % v_reply)
                    show_voltage(v_reply)
                last_voltage_update_time = cur_time

if __name__ == "__main__":
    main(sys.argv[1:])

    
