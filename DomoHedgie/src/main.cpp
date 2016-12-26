/**
* DomoHedgie project
*
* URL: https://github.com/franciscoalario/GoldenAnt/wiki/DomoHedgie
* Description: This project will provide automatic control of the temperature of
* the house, being this one set by the user.
*
* The system will also control the light of the environment depending of the
* surrounding light. An optional light source can be implemented using a white
* LED allowing the user to select the intensity of the light.
*
* Created by Francisco Alario Salom, October 2016
*/

#include "Arduino.h"

#include <dht.h>

#include <Wire.h>
#include "RTClib.h"

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "i18n/DomoHedgie_i18n_es_ES.h"

/**
* ANALOG PINS
**/

#define ENVIRONMENTAL_LIGHT_SENSOR_PIN 0

/**
* DIGITAL PINS
**/

#define ROTARY_A_PIN 2 //ISR Pin 0
#define ROTARY_B_PIN 3 //ISR Pin 1

#define LED_HEATER_ON_PIN 4 //PWM Pin
#define LED_LOW_TEMPERATURE_PIN 5 //PWM Pin
#define LED_LIGHT_ON_PIN 6 //PWM Pin

#define MULTIPLEXER_S0_PIN 7
#define MULTIPLEXER_S1_PIN 8
#define MULTIPLEXER_S2_PIN 9
const int MULTIPLEXER_S_PINS[] = {MULTIPLEXER_S0_PIN, MULTIPLEXER_S1_PIN, MULTIPLEXER_S2_PIN};
#define MULTIPLEXER_Z_PIN 10

#define HEATER_RELAY_PIN 11
#define LIGHT_RELAY_PIN 12

#define TEMP_HUM_DIGITAL_SENSOR_PIN 15


#define ENTER_BUTTON_PIN 18 //ISR Pin 5
#define CANCEL_BUTTON_PIN 19 //ISR Pin 4

#define CLOCK_SDA_PIN 20
#define CLOCK_SCL_PIN 21

#define LCD_CS 30 // Chip Select
#define LCD_CD 31 // Command/Data
#define LCD_WR 32 // LCD Write
#define LCD_RD 33 // LCD Read
#define LCD_RESET 34 // LCD Reset

/**
* MULTIPLEXER INPUTS
**/
#define MULTIPLEXER_INPUT_NUM 3
#define HEATER_MODE_SWITCH_AUTO_MUX_INPUT 0
#define HEATER_MODE_SWITCH_OFF_MUX_INPUT 1
#define HEATER_MODE_SWITCH_ON_MUX_INPUT 2

#define LIGHT_MODE_SWITCH_AUTO_MUX_INPUT 3
#define LIGHT_MODE_SWITCH_OFF_MUX_INPUT 4
#define LIGHT_MODE_SWITCH_ON_MUX_INPUT 5

#define SHUTOFF_BUTTON_MUX_INPUT 6

/**
* SYSTEM NAMES
**/

#define HEATER_SYSTEM_NAME "HEATER"
#define TEMP_HUM_SYSTEM_NAME "TEMP/HUM"
#define RTC_SYSTEM_NAME "RTC"

/**
* LOG VARIABLES
**/
#define INFO_MESSAGE 1
#define WARNING_MESSAGE 2
#define ERROR_MESSAGE 3

/**
* TEMPERATURE VARIABLES
**/

//I.A.W. the DHT11 datasheet, the minimum interval between reading is 1000ms
//so the value of TEMP_HUM_READING_INTERVAL must be greater or equal to 1000 ms
#define TEMP_HUM_READING_INTERVAL 60000
#define MIN_TEMP_HUM_READING_INTERVAL 1000
#define MEASUREMENT_ATTEMPTS 5
dht DHT;
long lastTempLectureMillis;

/**
* DATETIME VARIABLES
**/

struct Datetime{
  int day;
  int month;
  int year;

  int hour;
  int minute;
  int second;
};

RTC_PCF8523 rtc;

/**
* MENU VARIABLES
**/

struct MenuItem{
  int id;
  String name;
};

const int mainMenuDimension = 6;
MenuItem mainMenu[mainMenuDimension];
int menuIndex;

/**
* BUTTONS VARIABLES
**/

#define PUSHED false
#define NOT_PUSHED true
#define WATCH_BUTTON true
#define IGNORE_BUTTON false
#define BUTTON_WAIT_INTERVAL 6000 //microseconds

