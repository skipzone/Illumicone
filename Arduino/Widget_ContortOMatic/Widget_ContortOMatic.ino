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

//#define TX_FAILURE_LED_PIN 2

constexpr uint8_t widgetId = 9;

constexpr uint32_t activeTxIntervalMs = 333L;
constexpr uint32_t inactiveTxIntervalMs = 1000L;
constexpr uint32_t gatherMeasurementsIntervalMs = 167L;
constexpr uint32_t noChangeRecalibrationIntervalMs = 15L * 1000L;
constexpr uint32_t inactiveRecalibrationIntervalMs = 60L * 1000L;

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
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11, 13) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 15

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(7, 8);    // Mega:  CE on pin 7, CSN on pin 8, also uses SPI bus (SCK on 52, MISO on 50, MOSI on 51)

ContortOMaticTouchDataPayload touchDataPayload;
ContortOMaticCalibrationDataPayload calibrationDataPayload;

static int capSenseReferenceValues[numCapSensePins];
static bool padIsTouched[numCapSensePins];
static bool padWasTouched[numCapSensePins];

static bool isActive;
static bool wasActive;
static uint32_t nextGatherMeasurementsMs;
static uint32_t nextTxMs;
static uint32_t nextNoChangeRecalibrationMs;
static uint32_t nextInactiveRecalibrationMs;


/******************
 * Implementation *
 ******************/

void calibrateCapSense()
{
  Serial.println(F("Calibrating..."));
  
  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    // Create reference value to account for stray
    // capacitance and capacitance of the pad.
    capSenseReferenceValues[i] = ADCTouch.read(capSensePins[i], capSenseNumSamplesForRef);
  }

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("---------- Calibration Values ----------"));
  for (byte i = 0; i < numCapSensePins; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(capSenseReferenceValues[i]);
  }
#endif

  // Send the calibration data to widgetRcvr for logging.
  calibrationDataPayload.widgetHeader.isActive = isActive;
  uint8_t capSensePinNum = 0;
  uint8_t setNum = 0;
  uint8_t payloadValueIdx = 0;
  while (capSensePinNum < numCapSensePins) {
    calibrationDataPayload.capSenseReferenceValues[payloadValueIdx++] = capSenseReferenceValues[capSensePinNum++];
    if (payloadValueIdx == 8) {
      calibrationDataPayload.setNum = setNum;
      sendPayload(&calibrationDataPayload, sizeof(calibrationDataPayload));
      ++setNum;
      payloadValueIdx = 0; 
    }
  }
}


void gatherMeasurements()
{
  int capSenseValues[numCapSensePins];

  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    capSenseValues[i] = ADCTouch.read(capSensePins[i], capSenseNumSamples);
  }

  // Figure out which pads are being touched and if there have been any touch changes.
  isActive = false;
  bool touchedSetChanged = false;
  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    int netValue = capSenseValues[i] - capSenseReferenceValues[i];
    if (netValue > capSenseThresholds[i]) {
      padIsTouched[i] = true;
      isActive = true;
    }
    else {
      padIsTouched[i] = false;
    }
    if (padWasTouched[i] != padIsTouched[i]) {
      padWasTouched[i] = padIsTouched[i];
      touchedSetChanged = true;
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

  // Adjust the transmission and auto-recalibration times if necessary.
  uint32_t now = millis();
  if (isActive) {
    if (!wasActive) {
      // Just went active, so transmit ASAP.
      nextTxMs = now;
    }
    if (touchedSetChanged) {
      nextNoChangeRecalibrationMs = now + noChangeRecalibrationIntervalMs;
    }
  }
  else {
    if (wasActive) {
      nextInactiveRecalibrationMs = now + inactiveRecalibrationIntervalMs;
    }
  }
  wasActive = isActive;
}


void sendMeasurements()
{
  // Put the patIsTouched data into the payload's bitfield, with element 0 in the low-order bit.
  touchDataPayload.padIsTouchedBitfield = 0;
  for (int i = numCapSensePins - 1; i >= 0; --i) {
    touchDataPayload.padIsTouchedBitfield = (touchDataPayload.padIsTouchedBitfield << 1) | padIsTouched[i];
  }

  touchDataPayload.widgetHeader.isActive = isActive;

  sendPayload(&touchDataPayload, sizeof(touchDataPayload));
}


void sendPayload(const void* payload, uint8_t payloadLength)
{
  if (!radio.write(payload, payloadLength)) {
#ifdef TX_FAILURE_LED_PIN      
    digitalWrite(TX_FAILURE_LED_PIN, HIGH);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
#ifdef TX_FAILURE_LED_PIN      
    digitalWrite(TX_FAILURE_LED_PIN, LOW);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
#endif
  // RF24 uses printf, so we need to call printf_begin even if we're not doing debug prints.
  printf_begin();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  touchDataPayload.widgetHeader.id = widgetId;
  touchDataPayload.widgetHeader.channel = 0;              // channel 0 carries touch data payloads

  calibrationDataPayload.widgetHeader.id = widgetId;
  calibrationDataPayload.widgetHeader.channel = 1;        // channel 1 carries calibration data payloads

  uint32_t now = millis();
  nextGatherMeasurementsMs = now;
  nextTxMs = now;
  nextNoChangeRecalibrationMs = now + noChangeRecalibrationIntervalMs;
  nextInactiveRecalibrationMs = now + inactiveRecalibrationIntervalMs;

  calibrateCapSense();
}


void loop()
{
  uint32_t now = millis();

  if ((int32_t) (now - nextGatherMeasurementsMs) >= 0) {
    nextGatherMeasurementsMs = now + gatherMeasurementsIntervalMs;
    gatherMeasurements();
  }

  if ((int32_t) (now - nextTxMs) >= 0) {
    nextTxMs = now + (isActive ? activeTxIntervalMs : inactiveTxIntervalMs);
    sendMeasurements();
  }

  if ((isActive && (int32_t) (now - nextNoChangeRecalibrationMs) >= 0)
      || (!isActive && (int32_t) (now - nextInactiveRecalibrationMs) >= 0))
  {
    calibrateCapSense();
    nextNoChangeRecalibrationMs = now + noChangeRecalibrationIntervalMs;
    nextInactiveRecalibrationMs = now + inactiveRecalibrationIntervalMs;
  }
}

