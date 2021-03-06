#include "FastLED.h"
#include <Wire.h>

// Number of RGB LEDs in the strand
#define NUM_LEDS 60
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

// Define the array of leds
CRGB leds[NUM_LEDS];
// Arduino pin used for Data
#define PIN 6 

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t startIndex = 0;
#define UPDATES_PER_SECOND 100

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

uint8_t starthue = 0;

void ChangePalettePeriodically();
void FillLEDsFromPaletteColors(uint8_t colorIndex);
void SetupBlackAndWhiteStripedPalette();
void SetupPurpleAndGreenPalette();
void SetupTotallyRandomPalette();

const int SLAVE_ADDRESS = 0x06;

enum State
{
    STATE_OK = 0,
    STATE_UNKNOWN_COMMAND,
    STATE_UNKNOWN_MODE,
    STATE_WRONG_BYTECOUNT
};

int i2c_state = STATE_OK;

enum Mode
{
  MODE_CYCLE,              // 0
  MODE_GROWING_BARS,
  MODE_FADE,
  MODE_CHASE,
  MODE_CHASE_MULTI,
  MODE_PERIODIC_PALETTE,   // 5
  MODE_RAINBOW,
  MODE_RAINBOW_GLITTER,
  MODE_CYLON,
  MODE_BOUNCE,
  MODE_CONFETTI,           // 10
  MODE_SINELON,
  MODE_BPM,
  MODE_JUGGLE,
  MODE_FIRE,
  MODE_LAST
};

uint8_t BeatsPerMinute = 62;

Mode mode = MODE_PERIODIC_PALETTE;
Mode old_mode = static_cast<Mode>(-1);

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
    // Set mode
    if (byteCount != 2)
    {
      while (--byteCount)
        Wire.read();
      i2c_state = STATE_WRONG_BYTECOUNT;
      return;
    }
    mode = static_cast<Mode>(Wire.read());
    i2c_state = STATE_OK;
    if (mode >= MODE_LAST)
      i2c_state = STATE_UNKNOWN_MODE;
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
  FastLED.addLeds<WS2811, PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(57600);
  Serial.println("LED Controller v 0.1");

  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
}

const static CRGB chase_colours[] = {
  CRGB::Yellow, CRGB::Green, CRGB::HotPink, CRGB::Blue, CRGB::Red, CRGB::White
};

void fadeall()
{
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i].nscale8(250);
}

