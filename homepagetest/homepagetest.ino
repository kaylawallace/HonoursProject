#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"

// For the Adafruit shield, these are the default.
#define TFT_DC 30
#define TFT_CS 53
#define TFT_BL 9

#define DHTPIN 7

#define GREEN    0x2BC9
#define WHITE    0xFFFF
#define YELLOW   0xEDAC 
#define BROWN    0xA3A9
#define BLACK    0x0000

  // TODO: USE THIS TO DIM BACKLIGHT
  // analogWrite(TFT_BL, 10);   

  // could be useful?
  // tft.readcommand8(ILI9341_DISPON);

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_BL);
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI
//9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
GFXcanvas1 canvas(200, 50);

const int btnPin1 = 2;
const int btnPin2 = 4;
const int btnPin3 = 7;
const int waterSensorPin = 5;
const unsigned long oneMin = 60000; //1 min in millis
const unsigned long sleepTime = 5*oneMin; //5 mins
int btnState1 = 0;
int btnState2 = 0;
int btnState3 = 0;
int prevBtnState3 = 0;
int liquidlevel = 0;
int prevlevel = 0;
int pageNum; 
int prevPageNum;
bool disabled;
bool needsRefill;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Home page test!"); 
  pinMode(btnPin1, INPUT);
  pinMode(btnPin2, INPUT);
  pinMode(btnPin3, INPUT);
  pinMode(waterSensorPin, INPUT);
  pinMode(TFT_BL, OUTPUT);

  tft.begin();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setFont();
  
  drawHomePage();
}

void loop() {
   // put your main code here, to run repeatedly:
   btnState1 = digitalRead(btnPin1);
   btnState2 = digitalRead(btnPin2);
   btnState3 = digitalRead(btnPin3);

   // only run this section if current page = home page 
   if (pageNum == 0) {
      if (btnState1 == HIGH && !disabled) {
          Serial.println("btn 1");
          disabled = true;
          pageNum = 1;
          drawOptionsPages(pageNum);
      }
      if (btnState2 == HIGH && !disabled) {
        Serial.println("btn 2");
        disabled = true;
        pageNum = 2;
        drawOptionsPages(pageNum);
      }
      if (btnState3 == HIGH && !disabled) {
        Serial.println("btn 3");
        disabled = true;
        pageNum = 3;
        drawOptionsPages(pageNum);

      }
   }
   else {
     checkWaterLevel();
   }
}

void checkWaterLevel() { 
  liquidlevel = digitalRead(waterSensorPin);
  if (liquidlevel == 0 && prevlevel == 1) {
    Serial.println("PLEASE REFILL");
    drawRefillPage();
    prevPageNum = pageNum;
    pageNum = 4;
    needsRefill = true;
    disabled = false; 
  }

  if (btnState3 == HIGH && prevBtnState3 == LOW) {   
    if (needsRefill && liquidlevel == HIGH) {
      Serial.println("msg dismissed");
      needsRefill = false; 
      pageNum = prevPageNum;
      drawOptionsPages(pageNum);
    }
  }
  prevBtnState3 == btnState3;
  prevlevel = liquidlevel;
  
}

void drawHomePage() {
  pageNum = 0;
  fillScreenBg(GREEN);
  writeText(10, 10, "hello, what would you", YELLOW);
  writeText(10, 30, "like to plant today?", YELLOW);

  drawBtn(25, 60, 270, 50, 25, YELLOW, BROWN);  
  drawBtn(25, 120, 270, 50, 25, YELLOW, BROWN);  
  drawBtn(25, 180, 270, 50, 25, YELLOW, BROWN);


  writeText(110, 80, "seedling", BLACK);
  writeText(70, 140, "leafy herbs/veg", BLACK);
  writeText(40, 200, "sun-loving fruit/veg", BLACK);
}

void drawOptionsPages(int pageNum) {
  if (pageNum == 1) {
    fillScreenBg(GREEN);
    drawBtn(25, 30, 270, 50, 25, YELLOW, BROWN); 
    writeText(110, 45, "seedling", BLACK); 
    writeText(100, 120, "growing...", YELLOW);
  }
  else if (pageNum == 2) {
    fillScreenBg(GREEN);
    drawBtn(25, 30, 270, 50, 25, YELLOW, BROWN);
    writeText(70, 45, "leafy herbs/veg", BLACK); 
    writeText(100, 120, "growing...", YELLOW);
  }
  else if (pageNum == 3) {
    fillScreenBg(GREEN);
    drawBtn(25, 30, 270, 50, 25, YELLOW, BROWN);
    writeText(40, 45, "sun-loving fruit/veg", BLACK); 
    writeText(100, 120, "growing...", YELLOW);
  }
}

void drawRefillPage() {
  fillScreenBg(GREEN);
  drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
  writeText(57, 62, "REFILL WATER TANK", BLACK);
  drawBtn(75, 150, 170, 60, 25, YELLOW, BROWN);
  writeText(137, 172, "DONE", BLACK);
}

void fillScreenBg(uint16_t colour) {
  tft.fillScreen(colour);
}

void drawMsgBox(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r, uint16_t fill_col, uint16_t outline_col) {
 
 if (fill_col != NULL) {
  tft.fillRoundRect(x0, y0, w, h, r, fill_col);
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
 }
 else {
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
 }
 
}

void drawBtn(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r, uint16_t fill_col, uint16_t outline_col) {
  tft.fillRoundRect(x0, y0, w, h, r, fill_col);
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
}

void writeText(uint16_t cursor_x, uint16_t cursor_y, String text, uint16_t font_col) {  
  tft.setTextColor(font_col);  
  tft.setCursor(cursor_x, cursor_y);
  tft.println(text);
}