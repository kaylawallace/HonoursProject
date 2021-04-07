#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"
#include <DS3231.h>

// TFT LCD screen pin definitions
#define TFT_DC 30
#define TFT_CS 53
#define TFT_BL 11

// Temperature/Humidity sensor definitions
#define DHTPIN 24
#define DHTTYPE DHT11 

// Soil moisture sensor, water pump & water level sensor pin definitions
#define MOISTURESENSORPIN A0
#define PUMPPIN A3
#define WATERLVL 3

// Pin definitions for each LED colour from the grow light 
#define RED_LED 6
#define BLUE_LED 5
#define GREEN_LED 10
#define WHITE_LED 9

// Real-time clock module pin definitions 
#define RTC_SDA 20
#define RTC_SCL 21

// Colour definitions for drawing the UI on the TFT LCD screen
#define GREEN    0x2BC9
#define WHITE    0xFFFF
#define YELLOW   0xEDAC 
#define BROWN    0xA3A9
#define BLACK    0x0000

#define BTN1 2
#define BTN2 4
#define BTN3 7 

// Initialise screen using Adafruit class library
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_BL);

// Initialise temperature/humidity sensor var using DHT class library
DHT dht(DHTPIN, DHTTYPE);

// Initialise real-time clock module vars using the DS3231 class library
DS3231  rtc(RTC_SDA, RTC_SCL);
Time t;

const unsigned long oneMin = 60000; //1 min in millis
unsigned long prevMillis = 0;
unsigned long prevMillis2 = 0;
unsigned long prevMillis3 = 0;
unsigned long prevMillis6 = 0;
int btnState1 = 0;
int btnState2 = 0;
int btnState3 = 0;
int prevBtnState3 = 0;
int liquidlevel = 0;
int prevlevel = 0;
int pageNum; 
int prevPageNum;
bool disabled;      // True when buttons are disabled 
bool needsRefill;   // True when the water tank needs refilled
bool tempTooLow;    // True when the air temp./humidity is too low
bool sleeping;      // True when the screen is dimmed
float soilmoisture; // Value read in from soil moisture sensor  

void setup() {
  Serial.begin(9600);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(BTN3, INPUT);
  pinMode(WATERLVL, INPUT);  
  pinMode(PUMPPIN, OUTPUT);  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(TFT_BL, OUTPUT);

  tft.begin();
  dht.begin();
  rtc.begin();

  //Serial.println("Screen Diagnostics"); 
  // Read diagnostics (optional but can help debug problems)
  // uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  // Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDMADCTL);
  // Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDPIXFMT);
  // Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDIMGFMT);
  // Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  // x = tft.readcommand8(ILI9341_RDSELFDIAG);
  // Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 

  // Set the screen defaults 
  tft.setRotation(1);
  tft.setTextSize(2);

  pageNum = 0;
  drawPage(pageNum);
}

void loop() {
  btnState1 = digitalRead(BTN1);
  btnState2 = digitalRead(BTN2);
  btnState3 = digitalRead(BTN3);

  // Only run this section if current page = home page 
  // Set disabled to true once an option has been picked so that the buttons only work when intended to 
  if (pageNum == 0) {
    if (btnState1 == HIGH && !disabled) {
        disabled = true;
        pageNum = 1;
        drawPage(pageNum);
    }
    if (btnState2 == HIGH && !disabled) {
      disabled = true;
      pageNum = 2;
      drawPage(pageNum);
    }
    if (btnState3 == HIGH && !disabled) {
      disabled = true;
      pageNum = 3;
      drawPage(pageNum);
    }
  }
  // When on any other page except the home page - run the sensors
  else {   
    checkWaterLevel(oneMin/3);
    checkTempAndHum(oneMin);
    checkSoilMoisture(oneMin/2);   
    controlGrowLight();

    sleepScreen(oneMin*15);
    wakeScreen();

    // Button 1 acts as back to homepage button when on pages 1/2/3
    if (btnState1 == HIGH) {
      pageNum = 0;
      disabled = false;
      drawPage(pageNum);
    }
  }
}

// Test method to output current day of week, date, and time received from RTC module
void checkTime() {
  Serial.print(rtc.getDOWStr());
  Serial.print(" ");

  Serial.print(rtc.getDateStr());
  Serial.print(" -- ");

  Serial.print(rtc.getTimeStr());
  Serial.println(); 

  delay(1000);
}

