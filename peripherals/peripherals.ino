#include <Wire.h>

#include "SerialMP3Player.h"

const int LED_PIN = 13;
const int BUSY_PIN = A0;
const int PWM_PINS[] =
    {
        3, // TCCR2B
        5, // TCCR0B
        6, // 
        9  // 
    };
const int SLAVE_ADDRESS = 0x05;
const int FLASH_RATE = 100; // Flash half period in ms

bool debug_on = true;

const int sounds_per_bank[] = {
    // 01/001-081
    81,
    // 01/082-084
    3,
    // 01/085-109
    25
};

SerialMP3Player player(4, 2); // RX, TX

// Index of sound to play. -1 means random.
int sound_index = -1;

// Bank of sound to play. -1 means random.
int sound_bank = -1;

// Flag to start playing sound
bool do_play = false;

void receiveData(int byteCount)
{
    int c = Wire.read();
    --byteCount;
    switch (c)
    {
    case 0:
        // Play random sound
        sound_index = -1;
        do_play = true;
        break;

    case 1:
        // Play specific sound
        sound_index = Wire.read();
        --byteCount;
        do_play = true;
        break;

    case 2:
        // Play random sound from specific bank
        sound_bank = Wire.read();
        sound_index = -1;
        do_play = true;
        break;

    case 3:
        // Play specific sound from specific bank
        sound_bank = Wire.read();
        sound_index = Wire.read();
        do_play = true;
        break;

    case 10:
    case 11:
    case 12:
    case 13:
        // PWM 1-4
        analogWrite(PWM_PINS[c - 10], Wire.read());
        --byteCount;
        break;

    default:
        break;
    }

    while (byteCount--)
        Wire.read();
}

void sendData()
{
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Peripherals v 0.2");

    //TCCR0B = TCCR0B & B11111000 | B00000001;

    for (auto pin : PWM_PINS)
        pinMode(pin, OUTPUT);

    player.begin(9600);
    //player.showDebug(true);
    delay(500);
    player.sendCommand(CMD_SEL_DEV, 0, 2);   //select sd-card
    delay(500);
    
    player.setVol(30);
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
    const int BS = 30;
    if (index >= BS)
    {
        Serial.println("ERROR: Integer too long");
        return -1;
    }
    char intbuf[BS];
    memcpy(intbuf, buffer, index);
    intbuf[index] = 0;
    next = index+1;
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
        {
            int index;
            const int n = get_int(buffer+1, BUF_SIZE-1, index);
            int total = 0;
            for (int i = 0; i < sizeof(sounds_per_bank)/sizeof(sounds_per_bank[0]); ++i)
                total += sounds_per_bank[i];
            if (n < 0 || n >= total)
            {
                Serial.println("ERROR: Invalid parameters to 'P'");
                return;
            }
            sound_index = n;
            do_play = true;
            Serial.println("OK");
        }
        break;

        // R: Play random sound
    case 'R':
    case 'r':
        sound_index = -1;
        do_play = true;
        Serial.println("OK");
        break;

        // R: Play random sound from bank <n>
    case 'B':
    case 'b':
        {
            int index;
            const int n = get_int(buffer+1, BUF_SIZE-1, index); 
            sound_index = -1;
            sound_bank = n;
            do_play = true;
            Serial.println("OK");
        }
        break;

        // O: Control output
    case 'o':
    case 'O':
        {
            int index;
            const int port = get_int(buffer+1, BUF_SIZE-1, index);
            if (port < 0 || port > (int) (sizeof(PWM_PINS)/sizeof(PWM_PINS[0])))
            {
                Serial.println("ERROR: Invalid port parameter to 'O'");
                return;
            }
            const int value = get_int(buffer+index, BUF_SIZE-1, index);
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
            int num = sound_index;
            do_play = false;
            digitalWrite(LED_PIN, HIGH);
            int total = 0;
            for (int i = 0; i < sizeof(sounds_per_bank)/sizeof(sounds_per_bank[0]); ++i)
                total += sounds_per_bank[i];
            if (num < 0)
            {
                if (sound_bank >= 0)
                {
                    num = 1;
                    for (int i = 0; i < sound_bank; ++i)
                        num += sounds_per_bank[i];
                    num += random(sounds_per_bank[sound_bank]+1);
                    Serial.print("Bank ");
                    Serial.print(sound_bank);
                    Serial.print(" random: ");
                    Serial.println(num);
                }
                else
                    num = random(total);
            }
            else if (num >= total)
            {
                Serial.println("Invalid sound index");
                break;
            }
            if (debug_on)
            {
                Serial.print("Play ");
                Serial.println(num);
            }
            sound_bank = -1;
            player.play(num);
            play_start = millis();
            state = STATE_PLAYING;
        }
        break;

    case STATE_PLAYING:
        {
            player.qStatus();
            while (!player.available())
                delay(10);
            bool busy = false;
            uint8_t status = 0;
            uint16_t value = 0;
            if (player.getAnswer(status, value))
                busy = status == 1;
            if (!busy)
            {
                digitalWrite(LED_PIN, LOW);
                state = STATE_WAIT;
                wait_start = millis();
            }
            else if ((millis() - play_start) > 3000)
            {
                Serial.println("TIMEOUT");
                state = STATE_IDLE;
            }
        }
        break;

    case STATE_WAIT:
        if ((millis() - wait_start) > 2000)
        {
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
