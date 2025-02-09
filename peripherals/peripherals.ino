#include <TimerOne.h>
#include <Wire.h>

#include "SerialMP3Player.h"

const int BUSY_PIN = A0;
const int PWM_PINS[] =
{
    3, // Timer2
    5, // Timer0
    6, // Timer0
    9  // Timer1
};
const int SLAVE_ADDRESS = 0x05;

bool debug_on = true;

SerialMP3Player player(4, 2); // RX, TX

// Index of sound to play (1-255)
uint8_t sound_index = 0;

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
    case 1:
        // Play sound
        sound_index = Wire.read();
        if (debug_on)
        {
            Serial.print("i2c sound ");
            Serial.println(sound_index);
        }
        --byteCount;
        do_play = true;
        break;

    case 4:
        // Set volume
        {
            uint8_t vol = Wire.read() & 31;
            player.setVol(vol);
            if (debug_on)
            {
                Serial.print("i2c vol ");
                Serial.println(vol);
            }
            --byteCount;
        }
        break;

    case 10:
    case 11:
    case 12:
    case 13:
        // PWM 1-4
        {
            blink_count = 2;
            blink_interval = 100;
            uint8_t val = Wire.read();
            analogWrite(PWM_PINS[c - 10], val);
            if (debug_on)
            {
                Serial.print("i2c pwm ");
                Serial.println(val);
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
    Serial.println("Peripherals v 0.4");

    for (auto pin : PWM_PINS)
        pinMode(pin, OUTPUT);

    Timer1.initialize(1020);
    Timer1.attachInterrupt(timer_interrupt);

    player.begin(9600);
    //player.showDebug(true);
    delay(500);
    player.sendCommand(CMD_SEL_DEV, 0, 2);   //select sd-card
    delay(500);
    
    player.setVol(10);
    delay(20);

    Wire.begin(SLAVE_ADDRESS);
    Wire.onReceive(receiveData);
    Wire.onRequest(sendData);
}

enum State {
    STATE_IDLE,
    STATE_PLAYING,
    STATE_WAIT
} state = STATE_IDLE;

unsigned long wait_start = 0;
unsigned long play_start = 0;

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
        // P<n>: Play sound <n>
    case 'P':
    case 'p':
        sound_index = get_int(buffer+1, BUF_SIZE-1);
        do_play = true;
        Serial.println("OK");
        break;

        // V: Set volume
    case 'v':
    case 'V':
        {
            const int vol = get_int(buffer+1, BUF_SIZE-1);
            if (vol < 0 || vol > 30)
            {
                Serial.println("ERROR: Invalid volume");
                return;
            }
            player.setVol(vol);
            delay(20);
            Serial.println("OK");
        }
        break;

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

        // S: Stop play
    case 's':
    case 'S':
        player.stop();
        Serial.println("OK");
        break;

    case 'h':
    case 'H':
        Serial.println("Commands:\r\n"
                       "p <idx>\t\t"            "Play sound\r\n"
                       "v <vol>\t\t"            "Set volume\r\n"
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
    switch (state)
    {
    case STATE_IDLE:
        if (do_play)
        {
            blink_count = 1000;
            blink_interval = 50;
            do_play = false;
            if (debug_on)
            {
                Serial.print("Play ");
                Serial.println(sound_index);
            }
            player.play(sound_index);
            play_start = millis();
            state = STATE_PLAYING;
        }
        break;

    case STATE_PLAYING:
        {
            player.qStatus();
            int i = 0;
            while (!player.available())
            {
                if (i++ < 100)
                {
                    Serial.println("TIMEOUT");
                    blink_interval = 0;
                    state = STATE_IDLE;
                    break;
                }
                delay(10);
            }
            bool busy = false;
            uint8_t status = 0;
            uint16_t value = 0;
            if (player.getAnswer(status, value))
                busy = status == 1;
            if (!busy)
            {
                state = STATE_WAIT;
                wait_start = millis();
            }
            else
            {
                const auto elapsed = millis() - play_start;
                if (elapsed > 3000)
                {
                    Serial.println("TIMEOUT");
                    blink_interval = 0;
                    state = STATE_IDLE;
                }
            }
        }
        break;

    case STATE_WAIT:
        if ((millis() - wait_start) > 2000)
        {
            blink_interval = 0;
            state = STATE_IDLE;
            if (debug_on)
                Serial.println("Ready");
        }
        break;
    }

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