unsigned long previousMicros = 0;
boolean loopPrevState = NOT_PUSHED;
volatile boolean previousButtonState = NOT_PUSHED;
volatile boolean debouncedButtonState = NOT_PUSHED;
volatile boolean bounceState = false;

/**
* ROTARY ENCODER VARIABLES
**/

static boolean rotating=false;
bool rotaryFlag = false;

#define ROTARY_DELAY 2

/**
* HEATER VARIABLES
**/

boolean heaterOn = false;
int selectedTemp;
long millisHeater;
float heaterTotalSeconds;
#define MIN_TEMP_ALLOWED 23
#define MAX_TEMP_ALLOWED 30

#define HEATER_MODE_AUTO 1
#define HEATER_MODE_OFF 2
#define HEATER_MODE_ON 3
#define HEATER_SAFE_MODE 4

#define HEATER_SAFE_MODE_NIGHTTIME_ON 3600000
#define HEATER_SAFE_MODE_NIGHTTIME_OFF 1800000
#define HEATER_SAFE_MODE_DAYTIME_ON 1800000
#define HEATER_SAFE_MODE_DAYTIME_OFF 7200000
int heaterMode;
long millisSafeMode;

/**
* GRAPHIC VARIABLES
*/

boolean displayOn = true;
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
#define TFT_BACKGROUND_COLOR 0x2966
#define TFT_DEBUG 0x03E0
#define TFT_SEPATATOR_BAR 0xED00
#define TFT_HEIGHT 320
#define TFT_WIDTH 480

#define TFT_TEMP_OK 0x8602
#define TFT_TEMP_HOT 0xE024
#define TFT_TEMP_COLD 0x0E5D
#define TFT_TEMP_OFF 0xEF5D

#define TFT_LIGHT_ON 0x649e
#define TFT_LIGHT_OFF 0xFFFF

#define TFT_CLOCK_COLOR 0xFFFF
#define TFT_CLOCK_COLON_OFF 0x39C4

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
//CLOCK
uint32_t targetTime = 0;
uint8_t hh = 23, mm = 59, ss = 50;//TEMP TIME
byte omm = 99, ohh = 99;
int xClockPos =280;
int yClockPos = 35; // Top left corner of clock text, about half way down
int colonYOffset = -2;
int textYOffset = -29;
int textXOffset = 3;
uint8_t fillRectPlus = 3;
uint8_t charWidth = 24;
uint8_t charHeight = 31;

/**
* DATETIME METHODS
**/

Datetime getDateTime(){
  Datetime myTime;
  DateTime rtcTime = rtc.now();
  myTime.day = rtcTime.day();
  myTime.month = rtcTime.month();
  myTime.year = rtcTime.year();
  myTime.hour = rtcTime.hour();
  myTime.minute = rtcTime.minute();
  myTime.second = rtcTime.second();

  return myTime;
}

String datetimeToString(Datetime date){
  String s = "";
  if(date.day>=1 && date.day<=9) s.concat("0");
  s.concat(date.day);
  s.concat("/");
  if(date.month>=1 && date.month<=9) s.concat("0");
  s.concat(date.month);
  s.concat("/");
  s.concat(date.year);
  s.concat(" ");

  if(date.hour>=0 && date.hour<=9) s.concat("0");
  s.concat(date.hour);
  s.concat(":");
  if(date.minute>=0 && date.minute<=9) s.concat("0");
  s.concat(date.minute);
  s.concat(":");
  if(date.second>=0 && date.second<=9) s.concat("0");
  s.concat(date.second);

  return s;
}

/**
* LOG METHODS
**/

void logMessage(String systemName, String message){
  String log = systemName;
  Datetime now = getDateTime();
  log.concat(now.day); log.concat("/");
  log.concat(now.month); log.concat("/");
  log.concat(now.year); log.concat(" ");

  log.concat(now.hour); log.concat(":");
  log.concat(now.minute); log.concat(":");
  log.concat(now.second); log.concat(";");
  log.concat(message);

  Serial.println(log);

  //TODO
}

/**
* CLOCK METHODS (Continued)
**/

void setDateTime(Datetime now){
  Datetime oldDate = getDateTime();
  rtc.adjust(DateTime(now.year, now.month, now.day, now.hour, now.minute, now.second));
  String message = "Date and time changed from \"";
  message.concat(datetimeToString(oldDate));
  message.concat("\" to \"");
  message.concat(datetimeToString(getDateTime()));
  message.concat("\"");
  logMessage(RTC_SYSTEM_NAME, message);
}

