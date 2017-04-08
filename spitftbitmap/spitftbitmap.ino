/***************************************************
  This is our Bitmap drawing example for the Adafruit HX8357 Breakout
  ----> http://www.adafruit.com/products/2050

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <Adafruit_GFX.h>    // Core graphics library
#include "Adafruit_HX8357.h"
#include <SPI.h>
#include <SD.h>
#include <math.h>

#define M_PI 3.14159265358979323846
#define MAX_VEL 40
#define LEN 200
#define CENTER (480/2)
#define BARS 5

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.

#define TFT_DC 9
#define TFT_CS 10
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
#define SD_CS 4

void setup(void) {
  Serial.begin(9600);

  tft.begin(HX8357D);
  tft.fillScreen(HX8357_BLACK);

  //Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    //Serial.println("failed!");
  }
  //Serial.println("OK!");

  // set the screen orientation
  tft.setRotation(1);
  // draw the background
  bmpDraw("back.bmp", 30, 50);

}

void loop() {
  // make some variables
  float dist, vel = 0, angle;
  
  int16_t start_x;
  int16_t start_y;
  int16_t end_x;
  int16_t end_y;
  int16_t x_offsets[BARS];
  int16_t y_offsets[BARS];

  int16_t prev_start_x;
  int16_t prev_start_y;
  int16_t prev_end_x;
  int16_t prev_end_y;
  int16_t prev_x_offsets[BARS];
  int16_t prev_y_offsets[BARS];
  //while(Serial.available() < 9);
  //parse_data(&dist, &vel);
  vel += 1;
  calculate_needle_angle(vel, &angle);
  calculate_needle_positions(&start_x, &start_y, &end_x, &end_y, &x_offsets, &y_offsets, angle);

  // draw over black needle
  draw_needle(prev_start_x, prev_start_y, prev_end_x, prev_end_y, prev_x_offsets, prev_y_offsets, HX8357_BLACK);
  draw_needle(start_x, start_y, end_x, end_y, x_offsets, y_offsets, HX8357_WHITE);

  // TODO draw black box
  // TODO draw distance
  
  // copy previous values
  prev_start_x = start_x;
  prev_start_y = start_y;
  prev_end_x = end_x;
  prev_end_y = end_y;
  int16_t bar;
  for(bar = 0; bar < BARS; bar++)
  {
    prev_x_offsets[bar] = x_offsets[bar];
    prev_y_offsets[bar] = y_offsets[bar];
  }
}

void calculate_needle_angle(int16_t vel, float *angle)
{
  vel = (vel > MAX_VEL) ? 40 : vel;
  *angle = 180 - (180.0*vel/MAX_VEL);
}

void calculate_needle_positions(int16_t *start_x, int16_t *start_y, int16_t *end_x, int16_t *end_y, int16_t **x_offsets, int16_t **y_offsets, float angle)
{
  int16_t bar;
  for (bar = 0; bar < BARS; bar++)
  {
    (*x_offsets)[bar] = (bar-((BARS/2)))*cos((angle-90)*M_PI/180);
    (*y_offsets)[bar] = (bar-((BARS/2)))*sin((angle-90)*M_PI/180);
  }
  // calculate the start point
  *start_x = CENTER
  *start_y = 320 - 10;
  // calculate the end point
  *end_x = start_x + (LEN*cos(angle*M_PI/180));
  *end_y = start_y - (LEN*sin(angle*M_PI/180));
}

void draw_needle(int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, int16_t *x_offsets, int16_t *y_offsets, int16_t color)
{
  int bar;
  for(bar = 0; bar < BARS; bar++)
  {
    tft.drawLine(start_x + x_offsets[bar], start_y + y_offsets[bar], end_x + x_offsets[bar], end_y + y_offsets[bar], color);
  }
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  //Serial.println();
  //Serial.print(F("Loading image '"));
  //Serial.print(filename);
  //Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    //Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    //Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    //Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    //Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      //Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        //Serial.print(F("Image size: "));
        //Serial.print(bmpWidth);
        //Serial.print('x');
        //Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        //Serial.print(F("Loaded in "));
        //Serial.print(millis() - startTime);
        //Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) //Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

