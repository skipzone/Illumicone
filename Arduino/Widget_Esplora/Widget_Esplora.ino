/*****************************************************************
 *                                                               *
 * Arduino Esplora Widget                                        *
 *                                                               *
 * Platform:  Arduino Esplora                                    *
 *                                                               *
 * by Ross Butler, February 2018                             )'( *
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

#include <Esplora.h>

#include "illumiconeWidget.h"

#ifdef ENABLE_DEBUG_PRINT
#include "printf.h"
#endif


/************************
 * Widget Configuration *
 ************************/

//#define WIDGET_RADIO_TESTER
#define FOURPLAY_4_2

#if defined(WIDGET_RADIO_TESTER)
  #define WIDGET_ID 11  // FourPlay-4-3
  #define NUM_CHANNELS 4
  #define TX_INTERVAL_MS 100L
  #define STATS_PRINT_INTERVAL_MS 1000L
  //#define LED_PIN 2
  #define TX_PIPE_ADDRESS "0wdgt"       // 0 for tx stress
  #define TX_RETRY_DELAY_MULTIPLIER 7
  #define PAYLOAD_TYPE StressTestPayload
#elif defined(FOURPLAY_4_2)
  #define WIDGET_ID 10
  #define NUM_ENCODERS 4
  #define VELOCITY_DIVISOR 2
  #define SAMPLE_INTERVAL_MS 5
  #define ACTIVE_TX_INTERVAL_MS 10L
  #define INACTIVE_TX_INTERVAL_MS 1000L
  #define SPIN_ACTIVITY_DETECT_MS 50
  #define SPIN_INACTIVITY_TIMEOUT_MS 500
  #define NUM_STEPS_PER_REV 20
  #define TX_PIPE_ADDRESS "1wdgt"
  #define TX_RETRY_DELAY_MULTIPLIER 3
  #define PAYLOAD_TYPE PositionVelocityPayload
#else
  #error Unspecified or unrecognized widget simulation.
#endif


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
// defined in widget configuration

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
// defined in widget configuration

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(3, 11);    // CE on pin 3, CSN on pin 11, also uses SPI bus (SCK on 15, MISO on 14, MOSI on 16)

PAYLOAD_TYPE payload;

static bool isActive;
static bool wasActive;

static uint16_t centerJoystickX;
static uint16_t centerJoystickY;


/******************
 * Implementation *
 ******************/

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

  centerJoystickX = Esplora.readJoystickX();
  centerJoystickY = Esplora.readJoystickY();
}


#if defined(WIDGET_RADIO_TESTER)
void doWidgetTester(uint32_t now)
{
  static int32_t lastTxMs;
  static int32_t lastStatsPrintMs;
  static int32_t lastStatsPrintPayloadNum;

  if (now - lastTxMs >= TX_INTERVAL_MS) {
    lastTxMs = now;

    ++payload.payloadNum;
    ++payload.widgetHeader.channel;
    if (payload.widgetHeader.channel >= NUM_CHANNELS) {
      payload.widgetHeader.channel = 0;
    }
    //payload.widgetHeader.isActive = !payload.widgetHeader.isActive;
    payload.widgetHeader.isActive = true;

    //  Send the time.  This will block until complete.
    if (!radio.write(&payload, sizeof(payload))) {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, HIGH);
#endif
      ++payload.numTxFailures;
      //Serial.print(F("radio.write failed for "));
      //Serial.println(payload.payloadNum);
    }
    else {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, LOW);
#endif
    }
  }

#ifdef ENABLE_DEBUG_PRINT
  uint32_t statsIntervalMs = now - lastStatsPrintMs;
  if (statsIntervalMs >= STATS_PRINT_INTERVAL_MS) {
    uint32_t sendRate = (payload.payloadNum - lastStatsPrintPayloadNum) * 1000L / statsIntervalMs;
    uint32_t pctFail = payload.numTxFailures * 100L / payload.payloadNum;
    Serial.print(F("---------- "));
    Serial.print(payload.payloadNum);
    Serial.print(F(" sent, "));
    Serial.print(payload.numTxFailures);
    Serial.print(F(" failures ("));
    Serial.print(pctFail);
    Serial.print(F("%), "));
    Serial.print(sendRate);
    Serial.print(F("/s over last "));
    Serial.print(statsIntervalMs);
    Serial.println(F(" ms"));
    lastStatsPrintMs = now;
    lastStatsPrintPayloadNum = payload.payloadNum;
  }
#endif

}
#endif