/**
* GRAPHIC METHODS
*/

void cleanScreen(){
  tft.fillScreen(TFT_BACKGROUND_COLOR);
}

boolean isDisplayOn(){
  return displayOn;
}

void turnOnDisplay(){
  if(!isDisplayOn()){
    //TODO
    //Turn display on
    displayOn = true;
  }
}

void turnOffDisplay(){
  if(isDisplayOn()){
    //TODO
    //Turn display off
    displayOn = false;
  }
}

void getTextBounds(String s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
  int str_len = s.length() + 1;
  int16_t  x2, y2;
  uint16_t w1, h1;
  char char_array[str_len];
  s.toCharArray(char_array, str_len);
  tft.getTextBounds(char_array, x, y, &x2, &y2, &w1, &h1);
  *x1 = x2;
  *y1 = y2;
  *w = w1;
  *h = h1;
 }

void updateScreenDate(){
  Datetime now = getDateTime();
  tft.setFont(&FreeMonoBold18pt7b);
  tft.setTextColor(TFT_WHITE, TFT_BACKGROUND_COLOR);
  int xDatePos = 294;
  int yDatePos = 70;
  String date = "";
  if(now.day<10) date.concat("0");
  date.concat(now.day);
  date.concat(C_DATE_SEPARATOR);
  if(now.month<10) date.concat("0");
  date.concat(now.month);
  date.concat(C_DATE_SEPARATOR);
  date.concat(now.year-2000);
  tft.setCursor(xDatePos, yDatePos);
  tft.print(date);
}

void updateScreenClock(){
  if (targetTime < millis()) {
    // Set next update for 1 second later
    targetTime = millis() + 1000;
    // Adjust the time values by adding 1 second
    ss++;              // Advance second
    if (ss == 60) {    // Check for roll-over
      ss = 0;          // Reset seconds to zero
      omm = mm;        // Save last minute time for display update
      mm++;            // Advance minute
      Datetime now = getDateTime();
      mm = now.minute;
      hh = now.hour;
      if (mm > 59) {   // Check for roll-over
        mm = 0;
        ohh = hh;
        hh++;          // Advance hour
        if (hh > 23) { // Check for 24hr roll-over (could roll-over on 13)
          hh = 0; // 0 for 24 hour clock, set to 1 for 12 hour clock
          updateScreenDate();
        }
      }
    }

    tft.setFont(&FreeMonoBold24pt7b);
    String s = "";

    if(ohh != hh){
      //Update hours
      ohh = hh;
      tft.setTextColor(TFT_CLOCK_COLOR);
      tft.setCursor(xClockPos, yClockPos);
      s = (hh<10)?"0":"";
      s.concat(hh);
      tft.fillRect(xClockPos+textXOffset, yClockPos+textYOffset, fillRectPlus+charWidth*2, charHeight, TFT_BACKGROUND_COLOR);
      tft.print(s);
    }

    if(ss%2==0) tft.setTextColor(TFT_CLOCK_COLON_OFF);
    tft.setCursor(xClockPos+charWidth*2, yClockPos+colonYOffset);
    tft.print(C_HOUR_SEPARATOR);

    if(omm != mm){
      //Update minutes
      omm = mm;
      tft.setTextColor(TFT_CLOCK_COLOR);
      tft.setCursor(xClockPos+charWidth*3, yClockPos);
      s = (mm<10)?"0":"";
      s.concat(mm);
      tft.fillRect(xClockPos+textXOffset+charWidth*3, yClockPos+textYOffset, fillRectPlus+charWidth*2, charHeight, TFT_BACKGROUND_COLOR);
      tft.print(s);
    }

    if(ss%2==0) tft.setTextColor(TFT_CLOCK_COLON_OFF);
    tft.setCursor(xClockPos+charWidth*5, yClockPos+colonYOffset);
    tft.print(C_HOUR_SEPARATOR);

    //Update seconds
    tft.setTextColor(TFT_CLOCK_COLOR);
    tft.setCursor(xClockPos+charWidth*6, yClockPos);
    s = (ss<10)?"0":"";
    s.concat(ss);
    tft.fillRect(xClockPos+textXOffset+charWidth*6, yClockPos+textYOffset, fillRectPlus+charWidth*2, charHeight, TFT_BACKGROUND_COLOR);
    tft.print(s);
  }
}

