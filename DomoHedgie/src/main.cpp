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

#define HEATER_MODE_SWITCH_AUTO_PIN 7
#define HEATER_MODE_SWITCH_OFF_PIN 8
#define HEATER_MODE_SWITCH_ON_PIN 9
#define HEATER_RELAY_PIN 10

#define LIGHT_MODE_SWITCH_AUTO_PIN 11
#define LIGHT_MODE_SWITCH_OFF_PIN 12
#define LIGHT_MODE_SWITCH_ON_PIN 13
#define LIGHT_RELAY_PIN 14

#define TEMP_HUM_DIGITAL_SENSOR_PIN 15

#define ENTER_BUTTON_PIN 18 //ISR Pin 5
#define CANCEL_BUTTON_PIN 19 //ISR Pin 4
#define SHUTOFF_BUTTON_PIN 20 //ISR Pin 3

/**
* SYSTEM NAMES
**/

#define HEATER_SYSTEM_NAME "HEATER"
#define TEMP_HUM_SYSTEM_NAME "TEMP/HUM"

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

/**
* DATETIME METHODS
**/

Datetime getDateTime(){
  //TODO Getting time properly from the clock
  Datetime now;
  now.day = 1;
  now.month = 1;
  now.year = 1970;

  now.hour = now.minute = now.second = 0;

  return now;
}

/**
* LOG METHODS
**/

void log(String systemName, String message){
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
        log(TEMP_HUM_SYSTEM_NAME, "Timeout error occured");
        break;
      case -1://CHECKSUM error
        log(TEMP_HUM_SYSTEM_NAME, "Checksum error occured");
        break;
      case 0://OK
        break;
    }
  }

  return result;
}

/**
* TEMPERATURE / HUMIDITY SENSOR METHODS
**/

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
      log(TEMP_HUM_SYSTEM_NAME, "Safe mode activated");
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
    log(HEATER_SYSTEM_NAME, message);
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
    log(HEATER_SYSTEM_NAME, message);
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
        case SHUTOFF_BUTTON_PIN:
          executeShutOffButton();
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
* The ISR method fot the Shut off button
* args: none
* return: none
*/
void handleShutoffButton(){
  handleButton(SHUTOFF_BUTTON_PIN);
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
  attachInterrupt(digitalPinToInterrupt(ENTER_BUTTON_PIN), handleEnterButton, LOW);
}

void initCancelButton(){
  pinMode(CANCEL_BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CANCEL_BUTTON_PIN), handleCancelButton, LOW);
}

void initShutoffButton(){
  pinMode(SHUTOFF_BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SHUTOFF_BUTTON_PIN), handleShutoffButton, LOW);
}

void initRotaryEncoder(){
  pinMode(ROTARY_A_PIN, INPUT);
  pinMode(ROTARY_B_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ROTARY_A_PIN), rotEncoder, CHANGE);
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

/**
* MAIN METHODS
**/

void setup()
{
  Serial.begin(9600);
  Serial.println("INIT");

  initMenuItems();
  initRotaryEncoder();
  initEnterButton();
  initShutoffButton();
  initHeater();
  initTempHumSensor();
}

void loop()
{
  long now = millis();

  handleRotaryEncoder();
  handleTempHumSensor(now);
  handleHeater();
}
