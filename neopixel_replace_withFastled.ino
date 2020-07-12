#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>

#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp8266.h>

#define LED_PIN     2
#define COLOR_ORDER GRB 
#define CHIPSET     WS2812B
#define NUM_LEDS    30

#define FRAMES_PER_SECOND 120

bool gReverseDirection = false;

int ButStat = 0;

CRGB leds[NUM_LEDS];

char auth[] = "_nAYTB9j-9EBGCGijTC_fykYK8NO-pjP";
char ssid[] = "Bhallas Tower";
char pass[] = "bhalla123";

CRGBPalette16 gPal;

/* */
int LedBrightness=100;

/*
int arr[5][3] = {
                0,   0,  10,
                80,  80, 0, 
                150, 50, 0, 
                200, 34, 0,
                255, 85, 0  
            };  */

//String lastColor = "#000000";
#define COOLING  55
#define SPARKING 120
uint8_t gHue = 0;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);  
  FastLED.delay(3000); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( LedBrightness );
}

void loop()
{
  Blynk.run();
}

BLYNK_WRITE(V0)
{
  int r, g, b;
  r = param[0].asInt();
  g = param[1].asInt();
  b = param[2].asInt();

  for(int i = 0; i< NUM_LEDS; i++)
  {
    leds[i] = CRGB(r, g, b);
    FastLED.delay(0.2);
  }
  FastLED.show();
}

BLYNK_WRITE(V1)
{
  int ch = param.asInt();
  if(ButStat == 1)
  {
  switch(ch)
  {
    fill_solid( leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    case 1:
    { 
    Fire2012(); // run simulation frame
    FastLED.show(); // display this frame
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    }
    break;
    case 2:
    {
      Fire2012WithPalette(); // run simulation frame, using palette colors
      FastLED.show(); // display this frame
      FastLED.delay(1000 / FRAMES_PER_SECOND);
    }
    break;
    case 3:
    {
      rainbow();
      FastLED.show();
    }
    break;
    case 4:
    {
      theaterChase(Wheel(random(256)), 1, 50);
    }
    break;
    case 5:
    {
      lightning(CRGB::LightBlue, 20, 5, 50);
    }
    break;
  }
  }
  Blynk.syncVirtual(V1);
}

BLYNK_WRITE(V2)
{
    LedBrightness = param.asInt();
    FastLED.setBrightness(LedBrightness);
    FastLED.show();
    //Blynk.syncVirtual(V0);
}

BLYNK_WRITE(V4)
{

  ButStat = param.asInt();
}

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

void Fire2012WithPalette()
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
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void theaterChase(CRGB c, int cycles, int speed){ // TODO direction

  for (int j=0; j<cycles; j++) {  
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUM_LEDS; i=i+3) {
        int pos = i+q;
        leds[pos] = c;    //turn every third pixel on
      }
      FastLED.show();

      FastLED.delay(speed);

      for (int i=0; i < NUM_LEDS; i=i+3) {
        leds[i+q] = CRGB::Black;        //turn every third pixel off
      }
    }
  }
}

CRGB Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Random flashes of lightning
void lightning(CRGB c, int simultaneous, int cycles, int speed)
{
  int flashes[simultaneous];

  for(int i=0; i<cycles; i++){
    for(int j=0; j<simultaneous; j++){
      int idx = random(NUM_LEDS);
      flashes[j] = idx;
      leds[idx] = c ? c : Wheel(random(256));
    }
    FastLED.show();
    delay(speed);
    for(int s=0; s<simultaneous; s++){
      leds[flashes[s]] = CRGB::Black;
    }
    delay(speed);
  }
}
