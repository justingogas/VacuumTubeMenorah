/* Title: vacuumTubeMenorah
 * Author: Justin Gogas
 * Description: This project is for a 9x WS2812 LED string with a separate switch for each LED so that each LED can light individually,
 *   like a candle for a real menorah.  There are two modes: a flicker flame effect and a rainbow RGB effect.  A switch is attached
 *   to change the mode from one to another.  Vacuum tubes with their bottoms drilled out to make room for a 5mm WS2812 purchased from
 *   SparkFun (https://www.sparkfun.com/products/12986).  This is done on an Arduino Pro Mini from SparkFun and has enough IO pins to
 *   work.  This code could easily be adapted if more or less LEDs are needed by setting constants up or down and adding more or less
 *   array structures.
 *  
 * Wiring: These are numbered according to how I had to wire my switches and LEDs in a way that made sense for my enclosure and the 
 *   direction things were pointing.  You can adjust these to match how you wire things up to yours.
 *   Pin 2 with pullup activated -> ground (controls LED 5).
 *   Pin 3 with pullup activated -> ground (controls LED 6).
 *   Pin 4 with pullup activated -> ground (controls LED 7).
 *   Pin 5 with pullup activated -> ground (controls LED 8).
 *   Pin 6 with pullup activated -> ground (controls LED 1).  This is out of the LED order because pin 13 could not be used (see below).
 *   Pin 8 with pullup activated -> ground (controls mode switching).
 *   Pin 9 -> data in for first WS2812 LED.  Data out for that LED goes into the data in for the next, and LED 8's data out goes nowhere.
 *   Pin 10 with pullup activated -> ground (controls LED 4).
 *   Pin 11 with pullup activated -> ground (controls LED 3).
 *   Pin 12 with pullup activated -> ground (controls LED 2).
 *   Pin 13 -> not connected because it is connected to the onboard LED on Pro Mini and requires extra resistance to be an input, which doesn't fit the JST connectors I was using.
 *   Pin A0 with pullup activated -> ground (controls LED 0).
 *
  * Improvements: I used a shelf to mount the LEDs and switches to, but adding the circuitry for the switches, LEDs, power, wiring, and
 *   Arduino turned out to be a very tight fit.  Half the production time for this project was spent fighting the cramped quarters.
 *   Unless the enclosure has ample space, I would recommend having the microcontroller in a separate enclosure near the power and
 *   run minimal wires up to the menorah.  Instead of wires for all the switches, one can either create some stepped ladder of an
 *   analog voltage to return to the microcontroller over a single wire, or some sort of IC to take digital inputs and convert them
 *   into a serial signal, or I2C interface.  That would take the number of wires from a control box near the plug to the menorah from
 *   8 switches + 1 LED data + power + ground = 11 wires to 1 or 2 switches + 1 LED data + power + ground = 4 or 5 wires.
 *    
 *   With more space and time, I would also add a potentiometer to control the total brightness, and have even more effect modes
 *   (probably just add the other modes from NeoPixel strandtest example) that are cycled from a button instead of a switch.
*/

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Define constants that will be used.
#define BRIGHTNESS 100
#define LED_COUNT 9
#define LED_PIN 9
#define MODE_PIN 8
#define TOTAL_MODES 2

// Defaults for the flicker candle mode.  The three RGB values together is a good starting point for a red/yellow flame.
#define flickerStartRed 255
#define flickerStartGreen 96
#define flickerStartBlue 12

#define flickerDelayHigh 50
#define flickerDelayLow 50
#define flickerFlickerHigh 40
#define flickerFlickerLow 0


// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


// Define a holding structure for switch pins and their current state to know which LEDs to turn on and off.
typedef struct {
  int order;      // The WS2812 order of the LED.
  uint8_t pin;    // The Arduino pin number of the LED.  Not sequential because of the number of available pins on each side of the board (there are not 9 sequential on a Pro Mini).
  boolean state;  // Whether the switch that corresponds to that LED is on or off.
} candleLed;

// Define a holding structure for the flicker candles so that each candle has a different
typedef struct {
    int red;
    int green;
    int blue;
    int delay;    // The random time that this candle will stay on its color before resetting.
    int flicker;  // The random value that will be subtracted from the other colors to generate a difference in total color between resets.
    int time;     // The count of milliseconds that this candle has been lit.
} candleFlicker;

// Create 9 candle LEDs that map from a pin to an LED in a distinct order, and the state of the switch.
candleLed candles[LED_COUNT] = {
  { 0, A0, false },
  { 1, 6, false },
  { 2, 12, false },
  { 3, 11, false },
  { 4, 10, false },
  { 5, 2, false },
  { 6, 3, false },
  { 7, 4, false },
  { 8, 5, false }
};


