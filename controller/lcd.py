import serial, time

class LcdDriver:
    
    global no_display
    no_display = False
    WAIT_TIME = 0.04
    COLUMNS = 16

    def __init__(self, disabled, serial_port = "/dev/lcdsmartie"):
        self.disabled = disabled
        if not self.disabled:
            self.lcd = serial.Serial(serial_port, 9600,
                                     serial.EIGHTBITS,
                                     serial.PARITY_NONE,
                                     serial.STOPBITS_ONE,
                                     timeout = 5,
                                     rtscts = False)

    def backlight_off(self):
        if not self.disabled:
            command = ["\xFE","\x46","\xFE","\x46"]
            for item in command:
                lcd.write(item)
            time.sleep(self.WAIT_TIME)
   
    def backlight_on(self):
        if not self.disabled:
            command = ["\xFE","\x42","\x00","\xFE","\x42","\x00"]
            for item in command:
                lcd.write(item)   
            time.sleep(self.WAIT_TIME)
   
    def write_line(self, data, line):
        if not self.disabled:
            data = data.ljust(self.COLUMNS)
            data = data [:self.COLUMNS]
            command = ["\xFE", "\x47", "\x01", "\x01" if line == 1 else "\x02", data]   
            for item in command:
                self.lcd.write(item)
            time.sleep(self.WAIT_TIME)

if __name__ == "__main__":
    l = LcdDriver(False)
    for x in range(0, LcdDriver.COLUMNS):
        s = '-'*x + '|' + '-'*(LcdDriver.COLUMNS-1-x)
        l.write_line(s, 1)
        s = '.'*x + '*' + '.'*(LcdDriver.COLUMNS-1-x)
        l.write_line(s, 2)
        time.sleep(0.2)
    l.write_line(LcdDriver.COLUMNS*' ', 1)
    l.write_line(LcdDriver.COLUMNS*' ', 2)
        
