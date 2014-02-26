#include <LiquidCrystal.h>
#include <math.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

const int backlightPin = 6;
      int brightness = 0;
const int fadeLen = 768;     // Duration of the fade in/out. Has to be a multiple
                             // of 256.
const int fridgeTempPin = A4;
const int freezerTempPin = A2;
const int piezoPin = 3;
const int timeFactor = 1;   // realtime factor for debug. 1 means normal loop time.
const int switchPin = 2;

const int trendDelta = 1;  // ºC difference until a trend change is decreted

int storedCnt = 0;

float avgFridgeTemp, prevAvgFridgeTemp, avgFreezerTemp, prevAvgFreezerTemp, sumAvgFridgeTemp, sumAvgFreezerTemp;
float storedFridgeTemp[12];
float storedFreezerTemp[12];

unsigned long loopMillis, lastStoreTempMillis, lastCalcTrendMillis;

//used to know what alarm is triggered
const int NO_ALARM = 0;
const int LOW_TEMP = 1;
const int HIGH_TEMP = 2;
const int CRITICAL_TEMP = 3;
int currFridgeAlarm = NO_ALARM;
int currFreezerAlarm = NO_ALARM;
int prevFridgeAlarm = NO_ALARM;
int prevFreezerAlarm = NO_ALARM;
byte fridgeAlarmChanged, freezerAlarmChanged, alarmAck;


// some custom smileys and symbols for the LCD
const int charDegree = 1;
byte degree[8] = {
  4,10,10,4,0,14,0,0};

const int charTrendUp = 2;
byte rising[8] = {
  0,0,4,14,31,0,0,0};

const int charTrendSteady = 3;
byte steady[8] = {
  0,8,12,14,12,8,0,0};

const int charTrendDown = 4;
byte falling[8] = {
  0,0,31,14,4,0,0,0};

const int charHappy = 5;
byte happy[8] = {
  10,10,0,0,17,17,14,0};

const int charSad = 6;
byte sad[8] = {
  10,10,0,0,14,17,17,0};

const int charCritical = 7;
byte critical[8] = {
  17,10,17,0,14,17,17,14};
  
const int charNA = 8;
byte na[8] = {
  31,17,14,4,10,21,31,0};
  //0,14,21,23,17,14,0,0};
  //9,13,11,9,6,9,15,9};
  
