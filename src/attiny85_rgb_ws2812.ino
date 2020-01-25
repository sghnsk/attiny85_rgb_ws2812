// NeoPixel Star, (for 7-point IKEA star lamp) using the Adafruit NeoPixel library
//    by Zonker Harris, started at Hacker Dojo Grand Re-opening, NOV 11, 2016
//  loosely based on the SF Giants sign project v0.5, with an improved color array.
//  version 0.3 - adds a Mode pushbutton, cycle between rainbow, rainbow cycle, ramdom-all
//
//  includes my NeoPixel-adjusted color libraries, Thanksgiving weekend 2016
//    Developed on a Microsoft Surface Pro 3, running Windows 10 Enterprise
//     using the Arduino.cc IDE version 1.6.11, Adafruit_NeoPixel version 1.0.6
//     (debug display was the Adafruit 16-neopixel ring https://www.adafruit.com/products/1463
//     and the AVR Core from Spence Konde  https://github.com/SpenceKonde/ATTinyCore 
//     and the SparkFun TinyUSB programming stick https://www.sparkfun.com/products/11801
//     ATtiny85:  https://www.sparkfun.com/products/9378 $2.27 ea.
//                http://www.digikey.com/product-detail/en/atmel/ATTINY85-20PU/ATTINY85-20PU-ND/735469  
//     NeoPixels:  https://learn.adafruit.com/adafruit-neopixel-uberguide
//                 https://learn.adafruit.com/neopixel-painter/overview 
//                 https://learn.adafruit.com/search?q=attiny85%20neopixel&
//
//  Thanks to Alex Glow, for clues about "Burn Bootloader" when changing ATtiny clock speeds
//
//  All serial debugging removed, since I don't want to add SoftwareSerial (takes up space...)
//    all timings, and some routines, were tuned for better "random sequence" performance.

/*    "ASCII Art" schematic of the circuit...
                                                          -----------
(+5v)--+----+---------------------+--------------------->| 5v
       | +  |                     |            R1        |     WS-2811
 C1   ===   |  C2             ----------     330 ohm     |    NeoPixels
100 uf |    | 0.1uf          |    8    5|----'\/\/`----->| DI   strip
 10-v+ |   ===   --------+---|7         |   1/4-watt     |
       |    |   | 0.1 uf |   |(ATtiny85)|                |       ===>
       |    |  ===  C3   |   |         4|--+------------>| GND
       |    |   |      | O    ----------   |              -----------
       |    |   |     [|                   |
       |    |   |      | O  (mode switch)  |
       |    |   |        |                 |
(GND)--+----+---+--------+-----------------
*/

/* Zonker's ATtiny85 clues... NOV 2016
 *  
 *  0) After I thought I was pretty comfortable with Arduino using analog, PWM, SPI, i2c
 *     and more, playing with servos, LCD displays and sensors, I finally decided to learn
 *     what it takes to fit small projects into the ATtiny85, and I struggled for two week
 *     before I learned one thing: Once you set all the cpu/clock/options in the IDE, the
 *     IDE will tell you it saw options change, and it's "recompiling"... that is, it's
 *     recompiling your SKETCH CODE to adapt to the options... but the CHIP still has the
 *     same options as before! To CHANGE THE CHIP SETTINGS, you need to click on the
 *     Tools -> Burn Bootloader command, clear at the bottom of the menu. Just when you 
 *     change the chip options. (This is a KEY SETTING to make NeoPixels work at 8 or 16 MHz!)
 *     
 *  1) The ATtiny85 comes set for 1 MHz. In order to run NeoPixels, you'll need to set the
 *     clock for at least 8 MHz (Internal) or 16 MHz (PLL, Internal). They can support faster
 *     speeds, up to 20 MHz for some versions of the part, using an external oscillator, but
 *     you sacrifice a pin to do that. 
 *     
 *  2) You can use a mini-breadboard/protoboard to plug in your Tiny, and use jumper wires
 *     to connect to another Arduino UNO/etc., and program the Arduino to be a programmer.
 *     OR, you can buy a pre-built board to make this easy and quick. For $20(US), I use
 *     the SparkFun USBtiny https://www.sparkfun.com/products/11801. NOTE: the USBtiny board
 *     has sockets for jumper wires for all 8 pins, but the programmer will have trouble
 *     programming chips if you have signals on pins....... Take them off to burn 
 *     your bootloader, and to upload to the chips.
 *     
 *  3) Not all AVR Core (board) libraries have ATtiny85, and fewer still include the 16 MHz PLL
 *     option. Add the collection from Spence Konde (https://github.com/SpenceKonde/ATTinyCore)
 *     by adding http://drazzy.com/package_drazzy.com_index.json as an external Board Manager 
 *     to your IDE preferences. Then, re-read ATtiny clue 0 above. ;-)
 */
 
/* Zonker's NeoPixel clues... DEC 2015  
 *  It was hard for me to learn this, I hope these clues will help others understand it faster.
 *  
 *  0) This is a collection of NeoPixel animations/sequence effects. It's my toybox.
 *     Some animations will look really good on small strips and rings, but not longer strips.
 *     Some are designed for longer runs (and timing for the delays should be tuned faster...)
 *     Some are actually designed for matrix boards, where the "string" weaves back and forth...
 *     NeoPixels are timing-sensitive, using "bit-banging" of the ports, not SPI, not I2C
 *     (You need a 16 MHz Arduino, clocking at 8 MHz won't work well. Adafruit Trinket *can*...)
 *     
 *  1) Check how many full color array sets are there. Black is the last, but not a set!
 *     All of my arrays are tuples of Red, Green, Blue on a given line, to make some color
 *     Some of my arrays have two complimentary sets for the colors, some have more...
 *     The number of RGB tuples in each color set must be reflected in the randomSmall/randomLarge functions!
 * 
 *  2) NeoPixels write colors using 0-255 for each value (8 bits, 0 is black, 255 is brightest).
 *     Don't confuse the values with ShiftBrite arrays, which use 0-1023 (10 bits)for each value!
 *     The color intensities do NOT scale well between the two. You can't just divide by 4 per value.
 *     (This is why I built "Color Tuner" devices, to play with hues and intensities...)
 * 
 *  3) The number of pixels in your string/ring/array matter!
 *     If numLeds is too large, colors don't wind up where you expect.
 *     If numLeds is too small, some pixels don't light, or pick random colors.
 * 
 *  4) You don't need global variables for everything. You can re-use variable names within functions
 *     BUT, your values for LOCAL variables die when the function ends! Use them or lose them!
 *     If data needs to leave the function, either use the "return()" value, or write to GLOBALS.
 * 
 *  5) I use a lot of comments in my code, to make it easier for me, and for you, my dear reader. ;-)
 *     They do NOT take up space in your Arduino memory! (They are ignored during the upload process.)
 *     It's probably better to keep them in the code, to remind you why the code is the way it is...
 *     
 *  6) I use the serial.Print feature a LOT for debugging!  (Use [ctrl]+[shift]+[M] to start it.)
 *     If you don't want all the serial output, COMMENT the line(s) out, rather than delete the line.
 *     (You may decide later that you want to re-enable the output for one effect or another. :-)
 *     ** ATtiny doesn't have serial output. You can TRY to use softwareSerial, but that library
 *        will eat up precious space for code... develop on a bigger Arduino, then move to a Tiny.
 * 
 *  7) By using the (lpixel, hpixel) combination, you can easily write to any contiguous section
 *     By default, these would normally be set to 1 and (numLeds - 1), respectively. But, if you start
 *     to use intermediary numbers for *any*, you will need to set them for *ALL* functions.
 *     (This was a derivative of code developed for the "Sheri Sign" project, where there was one
 *     string of Shiftbrite LEDs, representing the 5 letters in "Sheri", and each letter had
 *     it's own set of low- and high-pixel variables so that I could change the color of each
 *     letter (section) individually by using that letter's low and high to set the lpixel and hpixel
 *     for the desired effect.)
 *       (NOTE 1: The rainbow functions affect the strip, and don't take pixel range arguments)
 *       (NOTE 2: Raindrop will be another special-purpose function, Cycles will be an optional argument)
 */
/*     
 *  8) The ideas for the effect names came to me during a breakfast at IHOP in Fort Collins CO,
 *     and the suffixes denoted a 'class' of effect (write, fade, pop...), and then modifiers.
 *     I've listed some of the original variables here for reference.
 *     
 *     int lpixel = 0;  //  Remember, lowest number will be the LAST in the chain!
 *     int hpixel = 34;  // The highest number is clocked out last; the 1st in the chain!
 *     int letterMax = 9; // how many pixels in the largest letter?
 *     int letter1l = 0;  // the first pixel in the first character... (S)
 *     int letter1h = 7;  // the first pixel in the first character...
 *     int letter2l = 8;  // the first pixel in the first character... (h)
 *     int letter2h = 16;  // the first pixel in the first character...
 *     int letter3l = 17;  // the first pixel in the first character... (e)
 *     int letter3h = 24;  // the first pixel in the first character...
 *     int letter4l = 25;  // the first pixel in the first character... (r)
 *     int letter4h = 30;  // the first pixel in the first character...
 *     int letter5l = 31;  // the first pixel in the first character... (i)
 *     int letter5h = 34;  // the first pixel in the first character...
 *     int myLight = (analogRead(3));  /* This hasn't been implemented yet...
 *        (This is a photo-diode from vcc to A3 input, with an internal pull-up assigned)
 *       It was intended to tell if it's dark out, and reduce the pixel brightness
 *     int lightPin = A0;   // Cadmium photo-cell (light detector)
 *     
 *  9) The effect pre/suffixes determine what the basic function will be...
 *       pop_______  write all of the related pixels, then show the pixel array at once
 *       wipe_______  write the first pixel, show the array, write the next pixel, show...
 *       flash_______  write the pixel(s) with white/bright grey, show, then write the color(s), show...
 *       ____fade  After the main function, change the pixel color to a dimmer hue of that color, show...
 *       random_______  Each pixel will be a random color, sometimes with a random interval.
 *         (write a pixel, show the array, wait, write the next pixel with a new color, show, wait...)
 * 
 *  10) If you are debugging a new routine, set the loop for just that function.
 *      Once you like the output, you can go back to random
 *      (I'll probably add a rotary encoder, to select one of the effects by turning a knob!)
 */

// We'll use "presses" as a global variable. It must be declared VOLATILE to write in an ISR!
volatile int presses = 0;  // if > 0, button was pressed, so we should increment "mode"
/* Interrupt Service Routines (ISRs) are complicated, are a bit finicky to use, and there
 *  are not many well-documented resources. I found Nick Gammon's great example page, which
 *  helped me clear this up (http://gammon.com.au/interrupts).
 * ISRs should be kept short and simple since they interrupt counting "millis", and 
 *  will impact serial writes (while the ISR is running), among other things.
 * BUT, let me add "Try this in a simple sketch first! See that it works, and then add
 *  some simple delay to the main loop, to see if that affects your sketch. See if you can 
 *  modify a variable in the ISR, or make a decision based on reading a pin, or even try to
 *  do something a little more complex, and see the ISR affect a change.
 * You *can* do some more interesting things, where you can use a range of pins, which 
 *  will trigger one interrupt, but your code needs to look at which pins have changed.
 *  This is handy for things like Rotary Encoders, I have read (but I haven't made it work...)
 */

// Include the Adafruit_NeoPixel library  https://github.com/adafruit/Adafruit_NeoPixel
// (and check out the Adafruit Arduino Metro Mini! https://www.adafruit.com/product/2590 )
#include <Adafruit_NeoPixel.h>

// The code below will save you from having to change the pin numbers when you
//  change which boards to which you will be uploading.
//
// Set pins, depending on if it's an ATtiny85 or a Pro Mini (larger Arduino...)
#ifdef __AVR_ATtiny85__
#define PIN_NEOPIXEL 0
#define PIN_RANDOMSEED 1
#define PIN_SWITCH   2   // on ATtiny85's, Int 0 = PB2 (chip pin 7)
#else
#define PIN_RANDOMSEED A1
#define PIN_SWITCH   2   // on UNO's, Int 0 = pin D2, and Int 1 = pin D3
#define PIN_NEOPIXEL 6
#endif

// My IKEA Star project uses 14 neopixels... do you use more or less in your project?
const int numLeds = 30; // How many neopixels in the string? used to start the NeoPixel library

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_RGB Pixels are wired for RGB bitstream
// NEO_GRB Pixels are wired for GRB bitstream
// NEO_KHZ400 400 KHz bitstream (e.g. FLORA pixels)
// NEO_KHZ800 800 KHz bitstream (e.g. High Density LED strip)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

int myPass = 0;   //  how many times have we cycled through the main loop?
int myPrevious = 3;   // what was the last BaseColor used?
int mode = 0;  // this variable maps to the sequence that we WANT to select
int sequence = 1;  //  which sequence number is up next?
int myPreviousChoice = 5;  // what was the last sequence number picked?
int pixelCount = numLeds;  // how many pixels, in a pixel-based effect?
int loopCount = 1;  //how many cycles for word-based effects?
int nameDelay = 1500;  // name-delay is longer delay, usually between effect patterns
int icDelay = 250;  // inter-character delay, for pauses between transitions
int ipDelay = 60;  // inter-pixel delay, so you can see the wipe.fade patterns move quickly
int myBase = 0;
int lpixel = 0;
int hpixel = (numLeds);
int cycles = 1;
int speed = 15;
int pick = 5;

/* Prepare the colors array
 myBase is modulo 15, with R-G-B values, from brightest to dimmest
 0=white, 15=red, 30=yellow, 45=green, 60=blue, 75=teal, 90=purple
 value 135=black (all zeroes).
 myColorCode is modulo 3, defining one shade of a given color in R-G-B.
 You can use any modulo-3 number in the array, from 0 (white) to 135 (black).
 18=almost-brightest red, 51 is medium-green, 72 is dimmest-blue. */
//Modified for NeoPixels…
int colorSets = 19;
int myColors[] = {
  //(0, 0) brite yellow
  160, 171, 0,
  37, 45, 0,
  //(1, 6)  blue
  0, 0, 160,
  0, 0, 60,
  //(2, 12)  mango, dark orange
  145, 25, 0,
  75, 12, 0,
  //(3, 18)  emerald green
  0, 160, 0,
  0, 60, 0,
  //(4, 24)  dark pink
  135, 0, 26,
  45, 0, 10,
  //(5, 30)  salmon
  112, 24, 20,
  43, 12, 8,
  //(6, 36)  bright pink
  134, 0, 126,
  59, 0, 41,
  //(7, 42)  red
  160, 0, 0,
  60, 0, 0,
  //(8, 48)  orange yellow
  137, 104, 0,
  50, 44, 0,
  //(9, 54)  brite green-yellow
  110, 152, 0,
  59, 84, 0,
  //(10, 60)  dark purple
  62, 0, 114,
  31, 0, 56,
  //(11, 66)  fuschia
  180, 0, 157,
  53, 0, 42,
  //(12, 72)  light lime
  49, 154, 0,
  30, 75, 0,
  //(13, 78)  cyan
  0, 90, 117,
  0, 33, 45,
  //(14, 84)  dim blue
  6, 11, 100,
  3, 8, 50,
  //(15, 90)  teal
  0, 167, 43,
  0, 70, 20,
  //(16, 96)  orange
  160, 60, 0,
  66, 28, 0,
  //(17, 102)  light green
  43, 175, 10,
  22, 64, 8,
  //(18, 108)  lavender
  146, 104, 182,
  89, 71, 106,
  //(19, 114) black
  0, 0, 0};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// Interrupt Service Routine (ISR)
