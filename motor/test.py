import smbus
import time

MOTOR_ADDRESS = 4
                             
MOTOR_STATUS = 0x00
MOTOR_PWR = 0x01
MOTOR_VOLTAGE = 0x02

# for RPI version 1, use "bus = smbus.SMBus(0)"
bus = smbus.SMBus(1)

def sm(bus, powerL, powerR):
    signs = (1 if powerL < 0 else 0) | (2 if powerR < 0 else 0)
    data = [signs, abs(powerL), abs(powerR)]
    for i in range(1, 3):
        try:
            bus.write_i2c_block_data(MOTOR_ADDRESS, MOTOR_PWR, data)
            return
        except IOError:
            print("Retry %d" % i)

delay = 0.05
for i in range(1, 5):
    print("Left")
    for powerL in range(-255, 255):
        sm(bus, powerL, 0)
        time.sleep(delay)
    sm(bus, 0, 0)
    time.sleep(1)
    print("Right")
    for powerR in range(-255, 255):
        sm(bus, 0, powerR)
        time.sleep(delay)

sm(bus, 0, 0)
    