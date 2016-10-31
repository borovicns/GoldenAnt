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

/**
MENU VARIABLES
**/

struct MenuItem{
  int id;
  String name;
};

const int mainMenuDimension = 6;
MenuItem mainMenu[mainMenuDimension];
int menuIndex;

/**
ROTARY ENCODER VARIABLES
**/

static boolean rotating=false;
bool rotaryFlag = false;

const int rotaryPinA = 2;
const int rotaryPinB = 3;
const int rotaryDelay = 2;

/**
INIT MODULES
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

void initRotaryEncoder(){
  pinMode(rotaryPinA, INPUT);
  pinMode(rotaryPinB, INPUT);
  attachInterrupt(digitalPinToInterrupt(rotaryPinA), rotEncoder, CHANGE);
}

/**
GRAPHIC METHODS
**/

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
}

/**
CONTROL METHODS
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
* This method handles the behaviour of the rotary encoder detecting the rotation
* direction and calling for updating the main menu.
* args: none
* return: none
*/
void handleRotaryEncoder(){
  while(rotating){
    delay(rotaryDelay);
    if (digitalRead(rotaryPinB) == digitalRead(rotaryPinA)){
      if(!rotaryFlag){
        //CW
        rotaryFlag = true;
        moveMenuIndexForward();
        updateMainMenu();
      }
      else rotaryFlag = false;
    }
    else{
      if(!rotaryFlag){
        //CCW
        rotaryFlag = true;
        moveMenuIndexBackward();
        updateMainMenu();
      }
      else rotaryFlag = false;
    }
    rotating=false; // Reset the flag
  }
}

void setup()
{
  Serial.begin(9600);

  initMenuItems();
  initRotaryEncoder();
}

void loop()
{
  handleRotaryEncoder();
}