void paintGradient(int x, int y, int lines, uint16_t initialColour, uint16_t finalColour){
  /*uint8_t initialRed = ((initialColour >> 11) & 0x1F);
  uint8_t initialGreen = ((initialColour >> 5) & 0x3F);
  uint8_t initialBlue = (initialColour & 0x1F);*/
  double gradientHeight = (double)lines;

  double ini = ((initialColour >> 11) & 0x1F);
  double fin = ((finalColour >> 11) & 0x1F);
  double redLeap = (fin-ini)/gradientHeight;

  ini = ((initialColour >> 5) & 0x3F);
  fin = ((finalColour >> 5) & 0x3F);
  double greenLeap = (fin-ini)/gradientHeight;

  ini = (initialColour & 0x1F);
  fin = (finalColour & 0x1F);
  double blueLeap = (fin-ini)/gradientHeight;

  for(int i=0;i<gradientHeight-1;i++){
    int red = ((initialColour >> 11) & 0x1F) + ((int)(redLeap*i));
    int green = ((initialColour >> 5) & 0x3F) + ((int)(greenLeap*i));
    int blue = (initialColour & 0x1F) + ((int)(blueLeap*i));
    tft.drawFastHLine(x, y-i, TFT_WIDTH, red << 11 | green << 5 | blue);
  }
  tft.drawFastHLine(x, y-gradientHeight, TFT_WIDTH, finalColour);
}

 void updateMainScreenTemperatureSection(){
   int relPosXTemp = 0;
   int relPosYTemp = 100;
   int tempSectionHeight = 111;

   tft.setFont(&FreeSansBold9pt7b);
   tft.fillRect(relPosXTemp, relPosYTemp, TFT_WIDTH, tempSectionHeight, TFT_BACKGROUND_COLOR); //RESET TEMP SECTION
   tft.fillRect(relPosXTemp, relPosYTemp, TFT_WIDTH, 3, TFT_SEPATATOR_BAR);

   int16_t  x, y;
   uint16_t w, h;
   getTextBounds(S_MAIN_SCREEN_TEMPERATURE, relPosXTemp+5, relPosYTemp+2, &x, &y, &w, &h);
   tft.fillRoundRect(relPosXTemp+5, relPosYTemp, w+7, 21, 3, TFT_SEPATATOR_BAR);
   tft.setTextColor(TFT_WHITE);
   tft.setCursor(relPosXTemp+7, relPosYTemp+14);
   tft.setTextSize(1);
   tft.print(S_MAIN_SCREEN_TEMPERATURE);

   paintGradient(0, relPosYTemp+tempSectionHeight, 60, TFT_TEMP_OK, TFT_BACKGROUND_COLOR);

   tft.setTextSize(1);
   tft.setTextColor(TFT_WHITE);
   tft.setCursor(relPosXTemp+7, relPosYTemp+45);

   tft.setFont(&FreeMono12pt7b);
   getTextBounds(S_MAIN_SCREEN_TEMPERATURE_MODE, relPosXTemp+7, relPosYTemp+45, &x, &y, &w, &h);
   int modeLength = w;
   int modeHeight = h;

   getTextBounds(S_MAIN_SCREEN_TEMPERATURE_HEATER, relPosXTemp+7, relPosYTemp+modeHeight+15+45, &x, &y, &w, &h);
   int heaterLength = w;
   int heaterHeight = h;

   getTextBounds(S_MAIN_SCREEN_TEMPERATURE_SET_TEMP, relPosXTemp+7, relPosYTemp+heaterHeight+15+45, &x, &y, &w, &h);
   int setTempLength = w;

   int maxLength = modeLength;
   if(heaterLength>maxLength) maxLength = heaterLength;
   if(setTempLength>maxLength) maxLength = setTempLength;

   int offset = 2;
   tft.setCursor(maxLength-modeLength+offset, relPosYTemp+45);
   tft.setFont(&FreeMono12pt7b);
   tft.print(S_MAIN_SCREEN_TEMPERATURE_MODE);
   tft.setFont(&FreeMonoBold12pt7b);
   tft.print("AUTO");

   tft.setCursor(maxLength-heaterLength+offset, relPosYTemp+45+modeHeight+15);
   tft.setFont(&FreeMono12pt7b);
   tft.print(S_MAIN_SCREEN_TEMPERATURE_HEATER);
   tft.setFont(&FreeMonoBold12pt7b);
   tft.println("ON");

   tft.setCursor(maxLength-setTempLength+offset, relPosYTemp+45+modeHeight+15+heaterHeight+15);
   tft.setFont(&FreeMono12pt7b);
   tft.print(S_MAIN_SCREEN_TEMPERATURE_SET_TEMP);
   tft.setFont(&FreeMonoBold12pt7b);
   tft.println("25C");

   String currentTemp = "25"; //TODO Get actual current temp
   int currentTempXRelPos = 330;
   int currentTempYRelPos = relPosYTemp+23;
   tft.setFont(&FreeMono9pt7b);
   tft.setTextSize(1);
   getTextBounds(S_MAIN_SCREEN_TEMPERATURE_CURRENT_TEMP, 0, 0, &x, &y, &w, &h);
   tft.setCursor(currentTempXRelPos-(w/2), currentTempYRelPos);
   int currentTempHeight = h;
   tft.print(S_MAIN_SCREEN_TEMPERATURE_CURRENT_TEMP);
   tft.setFont(&FreeSansBold24pt7b);
   tft.setTextSize(2);
   getTextBounds(currentTemp, 0, 0, &x, &y, &w, &h);
   tft.setCursor(currentTempXRelPos-(w/2), currentTempYRelPos+currentTempHeight+65);
   tft.print(currentTemp);
 }

 void updateMainScreenLightSection(){
   int relPosXLight = 0;
   int relPosYLight = 212;

   int16_t  x, y;
   uint16_t w, h;
   tft.setFont(&FreeSansBold9pt7b);
   tft.fillRect(relPosXLight, relPosYLight, TFT_WIDTH, 3, TFT_SEPATATOR_BAR);
   getTextBounds(S_MAIN_SCREEN_LIGHT, relPosXLight+5, relPosYLight+2, &x, &y, &w, &h);
   tft.fillRoundRect(relPosXLight+5, relPosYLight, w+7, 21, 3, TFT_SEPATATOR_BAR);
   tft.setTextColor(TFT_WHITE);
   tft.setCursor(relPosXLight+7, relPosYLight+14);
   tft.setTextSize(1);
   tft.print(S_MAIN_SCREEN_LIGHT);

   paintGradient(0, TFT_HEIGHT, 60, TFT_TEMP_OFF, TFT_BACKGROUND_COLOR);
 }

