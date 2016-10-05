#include <Servo.h>

const int SERVO_PINS[] = { 3, 5, 6 };

const int BUF_SIZE = 200;

int curr_pos[] = { 80, 150, 50 };
int limits[][2] = {
    // Base
    { 0, 180},
    { 30, 180 },
    { 0, 125 }  // 125: Horizontal
};

Servo servo1;
Servo servo2;
Servo servo3;

Servo* servos[] = { &servo1, &servo2, &servo3 };
int index = 0;
char buffer[BUF_SIZE];

void setup()
{
    for (size_t i = 0; i < sizeof(servos)/sizeof(servos[0]); ++i)
    {
        pinMode(SERVO_PINS[i], OUTPUT);
        servos[i]->write(curr_pos[i]);
        servos[i]->attach(SERVO_PINS[i]);
    }
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
#if 0
            case 3:
                servo4.write(value);
                break;
            case 4:
                servo5.write(value);
                break;
#endif
            default:
                Serial.println("ERROR: Invalid parameters to 'S'");
                break;
            }
        }
        break;

        // M: Move to absolute position
    case 'M':
    case 'm':
        {
            int index;
            const int axis = get_int(buffer+1, BUF_SIZE-1, index); 
            int value = get_int(buffer+index, BUF_SIZE-1, index); 
            if (value < limits[axis][0])
            {
                Serial.print("ARM: Above ");
                Serial.println(limits[axis][0]);
                value = limits[axis][0];
            }
            if (value > limits[axis][1])
            {
                Serial.print("ARM: Below ");
                Serial.println(limits[axis][1]);
                value = limits[axis][1];
            }
            Serial.print(axis);
            Serial.print(": ");
            Serial.println(value);
            switch (axis)
            {
            case 0:
            case 1:
            case 2:
                {
                    const int step = (value > curr_pos[axis]) ? 1 : -1;
                    for (int i = curr_pos[axis]; i != value; i += step)
                    {
                        servos[axis]->write(i);
                        delay(10);
                    }
                    curr_pos[axis] = value;
                }
                break;
#if 0
            case 3:
                servo4.write(value);
                break;
            case 4:
                servo5.write(value);
                break;
#endif
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