#if defined(FOURPLAY_4_2)
void doFourPlay4x(uint32_t now)
{
//  static int32_t lastEncoderValues[NUM_ENCODERS];
//  static uint32_t lastEncoderInactiveMs[NUM_ENCODERS];
//  static uint32_t lastEncoderChangeMs[NUM_ENCODERS];

  static int32_t rawVelocitySum[NUM_ENCODERS];
  static uint8_t numVelocitySamples;
  static int32_t milliSteps[NUM_ENCODERS];
  static int16_t position[NUM_ENCODERS];
  static int32_t lastMeasurementMs;
  static int32_t lastTxMs;

  if (now - lastMeasurementMs >= SAMPLE_INTERVAL_MS) {
    lastMeasurementMs = now;
    for (uint8_t wheelIdx = 0; wheelIdx < NUM_ENCODERS; ++wheelIdx) {
      int16_t rawVelocity;
      bool buttonPressed = Esplora.readButton(SWITCH_2) == LOW;
      switch (wheelIdx) {
        case 0:
          rawVelocity = !buttonPressed ? Esplora.readJoystickX() - centerJoystickX : 0;
          break;
        case 1:
          rawVelocity = !buttonPressed ? Esplora.readJoystickY() - centerJoystickY : 0;
          break;
        case 2:
          rawVelocity = buttonPressed ? Esplora.readJoystickX() - centerJoystickX : 0;
          break;
        case 3:
          rawVelocity = buttonPressed ? Esplora.readJoystickY() - centerJoystickY : 0;
          break;
      }
      rawVelocitySum[wheelIdx] += rawVelocity;
//#ifdef ENABLE_DEBUG_PRINT
//      Serial.print(wheelIdx);
//      Serial.print(F(" sample:  "));
//      Serial.print(rawVelocity);
//      Serial.print(F(" "));
//      Serial.println(rawVelocitySum[wheelIdx]);
//#endif
    }
    ++numVelocitySamples;
  }

  if (numVelocitySamples > 0 && now - lastTxMs >= ACTIVE_TX_INTERVAL_MS) {

    bool anyWheelsActive = false;
    for (uint8_t wheelIdx = 0; wheelIdx < NUM_ENCODERS; ++wheelIdx) {
      if (rawVelocitySum[wheelIdx] != 0) {
        anyWheelsActive = true;
        break;
      }
    }
    
    if (anyWheelsActive || wasActive || now - lastTxMs >= INACTIVE_TX_INTERVAL_MS) {
      lastTxMs = now;

      for (uint8_t wheelIdx = 0; wheelIdx < NUM_ENCODERS; ++wheelIdx) {

        int32_t avgRawVelocity = rawVelocitySum[wheelIdx] / numVelocitySamples;
        rawVelocitySum[wheelIdx] = 0;
        int32_t rpm = avgRawVelocity / VELOCITY_DIVISOR;

        int32_t measurementIntervalMs = numVelocitySamples * SAMPLE_INTERVAL_MS;
        milliSteps[wheelIdx] += (int32_t) NUM_STEPS_PER_REV * rpm * measurementIntervalMs / 60L;
//#ifdef ENABLE_DEBUG_PRINT
//      Serial.print(wheelIdx);
//      Serial.print(F(" measmt:  measurementIntervalMs="));
//      Serial.print(measurementIntervalMs);
//      Serial.print(F(" milliSteps[wheelIdx]="));
//      Serial.println(milliSteps[wheelIdx]);
//#endif

        if (abs(milliSteps[wheelIdx]) >= 1000) {
          int32_t numSteps = milliSteps[wheelIdx] / 1000L;
          milliSteps[wheelIdx] -= numSteps * 1000L;
          position[wheelIdx] += numSteps;
//#ifdef ENABLE_DEBUG_PRINT
//      Serial.print(F(" numSteps="));
//      Serial.print(numSteps);
//      Serial.print(F(" milliSteps[wheelIdx]="));
//      Serial.print(milliSteps[wheelIdx]);
//      Serial.print(F(" position[wheelIdx]="));
//      Serial.print(position[wheelIdx]);
//#endif
        }

        payload.widgetHeader.channel = wheelIdx;
        payload.widgetHeader.isActive = rpm != 0;
        payload.position = position[wheelIdx];
        payload.velocity = rpm;
    
        if (!radio.write(&payload, sizeof(payload))) {
#ifdef TX_FAILURE_LED_PIN      
          digitalWrite(TX_FAILURE_LED_PIN, HIGH);
#endif
        }
        else {
#ifdef TX_FAILURE_LED_PIN
          digitalWrite(TX_FAILURE_LED_PIN, LOW);
#endif
        }
      }
      numVelocitySamples = 0;
      wasActive = anyWheelsActive;
    }
  }
}
#endif


void loop()
{
  uint32_t now = millis();

#if defined(WIDGET_RADIO_TESTER)
  doWidgetTester(now);
#elif defined(FOURPLAY_4_2)
  doFourPlay4x(now);
#endif
}

