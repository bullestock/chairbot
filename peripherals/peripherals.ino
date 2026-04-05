#include <TimerOne.h>
#include <Wire.h>

enum class I2c_cmd
{
    Pwm1 = 10,
    Pwm2,
    Pwm3,
    Pwm4,
    Uart1 = 20,
};
    
const int PWM_PINS[] =
{
    3, // Timer2
    5, // Timer0
    6, // Timer0
    9  // Timer1
};
const int SLAVE_ADDRESS = 0x05;

bool debug_on = true;

// Flag to start playing sound
bool do_play = false;

// milliseconds
int blink_interval = 0;
int blink_count = 0;

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

    default:
        digitalWrite(LED_BUILTIN, 0);
        break;
    }

    while (byteCount--)
        Wire.read();
}

void sendData()
{
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
    Serial.println("Peripherals v 0.5");

    for (auto pin : PWM_PINS)
        pinMode(pin, OUTPUT);

    Timer1.initialize(1020);
    Timer1.attachInterrupt(timer_interrupt);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
}

int get_int(const char* buffer, int len, int* next = nullptr)
{
    int index = 0;
    while (buffer[index] && isspace(buffer[index]) && len)
    {
        ++index;
        --len;
    }
    while (buffer[index] && !isspace(buffer[index]) && len)
    {
        ++index;
        --len;
    }
    const int BS = 30;
    if (index >= BS)
    {
        Serial.println("ERROR: Integer too long");
        return -1;
    }
    char intbuf[BS];
    memcpy(intbuf, buffer, index);
    intbuf[index] = 0;
    if (next)
        *next = index+1;
    return atoi(intbuf);
}

int index = 0;
const int BUF_SIZE = 200;
char buffer[BUF_SIZE];

void process(const char* buffer)
{
    switch (buffer[0])
    {
        // O: Control output
    case 'o':
    case 'O':
        {
            blink_count = 2;
            blink_interval = 100;
            int index;
            const int port = get_int(buffer+1, BUF_SIZE-1, &index);
            if (port < 0 || port > (int) (sizeof(PWM_PINS)/sizeof(PWM_PINS[0])))
            {
                Serial.println("ERROR: Invalid port parameter to 'O'");
                return;
            }
            const int value = get_int(buffer+index, BUF_SIZE-1, &index);
            if (value < 0 || value > 255)
            {
                Serial.println("ERROR: Invalid value parameter to 'O'");
                return;
            }
            analogWrite(PWM_PINS[port], value);
            Serial.println("OK");
        }
        break;

    case 'h':
    case 'H':
        Serial.println("Commands:\r\n"
                       "o <idx> <val>\t"        "Set PWM output\r\n"
                       "s\t\t"                    "Stop sound");
        break;
        
    default:
        Serial.println("ERROR: Unknown command");
        break;
    }
}

void loop()
{
    if (Serial.available())
    {
       // Command from PC
       char c = Serial.read();
       if ((c == '\r') || (c == '\n'))
       {
           buffer[index] = 0;
           index = 0;
           process(buffer);
       }
       else
       {
           if (index >= BUF_SIZE)
           {
               Serial.println("Error: Line too long");
               index = 0;
               return;
           }
           buffer[index++] = c;
       }
    }
    
    delay(10);
}

// To flash:
// arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano:cpu=atmega328old

// Local Variables:
// compile-command: arduino-cli compile --fqbn arduino:avr:nano
// End:
