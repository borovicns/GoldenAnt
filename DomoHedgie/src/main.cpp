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
#include <TFT_HX8357.h>

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

RTC_DS1307 RTC; // Tiny RTC Module

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
TFT_HX8357 tft = new TFT_HX8357(480, 320);
const uint16_t backgroundDisplay = TFT_NAVY;
#define TFT_HEIGHT 320
#define TFT_WIDTH 480
//CLOCK
uint32_t targetTime = 0;
uint8_t hh = 0, mm = 0, ss = 0;//TEMP TIME
byte omm = 99, oss = 99;
byte xcolon = 0;
int  xsecs = 0;
unsigned int colour = 0;
int clockTextSize = 4;

/**
* DATETIME METHODS
**/

Datetime getDateTime(){
  DateTime rtcTime = RTC.now();
  Datetime myTime;
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

void setDateTime(Datetime now){
  //Datetime oldDate = getDateTime();
  DateTime rtcTime = DateTime(now.year, now.month, now.day, now.hour, now.minute, now.second);
  RTC.adjust(rtcTime);
  /*String message = "Date and time changed from \"";
  message.concat(datetimeToString(oldDate));
  message.concat("\" to \"");
  message.concat(datetimeToString(getDateTime()));
  message.concat("\"");
  logMessage(RTC_SYSTEM_NAME, message);*/
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
* GRAPHIC METHODS
*/

void cleanScreen(){
  tft.fillScreen(backgroundDisplay);
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
      if (mm > 59) {   // Check for roll-over
        mm = 0;
        hh++;          // Advance hour
        if (hh > 23) { // Check for 24hr roll-over (could roll-over on 13)
          hh = 0;      // 0 for 24 hour clock, set to 1 for 12 hour clock
        }
      }
    }

    // Update digital time
    int xpos =250;
    int ypos = 20; // Top left corner of clock text, about half way down
    //int ysecs = ypos + 24;
    int ysecs = ypos + 0;

    if (omm != mm) { // Redraw hours and minutes time every minute
      omm = mm;
      // Draw hours and minutes
      if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, clockTextSize); // Add hours leading zero for 24 hr clock
      xpos += tft.drawNumber(hh, xpos, ypos, clockTextSize);             // Draw hours
      xcolon = xpos; // Save colon coord for later to flash on/off later
      xpos += tft.drawChar(':', xpos, ypos - 8, clockTextSize);
      if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, clockTextSize); // Add minutes leading zero
      xpos += tft.drawNumber(mm, xpos, ypos, clockTextSize);             // Draw minutes
      xsecs = xpos; // Save seconds 'x' position for later display updates
    }
    if (oss != ss) { // Redraw seconds time every second
      oss = ss;
      xpos = xsecs;

      if (ss % 2) { // Flash the colons on/off
        tft.setTextColor(TFT_WHITE, backgroundDisplay);        // Set colour to grey to dim colon
        tft.drawChar(':', xcolon, ypos - 8, clockTextSize);     // Hour:minute colon
        xpos += tft.drawChar(':', xsecs, ysecs, clockTextSize); // Seconds colon
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);    // Set colour back to yellow
      }
      else {
        tft.drawChar(':', xcolon, ypos - 8, clockTextSize);     // Hour:minute colon
        xpos += tft.drawChar(':', xsecs, ysecs, clockTextSize); // Seconds colon
      }

      //Draw seconds
      if (ss < 10) xpos += tft.drawChar('0', xpos, ysecs, clockTextSize); // Add leading zero
      tft.drawNumber(ss, xpos, ysecs, clockTextSize);                     // Draw seconds
    }
  }
}

void updateMainScreen(){
  int relPosXTemp = 0;
  int relPosYTemp = 106;

  tft.fillRect(relPosXTemp, relPosYTemp, TFT_WIDTH, 3, TFT_ORANGE);
  /*
  int16_t  x1, y1;
  uint16_t w, h;
  tft.getTextBounds("Temperature", 5, 90, &x1, &y1, &w, &h);
  tft.fillRoundRect(5, 90, w, h, 3, TFT_ORANGE);
  */

  tft.fillRoundRect(relPosXTemp+5, relPosYTemp-182, 134, 20, 3, TFT_ORANGE);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(relPosXTemp+7, relPosYTemp-14);
  tft.setTextSize(2);
  tft.print("Temperatura");

  int relPosXLight = 0;
  int relPosYLight = 212;

  tft.fillRect(relPosXLight, relPosYLight, TFT_WIDTH, 3, TFT_ORANGE);
  /*
  int16_t  x1, y1;
  uint16_t w, h;
  tft.getTextBounds("Light", 5, 90, &x1, &y1, &w, &h);
  tft.fillRoundRect(5, 90, w, h, 3, TFT_ORANGE);
  */

  tft.fillRect(relPosXLight+5, relPosYLight-16, 134, 16, TFT_ORANGE);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(relPosXLight+7, relPosYLight-14);
  tft.setTextSize(2);
  tft.print("Luz");
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

void turnOnHeater(){
  if(!isHeaterOn()){
    digitalWrite(HEATER_RELAY_PIN, HIGH);
    startHeaterTimeTracking();

    String mode;
    switch(getHeaterMode()){
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

    String mode;
    switch(getHeaterMode()){
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
  Wire.begin();
  RTC.begin();

  /*if (! RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }*/
}

void initDisplay(){
  tft.init();
  tft.setRotation(5);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_DARKGREEN);
  tft.setTextFont(1);
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
  /*tft.setCursor(10, 10);
  tft.print("DISPLAY: INIT DONE");
  initMenuItems();
  tft.setCursor(10, 40);
  tft.print("MENU: INIT DONE");
  initRotaryEncoder();
  tft.setCursor(10, 60);
  tft.print("ROTARY ENCODER: INIT DONE");
  initEnterButton();
  tft.setCursor(10, 80);
  tft.print("ENTER BUTTON: SET DONE");
  initCancelButton();
  tft.setCursor(10, 100);
  tft.print("CANCEL BUTTON: SET DONE");
  initHeater();
  tft.setCursor(10, 120);
  tft.print("HEATER: INIT DONE");
  initTempHumSensor();
  tft.setCursor(10, 140);
  tft.print("TEMP / HUM SENSOR: INIT DONE");
  //initRTC();
  tft.setCursor(10, 160);
  tft.print("RTC: INIT DONE");
  delay(200);
  */
  cleanScreen();

  updateMainScreen();
}

void loop()
{
  //long now = millis();

  //handleRotaryEncoder();
  //handleTempHumSensor(now);
  //handleHeater();
  updateScreenClock();
}
