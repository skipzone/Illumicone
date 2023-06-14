/*****************************************************************
 *                                                               *
 * Ray's Eye Widget                                              *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
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
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 1
#define TX_INTERVAL_MS 250L
#define ILLUMINATION_OFF_THRESHOLD 500
#define ILLUMINATION_ON_THRESHOLD 400
#define PHOTOSENSOR_POWER_PIN 2
#define PHOTOSENSOR_SIGNAL_PIN A2
#define ILLUMINATION_LED_PIN 3

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "1wdgt"

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
#define RF_CHANNEL 97

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_LOW


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

PositionVelocityPayload payload;


/******************
 * Implementation *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(57600);
  printf_begin();
#endif

  pinMode(PHOTOSENSOR_POWER_PIN, OUTPUT);
  digitalWrite(PHOTOSENSOR_POWER_PIN , LOW);

  pinMode(ILLUMINATION_LED_PIN, OUTPUT);
  digitalWrite(ILLUMINATION_LED_PIN, HIGH);

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = true;
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;
  static bool illuminationIsOff;

  uint32_t now = millis();
  if (now - lastTxMs >= TX_INTERVAL_MS) {

    digitalWrite(PHOTOSENSOR_POWER_PIN, HIGH); 
    delay(1);
    unsigned int photosensorValue = analogRead(PHOTOSENSOR_SIGNAL_PIN);
    //digitalWrite(PHOTOSENSOR_POWER_PIN, LOW); 

#ifdef ENABLE_DEBUG_PRINT
    Serial.print("Sending ");
    Serial.println(photosensorValue);
#endif

    if (!illuminationIsOff && photosensorValue > ILLUMINATION_OFF_THRESHOLD) {
      digitalWrite(ILLUMINATION_LED_PIN, LOW);
      illuminationIsOff = true;
    }
    else if (illuminationIsOff && photosensorValue < ILLUMINATION_ON_THRESHOLD) {
      digitalWrite(ILLUMINATION_LED_PIN, HIGH);
      illuminationIsOff = false;
    }

    payload.position = photosensorValue;
    payload.velocity = 0;

//#ifdef ENABLE_DEBUG_PRINT
//    Serial.println(F("Calling radio.write."));
//#endif
    if (!radio.write(&payload, sizeof(payload), !WANT_ACK)) {
        radio.write(&payload, sizeof(payload));
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("radio.write failed."));
#endif
    }
    else {
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("radio.write succeeded."));
#endif
    }
    lastTxMs = now;
  }

}
