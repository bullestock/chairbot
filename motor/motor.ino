#include <Wire.h>

#define TEST_MODE       0

const int INTERNAL_LED = 13;
const int R_PWM_A = 5;
const int L_PWM_A = 3;
const int R_EN_A = 6;
const int L_EN_A = 4;
const int R_IS_A = 7;
const int L_IS_A = 2;
const int R_PWM_B = 9;
const int L_PWM_B = 11;
const int R_EN_B = A2;
const int L_EN_B = 12;
const int R_IS_B = A3;
const int L_IS_B = 13;
const int BRAKE = 8;
const int V_SENSE = A0;
const int BUF_SIZE = 200;
const int MAX_IDLE_COUNT = 1000;
const int SLAVE_ADDRESS = 0x04;

void run_test();
void set_power(int motor, int power);

void setPwmFrequency(int pin, int divisor)
{
    byte mode;
    if (pin == 5 || pin == 6 || pin == 9 || pin == 10)
    {
        switch (divisor)
        {
        case 1: mode = 0x01; break;
        case 8: mode = 0x02; break;
        case 64: mode = 0x03; break;
        case 256: mode = 0x04; break;
        case 1024: mode = 0x05; break;
        default: return;
        }
        if (pin == 5 || pin == 6)
        {
            TCCR0B = (TCCR0B & 0b11111000) | mode;
        }
        else
        {
            TCCR1B = (TCCR1B & 0b11111000) | mode;
        }
    }
    else if (pin == 3 || pin == 11)
    {
        switch (divisor)
        {
        case 1: mode = 0x01; break;
        case 8: mode = 0x02; break;
        case 32: mode = 0x03; break;
        case 64: mode = 0x04; break;
        case 128: mode = 0x05; break;
        case 256: mode = 0x06; break;
        case 1024: mode = 0x7; break;
        default: return;
        }
        TCCR2B = (TCCR2B & 0b11111000) | mode;
    }
}

enum State
{
    STATE_OK = 0,
    STATE_UNKNOWN_COMMAND,
    STATE_WRONG_BYTECOUNT
};

int i2c_state = STATE_OK;
uint16_t i2c_voltage = 0;
bool reading_voltage = false;

int idle_count = 0;

void receiveData(int byteCount)
{
    idle_count = 0;
    int c = Wire.read();
    switch (c)
    {
    case 0:
        // Read status
        if (byteCount != 1)
        {
            Serial.print("ERROR: Cmd 00 expected 1 byte, got ");
            Serial.println(byteCount);
            while (--byteCount)
                Wire.read();
            i2c_state = STATE_WRONG_BYTECOUNT;
        }
        break;

    case 1:
        {
            // Control motors
            if (byteCount != 4)
            {
                Serial.print("ERROR: Cmd 01 expected 4 bytes, got ");
                Serial.println(byteCount);
                while (--byteCount)
                    Wire.read();
                i2c_state = STATE_WRONG_BYTECOUNT;
                return;
            }
            const int signs = Wire.read();
            int power_left = Wire.read();
            int power_right = Wire.read();
            if (signs & 0x01)
                power_left = -power_left;
            if (signs & 0x02)
                power_right = -power_right;
            set_power(0, power_left);
            set_power(1, power_right);
            i2c_state = STATE_OK;
        }
        break;

    case 2:
        // Read battery voltage
        {
            const float v = analogRead(V_SENSE)/1023.0*5*5;
            i2c_voltage = static_cast<uint16_t>(v*100);
            reading_voltage = true;
        }
        break;
        
    default:
        Serial.print("ERROR: Unknown command ");
        Serial.println(c);
        while (--byteCount)
            Wire.read();
        i2c_state = STATE_UNKNOWN_COMMAND;
        break;
    }
}

void sendData()
{
    if (reading_voltage)
    {
        Wire.write(reinterpret_cast<const uint8_t*>(&i2c_voltage), sizeof(i2c_voltage));
        reading_voltage = false;
    }
    Wire.write(i2c_state);
}

