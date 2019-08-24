#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial mySerial(10, 11); // RX, TX

const int LED_PIN = 13;
const int BUSY_PIN = A0;
const int PWM1_PIN = 5;
const int PWM2_PIN = 3;
const int SLAVE_ADDRESS = 0x05;
const int FLASH_RATE = 100; // Flash half period in ms

bool debug_on = true;

const int sounds_per_bank[] = {
    // 01/001-082
    82,
    // 02/001-002
    2,
    // 03/001-025
    25
};

// http://www.mat54-wiki.nl/mat54/index.php/DFPlayer_Mini_SKU:DFR0299
class DFPlayer
{
public:
    DFPlayer(HardwareSerial& _s)
        : m_hardware_serial(&_s),
          m_software_serial(nullptr)
    {
        init_buf();
    }
   
    DFPlayer(SoftwareSerial& _s)
        : m_hardware_serial(nullptr),
          m_software_serial(&_s)
    {
        init_buf();
    }
   
    void play_physical(uint16_t num)
    {
        send_cmd(0x03, num);
        while (1)
        {
            delay(50);
            if (digitalRead(BUSY_PIN))
                break;
        }
    }

    void start_play_physical(uint16_t num)
    {
        send_cmd(0x03, num);
    }

    void stop_play()
    {
        send_cmd(0x16);
    }

    bool is_busy() const
    {
        return !digitalRead(BUSY_PIN);
    }

    void set_volume(uint16_t volume)
    {
        send_cmd(0x06, volume);
    }

    uint16_t get_num_flash_files()
    {
        flush();
        send_cmd_feedback(0x48);
        return get_reply();
    }

    void flush()
    {
        if (m_hardware_serial)
            while (m_hardware_serial->available())
                ;
        else
            while (m_software_serial->available())
                ;
    }
   
    uint16_t get_reply()
    {
        delay(50);
      
        uint8_t buf[10];
        int n = 0;
        while (n < 10)
        {
            uint8_t c;
            if (!get_char(c))
            {
                Serial.println("exhausted");
                return 0;
            }
            if ((n == 0) && (c != 0x7E))
            {
                Serial.println("wrong");
                continue;
            }
            buf[n++] = c;
        }
#if 0
      char s[40];
      sprintf(s, "Reply: ");
      for (int i = 0; i < 10; i++)
         sprintf(s+strlen(s)-1, "%02X ", buf[i]);
      Serial.println(s);
#endif
      if ((n == 10) && (buf[9] == 0xEF))
         return buf[5]*256+buf[6];
      return 0;
   }
   
private:
   bool get_char(uint8_t& c)
   {
      if (m_hardware_serial && m_hardware_serial->available())
      {
         c = m_hardware_serial->read();
         return true;
      }
      if (m_software_serial && m_software_serial->available())
      {
         c = m_software_serial->read();
         return true;
      }
      return false;
   }
   
   void init_buf()
   {
      send_buf[0] = 0x7E;
      send_buf[1] = 0xFF;
      send_buf[2] = 0x06;
      send_buf[9] = 0xEF;
   }
   
   void send_cmd(uint8_t cmd, uint16_t arg = 0)
   {
      send_buf[3] = cmd;
      send_buf[4] = 0;
      fill_uint16_bigend((send_buf+5), arg);
      fill_checksum();
      send();
   }

   void send_cmd_feedback(uint8_t cmd, uint16_t arg = 0)
   {
      send_buf[3] = cmd;
      send_buf[4] = 1;
      fill_uint16_bigend((send_buf+5), arg);
      fill_checksum();
      send();
   }

   static void fill_uint16_bigend(uint8_t *thebuf, uint16_t data)
   {
      *thebuf =	(uint8_t)(data>>8);
      *(thebuf+1) =	(uint8_t)data;
   }

   uint16_t get_checksum(uint8_t *thebuf)
   {
      uint16_t sum = 0;
      for(int i = 1; i < 7; i++)
      {
         sum += thebuf[i];
      }
      return -sum;
   }


   void fill_checksum()
   {
      uint16_t checksum = get_checksum(send_buf);
      fill_uint16_bigend(send_buf+7, checksum);
   }

   void send()
   {
      if (m_hardware_serial)
         for (int i = 0; i < 10; i++)
            m_hardware_serial->write(send_buf[i]);
      else
         for (int i = 0; i < 10; i++)
            m_software_serial->write(send_buf[i]);
	}

   uint8_t send_buf[10];
   HardwareSerial* m_hardware_serial;
   SoftwareSerial* m_software_serial;
};

DFPlayer player(mySerial);

int num_flash_files = 0;

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
        // PWM 1
        analogWrite(PWM1_PIN, Wire.read());
        --byteCount;
        break;

    case 3:
        // PWM 2
        analogWrite(PWM2_PIN, Wire.read());
        --byteCount;
        break;

    case 4:
        // Play random sound from specific bank
        sound_bank = Wire.read();
        sound_index = -1;
        do_play = true;
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
    Serial.println("Peripherals v 0.1");

    pinMode(PWM1_PIN, OUTPUT);
    pinMode(PWM2_PIN, OUTPUT);
    
    mySerial.begin(9600);
	delay(10);
    player.set_volume(10);
	delay(10);
    while (num_flash_files == 0)
    {
        num_flash_files = player.get_num_flash_files();
        digitalWrite(LED_PIN, 1);
        delay(100);
        digitalWrite(LED_PIN, 0);
        delay(100);
    }
    if (debug_on)
    {
        Serial.print("Files on flash: ");
        Serial.println(num_flash_files);
    }

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
            if (n < 0 || n >= num_flash_files)
            {
                Serial.println("ERROR: Invalid parameters to 'G'");
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
            if (port < 0 || port > 1)
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
            analogWrite(port == 0 ? PWM1_PIN : PWM2_PIN, value);
            Serial.println("OK");
        }
        break;

        // S: Stop play
    case 's':
    case 'S':
        player.stop_play();
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
            if (num < 0)
            {
                if (sound_bank >= 0)
                {
                    num = 0;
                    for (int i = 0; i < sound_bank; ++i)
                        num += sounds_per_bank[i];
                    num += random(sounds_per_bank[sound_bank]);
                    Serial.print("Bank ");
                    Serial.print(sound_bank);
                    Serial.print(" random: ");
                    Serial.println(num);
                }
                else
                    num = random(num_flash_files);
            }
            else if (num >= num_flash_files)
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
            player.start_play_physical(num);
            play_start = millis();
            state = STATE_PLAYING;
        }
        break;

    case STATE_PLAYING:
        {
            const bool busy = player.is_busy();
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
