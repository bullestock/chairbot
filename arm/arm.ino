#include <Servo.h>
#include <Wire.h>

const int SERVO_PINS[] = { 3, 5, 6, 9 };

const int BUF_SIZE = 200;

const int SLAVE_ADDRESS = 0x05;

int curr_pos[] = { 150, 80, 50, 90 };
int limits[][2] = {
    { 0, 180 },
    // Base
    { 0, 180},
    { 0, 180 },  // 125: Horizontal
    { 0, 180 }
};

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

Servo* servos[] = { &servo1, &servo2, &servo3, &servo4 };

const int NOF_AXES = sizeof(servos)/sizeof(servos[0]);

int index = 0;
char buffer[BUF_SIZE];

enum State
{
    STATE_OK = 0,
    STATE_UNKNOWN_COMMAND,
    STATE_WRONG_BYTECOUNT,
    STATE_INVALID_AXIS
};

int i2c_state = STATE_OK;

void receiveData(int byteCount)
{
    int c = Wire.read();
    switch (c)
    {
    case 0:
        // Read status
        if (byteCount != 1)
        {
            while (--byteCount)
                Wire.read();
            i2c_state = STATE_WRONG_BYTECOUNT;
        }
        break;

    case 1:
        {
            // Move to position
            if (byteCount != 3)
            {
                while (--byteCount)
                    Wire.read();
                return;
            }
            const int axis = Wire.read();
            if (axis >= NOF_AXES)
            {
                i2c_state = STATE_INVALID_AXIS;
                return;
            }
            int value = Wire.read();
            if (value < limits[axis][0])
            {
                value = limits[axis][0];
            }
            if (value > limits[axis][1])
            {
                value = limits[axis][1];
            }
            curr_pos[axis] = value;
            i2c_state = STATE_OK;
        }
        break;

    default:
        while (--byteCount)
            Wire.read();
        i2c_state = STATE_UNKNOWN_COMMAND;
        break;
    }
}

void sendData()
{
    Wire.write(i2c_state);
}

void setup()
{
    for (size_t i = 0; i < sizeof(servos)/sizeof(servos[0]); ++i)
    {
        pinMode(SERVO_PINS[i], OUTPUT);
        servos[i]->write(curr_pos[i]);
        servos[i]->attach(SERVO_PINS[i]);
    }
    Serial.begin(57600);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
}

int get_int(const char* buffer, int len)
{
    char intbuf[BUF_SIZE];
    memcpy(intbuf, buffer, max(BUF_SIZE-1, len));
    intbuf[len] = 0;
    return atoi(intbuf);
}

int get_int(const char* buffer, int len, int& next)
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
    char intbuf[BUF_SIZE];
    memcpy(intbuf, buffer, index);
    intbuf[index] = 0;
    next = index+1;
    return atoi(intbuf);
}

void process(const char* buffer)
{
    switch (buffer[0])
    {
        // G: Go to absolute position
    case 'G':
    case 'g':
        {
            int index;
            const int axis = get_int(buffer+1, BUF_SIZE-1, index); 
            int value = get_int(buffer+index, BUF_SIZE-1, index); 
            Serial.print(axis);
            Serial.print(": ");
            Serial.println(value);
            switch (axis)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                if (value < limits[axis][0])
                {
                    Serial.print("Hit lower limit ");
                    Serial.println(limits[axis][0]);
                    value = limits[axis][0];
                }
                if (value > limits[axis][1])
                {
                    Serial.print("Hit upper limit ");
                    Serial.println(limits[axis][1]);
                    value = limits[axis][1];
                }
                servos[axis]->write(value);
                curr_pos[axis] = value;
                break;

            default:
                Serial.println("ERROR: Invalid parameters to 'G'");
                break;
            }
        }
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
    else
    {
        delay(10);
    }
    for (size_t i = 0; i < sizeof(servos)/sizeof(servos[0]); ++i)
    {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(curr_pos[i]);
        servos[i]->write(curr_pos[i]);
    }
}