void updateMainScreen(){
  updateMainScreenTemperatureSection();
  updateMainScreenLightSection();
}

/**
* MULTIPLEXER METHODS
**/

int getMuxInputState(int input){
  for(int i=0;i<MULTIPLEXER_INPUT_NUM;i++){
    digitalWrite(MULTIPLEXER_S_PINS[i], bitRead(input, i));
  }

  return digitalRead(MULTIPLEXER_Z_PIN);
}

/**
* TEMPERATURE / HUMIDITY SENSOR METHODS
**/

/**
* Internal method. Checks if it is necessary to make a new reading of the
* temperature / humidity or if it is allowed to use the last reading values.
* args: bool force. False means that the last reading values could be used if it
* is allowed. True means to force a new reading.
* args: long millis. Current millis to check if it is necessary to make a new
* reading depending on when the last reading was.
* return: 0 Ok, -1 Checksum error, -2 timeout.
*/
int readTempHum(bool force, long millis){
  int result = 0;
  if(force || (millis - lastTempLectureMillis)>=MIN_TEMP_HUM_READING_INTERVAL){
    result = DHT.read11(TEMP_HUM_DIGITAL_SENSOR_PIN);
    lastTempLectureMillis = millis;

    switch(result){
      case -2://TIMEOUT
        logMessage(TEMP_HUM_SYSTEM_NAME, "Timeout error occured");
        break;
      case -1://CHECKSUM error
        logMessage(TEMP_HUM_SYSTEM_NAME, "Checksum error occured");
        break;
      case 0://OK
        break;
    }
  }

  return result;
}

/**
* Gets the temperature from the digital sensor.
* args: none
*return: The temperature in Celsius
*/
float getTemperature(){
    return DHT.temperature;
}

/**
* Gets the humidity from the digital sensor.
* args: none
*return: The humidity expressed in %
*/
float getHumidity(){
  return DHT.humidity;
}

