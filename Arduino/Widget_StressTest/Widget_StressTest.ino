/*****************************************************************
 *                                                               *
 * nRF24 Stress Test Widget                                      *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, June 2016                                 )'( *
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

#define ENABLE_DEBUG_PRINT


#include "illumiconeWidget.h"

#ifdef ENABLE_DEBUG_PRINT
#include "printf.h"
#endif


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 0
#define TX_INTERVAL_MS 100L
#define LED_ON_MS 5L
#define STATS_PRINT_INTERVAL_MS 5000L
#define LED_RED_PIN 3
#define LED_GREEN_PIN 5
#define LED_BLUE_PIN 6

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "0wdgt"       // 0 for tx stress

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for
// TX_RETRY_DELAY_MULTIPLIER.  15 is the maximum value for TX_MAX_RETRIES.
#define WANT_ACK true
#define TX_RETRY_DELAY_MULTIPLIER 1
#define TX_MAX_RETRIES 0
//#define WANT_ACK false
//#define TX_RETRY_DELAY_MULTIPLIER 0
//#define TX_MAX_RETRIES 0

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

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

StressTestPayload payload;


/******************
 * Implementation *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef LED_BLUE_PIN
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(LED_BLUE_PIN, HIGH);
#endif
#ifdef LED_GREEN_PIN
  pinMode(LED_GREEN_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);
#endif
#ifdef LED_RED_PIN
  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, LOW);
#endif

  if (!configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                      TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                      RF_CHANNEL)) {
    // Nothing else to do except leave the blue LED on.
    while (true);
  }

#ifdef ENABLE_DEBUG_PRINT
  radio.printDetails();
#endif

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

#ifdef LED_BLUE_PIN
  digitalWrite(LED_BLUE_PIN, LOW);
#endif
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastLedOnMs;
  static int32_t lastStatsPrintMs;
  static int32_t lastStatsPrintPayloadNum;

  uint32_t now = millis();

  if (lastLedOnMs && now - lastLedOnMs >= LED_ON_MS) {
    lastLedOnMs = 0;
#ifdef LED_GREEN_PIN
    digitalWrite(LED_GREEN_PIN, LOW);
#endif
#ifdef LED_RED_PIN
    digitalWrite(LED_RED_PIN, LOW);
#endif
  }

  if (now - lastTxMs >= TX_INTERVAL_MS) {

    ++payload.payloadNum;
    payload.widgetHeader.channel = 0;
    payload.widgetHeader.isActive = true;

    uint32_t txStartUs = micros();
    bool txSuccessful = radio.write(&payload, (uint8_t) sizeof(payload), !WANT_ACK);
    uint32_t txEndUs = micros();

    payload.lastTxUs = txEndUs - txStartUs;

    if (!txSuccessful) {
#ifdef LED_RED_PIN
      digitalWrite(LED_RED_PIN, HIGH);
      lastLedOnMs = now;
#endif
      ++payload.numTxFailures;
      Serial.print(F("radio.write failed for "));
      Serial.println(payload.payloadNum);
    }
    else {
#ifdef LED_GREEN_PIN
      digitalWrite(LED_GREEN_PIN, HIGH);
      lastLedOnMs = now;
#endif
    }

    lastTxMs = now;
  }

#ifdef ENABLE_DEBUG_PRINT
  now = millis();
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
