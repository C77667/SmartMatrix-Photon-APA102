/*
  Demo intended to compare APA102 refreshing using Temporal Dithering, no Dithering, and SmartMatrix with Dynamic Global Brightness Control
  Set MODE and BRIGHTNESS defines at the top to reproduce the results in the demo video (MODE_SMARTMATRIX_COLORCORRECTION wasn't used in the video)

  Connect APA102 Data Pin to D2
  Connect APA102 Clock Pin to D4
*/

#define MODE_SMARTMATRIX_NOCOLORCORRECTION   0
#define MODE_SMARTMATRIX_COLORCORRECTION     1
#define MODE_FASTLED_DITHERING               2
#define MODE_FASTLED_NODITHERING             3

// SET SKETCH OPTIONS HERE:
#define MODE MODE_SMARTMATRIX_NOCOLORCORRECTION
#define BRIGHTNESS 255

#include "application.h"

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
  #include "SmartMatrix3_Photon_Apa102/SmartMatrix3_Photon_Apa102.h"
#endif

#include "FastLEDSmartMatrix/FastLEDSmartMatrix.h"
FASTLED_USING_NAMESPACE;

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
  #define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
  const uint8_t kMatrixWidth = 16;        // known working: 16, 32, 48, 64
  const uint8_t kMatrixHeight = 16;       // known working: 32, 64, 96, 128
  const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
  const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
  const uint8_t kPanelType = 0;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
  const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
  const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
  const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
  
  SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
  SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
  SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
  
  rgb24 *buffer;
  
  const uint16_t NUM_LEDS = kMatrixWidth * kMatrixHeight;
#else
  #define DATA_PIN D2
  #define CLOCK_PIN D4
  
  #define COLOR_ORDER BGR
  #define CHIPSET     APA102
  
  // Params for width and height
  const uint8_t kMatrixWidth = 16;
  const uint8_t kMatrixHeight = 16;
  
  const uint16_t NUM_LEDS = kMatrixWidth * kMatrixHeight;
  
  CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
  CRGB* leds( leds_plus_safety_pixel + 1);
#endif

const uint8_t scale = 256 / kMatrixWidth;

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
  uint16_t XY(uint8_t x, uint8_t y) {
    return kMatrixWidth * y + x;
  }
#else
  uint16_t XY( uint8_t x, uint8_t y)
  {
    uint16_t i;
    
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
    
    return i;
  }
#endif

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
  // scale the brightness of all pixels down
  void dimAll(byte value)
  {
    for (int i = 0; i < NUM_LEDS; i++) {
      CRGB c = CRGB(buffer[i].red, buffer[i].green, buffer[i].blue);
      c.nscale8(value);
      buffer[i] = c;
    }
  }
#else
  // scale the brightness of all pixels down
  void dimAll(byte value)
  {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].nscale8(value);
    }
  }
#endif

void setup() {
#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.begin();
  matrix.setBrightness(BRIGHTNESS);
  backgroundLayer.enableColorCorrection(false);
#else
  FastLED.addLeds<CHIPSET, DATA_PIN, CLOCK_PIN, COLOR_ORDER, DATA_RATE_MHZ(1)>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );

  #if (MODE == MODE_FASTLED_DITHERING)
    FastLED.setDither(BINARY_DITHER);
  #else
    FastLED.setDither(DISABLE_DITHER);
  #endif

  FastLED.show();  
#endif

  delay(2000);
}

void loop() {
  EVERY_N_MILLISECONDS(1000/30) {

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
    buffer = backgroundLayer.backBuffer();
#endif      

    dimAll(250);

    static uint8_t theta = 0;
    static uint8_t hue = 0;
    
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      uint8_t y = quadwave8(x * 2 + theta) / scale;
#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
      buffer[XY(x, y)] = CRGB(CHSV(x + hue, 255, 255));
#else
      leds[XY(x, y)] = CHSV(x + hue, 255, 255);
#endif      
    }

    theta++;
    hue++;

#if (MODE == MODE_SMARTMATRIX_NOCOLORCORRECTION || MODE == MODE_SMARTMATRIX_COLORCORRECTION)
    backgroundLayer.swapBuffers(true);
#else
    FastLED.show();
#endif
  }

#if (MODE == MODE_FASTLED_DITHERING)
  // FastLED needs to update LEDs during idle time to do dithering
  FastLED.show();
#endif
}