void handleTempHumSensor(long millis){
  bool flag = true;
  if((millis - lastTempLectureMillis) >= TEMP_HUM_READING_INTERVAL){
    if(readTempHum(false, millis)!=0){
      //ERROR DURING TEMPERATURE AND HUMIDITY MEASUREMENT
      int attempts = 0;
      flag = false;
      while(attempts<MEASUREMENT_ATTEMPTS && !flag){
        attempts++;
        delay(3000);
        if(readTempHum(true, millis)==0) flag = true;
      }
    }

    if(!flag){
      //TODO Not possible temperature and humidity measurement.
      //Suggest to reboot the device.
      //Put the heater in safety mode
      heaterMode = HEATER_SAFE_MODE;
      logMessage(TEMP_HUM_SYSTEM_NAME, "Safe mode activated");
      //Sound alarm
      //Enter in mode alarm
    }
  }
}

/**
* HEATER METHODS
**/

void startHeaterTimeTracking(){
  millisHeater = millis();
}

void updateHeaterTimeTracking(){
  if(millisHeater != 0){
    long now = millis();
    heaterTotalSeconds += ((now-millisHeater)/1000);

    millisHeater = now;
  }
}

void stopHeaterTimeTracking(){
  updateHeaterTimeTracking();
  millisHeater = 0;
}

boolean isHeaterOn(){
  return heaterOn;
}

int getHeaterMode(){
  if(heaterMode != HEATER_SAFE_MODE){
    if(getMuxInputState(HEATER_MODE_SWITCH_AUTO_MUX_INPUT) == LOW) heaterMode = HEATER_MODE_AUTO;
    else if(getMuxInputState(HEATER_MODE_SWITCH_OFF_MUX_INPUT) == LOW) heaterMode = HEATER_MODE_OFF;
    else if(getMuxInputState(HEATER_MODE_SWITCH_ON_MUX_INPUT) == LOW) heaterMode = HEATER_MODE_ON;
  }
  return heaterMode;
}

String getHeaterModeName(int heaterMode){
  String mode = "";
  switch(heaterMode){
    case HEATER_MODE_AUTO:
      mode = "AUTO";
      break;
    case HEATER_MODE_OFF:
      mode = "OFF";
      break;
    case HEATER_MODE_ON:
      mode = "ON";
      break;
  }

  return mode;
}

void turnOnHeater(){
  if(!isHeaterOn()){
    digitalWrite(HEATER_RELAY_PIN, HIGH);
    startHeaterTimeTracking();

    String mode = getHeaterModeName(getHeaterMode());
    String message = "";
    message.concat(mode); message.concat("#");message.concat("ON#");
    message.concat(heaterTotalSeconds);
    logMessage(HEATER_SYSTEM_NAME, message);
  }
}

void turnOffHeater(){
  if(isHeaterOn()){
    digitalWrite(HEATER_RELAY_PIN, LOW);
    stopHeaterTimeTracking();

    String mode = getHeaterModeName(getHeaterMode());
    String message = "";
    message.concat(mode); message.concat("#");message.concat("OFF#");
    message.concat(heaterTotalSeconds);
    logMessage(HEATER_SYSTEM_NAME, message);
  }
}

void heaterSafeMode(){
  Datetime now = getDateTime();
  if((now.hour>20 && now.minute>30) || (now.hour<8 && now.minute < 30)){
    //nighttime
    if(isHeaterOn()){
      if(millisSafeMode > HEATER_SAFE_MODE_NIGHTTIME_ON){
        millisSafeMode = 0;
        turnOffHeater();
      }
      else{
        millisSafeMode += (millis() - millisSafeMode);
      }
    }
    else{
      if(millisSafeMode > HEATER_SAFE_MODE_NIGHTTIME_OFF){
        millisSafeMode = 0;
        turnOnHeater();
      }
      else{
        millisSafeMode += (millis() - millisSafeMode);
      }
    }
  }
  else{
    //daytime
    if(isHeaterOn()){
      if(millisSafeMode > HEATER_SAFE_MODE_DAYTIME_ON){
        millisSafeMode = 0;
        turnOffHeater();
      }
      else{
        millisSafeMode += (millis() - millisSafeMode);
      }
    }
    else{
      if(millisSafeMode > HEATER_SAFE_MODE_DAYTIME_OFF){
        millisSafeMode = 0;
        turnOnHeater();
      }
      else{
        millisSafeMode += (millis() - millisSafeMode);
      }
    }
  }
}

void handleHeater(){
  switch(getHeaterMode()){
    case HEATER_MODE_AUTO:
      if(selectedTemp > getTemperature()) turnOnHeater();
      else turnOffHeater();
      break;
    case HEATER_MODE_ON:
      if(MAX_TEMP_ALLOWED > getTemperature()) turnOnHeater();
      else turnOffHeater();
      break;
    case HEATER_MODE_OFF:
      turnOffHeater();
      break;
    case HEATER_SAFE_MODE:
      //TODO Announce safe mode on the display
      heaterSafeMode();
      break;
  }
}

