/*****************************************************************
 *                                                               *
 * Ross's Black Box with Switches and Pots                       *
 *                                                               *
 * Platform:  Pololu A-Star                                      *
 *                                                               *
 * by Ross Butler, January 2020                              )'( *
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

constexpr uint8_t widgetId = 31;

constexpr uint32_t activeTxIntervalMs = 50L;
constexpr uint32_t inactiveTxIntervalMs = 2000L;  // should be a multiple of activeTxIntervalMs

constexpr uint8_t numSwitches = 8;
constexpr uint8_t switchPins[numSwitches] = {2, 4, 7, 8, A4, A3, A2, A1};
constexpr uint8_t numPots = 3;
constexpr uint8_t potPins[numPots] = {A7, A6, A5};
constexpr uint8_t buttonPin = A0;
constexpr uint8_t ledRedPin = 3;
constexpr uint8_t ledGreenPin = 5;
constexpr uint8_t ledBluePin = 6;


// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "2wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK true
#define TX_RETRY_DELAY_MULTIPLIER 15    // use 15 when getting acks
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
// Illumicone uses channel 97.  The Electric Garden theremin uses channel 80.
#define RF_CHANNEL 97

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_HIGH


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static MeasurementVectorPayload payload;

static bool isActive;
static bool wasActive;


/******************
 * Implementation *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

  for (uint8_t i = 0; i < numSwitches; ++i) {
    pinMode(switchPins[i], INPUT);
    digitalWrite(switchPins[i], HIGH);
  }

  for (uint8_t i = 0; i < numPots; ++i) {
    pinMode(potPins[i], INPUT);
  }

  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);

  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledBluePin, OUTPUT);
  analogWrite(ledRedPin, 128);
  analogWrite(ledGreenPin, 0);
  analogWrite(ledBluePin, 0);

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);

  payload.widgetHeader.id = widgetId;
  payload.widgetHeader.channel = 0;
}


void sendMeasurements()
{
  uint16_t switchValue = 0;
  for (uint8_t i = 0; i < numSwitches; ++i) {
    switchValue |= ((digitalRead(switchPins[i]) == LOW) << (numSwitches - 1 - i));
  }
  payload.measurements[0] = (int16_t) switchValue;

  for (uint8_t i = 0; i < numPots; ++i) {
    payload.measurements[i + 1] = analogRead(potPins[i]);
  }

  payload.widgetHeader.isActive = isActive;

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * (numPots + 1), !WANT_ACK)) {
    analogWrite(ledRedPin, 64);
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
    analogWrite(ledRedPin, 0);
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }
}


void loop()
{
  static int32_t lastTxMs;

  uint32_t now = millis();

  if (digitalRead(buttonPin)) {
    isActive = false;
    analogWrite(ledGreenPin, 0);
    analogWrite(ledBluePin, 64);
  }
  else {
    isActive = true;
    analogWrite(ledGreenPin, 64);
    analogWrite(ledBluePin, 0);
  }

  if (now - lastTxMs >= activeTxIntervalMs) {
    if (isActive || wasActive || now - lastTxMs >= inactiveTxIntervalMs) {
      lastTxMs = now;
      sendMeasurements();
      wasActive = isActive;
    }
  }
}
