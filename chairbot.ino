int INTERNAL_LED = 13;
int RPWM = 5;
int LPWM = 6;
int L_EN = 7;
int R_EN = 8;
int BRAKE = 9;

void setup()
{
    pinMode(INTERNAL_LED, OUTPUT); 

    pinMode(RPWM, OUTPUT);
    pinMode(LPWM, OUTPUT);
    pinMode(L_EN, OUTPUT);
    pinMode(R_EN, OUTPUT);
    pinMode(BRAKE, OUTPUT);

    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, LOW);
    digitalWrite(L_EN, LOW);
    digitalWrite(R_EN, LOW);
    digitalWrite(BRAKE, LOW);

    Serial.begin(57600);
    Serial.println("Chairbot ready");
}

int pwr1 = 0;

void loop()
{
#if 0
    if (Serial.available())
    {
        // Command from PC
        char c = Serial.read();
        switch (c)
        {
        case 'u':
            ++pwr1;
            break;
        case 'd':
            --pwr1;
            break;
        default:
            return;
        }
        if (pwr1 < 0)
            pwr1 = 0;
        if (pwr1 > MAX_PWR)
            pwr1 = MAX_PWR;
        Serial.println(pwr1);
    }
    analogWrite(PWM1, pwr1);
    delay(10);
#else
    digitalWrite(BRAKE, HIGH);
    digitalWrite(INTERNAL_LED, HIGH);
    Serial.println("EN High");
    analogWrite(RPWM,0);
    analogWrite(LPWM,0);
    digitalWrite(R_EN,HIGH);
    digitalWrite(L_EN,HIGH);
    delay(1000);
    for(int i=0;i<256;i++){
        analogWrite(RPWM,i);
        //  analogWrite(LPWM,255-i);
        delay(100);
    }
    digitalWrite(BRAKE, LOW);
    digitalWrite(INTERNAL_LED, LOW);
    delay(500);
    digitalWrite(BRAKE, HIGH);
    digitalWrite(INTERNAL_LED, HIGH);
    for(int i=255;i>0;i--){
        analogWrite(RPWM,i);
        analogWrite(LPWM,255-i);
        delay(100);
    }
    delay(500);
    Serial.println("EN LOW");
    digitalWrite(R_EN,LOW);
    digitalWrite(L_EN,LOW);
    delay(1000);
    for(int i=0;i<256;i++){
        analogWrite(RPWM,i);
        delay(10);
    }
    delay(500);
#endif
}