// The current mode, defaulting to flicker candles.  Mode 0 = flicker; mode 1 = rainbow.
uint8_t mode = 0;

// Create variables for calculating the passing of time for the flicker candles.
long flickerMillisecondsCurrent = 0;
long flickerMillisecondsDifference = 0;
long flickerMillisecondsLast = 0;

// Create 9 flicker candles for mode 0.
candleFlicker flickers[LED_COUNT] = {
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 },
  { flickerStartRed, flickerStartGreen, flickerStartBlue, 0, 0 }
};


void setup() {

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  // Activate pullup resistors for all switches and the mode switch.
  pinMode(MODE_PIN, INPUT_PULLUP);

  for (uint8_t candleIndex = 0; candleIndex < LED_COUNT; candleIndex++) {
    pinMode(candles[candleIndex].pin, INPUT_PULLUP);
  }

  // Start random values for the flicker candle uniqueness.
  randomSeed(analogRead(4));

  // Read the initial values of the switches and turn on / off LEDs based on their value.
  readSwitches();
}


void loop() {

  // There are two modes, a flicker candle effect and the standard NeoPixel flowing rainbow effect taken from the strandtest example.
  // The LED switches and mode switch is checked inside each mode loop, so LEDs can be turned on and off and the mode can be changed
  // during the processing for instant change feedback.
  
  if (mode == 0) {
    flicker();
  }

  else if (mode == 1) {
    rainbow(10);
  }
}


// Check the state of the LED switches and the mode switch, storing the result in the array of structs.  Return true if the state changed so logic can be run on whether the mode changed.
boolean readSwitches() {
  
  // Set each switch to on or off depending on its state.
  for (uint8_t candleIndex = 0; candleIndex < LED_COUNT; candleIndex++) {
    candles[candleIndex].state = (digitalRead(candles[candleIndex].pin) == LOW);    // Activate LED if its switch is flipped / conducting / low logic.
 candles[candleIndex].state = true;
  }

  // Get the state of the mode switch.
  uint8_t modeButtonState = digitalRead(MODE_PIN);

  // If the mode switch is in a different state than it needs to be, then flip the state and return true.
  if ( (modeButtonState == LOW && mode == 0) || (modeButtonState == HIGH && mode == 1)) {

    if (mode == 1) {
      mode = 0;
    } else {
      mode = 1;
    }

    return true;
  }

  return false; 
}


// Somewhat realistic candle flicker effect adapted from https://web.archive.org/web/20180721214611/www.walltech.cc/neopixel-fire-effect
void flicker() {

  // Only run the timing loop if more than one millisecond passed, since that is our smallest time quanta.
  flickerMillisecondsCurrent = millis();
  flickerMillisecondsDifference = flickerMillisecondsCurrent - flickerMillisecondsLast;

  if (flickerMillisecondsDifference > 1) {

    // Update the switches and mode.  If any change, only one millisecond of change will occur before the mode changes or LEDs turn on / off.
    readSwitches();

    // Calculate the time difference to only change the flicker candles if more than one millisecond has elapsed.  This throttles the 
    // candle animations to match a specific timing.
    flickerMillisecondsLast = flickerMillisecondsCurrent;

    for (uint8_t i = 0; i < LED_COUNT; i++) {

      // Each candle has its own random delay countdown length.  When it reaches the end of its cycle, then reset its values.
      if (flickers[i].delay <= 0) {

        flickers[i].delay = random(flickerDelayLow, flickerDelayHigh);
        flickers[i].flicker = random(flickerFlickerLow, flickerFlickerHigh);

        // Select new initial values for the RGB.
        flickers[i].red = flickerStartRed - flickers[i].flicker; 
        flickers[i].green = flickerStartGreen - flickers[i].flicker;
        flickers[i].blue = flickerStartBlue - flickers[i].flicker;

        // Bring the RGB back to zero if it went negative from the flicker (normally only happens to blue since it stars so low).
        if (flickers[i].red < 0)  { flickers[i].red = 0; }
        if (flickers[i].green < 0)  { flickers[i].green = 0; }
        if (flickers[i].blue < 0)  { flickers[i].blue = 0; }
      }

      // Set the pixel values for each candle, turning it off if that switch was deactivated.
      if (candles[i].state) {
        strip.setPixelColor(i, flickers[i].green, flickers[i].red, flickers[i].blue);
      } else {
        strip.setPixelColor(i, 0);
      }

      // Count down each candle's delay.
      flickers[i].delay--;
    }

    strip.show();
  }
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.  Adapted from the NeoPixel library strandtest example and mostly changed for style consistency..
void rainbow(int wait) {

  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {

    // Check the switch status and exit if the mode changed between strand changes.
    if (readSwitches()) { break; }

    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...

      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());

      if (candles[i].state) {
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      } else {
        strip.setPixelColor(i, 0);
      }
    }

    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}