const int charClock0 = 9;
byte clock0[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock1 = 10;
byte clock1[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock2 = 11;
byte clock2[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock3 = 12;
byte clock3[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock4 = 13;
byte clock4[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock5 = 14;
byte clock5[8] = {
  0,14,21,23,17,14,0,0};
  
  const int charClock6 = 15;
byte clock6[8] = {
  0,14,21,23,17,14,0,0};


void setup() {
  Serial.begin(115200);
  lcd.begin(20,4);
  lcd.createChar(1, degree);
  lcd.createChar(2, rising);
  lcd.createChar(3, steady);
  lcd.createChar(4, falling);
  lcd.createChar(5, happy);
  lcd.createChar(6, sad);
  lcd.createChar(7, critical);
  lcd.createChar(8, na);
  pinMode(backlightPin, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  Serial.begin(115220);
  attachInterrupt(0, silenceAlarm, LOW); // (dirty ?) way to register silence alarm button presses
  fadeLCD(1,fadeLen); // a backlight fading effect looks cool
  lcd.home();
  lcd.print("  Fridge & Freezer  ");
  lcd.setCursor(0,1);
  lcd.print(" Temperature Monitor");
  lcd.setCursor(0,3);
  lcd.print("   Initialising...");
}

void loop() {
  if ((millis() - loopMillis) > (10000/timeFactor)) {
    if (fridgeAlarmChanged + freezerAlarmChanged > 0) {
      alarmAck = 0;
    };
    Serial.println("***** start loop *****");
    lcd.home();
    lcd.clear();
    lcd.print("Fridge:");
    lcd.setCursor(10,0);
    // averaging the readings for improved accuracy
    avgFridgeTemp = 0.3 * lm335(fridgeTempPin) + 0.7 * avgFridgeTemp;
    lcd.print(avgFridgeTemp);
    lcd.setCursor(16,0);
    lcd.write(1);
    lcd.setCursor(17,0);
    lcd.print("C");
    
    lcd.setCursor(0,2);
    lcd.print("Freezer:");
    lcd.setCursor(10,2);
    // averaging the readings for improved accuracy
    avgFreezerTemp = 0.3 * lm335(freezerTempPin) + 0.7 * avgFreezerTemp;
    lcd.print(avgFreezerTemp);
    lcd.setCursor(16,2);
    lcd.write(1);
    lcd.setCursor(17,2);
    lcd.print("C");
    
    // the temperature gets stored every 5 mins (or less if realtime factor is >1)
    storeTemp();
    // calculate the temperature trend vs the stored samples
    calcTrend(prevAvgFridgeTemp, avgFridgeTemp, 1);
    prevAvgFridgeTemp = avgFridgeTemp;
    calcTrend(prevAvgFreezerTemp, avgFreezerTemp, 2);
    prevAvgFreezerTemp = avgFreezerTemp;
    
    int prevFridgeAlarm2 = currFridgeAlarm;
    int prevFreezerAlarm2 = currFreezerAlarm;
    // checking if an alarm needs to be raised
    checkAlarm(avgFridgeTemp,1);
    checkAlarm(avgFreezerTemp,2);
    
    soundAlarm();
    
    loopMillis = millis();
  }
}


















/**************************************
**
**
**     FUNCTIONS START HERE
**
**
***************************************/








// my function to fade the backlight in or out
// mode 1 = fade in
// mode 2 = fade out
// mode 3 = on without fading (for blinking on alarm)
// mode 4 = off without fading (for blinking on alarm)
void fadeLCD(byte mode, int duration) {
  if (mode == 1) {
    for (brightness = 0; brightness <= 255; brightness++) {
      analogWrite(backlightPin, brightness);
      if (brightness != 255) {
        delay(duration/256);
      }
    }
  }
  if (mode == 0) {
    for (brightness = 255; brightness >= 0; brightness--) {
      analogWrite(backlightPin, brightness);
      if (brightness != 0) {
        delay(duration/256);
      }
    }
  }
  if (mode == 3) {
    digitalWrite(backlightPin, HIGH);
  }
  if (mode == 4) {
    digitalWrite(backlightPin, LOW);
  }
}








// now unused because I replaced the thermistors with LM335s hoping to get
// improved accuracy (hum... not quite there yet)
double tempC(int sensorPin) {
  int analogVal = analogRead(sensorPin);
  double temp;
  temp = log(((10240000/analogVal) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
  return temp;
}







// used to check if 5 minutes have passed since the last storage.
// if so, store a new value and print a '+' on the LCD to show when the
// value is stored
void storeTemp() {
  if (millis() - lastStoreTempMillis > 300000/timeFactor) {   // store a sample every 5 min
    storedFridgeTemp[storedCnt] = avgFridgeTemp;
    lcd.setCursor(19,1);
    lcd.print("+");
    Serial.print("storeTemp() avgFridgeTemp: ");
    Serial.println(avgFridgeTemp);
    
    storedFreezerTemp[storedCnt] = avgFreezerTemp;
    lcd.setCursor(19,3);
    lcd.print("+");
    Serial.print("storeTemp() avgFreezerTemp: ");
    Serial.println(avgFreezerTemp);
    
    // I want to stop at 12 because I am averaging and trending only over
    // the last 60 mins, overwriting the oldest value in the arrays
    if (storedCnt > 12) {
      storedCnt = 0;
    } else {
      storedCnt++;
    }
    
    lastStoreTempMillis = millis();
  }
}









// function to calculate the trend of the current temperature vs the average of the temperatures
// over the last 60 minutes. If the Arduino has been on less than 60 minutes then a nice hourglass
// is displayed instead of the up/steady/down trend arrow.
// case 1 = fridge sensor
// case 2 = freezer sensor
void calcTrend(float prevAvg, float currAvg, int sensorNum) {
  if (millis() < 3600000/timeFactor) {
    lcd.setCursor(19,0);
    lcd.write(charNA);
    lcd.setCursor(19,2);
    lcd.write(charNA);
  } else {
  //if (millis() - lastCalcTrendMillis > 3600000/timeFactor) {        //calc trend every hour
    switch (sensorNum) {
      case 1:
        sumAvgFridgeTemp = 0;
        for (int i=0; i <= 11; i++) {
          sumAvgFridgeTemp += storedFridgeTemp[i];
          Serial.print("calcTrend() sumAvgFridgeTemp: ");
          Serial.println(sumAvgFridgeTemp);
        }
        sumAvgFridgeTemp /= 12;
        Serial.print("calcTrend() sumAvgFridgeTemp/12= ");
        Serial.println(sumAvgFridgeTemp);
        if (currAvg > (sumAvgFridgeTemp + trendDelta)) {
          lcd.setCursor(19,0);
          lcd.write(2);
          return;
        }
        if ((currAvg <= (sumAvgFridgeTemp + trendDelta)) && ((currAvg >= sumAvgFridgeTemp - trendDelta))) {
          lcd.setCursor(19,0);
          lcd.write(3);
          return;
        }
        if (currAvg < (sumAvgFridgeTemp - trendDelta)) {
          lcd.setCursor(19,0);
          lcd.write(4);
          return;
        }
      case 2:
        sumAvgFreezerTemp = 0;
        for (int i=0; i <= 11; i++) {
          sumAvgFreezerTemp += storedFreezerTemp[i];
          Serial.print("calcTrend() sumAvgFreezerTemp: ");
          Serial.println(sumAvgFreezerTemp);
        }
        sumAvgFreezerTemp /= 12;
        Serial.print("calcTrend() sumAvgFreezerTemp/12= ");
        Serial.println(sumAvgFreezerTemp);
        if (currAvg > (sumAvgFreezerTemp + trendDelta)) {
          lcd.setCursor(19,2);
          lcd.write(2);
          return;
        }
        if ((currAvg <= (sumAvgFreezerTemp + trendDelta)) && ((currAvg >= sumAvgFreezerTemp - trendDelta))) {
          lcd.setCursor(19,2);
          lcd.write(3);
          return;
        }
        if (currAvg < (sumAvgFreezerTemp - trendDelta)) {
          lcd.setCursor(19,2);
          lcd.write(4);
          return;
        }
    }
  }
  lastCalcTrendMillis = millis();
}








// function to check if an alarm needs to be raised. the function is also signaling if the alarm
// raised is the same type as in the previous loop or not. this is in turned used for the silence
// button interrupt result parsing. I want to keep the piezo quiet if the silence alarm button was
// pressed before and the alarm type hasn't changed, but I want the piezo to sound if the alarm
// has been silenced before but its type has changed (i.e. went from HIGH_TEMP to CRITICAL_TEMP or
// from HIGH_TEMP to NO_ALARM to LOW_TEMP.
// case 1 = fridge sensor
// case 2 = freezer sensor
int checkAlarm(float currAvg,int sensorNum) {
  switch (sensorNum) {
    case 1:
      if (currAvg >= 0 && currAvg <= 3) {
        lcd.setCursor(2,1);
        lcd.print("LOW TEMP");
        lcd.setCursor(0,1);
        lcd.write(6);  // sad smiley
        currFridgeAlarm = LOW_TEMP;
        if (currFridgeAlarm != prevFridgeAlarm) {
        fridgeAlarmChanged = 1;
        } else {
          fridgeAlarmChanged = 0;
        }
        prevFridgeAlarm = currFridgeAlarm;
        return currFridgeAlarm;
      }
      if (currAvg > 3 && currAvg <= 5) {
        lcd.setCursor(2,1);
        lcd.print("OK");
        lcd.setCursor(0,1);
        lcd.write(5);  // happy smiley
        currFridgeAlarm = NO_ALARM;
        if (currFridgeAlarm != prevFridgeAlarm) {
        fridgeAlarmChanged = 1;
        } else {
          fridgeAlarmChanged = 0;
        }
        prevFridgeAlarm = currFridgeAlarm;
        return currFridgeAlarm;
      }
      if (currAvg > 5 && currAvg <= 10) {
        lcd.setCursor(2,1);
        lcd.print("HIGH TEMP");
        lcd.setCursor(0,1);
        lcd.write(6); // sad smiley
        currFridgeAlarm = HIGH_TEMP;
        if (currFridgeAlarm != prevFridgeAlarm) {
        fridgeAlarmChanged = 1;
        } else {
          fridgeAlarmChanged = 0;
        }
        prevFridgeAlarm = currFridgeAlarm;
        return currFridgeAlarm;
      }
      if (currAvg < 0 || currAvg > 10) {
        lcd.setCursor(2,1);
        lcd.print("CRITICAL TEMP");
        lcd.setCursor(0,1);
        lcd.write(7); // critical smiley
        currFridgeAlarm = CRITICAL_TEMP;
        if (currFridgeAlarm != prevFridgeAlarm) {
        fridgeAlarmChanged = 1;
        } else {
          fridgeAlarmChanged = 0;
        }
        prevFridgeAlarm = currFridgeAlarm;
      }
      
    case 2:
      if (currAvg >= -30 && currAvg <= -21) {
        lcd.setCursor(2,1);
        lcd.print("WARNING: low temp");
        lcd.setCursor(0,1);
        lcd.write(6);  // sad smiley
        currFreezerAlarm = LOW_TEMP;
        if (currFreezerAlarm != prevFreezerAlarm) {
        freezerAlarmChanged = 1;
        } else {
          freezerAlarmChanged = 0;
        }
        prevFreezerAlarm = currFreezerAlarm;
        return currFreezerAlarm;
      }
      if (currAvg > -21 && currAvg <= -15) {
        lcd.setCursor(2,3);
        lcd.print("OK");
        lcd.setCursor(0,3);
        lcd.write(5);  // happy smiley
        currFreezerAlarm = NO_ALARM;
        if (currFreezerAlarm != prevFreezerAlarm) {
        freezerAlarmChanged = 1;
        } else {
          freezerAlarmChanged = 0;
        }
        prevFreezerAlarm = currFreezerAlarm;
        return currFreezerAlarm;
      }
      if (currAvg > -15 && currAvg <= -5) {
        lcd.setCursor(2,3);
        lcd.print("WARNING: high temp");
        lcd.setCursor(0,3);
        lcd.write(6); // sad smiley
        currFreezerAlarm = HIGH_TEMP;
        if (currFreezerAlarm != prevFreezerAlarm) {
        freezerAlarmChanged = 1;
        } else {
          freezerAlarmChanged = 0;
        }
        prevFreezerAlarm = currFreezerAlarm;
        return currFreezerAlarm;
      }
      if (currAvg < -30 || currAvg > -5) {
        lcd.setCursor(2,3);
        lcd.print("CRITICAL TEMP");
        lcd.setCursor(0,3);
        lcd.write(7); // critical smiley
        currFreezerAlarm = CRITICAL_TEMP;
        if (currFreezerAlarm != prevFreezerAlarm) {
        freezerAlarmChanged = 1;
        } else {
          freezerAlarmChanged = 0;
        }
        prevFreezerAlarm = currFreezerAlarm;
      }
      return currFreezerAlarm;
  }
}





// quite straight forward
void silenceAlarm() {
  alarmAck = 1;
}






// function to sound the alarm, depending on its type. also checks if the piezo has been silenced
// and flashes the backlight.
void soundAlarm() {
  if (currFridgeAlarm == LOW_TEMP || currFreezerAlarm == LOW_TEMP) {
    if (alarmAck == 0) {
      analogWrite(piezoPin, 100);
      fadeLCD(4,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
      delay(50);
      analogWrite(piezoPin, 100);
      fadeLCD(4,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
    }
    if (alarmAck == 1) {
      fadeLCD(4,0);
      delay(100);
      fadeLCD(4,0);
      delay(50);
      fadeLCD(4,0);
      delay(100);
      fadeLCD(4,0);
    }
  }
  if (currFridgeAlarm == HIGH_TEMP || currFreezerAlarm == HIGH_TEMP) {
    if (alarmAck == 0) {
      analogWrite(piezoPin, 100);
      fadeLCD(4,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
    }
    if (alarmAck == 1) {
      fadeLCD(4,0);
      delay(100);
      fadeLCD(4,0);
    }
  }
  if (currFridgeAlarm == CRITICAL_TEMP || currFreezerAlarm == CRITICAL_TEMP) {
    if (alarmAck == 0) {
      analogWrite(piezoPin, 10);
      fadeLCD(3,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
      delay(50);
      analogWrite(piezoPin, 100);
      fadeLCD(3,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
      delay(50);
      analogWrite(piezoPin, 1000);
      fadeLCD(3,0);
      delay(100);
      analogWrite(piezoPin, 0);
      fadeLCD(4,0);
    }
    if (alarmAck == 1) {
    fadeLCD(3,0);
    delay(100);
    fadeLCD(4,0);
    delay(50);
    fadeLCD(3,0);
    delay(100);
    fadeLCD(4,0);
    delay(50);
    fadeLCD(3,0);
    delay(100);
    fadeLCD(4,0);
    }
  }
}


// calculates the LM335 output in ºC
float lm335(int sensorPin) {
  int analogVal = analogRead(sensorPin);
  float temp;
  temp = (analogVal / 1024.0) * 5; // 4.xxV is the mesured 5V on 5V pin
  temp = (temp * 100) - 273.15;
  Serial.println(analogVal);
  return temp;
}
