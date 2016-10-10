const int MOIS_SENS_PIN = 1;
//const int FEED_MOIST_SENS = 3;
const int DEFAULT_MOIST = 40;
int MINIMUM_MOIST;

const int GREEN_LED_PIN = 4;
const int RED_LED_PIN = 5;
const int SPEAKER_PIN = 6;

const int BUTTON_PIN = 7;

const int LIGHT_SENS_PIN = 0;
const int FEED_LIGHT_SENS_PIN = 2;
const int ALARM_LIGHT_LEVEL = 60;

const int NO_CONDITION_SET = 0;
const int USE_DEFAULT_MOIST_CONDITION = 1;
const int USE_CURRENT_MOIST_CONDITION = 2;
const int TEST_CONDITION = 3;

const long GREEN_COND_MEASUREMENT_DELAY = 600000;
const long RED_COND_MEASUREMENT_DELAY = 60000;
long measurementDelay;

int condition;
int prevCondition;
long mlls;
int loopDelay;
bool flagButton;

void setup() {
  Serial.begin(9600);
  initMoistureModule();
  initLightModule();
  initIndicationModule();
  initControlModule();
}

void loop() {
  if(readButton() == HIGH){
    handleButton();
  }
  else{
    if(flagButton){//Button has been just released and condition action must be taken
      settleCondition();
    }

    if(millis()-mlls >= measurementDelay) doMeasurement();
  }

  delay(loopDelay);
}

void initMoistureModule(){
  //pinMode(FEED_MOIST_SENS, OUTPUT);
  MINIMUM_MOIST = DEFAULT_MOIST;
}

void initLightModule(){
  pinMode(FEED_LIGHT_SENS_PIN, OUTPUT);
}

void initIndicationModule(){
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
}

void initControlModule(){
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);
  prevCondition = condition = USE_DEFAULT_MOIST_CONDITION;
  mlls = measurementDelay = 0;
  loopDelay = 200;
  flagButton = false;
}

int readMoisture(){
  int value = analogRead(MOIS_SENS_PIN);
  //300 completely wet and 1024 completely dry
  value = 100-map(value, 300, 1024, 0, 100);
  Serial.print("Moisture level: ");
  Serial.println(value);
  return value;
}

int readLight(){
  digitalWrite(FEED_LIGHT_SENS_PIN, HIGH);
  int value = analogRead(LIGHT_SENS_PIN);
  digitalWrite(FEED_LIGHT_SENS_PIN, LOW);
  value = map(value, 0, 1023, 0, 100);
  Serial.print("Light level: ");
  Serial.println(value);
  
  return value;
}

void doBlink(int LED, int duration, int times){
  for(int i=0;i<times;i++){
    digitalWrite(LED, HIGH);
    delay(duration);
    digitalWrite(LED, LOW);
    delay(duration);
  }
}

void doLoudBlink(int LED, int duration, int times, int freq){
  for(int i=0;i<times;i++){
    digitalWrite(LED, HIGH);
    tone(SPEAKER_PIN, freq, duration);
    delay(duration);
    digitalWrite(LED, LOW);
    delay(duration);
  }
}

void playTone(int freq, int duration, int interval, int times){
  for(int i=0;i<times;i++){
    tone(SPEAKER_PIN, freq, duration);
    delay(interval);
  }
}

int readButton(){
  return !digitalRead(BUTTON_PIN);
}

void handleButton(){
  if(!flagButton){
    Serial.println("Button pressed");
    doLoudBlink(GREEN_LED_PIN, 250, 1, 2000);
    mlls = millis();
    prevCondition = condition;
    condition = NO_CONDITION_SET;
    flagButton = true;
  }
  
  if(condition == NO_CONDITION_SET && millis() - mlls > 2000){
    condition = USE_CURRENT_MOIST_CONDITION;
    playTone(1000, 250, 0, 1);
  }
  else if(condition == USE_CURRENT_MOIST_CONDITION && millis() - mlls > 5000){
    condition = USE_DEFAULT_MOIST_CONDITION;
    playTone(2500, 250, 250, 2);
  }
  else if(condition == USE_DEFAULT_MOIST_CONDITION && millis() - mlls > 7000){
    condition = TEST_CONDITION;
    playTone(3000, 250, 250, 3);
  }
}

void settleCondition(){
  if(condition == NO_CONDITION_SET){
    condition = prevCondition;
  }
  else if(condition == USE_DEFAULT_MOIST_CONDITION){
    Serial.print("MIN MOISTURE LEVEL SET TO DEFAULT: ");
    Serial.println(DEFAULT_MOIST);
    MINIMUM_MOIST = DEFAULT_MOIST;
  }
  else if(condition == USE_CURRENT_MOIST_CONDITION){
    Serial.print("MIN MOISTURE LEVEL SET TO: ");
    MINIMUM_MOIST = readMoisture();
    Serial.println(MINIMUM_MOIST);
  }
  else if(condition == 3){//TEST CONDITION
    Serial.println("Current condition: TEST CONDITION");
    condition = prevCondition;
    doBlink(GREEN_LED_PIN, 250, 5);
    doBlink(GREEN_LED_PIN, 1000, 1);
    doLoudBlink(GREEN_LED_PIN, 250, 5, 2000);
    doBlink(RED_LED_PIN, 250, 5);
    doBlink(RED_LED_PIN, 1000, 1);
    doLoudBlink(RED_LED_PIN, 250, 5, 2000);
    playTone(100, 250, 0, 1);
    playTone(3500, 800, 0, 1);
    delay(100);
  }

  flagButton = false;
  mlls = measurementDelay = 0;
  if(condition == 1) Serial.println("Current condition: USE DEFAULT MOISTURE LEVEL");
  else if(condition == 2){
    Serial.print("Current condition: USE CUSTOM MOISTURE LEVEL: ");
    Serial.println(MINIMUM_MOIST);
  }
  else Serial.println("Current condition: UNKNOWN");
}

void doMeasurement(){
  int val = readMoisture();
  Serial.print("Measurement: ");
  Serial.println(val);
  mlls = millis();
  if(val >= MINIMUM_MOIST){
    doBlink(GREEN_LED_PIN, map(val, MINIMUM_MOIST, 100, 500, 1500), 1);
    measurementDelay = GREEN_COND_MEASUREMENT_DELAY;
    Serial.println("Delay set for green condition");
  }
  else{
    if(readLight() >= ALARM_LIGHT_LEVEL){
      doLoudBlink(RED_LED_PIN, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4), 2000);
    }
    else{
      doBlink(RED_LED_PIN, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4));
    }
    measurementDelay = RED_COND_MEASUREMENT_DELAY;
    Serial.println("Delay set for red condition");
  }
}