/**
* ROTARY ENCODER METHODS
**/

/**
* Returns the previos menu item to the current (selected) menu item
* args: none
* return: The MenuItem previous to the current one
*/
MenuItem previousMenuItem(){
  return mainMenu[((menuIndex-1)<0)?mainMenuDimension-1:menuIndex-1];
}

/**
* Returns the current (selected) menu item
* args: none
* return: The current (selected) menu item
*/
MenuItem currentMenuItem(){
  return mainMenu[menuIndex];
}

/**
* Returns the next menu item to the current (selected) menu item
* args: none
* return: The MenuItem next to the current one
*/
MenuItem nextMenuItem(){
  return mainMenu[((menuIndex+1)==mainMenuDimension)?0:menuIndex+1];
}

/**
* Moves one position forward the index to the selected menu item
* args: none
* return: The current menu item index
*/
int moveMenuIndexForward(){
  menuIndex = ((menuIndex+1)==mainMenuDimension)?0:menuIndex+1;
  return menuIndex;
}

/**
* Moves one position backward the index to the selected menu item
* args: none
* return: The current menu item index
*/
int moveMenuIndexBackward(){
  menuIndex = ((menuIndex-1)<0)?mainMenuDimension-1:menuIndex-1;
  return menuIndex;
}

/**
* Enable the enconder control methods to detect the rotation of the rotary
* encoder. This method is attached to the rotary encoder movement by the method
* AttachInterrupt.
* args: none
* return: none
*/
void rotEncoder(){
  rotating=true;
}

/**
* Updates the main menu displaying the selected menu item in accordance with the
* movement of the rotary encoder. Before and after the selected menu item are
* also displayed the previous and next menu items.
* args: none
* return: none
*/
void updateMainMenu(){
  //TODO update the main menu in accordance with the movement of the rotary
  //encoder displaying the menu items.

  //TEMPORARY CODE
  Serial.println(previousMenuItem().name);
  Serial.print("> ");
  Serial.println(currentMenuItem().name);
  Serial.println(nextMenuItem().name);
  Serial.println("\n");
}

/**
* This method handles the behaviour of the rotary encoder detecting the rotation
* direction and calling for updating the main menu.
* args: none
* return: none
*/
void handleRotaryEncoder(){
  while(rotating){
    delay(ROTARY_DELAY);
    if (digitalRead(ROTARY_B_PIN) == digitalRead(ROTARY_B_PIN)){
      if(!rotaryFlag){
        //CW
        rotaryFlag = true;

        if(isDisplayOn()){
          moveMenuIndexForward();
          updateMainMenu();
        }
        else turnOnDisplay();
      }
      else rotaryFlag = false;
    }
    else{
      if(!rotaryFlag){
        //CCW
        rotaryFlag = true;

        if(isDisplayOn()){
          moveMenuIndexBackward();
          updateMainMenu();
        }
        else turnOnDisplay();
      }
      else rotaryFlag = false;
    }
    rotating=false;
  }
}

/**
* Executes what is necessary when the Enter button is pressed. It depends on
* the state of the menu.
* args: none
* return: none
*/
void executeEnterButton(){
  if(isDisplayOn()){
    //TEMPORARY CODE
    Serial.print("Option: ");
    Serial.println(mainMenu[menuIndex].name);
  }
  else turnOnDisplay();
}

/**
* Executes what is necessary when the Cancel button is pressed. It depends on
* the state of the menu.
* args: none
* return: none
*/
void executeCancelButton(){
  if(isDisplayOn()){
    //TODO
  }
  else turnOnDisplay();
}

/**
* Executes what is necessary to shut off the display but the device is still
* running. One pulsation turns off the display and another pulsation turns
* it on, also any movement of the rotary encoder or the Enter and Exit/Cancel buttons
* turn the display on
* args: none
* return: none
*/

void executeShutOffButton(){
  if(isDisplayOn()) turnOffDisplay();
  else turnOnDisplay();
}

