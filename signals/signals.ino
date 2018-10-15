#include <SoftwareSerial.h>
#include <Wire.h>

SoftwareSerial mySerial(10, 11); // RX, TX

const int LED_PIN = 13;
const int BUSY_PIN = 12;
const int HEADLIGHT_PIN = 2;
const int HORN_PIN = 9;
const int KNIGHTRIDER_PIN = 8;
const int SLAVE_ADDRESS = 0x05;
const int FLASH_RATE = 100; // Flash half period in ms

bool debug_on = true;

bool mosfet_state = false;
bool mosfet_steady = true;
unsigned long flash_tick = 0;

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

// Flag to start playing sound
bool do_play = false;

// LED state
bool led_state = false;
bool knightrider_state = false;
bool horn_on = false;
unsigned long horn_tick = 0;

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
        // Switch headlight on/off
        mosfet_state = Wire.read();
        mosfet_steady = true;
        --byteCount;
        break;

    case 3:
        // Turn headlight blink on/off
        mosfet_state = Wire.read();
        mosfet_steady = false;
        flash_tick = millis();
        --byteCount;
        break;

    case 4:
        // Control LED
        led_state = Wire.read();
        --byteCount;
        break;
        
    case 5:
        // Sound horn
        horn_on = true;
        horn_tick = millis();
        --byteCount;
        break;
        
    case 6:
        // Control Knight Rider
        knightrider_state = Wire.read();
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
    Serial.println("Signals v 0.4");

    pinMode(HEADLIGHT_PIN, OUTPUT);
    pinMode(HORN_PIN, OUTPUT);
    pinMode(KNIGHTRIDER_PIN, OUTPUT);
    
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
                num = random(num_flash_files);
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
            player.start_play_physical(num);
            play_start = millis();
            state = STATE_PLAYING;
        }
        else
            digitalWrite(LED_PIN, led_state);
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
            else if ((millis() - play_start) > 30000)
            {
                Serial.println("TIMEOUT");
                state = STATE_IDLE;
            }
        }
        break;

    case STATE_WAIT:
        if ((millis() - wait_start) > 5000)
        {
            state = STATE_IDLE;
            if (debug_on)
                Serial.println("Ready");
        }
        break;
    }

    const auto now = millis();
    digitalWrite(HEADLIGHT_PIN, mosfet_state);
    if (!mosfet_steady)
    {
        if (now - flash_tick > FLASH_RATE)
        {
            flash_tick = now;
            mosfet_state = !mosfet_state;
        }
    }

    digitalWrite(KNIGHTRIDER_PIN, knightrider_state);

    if (horn_on && (now - horn_tick > 500))
        horn_on = false;

    digitalWrite(HORN_PIN, horn_on);
}
