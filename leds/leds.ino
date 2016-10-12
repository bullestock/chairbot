#include "FastLED.h"
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

void setup()
{
  FastLED.addLeds<WS2811, PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(57600);
  Serial.println("LED Controller v 0.1");
}

enum Mode
{
   MODE_CYCLE,
   MODE_GROWING_BARS,
   MODE_FADE,
   MODE_CHASE,
   MODE_CHASE_MULTI,
   MODE_PERIODIC_PALETTE,
   MODE_RAINBOW,
   MODE_CYLON,
   MODE_BOUNCE
};

Mode mode = MODE_PERIODIC_PALETTE;

const static CRGB chase_colours[] = {
  CRGB::Yellow, CRGB::Green, CRGB::HotPink, CRGB::Blue, CRGB::Red, CRGB::White
};

void fadeall()
{
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i].nscale8(250);
}

const int BUF_SIZE = 20;
int index = 0;
char buffer[BUF_SIZE];

int current_led = 0;
int current_loop = 0;
bool growing = true;

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
    FastLED.show();
    ++current_led;
    delay(30);
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
      FastLED.show();
      delay(10);
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
      FastSPI_LED.show();
      delay(10);
    }
    ++current_led;
    break;

  case MODE_FADE:
    // Fade in/fade out
    for(int j = 0; j < 3; j++ ) { 
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
        if (Serial.available())
          break;
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
        if (Serial.available())
          break;
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
      }
      leds[NUM_LEDS-1] = CRGB::Black;
      if (Serial.available())
        break;
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
      }
      leds[NUM_LEDS-1] = CRGB::Black;
      FastLED.show();
      delay(50);
      if (Serial.available())
        break;
      for (int i = 0; i < NUM_LEDS; ++i)
      {
        if (i)
          leds[NUM_LEDS-i] = CRGB::Black;
        leds[NUM_LEDS-1-i] = chase_colours[c];
        FastLED.show();
        delay(50);
      }
      leds[NUM_LEDS-1] = CRGB::Black;
      if (Serial.available())
        break;
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
      }
      leds[NUM_LEDS-1] = CRGB::Black;
      if (Serial.available())
        break;
    }
    break;

  case MODE_PERIODIC_PALETTE:
    ChangePalettePeriodically();
    
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors(startIndex);
    
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    break;

  case MODE_RAINBOW:
    fill_rainbow(leds, NUM_LEDS, --starthue, 20);
    FastLED.delay(8);
    break;

  case MODE_CYLON:
    if (current_led >= NUM_LEDS)
      current_led = 0;
    // Set the i'th led to red 
    leds[current_led] = CHSV(starthue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    FastLED.delay(10);
    ++current_led;
    break;

  default:
    memset(leds, 0, NUM_LEDS * 3);
    FastLED.show();
    break;
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
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if(lastSecond != secondHand) {
        lastSecond = secondHand;
        if(secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if(secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if(secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if(secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if(secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if(secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if(secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if(secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if(secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if(secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if(secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
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
