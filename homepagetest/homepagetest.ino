#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"
#include <DS3231.h>


// For the Adafruit shield, these are the default.
#define TFT_DC 30
#define TFT_CS 53
#define TFT_BL 11

#define DHTPIN 24
#define DHTTYPE DHT11 

#define MOISTURESENSORPIN A0
#define PUMPPIN A3

#define RED_LED 6
#define BLUE_LED 5
#define GREEN_LED 10
#define WHITE_LED 9
int brightness = 255;
int gBright = 0;
int rBright = 0;
int bBright = 0;
int wBright = 0;
int fadeSpeed = 10;

#define GREEN    0x2BC9
#define WHITE    0xFFFF
#define YELLOW   0xEDAC 
#define BROWN    0xA3A9
#define BLACK    0x0000

#define RTC_SDA 20
#define RTC_SCL 21


// Init the DS3231 using the hardware interface
DS3231  rtc(RTC_SDA, RTC_SCL);
Time t;

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
DHT dht(DHTPIN, DHTTYPE);

const int btnPin1 = 2;
const int btnPin2 = 4;
const int btnPin3 = 7;
const int waterSensorPin = 3;
const unsigned long oneMin = 60000; //1 min in millis
const unsigned long sleepTime = 5*oneMin; //5 mins 
unsigned long prevMillis = 0;
unsigned long prevMillis2 = 0;
unsigned long prevMillis3 = 0;
unsigned long prevMillis4 = 0;
unsigned long prevMillis5 = 0;
unsigned long prevMillis6 = 0;
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
bool tempTooLow;
bool sleeping;
float soilmoisture;
bool refillTank;
bool tempHumTooLow;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Home page test!"); 
  pinMode(btnPin1, INPUT);
  pinMode(btnPin2, INPUT);
  pinMode(btnPin3, INPUT);
  pinMode(waterSensorPin, INPUT);
  pinMode(TFT_BL, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(TFT_DC, INPUT);
  pinMode(TFT_CS, OUTPUT);
  //pinMode(TFT_BL, INPUT);

//   #define TFT_DC 30
// #define TFT_CS 53
// #define TFT_BL 9
 // pinMode(blinkPin, OUTPUT);

  tft.begin();
  dht.begin();
  rtc.begin();

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

   pinMode(GREEN_LED, OUTPUT);
   pinMode(RED_LED, OUTPUT);
   pinMode(BLUE_LED, OUTPUT);
   pinMode(WHITE_LED, OUTPUT);

  //  rtc.setDOW(MONDAY);
  //  rtc.setTime(10, 36, 0);
  //  rtc.setDate(5, 4, 2021);
  
  // rtc.setTime(__TIME__);
  // rtc.setDate(__DATE__);

  drawHomePage();

  // TurnOnLEDs();
  // delay(1000);
  // TurnOffLEDs();
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
    t = rtc.getTime();
    
    checkWaterLevel(2000);
    checkTempAndHum(60000);
    checkSoilMoisture(30000);   
    controlGrowLight();

    // button 1 acts as back to homepage button when having selected option 1/2/3
    if (btnState1 == HIGH) {
      pageNum = 0;
      disabled = false;
      drawHomePage();
    }

    sleepScreen(oneMin*15);
    wakeScreen();
   }
}

void checkTime() {
  Serial.print(rtc.getDOWStr());
  Serial.print(" ");

  Serial.print(rtc.getDateStr());
  Serial.print(" -- ");

  Serial.print(rtc.getTimeStr());
  Serial.println(); 

  delay(1000);
}

void controlGrowLight() {
  int rBright;
  int bBright;
  int wBright;
  int offBright = 0;
  int startHour;
  int stopHour;

  if (pageNum == 1) {
    startHour = 5;
    stopHour = 23;
    rBright = 100;
    bBright = 255;
    wBright = 150;
  }
  else if (pageNum == 2) {
    startHour = 8;
    stopHour = 22;
    rBright = 150;
    bBright = 255;
    wBright = 150;
  }
  else if (pageNum == 3) {
    startHour = 8;
    stopHour = 22;
    rBright = 255;
    bBright = 200;
    wBright = 150;
  }

  if (t.hour >= startHour && t.hour < stopHour) {
    analogWrite(RED_LED, rBright);
    analogWrite(BLUE_LED, bBright);
    analogWrite(WHITE_LED, wBright);
  }
  else {
    analogWrite(RED_LED, offBright);
    analogWrite(BLUE_LED, offBright);
    analogWrite(WHITE_LED, offBright);
  }
}

void waterPlant(unsigned long waterTime) {
  digitalWrite(PUMPPIN, HIGH);
  delay(waterTime*1000); 
  digitalWrite(PUMPPIN, LOW);
}

