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

#include "illumiconeWidget.h"
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 1
#define NUM_CHANNELS 1
#define TX_INTERVAL_MS 250L
#define PHOTOSENSOR_POWER_PIN 2
#define PHOTOSENSOR_SIGNAL_PIN A2
//#define TX_FAILURE_LED_PIN 8
//#define ENABLE_DEBUG_PRINT


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "1wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 15

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


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

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = true;
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;

  uint32_t now = millis();
  if (now - lastTxMs >= TX_INTERVAL_MS) {

    digitalWrite(PHOTOSENSOR_POWER_PIN, HIGH); 
    delay(1);
    unsigned int photosensorValue = analogRead(PHOTOSENSOR_SIGNAL_PIN);
    //digitalWrite(PHOTOSENSOR_POWER_PIN, LOW); 

    payload.position = photosensorValue;
    payload.velocity = 0;

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
    
    lastTxMs = now;
  }

}

