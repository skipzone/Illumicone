/*****************************************************************
 *                                                               *
 * Naked's and Monty's Pump Widget                               *
 * (based on design of Kayla's Plunger Widget)                   *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, August 2016                               )'( *
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


#include "illumiconeWidget.h"

#ifdef ENABLE_DEBUG_PRINT
#include "printf.h"
#endif


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 6
#define NUM_CHANNELS 1
#define ACTIVE_TX_INTERVAL_MS 250L
#define INACTIVE_TX_INTERVAL_MS 60000L      // should be a multiple of ACTIVE_TX_INTERVAL_MS
#define PRESSURE_SAMPLE_INTERVAL_MS 10L
#define PRESSURE_SENSOR_SIGNAL_PIN A3

constexpr uint16_t activePressureThreshold = 120;
constexpr uint8_t numInactiveSendTries = 5; // when going inactive, transmit that fact this many times at the active rate

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "1wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK false
#define TX_RETRY_DELAY_MULTIPLIER 0     // use 13 when getting acks
#define TX_MAX_RETRIES 0                // use 15 when getting acks

// Possible data rates are RF24_250KBPS, RF24_1MBPS, or RF24_2MBPS.  (2 Mbps
// works with genuine Nordic Semiconductor chips only, not the counterfeits.)
#define DATA_RATE RF24_1MBPS

// Valid CRC length values are RF24_CRC_8, RF24_CRC_16, and RF24_CRC_DISABLED
#define CRC_LENGTH RF24_CRC_16

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
#define RF_CHANNEL 84

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

PositionVelocityPayload payload;

static bool isActive;
static uint8_t wasActiveCountdown;


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
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastSampleMs;
  static uint16_t numSamples;
  static int32_t pressureMeasmtSum;

  uint32_t now = millis();

  if (now - lastSampleMs >= PRESSURE_SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    // If we're inactive, don't average because we want to react as fast as
    // possible to pumping.  Hopefully, noise won't poke above the threshold.
    if (!isActive) {
      numSamples = 1;
      pressureMeasmtSum = analogRead(PRESSURE_SENSOR_SIGNAL_PIN);
    }
    else {
      ++numSamples;
      pressureMeasmtSum += analogRead(PRESSURE_SENSOR_SIGNAL_PIN);
    }
  }

  if (numSamples > 0 && now - lastTxMs >= ACTIVE_TX_INTERVAL_MS) {
    int16_t avgPressure = pressureMeasmtSum / numSamples;
    isActive = avgPressure > activePressureThreshold;
    if (isActive || wasActiveCountdown > 0 || now - lastTxMs >= INACTIVE_TX_INTERVAL_MS) {
      lastTxMs = now;

      payload.position = avgPressure;
      payload.velocity = numSamples;
      payload.widgetHeader.isActive = isActive;
      radio.write(&payload, sizeof(payload), !WANT_ACK);

      numSamples = 0;
      pressureMeasmtSum = 0L;

      if (isActive) {
        wasActiveCountdown = numInactiveSendTries;
      }
      else {
        if (wasActiveCountdown > 0) {
          --wasActiveCountdown;
        }
      }
    }
  }

}

