/*****************************************************************
 *                                                               *
 * Kelli's Steps Widget                                          *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
 *                                                               *
 *****************************************************************/

/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

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
// for some fucked up reason, if we don't enable debug print, lots of bad packets are received by widgetRcvr.  dafuque.
#define ENABLE_DEBUG_PRINT

#define NUM_SENSORS 5

#define TRIGGER_PIN_0  A0  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_0     A1  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_0 45  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define TRIGGER_PIN_1  A2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_1     A3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_1 50  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define TRIGGER_PIN_2  A4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_2     A5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_2 50  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define TRIGGER_PIN_3  2   // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_3     3   // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_3 50  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define TRIGGER_PIN_4  4   // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_4     5   // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE_4 50  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

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

NewPing* sonar[NUM_SENSORS];
//NewPing sonar[NUM_SENSORS] = {
//  NewPing(TRIGGER_PIN_0, ECHO_PIN_0, MAX_DISTANCE_0),
//  NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE_1),
//  NewPing(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE_2),
//  NewPing(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE_3),
//  NewPing(TRIGGER_PIN_4, ECHO_PIN_4, MAX_DISTANCE_4)
//};

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

    distance[i] = sonar[i]->ping_cm();
    Serial.print(i);
    Serial.print(": ");
    Serial.println(distance[i]);
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

//  pinMode(A0, OUTPUT);
//  pinMode(A1, INPUT);
//  pinMode(A2, OUTPUT);
//  pinMode(A3, INPUT);
//  pinMode(A4, OUTPUT);
//  pinMode(A5, INPUT);

  sonar[0] = new NewPing(TRIGGER_PIN_0, ECHO_PIN_0, MAX_DISTANCE_0);
  sonar[1] = new NewPing(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE_1);
  sonar[2] = new NewPing(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE_2);
  sonar[3] = new NewPing(TRIGGER_PIN_3, ECHO_PIN_3, MAX_DISTANCE_3);
  sonar[4] = new NewPing(TRIGGER_PIN_4, ECHO_PIN_4, MAX_DISTANCE_4);

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

