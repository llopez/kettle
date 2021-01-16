#include <OneWire.h>
#include <DallasTemperature.h>

#if defined(__AVR_ATtiny85__)
  #define relayPin 0
  #define ledPin 1
  #define modePin 4
  #define sensorPin 3
#else
  #define relayPin 3
  #define ledPin 4
  #define modePin 5
  #define sensorPin 2
  #define serialEnabled
#endif

#define mateStartTemp 70
#define mateEndTemp 80
#define teaStartTemp 80
#define teaEndTemp 90

OneWire oneWire(sensorPin);
DallasTemperature tempSensor(&oneWire);

static enum { MATE, TEA } mode = MATE;

static enum { WAIT, HEAT, READY } state = WAIT;
float temp = 0;

float startTemp;
float endTemp;

unsigned long currentTime = 0;
unsigned long lastHeatTime = 0;
unsigned long lastWaitTime = 0;
unsigned long lastPrintTime = 0;
unsigned long lastBlinkTime = 0;
unsigned int heatTime = 10000;
unsigned int waitTime = 0;
unsigned int blinkTime = 1000;

void setup() {
  #if defined(serialEnabled)
    Serial.begin(9600);
  #endif
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(modePin, INPUT_PULLUP);
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);
  tempSensor.begin();
  tempSensor.setResolution(9);
}

void loop() {
  digitalRead(modePin) == LOW ? mode = MATE : mode = TEA;

  if (mode == MATE) {
    startTemp = mateStartTemp;
    endTemp = mateEndTemp;
  } else {
    startTemp = teaStartTemp;
    endTemp = teaEndTemp;
  }
  
  currentTime = millis();


  #if defined(serialEnabled)
    if (currentTime - lastPrintTime > 1000) {
      printLog();
    }
  #endif

  if (currentTime - lastBlinkTime > blinkTime && state != READY) {
    lastBlinkTime = millis();
    digitalWrite(ledPin, !digitalRead(ledPin));
  }

  switch (state) {
    case WAIT:
      if (currentTime - lastWaitTime > waitTime) {
        temp = mesureTemp();
        
        if (temp > 0 && temp < 50) {
          heatTime = 30000;
          waitTime = 10000;
          lastHeatTime = millis();
          blinkTime = 1000;
          state = HEAT;
          digitalWrite(relayPin, HIGH);
        }
        if (temp >= 50 && temp < startTemp) {
          heatTime = 20000;
          waitTime = 10000;
          blinkTime = 500;
          lastHeatTime = millis();
          state = HEAT;
          digitalWrite(relayPin, HIGH);
        }
        if (temp >= startTemp && temp < startTemp + 5) {
          heatTime = 5000;
          waitTime = 10000;
          blinkTime = 250;
          lastHeatTime = millis();
          state = HEAT;
          digitalWrite(relayPin, HIGH);
        }
        if (temp >= (startTemp + 5) && temp < endTemp) {
          digitalWrite(ledPin, HIGH);
          state = READY;
        }
      }
    break;
    case HEAT:
      if (currentTime - lastHeatTime > heatTime) {
        lastWaitTime = millis();
        digitalWrite(relayPin, LOW);
        state = WAIT;
      }
    break;
    case READY:
      temp = mesureTemp();
      if (temp < startTemp) {
        lastWaitTime = millis();
        waitTime = 0;
        state = WAIT;
      }
    break;
    default:
      state = WAIT;
    break;
  }
}

float mesureTemp() {
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
}


#if defined(serialEnabled)
  
  void printLog() {
    lastPrintTime = millis();
    Serial.println("");
    Serial.print(temp);
    Serial.print(" - ");
    Serial.print(state == 0 ? "WAIT" : state == 1 ? "HEAT" : "READY");
    Serial.print(" - ");
    Serial.print(mode == 0 ? "MATE" : "TEA" );
  }

#endif
