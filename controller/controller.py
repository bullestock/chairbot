# Released by rdb under the Unlicense (unlicense.org)
# Based on information from:
# https://www.kernel.org/doc/Documentation/input/joystick-api.txt

import os, struct, array, serial, time, sys
from fcntl import ioctl

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

lcd = serial.Serial("/dev/lcdsmartie", 9600,
                    serial.EIGHTBITS,
                    serial.PARITY_NONE,
                    serial.STOPBITS_ONE,
                    timeout=5,
                    rtscts = False)
wait = 0.04

def BacklightOff():
   command = ["\xFE","\x46","\xFE","\x46"]
   for item in command:
      lcd.write(item)
   time.sleep(wait)
   
def BacklightOn():
   command = ["\xFE","\x42","\x00","\xFE","\x42","\x00"]
   for item in command:
      lcd.write(item)   
   time.sleep(wait)
   
def Writeline(data, line):
   data = data.ljust(16)
   data = data [:16]
   command = ["\xFE", "\x47", "\x01", "\x01" if line == 1 else "\x02", data]   
   for item in command:
      lcd.write(item)
   time.sleep(wait)

def UpdateLcd(s):
    Writeline(s, 2)
    
def show_voltage(v):
    UpdateLcd(v)

Writeline("Starting", 2)

try:
    motor = serial.Serial("/dev/ardumotor", 57600,
                          serial.EIGHTBITS,
                          serial.PARITY_NONE,
                          serial.STOPBITS_ONE,
                          timeout = 5,
                          rtscts = False)
except serial.serialutil.SerialException:
    print("Could not open motor driver")
    UpdateLcd("No motor driver")
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

last_tick = time.time()
last_voltage_update_tick = time.time()

# Main event loop
while True:
    time.sleep(0.1)

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

    cur_tick = time.time()
    elapsed = cur_tick - last_tick
    if elapsed >= 0.5:
        print("X %3d Y %3d L %3d R %3d" % (x, y, powerL, powerR))
        cmd = "M %d %d\n" % (powerL, powerR)
        motor.write(cmd)
        response = motor.readline()
        #print("RESPONSE: %s" % response)
        last_tick = cur_tick

    since_last_voltage_update = cur_tick - last_voltage_update_tick
    if since_last_voltage_update >= 60:
        motor.write("V\n")
        v_reply = motor.readline().rstrip()
        if not v_reply:
            print("Error reading battery voltage")
        else:
            print("Battery voltage %s\n" % v_reply)
            show_voltage(v_reply)
        last_voltage_update_tick = cur_tick

    
