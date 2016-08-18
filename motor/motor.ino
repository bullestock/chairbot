#define TEST_MODE       0

const int INTERNAL_LED = 13;
const int R_PWM_A = 5;
const int L_PWM_A = 4;
const int R_EN_A = 6;
const int L_EN_A = 3;
const int R_IS_A = 7;
const int L_IS_A = 2;
const int R_PWM_B = A1;
const int L_PWM_B = 11;
const int R_EN_B = A2;
const int L_EN_B = 12;
const int R_IS_B = A3;
const int L_IS_B = 13;
const int BRAKE = 8;
const int BUF_SIZE = 200;

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

    digitalWrite(R_PWM_A, LOW);
    digitalWrite(L_PWM_A, LOW);
    digitalWrite(L_EN_A, LOW);
    digitalWrite(R_EN_A, LOW);

    digitalWrite(R_PWM_B, LOW);
    digitalWrite(L_PWM_B, LOW);
    digitalWrite(L_EN_B, LOW);
    digitalWrite(R_EN_B, LOW);

    digitalWrite(BRAKE, LOW);

    Serial.begin(57600);
    Serial.println("Chairbot ready");
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
               // -127..127
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
    const int pwr = power+128;
    analogWrite(l_pwm_pin, pwr);
    analogWrite(r_pwm_pin, 255-pwr);
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
            set_power(0, left);
            set_power(1, right);
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
        }
        return;
        
    default:
        Serial.print("Error: Unknown command '");
        Serial.print(buffer[0]);
        Serial.println("'");
        return;
    }
}

void run_test()
{
    const int DELTA = 80;
    const int MAX_PWR = 128+DELTA;
    const int MIN_PWR = 128-DELTA;

    digitalWrite(BRAKE, HIGH);

    Serial.println("Motor A");
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    analogWrite(R_PWM_A, 0);
    analogWrite(L_PWM_A, 0);
    digitalWrite(R_EN_A, HIGH);
    digitalWrite(L_EN_A, HIGH);
    delay(1000);
    for (int i = MIN_PWR; i < MAX_PWR; i++)
    {
        Serial.print("Power ");
        Serial.println(i);
        analogWrite(R_PWM_A, i);
        analogWrite(L_PWM_A, 255-i);
        delay(100);
    }
    Serial.println("Off");
    Serial.println("Disable");
    digitalWrite(R_EN_A, LOW);
    digitalWrite(L_EN_A, LOW);
    Serial.println("Brake on");
    delay(500);
    digitalWrite(BRAKE, LOW);

    delay(5000);
    Serial.println("Motor B");
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(INTERNAL_LED, HIGH);
    delay(100);
    digitalWrite(INTERNAL_LED, LOW);
    delay(100);
    digitalWrite(BRAKE, HIGH);
    analogWrite(R_PWM_B, 0);
    analogWrite(L_PWM_B, 0);
    digitalWrite(R_EN_B, HIGH);
    digitalWrite(L_EN_B, HIGH);
    delay(1000);
    for (int i = MIN_PWR; i < MAX_PWR; i++)
    {
        Serial.print("Power ");
        Serial.println(i);
        analogWrite(R_PWM_B, i);
        analogWrite(L_PWM_B, 255-i);
        delay(100);
    }
    Serial.println("Off");
    analogWrite(R_PWM_B, 0);
    analogWrite(L_PWM_B, 0);
    digitalWrite(R_EN_B, LOW);
    digitalWrite(L_EN_B, LOW);
    digitalWrite(BRAKE, LOW);
    digitalWrite(INTERNAL_LED, LOW);
    delay(5000);
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
    delay(10);
#else
    run_test();
#endif
}
