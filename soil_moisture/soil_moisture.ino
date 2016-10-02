const int MOIS_SENS = 0;
const int FEED_MOIST_SENS = 3;
const int DEFAULT_MOIST = 40;
int MINIMUM_MOIST;

const int GREEN_LED = 5;
const int RED_LED = 4;
const int SPEAKER = 7;

const int BUTTON = 2;

const int LIGHT_SENS = 1;
const int FEED_LIGHT_SENS = 6;

const int NO_CONDITION = 0;
const int INITIAL_CONDITION = 1;
const int USE_CURRENT_MOIST_CONDITION = 2;
const int USE_DEFAULT_MOIST_CONDITION = 3;
const int TEST_CONDITION = 4;

int condition;
long mlls;
int loopDelay;

void setup() {
  Serial.begin(9600);
  initMoistureModule();
  initLightModule();
  initIndicationModule();
}

void loop() {
  if(readButton() == HIGH){
    handleButton();
  }
  else{
    if(condition == NO_CONDITION){
      int val = readMoisture();
      Serial.println(val);
      if(val >= MINIMUM_MOIST){
        doBlink(GREEN_LED, map(val, MINIMUM_MOIST, 100, 500, 1500), 1);
        //loopDelay = 60000;
      }
      else{
        if(readLight() >= 60){
          doLoudBlink(RED_LED, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4), 2000);
        }
        else{
          doBlink(RED_LED, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4));
        }
        //loopDelay = 30000;
      }
    }
    else if(condition == USE_CURRENT_MOIST_CONDITION){
      condition = NO_CONDITION;
      MINIMUM_MOIST = readMoisture();
      loopDelay = 0;
    }
    else if(condition == USE_DEFAULT_MOIST_CONDITION){
      condition = NO_CONDITION;
      MINIMUM_MOIST = DEFAULT_MOIST;
    }
    else if(condition == TEST_CONDITION){
      condition = NO_CONDITION;
      doBlink(GREEN_LED, 250, 5);
      doBlink(GREEN_LED, 1000, 1);
      doLoudBlink(GREEN_LED, 250, 5, 2000);
      doBlink(RED_LED, 250, 5);
      doBlink(RED_LED, 1000, 1);
      doLoudBlink(RED_LED, 250, 5, 2000);
      playTone(100, 250, 0, 1);
      playTone(3500, 800, 0, 1);
    }
    else{
      condition = NO_CONDITION;
    }
  }

  delay(loopDelay);
}

void initMoistureModule(){
  pinMode(FEED_MOIST_SENS, OUTPUT);
  MINIMUM_MOIST = DEFAULT_MOIST;
}

void initLightModule(){
  pinMode(FEED_LIGHT_SENS, OUTPUT);
}

void initIndicationModule(){
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
}

void initControlModule(){
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);
  condition = NO_CONDITION;
  mlls = 0;
  loopDelay = 0;
}

int readMoisture(){
  digitalWrite(FEED_MOIST_SENS, HIGH);
  int value = analogRead(MOIS_SENS);
  Serial.print("Real value: ");
  Serial.println(value);
  digitalWrite(FEED_MOIST_SENS, LOW);

  return map(value, 0, 1023, 0, 100);;
}

int readLight(){
  digitalWrite(FEED_LIGHT_SENS, HIGH);
  int value = analogRead(LIGHT_SENS);
  digitalWrite(FEED_LIGHT_SENS, LOW);
  
  return map(value, 0, 1023, 0, 100);
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
    tone(SPEAKER, freq, duration);
    delay(duration);
    digitalWrite(LED, LOW);
    delay(duration);
  }
}

void playTone(int freq, int duration, int interval, int times){
  for(int i=0;i<times;i++){
    tone(SPEAKER, freq, duration);
    delay(interval);
  }
}

int readButton(){
  return !digitalRead(BUTTON);
}

void handleButton(){
  loopDelay = 0;
  if(condition == NO_CONDITION){
    doLoudBlink(GREEN_LED, 250, 1, 2000);
    mlls = millis();
    condition = INITIAL_CONDITION;
  }
  else if(condition == INITIAL_CONDITION){
    if(millis() - mlls > 2000){
      condition = USE_CURRENT_MOIST_CONDITION;
      playTone(1000, 250, 0, 1);
    }
  }
  else if(condition == USE_CURRENT_MOIST_CONDITION){
    if(millis() - mlls > 5000){
      condition = USE_DEFAULT_MOIST_CONDITION;
      playTone(2500, 250, 250, 2);
    }
  }
  else if(condition == USE_DEFAULT_MOIST_CONDITION){
    if(millis() - mlls > 7000){
      condition = TEST_CONDITION;
      playTone(3000, 250, 250, 3);
    }
  }
}
