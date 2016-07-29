#include <Servo.h>

const int SERVO_OUT = 3;

const int BUF_SIZE = 200;

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;
Servo servo5;

int index = 0;
char buffer[BUF_SIZE];

void setup()
{
    pinMode(SERVO_OUT, OUTPUT);
    servo1.attach(SERVO_OUT);
    servo2.attach(SERVO_OUT+1);
    servo3.attach(SERVO_OUT+2);
    servo4.attach(SERVO_OUT+3);
    servo5.attach(SERVO_OUT+4);
    Serial.begin(57600);
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
    case 'S':
        {
            int index;
            const int axis = get_int(buffer+1, BUF_SIZE-1, index); 
            const int value = get_int(buffer+index, BUF_SIZE-1, index); 
            switch (axis)
            {
            case 0:
                servo1.write(value);
                break;
            case 1:
                servo2.write(value);
                break;
            case 2:
                servo3.write(value);
                break;
            case 3:
                servo4.write(value);
                break;
            case 4:
                servo5.write(value);
                break;
            default:
                Serial.println("ERROR: Invalid parameters to 'S'");
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
}