/*
* Method to turn the LED grow lights on/off based on the type of plant option chosen on the home page 
*/
void controlGrowLight() {
  int rBright;        // Brightness of red LED
  int bBright;        // Brightness of blue LED
  int wBright;        // Brightness of white LED 
  int offBright = 0;  // Brightness when LEDs are off (0)
  int startHour;      // Hour the LEDs should turn on 
  int stopHour;       // Hour the LEDs should turn off 

  t = rtc.getTime();

  // Set each brightness & start/stop hour for each page - values specific to type of plant being grown 
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

/*
* Method to turn the water pump on for a specific amount of time and then off again after that time is up
* Param: waterTime - the number of seconds the water pump should be turned on for 
*/
void waterPlant(unsigned long waterTime) {
  digitalWrite(PUMPPIN, HIGH);
  delay(waterTime*1000); 
  digitalWrite(PUMPPIN, LOW);
}

/*
* Method to check the current soil moisture levels, convert these to a percentage value,
* and call the waterPlant method if the soil moisture is too low based on the type of plant option 
* chosen on the home page 
* Param: interval - the number of millis that are to pass before the function runs (time delay between sensor readings)
*/
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

      // Calculate soil moisture percentage from average reading 
      // Using 650 instead of 1023 as a result of testing with the sensor
      moisturepercent = map(soilmoisture, 0, 650, 0, 100);

      // Set the moisture threshold specific to each page/type of plant being grown
      if (pageNum == 1) {
        if (moisturepercent < 80) {
          waterPlant(oneMin);
        }
      }
      else if (pageNum == 2) {
        if (moisturepercent < 70) {
          waterPlant(oneMin);
        }
      }
      else if (pageNum == 3) {
        if (moisturepercent < 60) {
          waterPlant(oneMin*1.5);
        }
      }
    }  
  }
}

/*
* Method to check the current air temperature and humidity and to notify the user if these readings are 
* too low based on the type of plant option chosen on the home page 
* Param: interval - the number of millis that are to pass before the function runs (time delay between sensor readings)
*/
void checkTempAndHum(unsigned long interval) {
  float h;   // Humidity reading
  float t;   // Temperature reading 
  unsigned long currMillis = millis();

  if (currMillis - prevMillis3 >= interval) {
    prevMillis3 = currMillis;
    h = dht.readHumidity();
    t = dht.readTemperature();
       
    // Serial.print(("Humidity: "));
    // Serial.print(h);
    // Serial.print(("%  Temperature: "));
    // Serial.print(t);
    // Serial.print(("°C "));
    // Serial.println();

    // Set temperature and humidity thresholds specific to the type of plant being grown 
    if (pageNum == 1) {
      if (h < 70 || t < 25) {
        prevPageNum = pageNum;
        pageNum = 5;
        drawPage(pageNum);
        disabled = false;
        tempTooLow = true;
      }
    }
    else if (pageNum == 2) {
      if (h < 50 || t < 20) {
        prevPageNum = pageNum;
        pageNum = 5;
        drawPage(pageNum);
        disabled = false;
        tempTooLow = true;
      }
    }
    else if (pageNum == 3) {
      if (h < 60 || t < 20) {
        prevPageNum = pageNum;
        pageNum = 5;
        drawPage(pageNum);
        disabled = false;
        tempTooLow = true;
      }
    }
  }

  // Dismiss the notification displayed when the temp./humidity is too low 
  if (btnState3 == HIGH && pageNum == 5 && pageNum != 4) {   
    if (tempTooLow) {
      tempTooLow = false; 
      pageNum = prevPageNum;
      drawPage(pageNum);
    }
  }
  prevBtnState3 = btnState3;
}

/*
* Method to check the current water level of the built-in water tank and notify the user if it needs refilled 
* Param: interval - the number of millis that are to pass before the function runs (time delay between sensor readings)
*/
void checkWaterLevel(unsigned long interval) { 
  unsigned long currMillis = millis();

  if (currMillis - prevMillis2 >= interval) {
    prevMillis2 = currMillis;
    liquidlevel = digitalRead(WATERLVL);

    if (liquidlevel == 0 && prevlevel == 1) {
      needsRefill = true;
    }
    else if (liquidlevel == 0 && prevlevel == 0) {
      needsRefill = true;
    }
  }

  // Only display the notification if it is not already shown 
  if (needsRefill && pageNum != 4) {
    prevPageNum = pageNum;  
    pageNum = 4;
    drawPage(pageNum);
  }

  // Dismiss the notification displayed if the water tank needs refilled 
  if (btnState3 == HIGH && prevBtnState3 == LOW) {   
    if (needsRefill && liquidlevel == HIGH) {
      needsRefill = false; 
      pageNum = prevPageNum;
      drawPage(pageNum);
    }
  }

  prevBtnState3 = btnState3;
  prevlevel = liquidlevel; 
}

