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

#include "illumiconeWidget.h"
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 11  // FourPlay-4-3
#define NUM_CHANNELS 4
#define TX_INTERVAL_MS 100L
#define STATS_PRINT_INTERVAL_MS 1000L
//#define LED_PIN 2


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "0wdgt"       // 0 for tx stress
//#define TX_PIPE_ADDRESS "1wdgt"       // 0 for tx stress

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 3

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(3, 11);    // CE on pin 3, CSN on pin 11, also uses SPI bus (SCK on 15, MISO on 14, MOSI on 16)

StressTestPayload payload;


/******************
 * Implementation *
 ******************/

void setup()
{
  delay(5000);
  
  Serial.begin(57600);
  printf_begin();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastStatsPrintMs;
  static int32_t lastStatsPrintPayloadNum;

  uint32_t now = millis();
  if (now - lastTxMs >= TX_INTERVAL_MS) {

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
    
    lastTxMs = now;
  }

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

}