void checkSoilMoisture(unsigned long interval) {
  unsigned long currMillis = millis();
  float moisturepercent;

  if (currMillis - prevMillis >= interval) {
    prevMillis = currMillis;

    if (pageNum == 1 || pageNum == 2 || pageNum == 3) {
      // take 100 readings and average it for more stable data
      for (int i=0; i<=100; i++) {
        soilmoisture = soilmoisture + analogRead(MOISTURESENSORPIN);
        delay(1);
      }
      soilmoisture = soilmoisture/100;

      // calculated with 650 instead of 1023 as average is never above ~650, even when submerged in water 
      moisturepercent = map(soilmoisture, 0, 650, 0, 100);
      Serial.print("Moisture Percent:");
      Serial.print(moisturepercent);
      Serial.println();

      //placeholder nums
       if (moisturepercent < 20 && pageNum != 4) {
      //   //trigger water pump for X seconds
         waterPlant(10);
       }

       if (pageNum == 1) {
         if (moisturepercent < 50) {
           waterPlant(oneMin);
         }
       }
       else if (pageNum == 2) {
         if (moisturepercent < 35) {
           waterPlant(oneMin);
         }
       }
       else if (pageNum == 3) {
         if (moisturepercent < 20) {
           waterPlant(oneMin*1.5);
         }
       }
    }  
  }
}

void checkTempAndHum(unsigned long interval) {
  float h;
  float t;
  unsigned long currMillis = millis();

  if (currMillis - prevMillis3 >= interval) {
    prevMillis3 = currMillis;
    h = dht.readHumidity();
    t = dht.readTemperature();
       
    Serial.print(("Humidity: "));
    Serial.print(h);
    Serial.print(("%  Temperature: "));
    Serial.print(t);
    Serial.print(("°C "));
    Serial.println();

    if (pageNum == 1) {
      if (h < 70 || t < 25) {
        //draw notif page for t/h too low 
        prevPageNum = pageNum;
        pageNum = 5;
        drawTempHumPage();
        disabled = false;
        tempTooLow = true;
      }
    }
    else if (pageNum == 2) {
      if (h < 50 || t < 20) {
        //draw notif page for t/h too low 
        prevPageNum = pageNum;
        pageNum = 5;
        drawTempHumPage();
        disabled = false;
        tempTooLow = true;
      }
    }
    else if (pageNum == 3) {
      if (h < 60 || t < 20) {
        //draw notif page for t/h too low 
        prevPageNum = pageNum;
        pageNum = 5;
        drawTempHumPage();
        disabled = false;
        tempTooLow = true;
      }
    }
  }

   // Serial.println(btnState3);
    if (btnState3 == HIGH && pageNum == 5 && pageNum != 4) {   
     // Serial.println("here");
      if (tempTooLow) {
        Serial.println("msg dismissed");
        tempTooLow = false; 
        pageNum = prevPageNum;
        drawOptionsPages(pageNum);
      }
  }
  prevBtnState3 = btnState3;

}

void checkWaterLevel(unsigned long interval) { 
  unsigned long currMillis = millis();

  if (currMillis - prevMillis2 >= interval) {
    prevMillis2 = currMillis;
    liquidlevel = digitalRead(waterSensorPin);

    if (liquidlevel == 0 && prevlevel == 1) {
      needsRefill = true;
    }
    else if (liquidlevel == 0 && prevlevel == 0) {
      needsRefill = true;
    }
  }

  if (needsRefill && pageNum != 4) {
    prevPageNum = pageNum;
    drawRefillPage();
    pageNum = 4;
  }

  if (btnState3 == HIGH && prevBtnState3 == LOW) {   
    Serial.println("here");
    if (needsRefill && liquidlevel == HIGH) {
      Serial.println("msg dismissed");
      needsRefill = false; 
      pageNum = prevPageNum;
      drawOptionsPages(pageNum);
    }
  }

  prevBtnState3 = btnState3;
  prevlevel = liquidlevel;
  
}

void sleepScreen(unsigned long interval) {
  unsigned long currMillis = millis();

  if (currMillis - prevMillis6 > interval) {
    if (pageNum == 1 || pageNum == 2 || pageNum == 3) {
      prevMillis6 = currMillis;
      analogWrite(TFT_BL, 10);  
      disabled = false;
      sleeping = true;
    }
  }
}

void wakeScreen() {
  if (btnState3 == HIGH) {
    if (sleeping) {
      analogWrite(TFT_BL, 100);  
      disabled = true;
    }
  }
 prevBtnState3 = btnState3; 
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

void drawTempHumPage() {
  fillScreenBg(GREEN);
  drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
  writeText(25, 57, "TEMPERATURE OR HUMIDITY", BLACK);
  writeText(120, 77, "TOO LOW", BLACK);
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