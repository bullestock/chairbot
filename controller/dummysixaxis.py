import array, serial, time, struct, select
from fcntl import ioctl

class DummyJoystick:
    # These constants were borrowed from linux/input.h
    AXIS_NAMES = {
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

    BUTTON_NAMES = {
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

    def __init__(self, device = '/dev/input/js0'):
        self.counter = 0
        self.axis_map = []
        self.button_map = []

        for axis in range(0, 20):
            axis_name = self.AXIS_NAMES.get(axis, 'unknown(0x%02x)' % axis)
            self.axis_map.append(axis_name)

        for btn in range(0, 0x2c3):
            btn_name = self.BUTTON_NAMES.get(btn, 'unknown(0x%03x)' % btn)
            self.button_map.append(btn_name)

        print(self.button_map)
    class Event:
        def __init__(self, js, evbuf):
            self.type = 0
            self.js = js
            if evbuf:
                self.time_stamp, self.value, self.type, self.number = struct.unpack('IhBB', evbuf)

        def is_valid(self):
            return (self.type != 0)

        def get_button(self):
            if self.type & 0x01:
                return True, self.js.button_map[self.number], self.value
            return False, '', 0

        def get_axis(self):
            if self.type & 0x02:
                return True, self.js.axis_map[self.number], self.value
            return False, '', 0
        
    def get_event(self):
        self.counter = self.counter + 1
        if self.counter == 1:
            event = self.Event(self, None)
            event.type = 1
            event.number = 0x123
            event.value = 0
            return event
        elif self.counter < 50:
            event = self.Event(self, None)
            event.type = 2
            event.number = 0
            event.value = self.counter*100
            return event
        elif self.counter < 51:
            event = self.Event(self, None)
            event.type = 1
            event.number = 0x123
            event.value = 1
            return event
        elif self.counter < 100:
            event = self.Event(self, None)
            event.type = 2
            event.number = 1
            event.value = self.counter*100
            return event
        else:
            self.counter = 0
        return None
