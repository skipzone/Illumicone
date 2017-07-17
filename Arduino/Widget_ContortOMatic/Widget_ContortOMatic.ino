/*****************************************************************
 *                                                               *
 * Contort-O-Matic Widget                                        *
 *                                                               *
 * Platform:  Arduino Mega 2560                                  *
 *                                                               *
 * by Ross Butler, June 2017                                 )'( *
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

//#define ENABLE_DEBUG_PRINT

#include <ADCTouch.h>
#include "illumiconeWidget.h"
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 9
#define ACTIVE_TX_INTERVAL_MS 250L
#define INACTIVE_TX_INTERVAL_MS 1000L
//#define TX_FAILURE_LED_PIN 2

constexpr uint32_t gatherMeasurementsIntervalMs = 500;

constexpr uint8_t numCapSensePins = 16;
constexpr uint8_t capSensePins[numCapSensePins] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15};
constexpr int capSenseNumSamplesForRef = 500;
constexpr int capSenseNumSamples = 10;
constexpr int capSenseThresholds[numCapSensePins] = { 20,  20,  20,  20,
                                                      20,  20,  20,  20,
                                                      20,  20,  20,  20,
                                                      20,  20,  20,  20};


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "5wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 5

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(7, 8);    // Mega:  CE on pin 7, CSN on pin 8, also uses SPI bus (SCK on 52, MISO on 50, MOSI on 51)

CustomPayload payload;

static int capSenseReferenceValues[numCapSensePins];
static bool padIsTouched[numCapSensePins];
bool isActive;


/******************
 * Implementation *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

  calibrateCapSense();
}


void calibrateCapSense()
{
  Serial.println(F("Calibrating..."));
  
  for (byte i = 0; i < numCapSensePins; ++i) {
    // Create reference value to account for stray
    // capacitance and capacitance of the pad.
    capSenseReferenceValues[i] = ADCTouch.read(capSensePins[i], capSenseNumSamplesForRef);
  }

  Serial.println(F("---------- Calibration Values ----------"));
  for (byte i = 0; i < numCapSensePins; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(capSenseReferenceValues[i]);
  }
}


void gatherMeasurements()
{
  int capSenseValues[numCapSensePins];
  
  isActive = false;

  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    capSenseValues[i] = ADCTouch.read(capSensePins[i], capSenseNumSamples);
  }

  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    int netValue = capSenseValues[i] - capSenseReferenceValues[i];
    if (netValue > capSenseThresholds[i]) {
      padIsTouched[i] = true;
      isActive = true;
    }
    else {
      padIsTouched[i] = false;
    }
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(i);
    Serial.print(":  ");
    Serial.print(capSenseValues[i]);
    Serial.print(" - ");
    Serial.print(capSenseReferenceValues[i]);
    Serial.print(" = ");
    Serial.print(netValue);
    Serial.print(" >? ");
    Serial.print(capSenseThresholds[i]);
    Serial.print(" -> ");
    Serial.println(padIsTouched[i]);
#endif
  }
}


void sendMeasurements()
{
  // TODO 6/22/2017 ross:  Send the pad-is-touched data as a bit field instead of one byte per pad.
  // First byte of payload is the nubmer of cap sense pins.  The remainder
  // of the bits are 1 if the corresponding pad is touched, 0 if not.
  payload.buf[0] = numCapSensePins;
  for (int i = 0; i < numCapSensePins; ++i) {
      payload.buf[i + 1] = padIsTouched[i];
  }

  payload.widgetHeader.isActive = isActive;

  if (!radio.write(&payload, sizeof(WidgetHeader) + numCapSensePins + 1)) {
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


void loop() {

  static int32_t lastTxMs;
  static uint32_t lastGatherMeasurementsMs;

  uint32_t now = millis();

  if ((int32_t) (now - lastGatherMeasurementsMs) >= 0) {
    lastGatherMeasurementsMs += gatherMeasurementsIntervalMs;
    gatherMeasurements();
  }

  if (now - lastTxMs >= (isActive ? ACTIVE_TX_INTERVAL_MS : INACTIVE_TX_INTERVAL_MS)) {
    sendMeasurements();
    lastTxMs = now;
  }

}