void setup()
{
    pinMode(INTERNAL_LED, OUTPUT); 

    pinMode(R_PWM_A, OUTPUT);
    pinMode(L_PWM_A, OUTPUT);
    pinMode(L_EN_A, OUTPUT);
    pinMode(R_EN_A, OUTPUT);
    pinMode(L_IS_A, INPUT);
    pinMode(R_IS_A, INPUT);

    pinMode(R_PWM_B, OUTPUT);
    pinMode(L_PWM_B, OUTPUT);
    pinMode(L_EN_B, OUTPUT);
    pinMode(R_EN_B, OUTPUT);
    pinMode(L_IS_B, INPUT);
    pinMode(R_IS_B, INPUT);

    pinMode(BRAKE, OUTPUT);
    pinMode(V_SENSE, INPUT);

    digitalWrite(R_PWM_A, LOW);
    digitalWrite(L_PWM_A, LOW);
    digitalWrite(L_EN_A, LOW);
    digitalWrite(R_EN_A, LOW);

    digitalWrite(R_PWM_B, LOW);
    digitalWrite(L_PWM_B, LOW);
    digitalWrite(L_EN_B, LOW);
    digitalWrite(R_EN_B, LOW);

    digitalWrite(BRAKE, LOW);

    // 1024 - brum
    // 64 - hyl
    // 8 - piv
    const int divisor = 8;
    // Pin 5: 1, 8, 64, 256, 1024 - base frequency 62500 Hz
    setPwmFrequency(R_PWM_A, divisor);
    // Pin 9: Base frequency 31250 Hz
    setPwmFrequency(R_PWM_B, divisor);

    // Pin 3+11: 1, 8, 32, 64, 128, 256, 1024 - base frequency 31250 Hz
    const int divisor2 = divisor;
    setPwmFrequency(L_PWM_A, divisor2); 
    setPwmFrequency(L_PWM_B, divisor2); 
    
    Serial.begin(57600);
    Serial.println("MOTOR: Controller v 0.3");

    Wire.begin(SLAVE_ADDRESS);

    // define callbacks for i2c communication
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
}

void brake_on()
{
    digitalWrite(BRAKE, LOW);
}

void brake_off()
{
    digitalWrite(BRAKE, HIGH);
}

void set_power(// 0-1
               int motor,
               // -255..255
               int power)
{
    if (power != 0)
        brake_off();
    const int l_pwm_pin = motor ? L_PWM_B : L_PWM_A;
    const int r_pwm_pin = motor ? R_PWM_B : R_PWM_A;
    const int l_enable_pin = motor ? L_EN_B : L_EN_A;
    const int r_enable_pin = motor ? R_EN_B : R_EN_A;
    digitalWrite(l_enable_pin, power != 0);
    digitalWrite(r_enable_pin, power != 0);
    if (power >= 0)
    {
        analogWrite(l_pwm_pin, power);
        analogWrite(r_pwm_pin, 0);
    }
    else
    {   
        analogWrite(l_pwm_pin, 0);
        analogWrite(r_pwm_pin, -power);
    }
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

char v_buf[20];

void process(const char* buffer)
{
    switch (buffer[0])
    {
    case 't':
    case 'T':
        run_test();
        break;
        
    case 'm':
    case 'M':
        {
            int index;
            const int left = get_int(buffer+1, BUF_SIZE-1, index); 
            const int right = get_int(buffer+index, BUF_SIZE-1, index);
            if ((left < -255) || (left > 255))
            {
                Serial.print("MOTOR: ERROR: Invalid power value: ");
                Serial.println(left);
                return;
            }
            if ((right < -255) || (right > 255))
            {
                Serial.print("MOTOR: ERROR: Invalid power value: ");
                Serial.println(right);
                return;
            }
            set_power(0, left);
            set_power(1, right);
            Serial.println("OK");
        }
        return;

    case 'b':
    case 'B':
        {
            int index;
            const int brake = get_int(buffer+1, BUF_SIZE-1, index); 
            if (brake)
                brake_on();
            else
                brake_off();
            Serial.println("OK");
        }
        return;

    case 'v':
    case 'V':
        {
            sprintf(v_buf, "%f", analogRead(V_SENSE)/1023.0*5*5);
            Serial.println(v_buf);
        }
        break;
        
    default:
        Serial.print("MOTOR: Error: Unknown command '");
        Serial.print(buffer[0]);
        Serial.println("'");
        return;
    }
}

void run_test()
{
    for (int i = -255; i < 256; i++)
    {
        Serial.print("A ");
        Serial.println(i);
        set_power(0, i);
        delay(100);
    }
    set_power(0, 0);
    delay(2000);
    for (int i = -255; i < 256; i++)
    {
        Serial.print("B ");
        Serial.println(i);
        set_power(1, i);
        delay(100);
    }
    set_power(1, 0);
    Serial.println("Test done");
}

int index = 0;
char buffer[BUF_SIZE];

void loop()
{
#if !TEST_MODE
    if (Serial.available())
    {
        // Command from PC
        char c = Serial.read();
        if ((c == '\r') || (c == '\n'))
        {
            buffer[index] = 0;
            index = 0;
            process(buffer);
            idle_count = 0;
        }
        else
        {
            if (index >= BUF_SIZE)
            {
                Serial.println("MOTOR: Error: Line too long");
                index = 0;
                return;
            }
            buffer[index++] = c;
        }
    }
    else
    {
        delay(10);
        if (MAX_IDLE_COUNT && (++idle_count > MAX_IDLE_COUNT))
        {
            idle_count = 0;
            set_power(0, 0);
            set_power(1, 0);
            Serial.println("MOTOR: Shut down due to inactivity");
        }
    }
    delay(10);
#else
    run_test();
#endif
}