/**
* Generic method to handle the pulsation of any button. The method receives the
* button pin and executes the proper method.
* args: int buttonPin - The button pin which has triggered this method.
* return: none
*/
void handleButton(int buttonPin){
  if (bounceState == WATCH_BUTTON) {
    boolean currentButtonState = digitalRead(buttonPin);
    if (previousButtonState != currentButtonState) {
      bounceState = IGNORE_BUTTON;
      previousMicros = micros();
    }
    previousButtonState = currentButtonState;
  }
  if (bounceState == IGNORE_BUTTON) {
    unsigned long currentMicros = micros();
    if ((unsigned long)(currentMicros - previousMicros) >= BUTTON_WAIT_INTERVAL) {
      debouncedButtonState = digitalRead(buttonPin);
      bounceState = WATCH_BUTTON;

      switch(buttonPin){
        case ENTER_BUTTON_PIN:
          executeEnterButton();
          break;
        case CANCEL_BUTTON_PIN:
          executeCancelButton();
          break;
      }
    }
  }
}

/**
* The ISR method for the Enter button
* args: none
* return: none
*/
void handleEnterButton() {
  handleButton(ENTER_BUTTON_PIN);
}

/**
* The ISR method for the Cancel button
* args: none
* return: none
*/
void handleCancelButton(){
  handleButton(CANCEL_BUTTON_PIN);
}

/**
* INIT MODULES
**/

void initMenuItems(){
  menuIndex = 0;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Set min. temperature";
  menuIndex++;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Set light intensity";
  menuIndex++;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Set date/time";
  menuIndex++;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Show heating time";
  menuIndex++;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Show lighting time";
  menuIndex++;

  mainMenu[menuIndex].id = menuIndex;
  mainMenu[menuIndex].name = "Turn off display";
  menuIndex++;

  menuIndex = 0;
}

void initEnterButton(){
  pinMode(ENTER_BUTTON_PIN, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(ENTER_BUTTON_PIN),
    handleEnterButton,
    LOW
  );
}

void initCancelButton(){
  pinMode(CANCEL_BUTTON_PIN, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(CANCEL_BUTTON_PIN),
    handleCancelButton,
    LOW
  );
}

void initRotaryEncoder(){
  pinMode(ROTARY_A_PIN, INPUT);
  pinMode(ROTARY_B_PIN, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(ROTARY_A_PIN),
    rotEncoder,
    CHANGE
  );
}

void initHeater(){
  selectedTemp = (MAX_TEMP_ALLOWED+MIN_TEMP_ALLOWED)/2;
  millisHeater = 0;
  heaterTotalSeconds = 0;
  millisSafeMode = 0;

  //TODO
  //Read the position of the Heater mode switch and act in consequence
}

void initTempHumSensor(){
  lastTempLectureMillis = 0;
}

void initRTC(){
  rtc.begin();
  if(!rtc.initialized()){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  Datetime now = getDateTime();
  hh = now.hour;
  mm = now.minute;
  ss = now.second;
}

void initDisplay(){
  tft.reset();

  uint16_t identifier = tft.readID();

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }

  tft.begin(identifier);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_DEBUG);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
}

/**
* MAIN METHODS
**/

void setup()
{
  Serial.begin(9600);
  Serial.println("INIT");

  initDisplay();
  /*tft.setCursor(200, 10);
  tft.print("DISPLAY: INIT DONE");
  initMenuItems();
  tft.setCursor(200, 40);
  tft.print("MENU: INIT DONE");
  initRotaryEncoder();
  tft.setCursor(200, 60);
  tft.print("ROTARY ENCODER: INIT DONE");
  initEnterButton();
  tft.setCursor(200, 80);
  tft.print("ENTER BUTTON: SET DONE");
  initCancelButton();
  tft.setCursor(200, 100);
  tft.print("CANCEL BUTTON: SET DONE");
  initHeater();
  tft.setCursor(200, 120);
  tft.print("HEATER: INIT DONE");
  initTempHumSensor();
  tft.setCursor(200, 140);
  tft.print("TEMP / HUM SENSOR: INIT DONE");
  //initRTC();
  tft.setCursor(200, 160);
  tft.print("RTC: INIT DONE");
  delay(200);*/

  initRTC();
  cleanScreen();

  updateMainScreen();
  updateScreenDate();
}

void loop()
{
  //long now = millis();

  //handleRotaryEncoder();
  //handleTempHumSensor(now);
  //handleHeater();
  updateScreenClock();

  /*tft.drawFastVLine(104, 0, 320, 0xFFFF);
  tft.setFont(&FreeMonoBold24pt7b);
  tft.setCursor(100, 140);
  tft.setTextColor(0xFFFF);
  tft.print("0");*/
}
