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
#define FOURPLAY
//#define PUMP

#if defined(WIDGET_RADIO_TESTER)
  #define WIDGET_ID 9  // FourPlay-4-3
  #define NUM_CHANNELS 4
  #define TX_INTERVAL_MS 100L
  #define STATS_PRINT_INTERVAL_MS 1000L
  //#define LED_PIN 2
  #define TX_PIPE_ADDRESS "0wdgt"       // 0 for tx stress
  #define PAYLOAD_TYPE StressTestPayload

#elif defined(FOURPLAY)
  #define WIDGET_ID 8
  #define ALT_WIDGET_ID 9
  #define NUM_ENCODERS 4
  #define VELOCITY_DIVISOR 4
  #define ACTIVITY_THRESHOLD 2
  #define SAMPLE_INTERVAL_MS 5
  #define ACTIVE_TX_INTERVAL_MS 10L
  #define INACTIVE_TX_INTERVAL_MS 1000L
  #define SPIN_ACTIVITY_DETECT_MS 50
  #define SPIN_INACTIVITY_TIMEOUT_MS 500
  #define NUM_STEPS_PER_REV 20
  #define TX_PIPE_ADDRESS "1wdgt"
  #define PAYLOAD_TYPE PositionVelocityPayload

#elif defined(PUMP)
  #define WIDGET_ID 6
  #define TX_INTERVAL_MS 250L
  #define TX_PIPE_ADDRESS "1wdgt"
  #define PAYLOAD_TYPE PositionVelocityPayload

#else
  #error Unspecified or unrecognized widget simulation.
#endif

// ---------- radio configuration (common to all configurations above) ----------

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK true
#define TX_RETRY_DELAY_MULTIPLIER 2     // use 2 when getting acks
#define TX_MAX_RETRIES 15               // use 15 when getting acks

// Possible data rates are RF24_250KBPS, RF24_1MBPS, or RF24_2MBPS.  (2 Mbps
// works with genuine Nordic Semiconductor chips only, not the counterfeits.)
#define DATA_RATE RF24_250KBPS

// Valid CRC length values are RF24_CRC_8, RF24_CRC_16, and RF24_CRC_DISABLED
#define CRC_LENGTH RF24_CRC_16

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
#define RF_CHANNEL 97

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_LOW


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

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
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
    if (!radio.write(&payload, sizeof(payload), !WANT_ACK)) {
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


#if defined(FOURPLAY)
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
  static bool button1Pressed;
  static bool button2Pressed;
  static bool button3Pressed;
  static bool button4Pressed;

  if (now - lastMeasurementMs >= SAMPLE_INTERVAL_MS) {
    lastMeasurementMs = now;
    for (uint8_t wheelIdx = 0; wheelIdx < NUM_ENCODERS; ++wheelIdx) {
      int16_t rawVelocity = 0;
      button1Pressed = Esplora.readButton(SWITCH_1) == LOW;
      button2Pressed = Esplora.readButton(SWITCH_2) == LOW;
      button3Pressed = Esplora.readButton(SWITCH_3) == LOW;
      button4Pressed = Esplora.readButton(SWITCH_4) == LOW;
      switch (wheelIdx) {
        case 0:
          rawVelocity = button1Pressed || button2Pressed ? Esplora.readJoystickX() - centerJoystickX : 0;
          break;
        case 1:
          rawVelocity = button1Pressed || button2Pressed ? Esplora.readJoystickY() - centerJoystickY : 0;
          break;
        case 2:
          rawVelocity = button3Pressed || button4Pressed ? Esplora.readJoystickX() - centerJoystickX : 0;
          break;
        case 3:
          rawVelocity = button3Pressed || button4Pressed ? Esplora.readJoystickY() - centerJoystickY : 0;
          break;
      }
      if (abs(rawVelocity) <= ACTIVITY_THRESHOLD) {
        rawVelocity = 0;
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
        if (button1Pressed || button3Pressed) {
          payload.widgetHeader.id = WIDGET_ID;
          radio.write(&payload, sizeof(payload), !WANT_ACK);
        }
        if (button2Pressed || button4Pressed) {
          payload.widgetHeader.id = ALT_WIDGET_ID;
          radio.write(&payload, sizeof(payload), !WANT_ACK);
        }
      }

      numVelocitySamples = 0;
      wasActive = anyWheelsActive;
    }
  }
}
#endif


#if defined(PUMP)
void doPump(uint32_t now)
{
  static int32_t lastTxMs;

  if (now - lastTxMs >= TX_INTERVAL_MS) {
    lastTxMs = now;

    payload.widgetHeader.channel = 0;
    payload.widgetHeader.isActive = true;
    payload.position = 1023 - Esplora.readSlider();   // seems that the API author holds the board upside down
    payload.velocity = 1;
    
    radio.write(&payload, sizeof(payload), !WANT_ACK);
  }
}
#endif


void loop()
{
  uint32_t now = millis();

#if defined(WIDGET_RADIO_TESTER)
  doWidgetTester(now);
#elif defined(FOURPLAY)
  doFourPlay4x(now);
#elif defined(PUMP)
  doPump(now);
#endif
}
