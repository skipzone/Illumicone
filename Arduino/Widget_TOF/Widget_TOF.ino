/*****************************************************************
 *                                                               *
 * VL53L1X Time-of-Flight Sensor Widget                          *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, November 2019                             )'( *
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

#include <Wire.h>
#include <VL53L1X.h>


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 27
#define NUM_CHANNELS 1
#define ACTIVE_TX_INTERVAL_MS 50L
#define INACTIVE_TX_INTERVAL_MS 1000L      // should be a multiple of ACTIVE_TX_INTERVAL_MS
#define SAMPLE_INTERVAL_MS 100L

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
#define RF_CHANNEL 97

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

VL53L1X sensor;

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

PositionVelocityPayload payload;

static bool isActive;
static uint8_t wasActiveCountdown;
static uint16_t currentDistanceMm;


/******************
 * Implementation *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.channel = 0;

  sensor.setTimeout(500);
  if (!sensor.init())
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println("Failed to detect and initialize sensor.");
#endif
    while (1);
  }

  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
  sensor.startContinuous(50);
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastSampleMs;

  uint32_t now = millis();

  if (now - lastTxMs >= ACTIVE_TX_INTERVAL_MS) {
    if (isActive || wasActiveCountdown > 0 || now - lastTxMs >= INACTIVE_TX_INTERVAL_MS) {
      lastTxMs = now;

      payload.position = currentDistanceMm;
      payload.velocity = 0;
      payload.widgetHeader.isActive = isActive;
      radio.write(&payload, sizeof(payload), !WANT_ACK);

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

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    sensor.read();
    if (sensor.ranging_data.range_status == VL53L1X::RangeValid) {
      isActive = true;
      currentDistanceMm = sensor.ranging_data.range_mm;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print("range: ");
      Serial.print(sensor.ranging_data.range_mm);
      Serial.print("\tstatus: ");
      Serial.print(VL53L1X::rangeStatusToString(sensor.ranging_data.range_status));
      Serial.print("\tpeak signal: ");
      Serial.print(sensor.ranging_data.peak_signal_count_rate_MCPS);
      Serial.print("\tambient: ");
      Serial.print(sensor.ranging_data.ambient_count_rate_MCPS);
      Serial.println();
#endif
    }
    else {
      isActive = false;
    }
  }

}
