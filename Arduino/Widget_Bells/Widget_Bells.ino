/*****************************************************************
 *                                                               *
 * Ray's Bells Widget                                            *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, August 2016, February 2018                )'( *
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

constexpr uint8_t widgetId = 3;

constexpr uint32_t activeTxIntervalMs = 200L;
constexpr uint32_t inactiveTxIntervalMs = 2000L;  // should be a multiple of activeTxIntervalMs
constexpr uint32_t soundSampleIntervalMs = 10;

constexpr uint8_t numAudioInputPins = 3;
constexpr uint8_t audioInputPins[numAudioInputPins] = {A1, A2, A3};
constexpr uint16_t bellActiveThreshold[numAudioInputPins] = {100, 200, 200};

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
#define TX_RETRY_DELAY_MULTIPLIER 0     // use 15 when getting acks
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

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static PositionVelocityPayload payload;

static uint16_t minSoundSample[numAudioInputPins];
static uint16_t maxSoundSample[numAudioInputPins];
static uint16_t ppSoundSample[numAudioInputPins];
static bool bellIsActive[numAudioInputPins];
static bool bellWasActive[numAudioInputPins];


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

  payload.widgetHeader.id = widgetId;

  for (uint8_t i = 0; i < numAudioInputPins; ++i) {
    minSoundSample[i] = UINT16_MAX;
    maxSoundSample[i] = 0;
  }
}


void gatherSoundMeasurements()
{
  for (uint8_t inputIdx = 0; inputIdx < numAudioInputPins; ++inputIdx) {
    uint16_t soundSample = analogRead(audioInputPins[inputIdx]);
    if (soundSample < minSoundSample[inputIdx]) {
      minSoundSample[inputIdx] = soundSample;
    }
    if (soundSample > maxSoundSample[inputIdx]) {
      maxSoundSample[inputIdx] = soundSample;
    }
  }
}


uint8_t calculatePpSoundSamples()
{
  // Returns true if any bell's p-p value exceeds its active threshold.

  bool anyBellIsActive = false;

  for (uint8_t inputIdx = 0; inputIdx < numAudioInputPins; ++inputIdx) {

    ppSoundSample[inputIdx] = maxSoundSample[inputIdx] - minSoundSample[inputIdx];
    minSoundSample[inputIdx] = UINT16_MAX;
    maxSoundSample[inputIdx] = 0;

    if (ppSoundSample[inputIdx] >= bellActiveThreshold[inputIdx]) {
      bellIsActive[inputIdx] = true;
      anyBellIsActive = true;
    }
    else {
      bellIsActive[inputIdx] = false;
    }

  }

  return anyBellIsActive;
}


void sendMeasurements(bool sendForActiveBells)
{
  for (uint8_t inputIdx = 0; inputIdx < numAudioInputPins; ++inputIdx) {
    if (bellIsActive[inputIdx] == sendForActiveBells || bellWasActive[inputIdx]) {

      // Remember if the bell is active so that we can send the first inactive
      // message as soon as possible (i.e., not wait for the next inactive
      // bell transmission).
      bellWasActive[inputIdx] = bellIsActive[inputIdx];

      payload.widgetHeader.channel = inputIdx;
      payload.widgetHeader.isActive = bellIsActive[inputIdx];
      payload.position = ppSoundSample[inputIdx];
      payload.velocity = 0;

      radio.write(&payload, sizeof(payload));
    }
  }
}


void loop()
{
  static int32_t lastActiveTxMs;
  static int32_t lastInactiveTxMs;
  static int32_t lastSoundSampleMs;

  uint32_t now = millis();

  if (now - lastSoundSampleMs >= soundSampleIntervalMs) {
    lastSoundSampleMs = now;
    gatherSoundMeasurements();
  }

  if (now - lastActiveTxMs >= activeTxIntervalMs) {
    lastActiveTxMs = now;

    bool anyBellIsActive = calculatePpSoundSamples();

    // When it is time to send messages for inactive bells, we send the messages
    // for them before the active bells so that we don't double-send for a bell
    // that has just gone inactive.
    if (now - lastInactiveTxMs >= inactiveTxIntervalMs) {
      lastInactiveTxMs = now;
      if (!anyBellIsActive) {
        sendMeasurements(false);
      }
    }

    if (anyBellIsActive) {
      sendMeasurements(true);
    }
  }

}

