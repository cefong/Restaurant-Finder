// -----------------------------------------
// Name: Celine Fong
// ID: 1580124
// CMPUT 275, Winter 2020
// 
// Weekly Exercise 1: Display and Joystick
// -----------------------------------------

#define SD_CS 10
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL   53

#include <Arduino.h>

// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>

// LCD and SD card will communicate using the Serial Peripheral Interface (SPI)
// e.g., SPI is used to display images stored on the SD card
#include <SPI.h>

// needed for reading/writing to SD card
#include <SD.h>

#include "lcd_image.h"


MCUFRIEND_kbv tft;

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320
#define YEG_SIZE 2048

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

#define JOY_CENTER   512
#define JOY_DEADZONE 64
#define BUFFER 400

#define CURSOR_SIZE 9

// the current cursor position on the display 
int cursorX, cursorY;

// the previous cursor postion on the display
int prevX, prevY;

// coordinates to draw middle of map
int yegMiddleX, yegMiddleY;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

void setup() {
  init();

  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT_PULLUP);

  //    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  Serial.print("ID = 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield
  
  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed! Is it inserted properly?");
    while (true) {}
  }
  Serial.println("OK!");

  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
  yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
  lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH - 60)/2;
  cursorY = DISPLAY_HEIGHT/2;

  redrawCursor(TFT_RED);
}

void redrawCursor(uint16_t colour) {
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, colour);
}

// redraws portion of map that cursor has just left to prevent black trail
void redrawMap() {
  int adjustX = prevX - CURSOR_SIZE/2;
  int adjustY = prevY - CURSOR_SIZE/2;
  int mapPosX = yegMiddleX + adjustX;
  int mapPosY = yegMiddleY + adjustY;
  lcd_image_draw(&yegImage, &tft, mapPosX, mapPosY,
                 adjustX, adjustY,
                 CURSOR_SIZE, CURSOR_SIZE);
}

void processJoystick() {
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  int buttonVal = digitalRead(JOY_SEL);

  // stores the previous position of the cursor
  prevX = cursorX;
  prevY = cursorY;

  // now move the cursor
  if (yVal < (JOY_CENTER - JOY_DEADZONE)) {
    if (yVal < (JOY_CENTER - JOY_DEADZONE - BUFFER)) {
      cursorY -= 5;
    }
    cursorY -= 1; // decrease the y coordinate of the cursor
  }
  else if (yVal > JOY_CENTER + JOY_DEADZONE) {
    if (yVal > (JOY_CENTER + JOY_DEADZONE + BUFFER)) {
      cursorY += 5;
    }
    cursorY += 1;
  }

  // remember the x-reading increases as we push left
  if (xVal > JOY_CENTER + JOY_DEADZONE) {
    if (xVal > JOY_CENTER + JOY_DEADZONE + BUFFER) {
      cursorX -= 5;
    }
    cursorX -= 1;
  }
  else if (xVal < JOY_CENTER - JOY_DEADZONE) {
    if (xVal < JOY_CENTER - JOY_DEADZONE - BUFFER) {
      cursorX += 5;
    }
    cursorX += 1;
  }

  // constrains cursor position to within the map patch displayed
  cursorX = constrain(cursorX, CURSOR_SIZE/2, (DISPLAY_WIDTH - 61 - CURSOR_SIZE/2));
  cursorY = constrain(cursorY, CURSOR_SIZE/2, (DISPLAY_HEIGHT - CURSOR_SIZE/2));

  // only redraws patch of Edmonton map if cursor position has changed
  // this is to prevent "flickering"
  if (prevX != cursorX or prevY != cursorY) {
    redrawMap();
  }

  // draw new cursor at new position
  redrawCursor(TFT_RED);
  delay(20);
}

int main() {
  setup();

  // query position of cursor
  while (true) {
    processJoystick();
  }

  Serial.end();
  return 0;
}
