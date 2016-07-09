/*****************************************************************
 *                                                               *
 * Kelli's Steps Widget                                          *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
 *                                                               *
 *****************************************************************/

#include "illumiconeWidget.h"
#include <NewPing.h>
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 4
#define MEASUREMENT_INTERVAL_MS 200L
#define GOOD_MEASUREMENT_SUSTAIN_MS 500L
#define MAX_VALID_DISTANCE_CM 100
//#define LED_PIN 2
//#define ENABLE_DEBUG_PRINT
#define NUM_SENSORS 2
#define TRIGGER_PIN_0  7  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_0     8  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_0 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define TRIGGER_PIN_1  A2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_1     A3 // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_1 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/
// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "2wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 11

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

MeasurementVectorPayload payload;

NewPing sonar[NUM_SENSORS] = {
  NewPing(TRIGGER_PIN_0, ECHO_PIN_0, MAX_DISTANCE_0),
  NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE_1)
};

int distance[NUM_SENSORS];
bool isActive;


/******************
 * Implementation *
 ******************/

void gatherMeasurements()
{
  static int lastGoodMeasurement[NUM_SENSORS];
  static unsigned long lastGoodMeasurementMs[NUM_SENSORS];

  uint32_t now = millis();

  bool currentlyActive = false;
  for (int i = 0; i < NUM_SENSORS; ++i) {

    distance[i] = sonar[i].ping_cm();
    if (distance[i] > MAX_VALID_DISTANCE_CM) {
      distance[i] = 0;
    }
    
    if (distance[i] > 0) {
      lastGoodMeasurement[i] = distance[i];
      lastGoodMeasurementMs[i] = now;
      currentlyActive = true;
    }
    else {
      if (now - lastGoodMeasurementMs[i] <= GOOD_MEASUREMENT_SUSTAIN_MS) {
        distance[i] = lastGoodMeasurement[i];
        currentlyActive = true;
      }
    }
  }

  if (currentlyActive) {
    isActive = true;
    sendMeasurements();
  }
  else {
    if (isActive) {
      isActive = false;
      // Tell the pattern controller that we're not active anymore.
      sendMeasurements();
    }
  }
}


void sendMeasurements()
{
    for (int i = 0; i < NUM_SENSORS; ++i) {
        payload.measurements[i] = distance[i];
    }

    payload.widgetHeader.isActive = isActive;

    if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * NUM_SENSORS)) {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, HIGH);
#endif
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("radio.write failed."));
#endif
    }
    else {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, LOW);
#endif
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("radio.write succeeded."));
#endif
    }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(57600);
  printf_begin();
#endif

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

  isActive = false;
}


void loop() {

  static int32_t lastMeasurementMs;
  static int32_t lastTxMs;

  uint32_t now = millis();
  if (now - lastMeasurementMs >= MEASUREMENT_INTERVAL_MS) {
    gatherMeasurements();
    lastMeasurementMs = now;
  }

}