/*
* Method to dim the backlight of the LCD screen to enter a 'sleep' state if an amount of time has passed with no activity
* on pages 1, 2 or 3
* Param: interval - the number of millis that are to pass before the function runs (time delay between sensor readings)
*/
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

/*
* Method to return the backlight to full brightness if the user presses a button to exit the 'sleep' state 
*/
void wakeScreen() {
  if (btnState3 == HIGH) {
    if (sleeping) {
      analogWrite(TFT_BL, 100);  
      disabled = true;
    }
  }
 prevBtnState3 = btnState3; 
}

/*
* Method to draw the home page on the LCD screen
*/
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

/*
* Method to draw each main page on the LCD screen
* Param: pageNum - the page number corresponding to the type of plant option chosen by the user on the home page 
*/
void drawPage(int pageNum) {
  if (pageNum == 0) {
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
  else if (pageNum == 1) {
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
  else if (pageNum == 4) {
    fillScreenBg(GREEN);
    drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
    writeText(57, 62, "REFILL WATER TANK", BLACK);
    drawBtn(75, 150, 170, 60, 25, YELLOW, BROWN);
    writeText(137, 172, "DONE", BLACK);
  }
  else if (pageNum == 5) {
    fillScreenBg(GREEN);
    drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
    writeText(25, 57, "TEMPERATURE OR HUMIDITY", BLACK);
    writeText(120, 77, "TOO LOW", BLACK);
    drawBtn(75, 150, 170, 60, 25, YELLOW, BROWN);
    writeText(137, 172, "DONE", BLACK);
  }
}

/*
* Method to draw the refill water tank notification page on the LCD screen
*/
void drawRefillPage() {
  fillScreenBg(GREEN);
  drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
  writeText(57, 62, "REFILL WATER TANK", BLACK);
  drawBtn(75, 150, 170, 60, 25, YELLOW, BROWN);
  writeText(137, 172, "DONE", BLACK);
}

/*
* Method to draw the notification paage for if the temperature or humidity are too low on the LCD screen
*/
void drawTempHumPage() {
  fillScreenBg(GREEN);
  drawBtn(10, 20, 300, 100, 25, WHITE, YELLOW);
  writeText(25, 57, "TEMPERATURE OR HUMIDITY", BLACK);
  writeText(120, 77, "TOO LOW", BLACK);
  drawBtn(75, 150, 170, 60, 25, YELLOW, BROWN);
  writeText(137, 172, "DONE", BLACK);
}

/*
* Method to colour fill the background of the LCD screen
* Param: colour - colour to fill the background 
*/
void fillScreenBg(uint16_t colour) {
  tft.fillScreen(colour);
}

/*
* Method to draw a message box on the LCD screen
* Params: x0 - starting x-position, y0 - starting y-position, w - width, h - height, outline_col = outline colour
* Optional: fill_col - fill colour 
*/
void drawMsgBox(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r, uint16_t fill_col, uint16_t outline_col) {
 if (fill_col != NULL) {
  tft.fillRoundRect(x0, y0, w, h, r, fill_col);
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
 }
 else {
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
 }
}

/*
* Method to draw a button on the LCD screen
* Params: x0 - starting x-position, y0 - starting y-position, w - width, h - height, fill_col - fill colour, outline_col = outline colour
*/
void drawBtn(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t r, uint16_t fill_col, uint16_t outline_col) {
  tft.fillRoundRect(x0, y0, w, h, r, fill_col);
  tft.drawRoundRect(x0, y0, w, h, r, outline_col);
}

/*
* Method to write text on the LCD screen
* Params: cursor_x - starting x-position, cursor_y - starting y-position, text - text to be written, font_col - font colour
*/
void writeText(uint16_t cursor_x, uint16_t cursor_y, String text, uint16_t font_col) {  
  tft.setTextColor(font_col);  
  tft.setCursor(cursor_x, cursor_y);
  tft.println(text);
}