void addGlitter(fract8 chanceOfGlitter) 
{
  if (random8() < chanceOfGlitter)
  {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

const int BUF_SIZE = 20;
int index = 0;
char buffer[BUF_SIZE];

int current_led = 0;
int current_loop = 0;
bool growing = true;

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

bool gReverseDirection = false;

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS-1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

#define CHECK_MODE()                \
  if (mode != old_mode)             \
  {                                 \
    old_mode = mode;                \
    current_led = current_loop = 0; \
    break;                          \
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
      mode = static_cast<Mode>(atoi(buffer));
      memset(leds, 0, NUM_LEDS * 3);
      current_led = 0;
      current_loop = 0;
      Serial.print("Mode ");
      Serial.println(mode);
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

  switch (mode)
  {
  case MODE_CYCLE:
    // one at a time
    if (current_loop >= 3)
      current_loop = 0;
    if (current_led >= NUM_LEDS)
    {
      current_led = 0;
      ++current_loop;
    }
    memset(leds, 0, NUM_LEDS * 3);
    switch(current_loop) { 
    case 0: leds[current_led].r = 255; break;
    case 1: leds[current_led].g = 255; break;
    case 2: leds[current_led].b = 255; break;
    }
    ++current_led;
    break;

  case MODE_GROWING_BARS:
    // growing/receeding bars
    if (current_led >= NUM_LEDS)
    {
      current_led = 0;
      ++current_loop;
    }
    if (growing)
    {
      if (current_loop >= 3)
      {
        current_loop = 0;
        growing = false;
        break;
      }
      switch (current_loop)
      { 
      case 0: leds[current_led].r = 255; break;
      case 1: leds[current_led].g = 255; break;
      case 2: leds[current_led].b = 255; break;
      }
    }
    else
    {
      if (current_loop >= 3)
      {
        current_loop = 0;
        growing = true;
        break;
      }
      switch (current_loop)
      { 
      case 0: leds[NUM_LEDS-1-current_led].r = 0; break;
      case 1: leds[NUM_LEDS-1-current_led].g = 0; break;
      case 2: leds[NUM_LEDS-1-current_led].b = 0; break;
      }
    }
    ++current_led;
    break;

  case MODE_FADE:
    // Fade in/fade out
    for (int j = 0; j < 3; j++ ) { 
      memset(leds, 0, NUM_LEDS * 3);
      for(int k = 0; k < 256; k++) { 
        for(int i = 0; i < NUM_LEDS; i++ ) {
          switch(j) { 
          case 0: leds[i].r = k; break;
          case 1: leds[i].g = k; break;
          case 2: leds[i].b = k; break;
          }
        }
        FastLED.show();
        delay(3);
        CHECK_MODE();
      }
      for(int k = 255; k >= 0; k--) { 
        for(int i = 0; i < NUM_LEDS; i++ ) {
          switch(j) { 
          case 0: leds[i].r = k; break;
          case 1: leds[i].g = k; break;
          case 2: leds[i].b = k; break;
          }
        }
        FastLED.show();
        delay(3);
        CHECK_MODE();
      }
    }
    break;

  case MODE_CHASE:
    memset(leds, 0, NUM_LEDS * 3);
    for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
    {
      for (int i = 0; i < NUM_LEDS; ++i)
      {
        if (i)
          leds[i-1] = CRGB::Black;
        leds[i] = chase_colours[c];
        FastLED.show();
        delay(50);
        CHECK_MODE();
      }
      leds[NUM_LEDS-1] = CRGB::Black;
    }
    break;

  case MODE_BOUNCE:
    for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
    {
      memset(leds, 0, NUM_LEDS * 3);
      FastLED.show();
      for (int i = 0; i < NUM_LEDS; ++i)
      {
        if (i)
          leds[i-1] = CRGB::Black;
        leds[i] = chase_colours[c];
        FastLED.show();
        delay(50);
        CHECK_MODE();
      }
      leds[NUM_LEDS-1] = CRGB::Black;
      FastLED.show();
      delay(50);
      for (int i = 0; i < NUM_LEDS; ++i)
      {
        if (i)
          leds[NUM_LEDS-i] = CRGB::Black;
        leds[NUM_LEDS-1-i] = chase_colours[c];
        FastLED.show();
        delay(50);
        CHECK_MODE();
      }
      leds[NUM_LEDS-1] = CRGB::Black;
    }
    break;
    
  case MODE_CHASE_MULTI:
    memset(leds, 0, NUM_LEDS * 3);
    for (size_t c = 0; c < sizeof(chase_colours)/sizeof(chase_colours[0]); ++c)
    {
      const int N = 4;
      for (int j = 0; j < N; ++j)
      {
        memset(leds, 0, NUM_LEDS * 3);
        for (int i = 0; i < NUM_LEDS; ++i)
        {
          if (((i+j) % N) == 0)
            leds[i] = chase_colours[c];
        }
        FastLED.show();
        delay(100);
        CHECK_MODE();
      }
      leds[NUM_LEDS-1] = CRGB::Black;
    }
    break;

  case MODE_PERIODIC_PALETTE:
    ChangePalettePeriodically();
    
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors(startIndex);
    break;

  case MODE_RAINBOW:
    fill_rainbow(leds, NUM_LEDS, --starthue, 20);
    break;

  case MODE_RAINBOW_GLITTER:
    fill_rainbow(leds, NUM_LEDS, --starthue, 20);
    addGlitter(80);
    break;

  case MODE_CYLON:
    if (current_led >= NUM_LEDS)
      current_led = 0;
    // Set the i'th led to red 
    leds[current_led] = CHSV(starthue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    fadeall();
    // Wait a little bit before we loop around and do it again
    FastLED.delay(10);
    ++current_led;
    break;

  case MODE_CONFETTI:
    // random colored speckles that blink in and fade smoothly
    {
      fadeToBlackBy(leds, NUM_LEDS, 10);
      int pos = random16(NUM_LEDS);
      leds[pos] += CHSV(starthue + random8(64), 200, 255);
    }
    break;
    
  case MODE_SINELON:
    // a colored dot sweeping back and forth, with fading trails
    {
      fadeToBlackBy(leds, NUM_LEDS, 20);
      int pos = beatsin16(13, 0, NUM_LEDS);
      leds[pos] += CHSV(starthue, 255, 192);
    }
    break;

  case MODE_BPM:
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    {
      uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
      for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = ColorFromPalette(PartyColors_p, starthue+(i*2), beat-starthue+(i*10));
    }
    break;

  case MODE_JUGGLE:
    // eight colored dots, weaving in and out of sync with each other
    {
      fadeToBlackBy(leds, NUM_LEDS, 20);
      byte dothue = 0;
      for(int i = 0; i < 8; i++)
      {
        leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
        dothue += 32;
      }
    }
    break;

  case MODE_FIRE:
    Fire2012();
    break;
    
  default:
    memset(leds, 0, NUM_LEDS * 3);
    FastLED.show();
    break;
  }
    
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

  EVERY_N_MILLISECONDS(20)
  {
    ++starthue;
  }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;
    
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
  // Change palette every 8 seconds
  const int secondHand = millis()/8000;
  static int lastSecond = 999;
    
  if(lastSecond != secondHand) {
    lastSecond = secondHand;
    const int pal = random(11);
    switch (pal)
    {
    case 0:  currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; break;
    case 1:  currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  break;
    case 2:  currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; break;
    case 3:  SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; break;
    case 4:  SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; break;
    case 5:  SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; break;
    case 6:  SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; break;
    case 7:  currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; break;
    case 8:  currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; break;
    case 9:  currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  break;
    case 10: currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; break;
    }
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for(int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV(random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV(HUE_PURPLE, 255, 255);
    CRGB green  = CHSV(HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};