void buttonSense() {
    presses++;
}

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode (PIN_RANDOMSEED, INPUT);  // just an antenna wire, to get random results as a seen
  
  int randomSeed(analogRead(1)); // give us a fairly random seed to start...

  pinMode(PIN_SWITCH, INPUT_PULLUP);  
  attachInterrupt(0, buttonSense, FALLING); 
  // on UNO's, Int 0 = pin D2, and Int 1 = pin D3
  // on ATtiny85, Int 0 = pin PB2 (chip pin 7)

  presses = 0;
  mode = 2;
  }

void loop() {
  myPass++ ;
  while (presses > 3) {
    presses = (presses - 3);
  }
  if(presses > 0) {
    mode = (mode + presses);
    presses = 0;
    if (mode > 3) {  // we've exceeded the number of animations, wrap to 1!
      mode = 1;
    }
  }

  // sequence = mode;  // Use a button to step through the sequences (comment the other two)
  sequence = randomChoice();  // operate fully random (comment the other two)
  //sequence = 1;  // to debug, pick the pattern you are debugging, comment the other two)
  
  if (sequence == 0) {
    blackNow();  // (If you select this manually, it looks like the pixels are all off...)
    delay(500);
    }
  if (sequence == 1) { // Random-all! Draw from the remaining effects below
    sequence = randomChoice();
    mode = 1;
    }
  if (sequence == 2) {
    // Set the pixels to a rainbow, then cycles all the pixels through the rainbow, *twice*
    int cycles = 1;
    if (mode == 1) { cycles = random(1,3); } // roll the dice, 2-4 repeats 
    speed = random(10,75);  // How slow should the color shift? Higher number = slower
    rainbowCycle(speed, cycles);  
    if (mode == 1) { delay(nameDelay); }  //  Add a pause to enjoy the effect
    }
  else if (sequence == 3) {
    cycles = 1;
    if (mode == 1) { cycles = random(2,4); } // roll the dice, 2-4 repeats 
    int speed = random(10,75);  // How slow should the colors shift? Higher number = slower
    rainbow(speed, cycles);
    //delay(nameDelay);  // Set all pixels to color, then cycles the pixels through the rainbow once
    }
  else if (sequence == 4) {
    /* This sequence 'wipes' all the pixels, one at a time, with a color, but it leads
     *  the wipe with a dark, unlit pixel (like an eraser that changes the stripe color) */
    int cycles = 1;
    if (mode == 1) { cycles = random(2,7); } // roll the dice, 2-7 repeats 
    blackLeadWipeColors(cycles, 50); // Black pixel at the lead, followed by colors, cycles, delay
    if (mode == 1) { delay(nameDelay); }  //  Add a pause to enjoy the effect
    }
  else if (sequence == 5) {
    // pick pixels at random, set each pixel to a random color
    wipeRandom(ipDelay, 1);
    int pixels = random(20,65);  // How many pixel changes? Pick a large range (low, high)
    randomPixels(500, pixels); // delay between changes, how many pixels to change
    }
  else if (sequence == 6) {  
    /* This sequence 'wipes' all the pixels, one at a time, with a color, but it leads
     *  the wipe with a bright, white color (like a meteor passing, with a colored tail). */
    cycles = 1;
    if (mode == 1) { cycles = random(2,7); } // roll the dice, 2-7 repeats 
    whiteLeadWipeColors(cycles, 30); // White pixel at the lead, followed by colors, cycles, delay
    if (mode == 1) { delay(nameDelay); }  //  Add a pause to enjoy the effect
    }
  else if (sequence == 7) { 
    /* This sequence 'wipes' all the pixels, one at a time, with a random color, but it leads
     *  the wipe with a dark, unlit pixel (like an eraser that changes the stripe color) */
    int cycles = 1;
    if (mode == 1) { int cycles = random(1,6); }  // roll the dice, 1-6 repeats 
    wipeRandom(ipDelay, cycles);  // set each pixel to a random color in sequence
    if (mode == 1) { delay(nameDelay); }  //  Add a pause to enjoy the effect
    }
  else if (sequence == 8) { 
    /* The way the IKEA Star is wired, one pixel is on the "top/left" side of each arm,
     *  and the next pixel is on the "bottom/right side of that arm. This sequence tries
     *  to paint the "top" with one color, and the "bottom" with another color.  */
    cycles = 1;
    if (mode == 1) { cycles = random(1,6); } // roll the dice, 1-6 repeats 
    int pause = (2000 * (random(1,6)) );  //  Add a pause to enjoy the effect
    wipeColorM2(cycles, pause); 
    }
  else if (sequence == 9) { 
    if (mode == 1) { cycles = random(1,6); } // roll the dice, 1-6 repeats 
    int color = randomSmall();  // pick the base color for all the pixels
    wipeColor(color, cycles, 35); // set all the pixels to that color
    if (mode == 1) { delay(nameDelay); }  //  Add a pause to enjoy the effect
    }
  else { 
  myBase = randomSmall();
  delay(icDelay);
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
int randomChoice() {
  sequence = (random(2, 9));  //  How many sequences do we have to choose from?
  if ( sequence == myPreviousChoice)  { sequence = (random(2, 9)); }
  if ( sequence == myPreviousChoice)  { sequence = (random(2, 9)); }
  if ( sequence == myPreviousChoice)  { sequence = (random(2, 9)); }  
  myPreviousChoice = sequence;
  return sequence;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
int randomSmall() {
  int result;
  result = (random(1, 17) * 6);
  //if (sensorValue < 500) { result = (result + 3); }
  if ( result == myPrevious)  { result = (random(0, colorSets) * 6); }
  if ( result == myPrevious)  { result = (random(0, colorSets) * 6); }
  if ( result == myPrevious)  { result = (random(0, colorSets) * 6); }
  myPrevious = result;
  return result;
}
  
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
int randomBig() {
  int result;
  result = (random (0, (colorSets * 2)) * 3);
  if ( result == myPrevious)  { result = (random(1, (colorSets * 2)) * 3); }
  if ( result == myPrevious)  { result = (random(1, (colorSets * 2)) * 3); }
  if ( result == myPrevious)  { result = (random(1, (colorSets * 2)) * 3); }
  myPrevious = result;
  return result;
}
  
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void blackNow() {
  for(uint8_t i=0; i < numLeds; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//   ** Original Adafruit example code
void rainbow(uint8_t wait, uint8_t cycles) { 
  //Sets all pixels to green, then cycles all the pixels at once through
  // the colors of the rainbow. Increase the delay to slow it down.
  for(int l=0; l < cycles; l++) {
    uint16_t i, j;
    for(j=0; j<256; j++) {
      for(i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel((i+j) & 255));
      }
      strip.show();
      delay(wait);
    }
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//   ** Original Adafruit example code
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t speed, uint8_t cycles) { 
  // This sets each pixel to a different color of the rainbow, start to end,
  // then cycles all the pixels through their respective rainbows, twice.
  // add some delay to slow down the cycle of the colors.
  for(int l=0; l < cycles; l++) {
    uint16_t i, j;
    for(j=0; j<256*2; j++) { 
      for(i=0; i< strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      }
      strip.show();
      delay(speed);
    }
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//   ** Original Adafruit example code
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void randomPixels(uint16_t wait, uint16_t pixels) { 
  for(int l=0; l < pixels; l++) {
    int i = random(0,numLeds);
    myBase = randomSmall();
    // Serial.println(myBase);
    int red = myColors[myBase];
    int green = myColors[myBase + 1];
    int blue = myColors[myBase + 2];
    strip.setPixelColor(i, red, green, blue);
    strip.show();
    delay(wait);
  }
  //delay(nameDelay);
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void randomPixels2(uint16_t wait, uint8_t pixels) { 
  for(int l=0; l < pixels; l++) {
    int i = random(0,numLeds);
    myBase = randomSmall();
    // Serial.println(myBase);
    int red = myColors[myBase];
    int green = myColors[myBase + 1];
    int blue = myColors[myBase + 2];
    strip.setPixelColor(i, red, green, blue);
    strip.show();
    delay(wait);
  }
  //delay(nameDelay);
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void wipeColor(uint8_t c, uint16_t cycles, uint16_t wait) { 
  for(int l=0; l < cycles; l++) {
    for(uint8_t i=0; i < numLeds; i++) {
      int red = myColors[c];
      int green = myColors[c + 1];
      int blue = myColors[c + 2];
      strip.setPixelColor(i, red, green, blue);
      strip.show();
      delay(ipDelay);
    }
    delay(wait);
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void wipeColorM2(uint8_t cycles, uint16_t wait) { 
  for(int l=0; l < cycles; l++) {
    int c = randomSmall();  // pick the color for the top/left
    int d = randomSmall();  // pick the color for the lower/right
    while ( c == d )  { d = randomSmall(); }  // if the two colors match, pick #2 again!
      for(uint8_t i=0; i < numLeds; i=i+2) {
        int cr = myColors[c];
        int cg = myColors[c + 1];
        int cb = myColors[c + 2];  
        int ar = myColors[d];
        int ag = myColors[d + 1];
        int ab = myColors[d + 2];
        strip.setPixelColor(i, ar, ag, ab);  //set the pixel to the first color
        strip.setPixelColor(i+1, ar, ab, ag); // set the next pixel to second color
        strip.show();
       }
      delay(wait);  //  Add a pause to enjoy the effect
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void blackLeadWipeColors(uint8_t cycles, uint16_t wait) { 
/* This sequence 'wipes' all the pixels, one at a time, with a color, but it leads
 *  the wipe with a dark, unlit pixel (like an eraser that changes the stripe color) */
  for(uint8_t l=0; l < cycles; l++) {
    strip.setPixelColor(0, 0);
    strip.show();
    delay(ipDelay);
    for(uint8_t c=0; c < cycles; c++) {
      myBase = randomSmall();
      // Serial.println(myBase);
      int red = myColors[myBase];
      int green = myColors[myBase + 1];
      int blue = myColors[myBase + 2];
      for(uint8_t i=0; i < numLeds; i++) {
        strip.setPixelColor(i, red, green, blue);
        strip.setPixelColor(i + 1, 0);
        strip.show();
        delay(ipDelay);
      }
    int pause = (500 * random(2, 8));
    delay(pause);
    }
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void whiteLeadWipeColors(uint8_t cycles, uint16_t wait) { 
/* This sequence 'wipes' all the pixels, one at a time, with a color, but it leads
*  the wipe with a bright, white color (like a meteor passing, with a colored tail). */
  for(uint8_t c=0; c < cycles; c++) {
    strip.setPixelColor(0, 235, 235, 235);
    strip.show();
    delay(ipDelay);
    myBase = randomSmall();
    // Serial.println(myBase);
    int red = myColors[myBase];
    int green = myColors[myBase + 1];
    int blue = myColors[myBase + 2];
    for(uint8_t i=0; i < numLeds; i++) {
      strip.setPixelColor(i, red, green, blue);
      strip.setPixelColor(i + 1, 235, 235, 235);
      strip.show();
      delay(ipDelay);
    }
    int pause = (500 * random(2,8));
    delay(pause);    
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
void wipeRandom(uint16_t wait, uint16_t cycles) { 
  for(uint16_t l=0; l < cycles; l++) {
    for(uint8_t i=0; i < numLeds; i++) {
      myBase = randomSmall();
      // Serial.println(myBase);
      int red = myColors[myBase];
      int green = myColors[myBase + 1];
      int blue = myColors[myBase + 2];
      strip.setPixelColor(i, red, green, blue);
      strip.show();
      delay(wait);
    }
    delay(ipDelay);
  }
  delay(nameDelay);
}

