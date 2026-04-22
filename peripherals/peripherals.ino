#include <TimerOne.h>
#include <Wire.h>

// arduino-cli sucks, so a symlink is needed
#include "i2c_cmd.h"

const int PWM_PINS[] =
{
    3, // Timer2
    5, // Timer0
    6, // Timer0
    9  // Timer1
};

const int SLAVE_ADDRESS = 0x05;

bool debug_on = false;

// Flag to start playing sound
bool do_play = false;

// milliseconds
int blink_interval = 0;
int blink_count = 0;

int buf_index = 0;
char buffer[I2C_BUF_SIZE+1];

void receiveData(int byteCount)
{
    digitalWrite(LED_BUILTIN, 1);
    uint8_t c = Wire.read();
    if (debug_on)
    {
        Serial.print("i2c rcv ");
        Serial.print(byteCount);
        Serial.print(": ");
        Serial.println(c);
    }
    --byteCount;
    switch (c)
    {
    case static_cast<int>(I2c_cmd::Pwm1):
    case static_cast<int>(I2c_cmd::Pwm2):
    case static_cast<int>(I2c_cmd::Pwm3):
    case static_cast<int>(I2c_cmd::Pwm4):
        {
            blink_count = 2;
            blink_interval = 100;
            uint8_t duty = Wire.read();
            uint8_t freq = Wire.read(); // not used yet
            analogWrite(PWM_PINS[c - 10], duty);
            if (debug_on)
            {
                Serial.print("i2c pwm ");
                Serial.print(duty);
                Serial.print(" ");
                Serial.println(freq);
            }
            --byteCount;
        }
        break;

    case static_cast<int>(I2c_cmd::Uart1_tx):
        {
            // Second byte is length
            uint8_t size = Wire.read();
            for (uint8_t i = 0; i < size; ++i)
                Serial.write(Wire.read());
        }
        break;

    default:
        digitalWrite(LED_BUILTIN, 0);
        break;
    }

    while (byteCount--)
        Wire.read();
}

void sendData()
{
    buffer[buf_index++] = 0;
    Wire.write(buffer, buf_index);
    buf_index = 0;
}

void timer_interrupt()
{
    static bool on = false;
    static int ticks_left = 0;

    if (blink_interval <= 0)
        return;
    if (++ticks_left >= blink_interval)
    {
        ticks_left = 0;
        on = !on;
        if (blink_count > 0)
            --blink_count;
        if (blink_count == 0)
            blink_interval = 0;
    }
    digitalWrite(LED_BUILTIN, on);
}

void setup()
{
    Serial.begin(115200);
    //Serial.println("Peripherals v 0.5");

    for (auto pin : PWM_PINS)
        pinMode(pin, OUTPUT);

    Timer1.initialize(1020);
    Timer1.attachInterrupt(timer_interrupt);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
}

void loop()
{
    if (Serial.available())
    {
       char c = Serial.read();
       Serial.print("char: ");
       Serial.println(c);
       if (buf_index >= I2C_BUF_SIZE)
       {
           Serial.println("Error: Line too long");
           buf_index = 0;
           return;
       }
       buffer[buf_index++] = c;
    }
}

// To flash:
// arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano:cpu=atmega328old

// Local Variables:
// compile-command: "arduino-cli compile --fqbn arduino:avr:nano
// End:
