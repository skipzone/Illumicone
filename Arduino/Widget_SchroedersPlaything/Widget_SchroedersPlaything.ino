/*****************************************************************
 *                                                               *
 * Schroeder's Plaything Widget                                  *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, June 2017                                 )'( *
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


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 5

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "1wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
//
// This widget wants acks because it is important to get the complete pairs of
// key-down and key-up messages.  Also, the pattern doesn't provide the needed
// stimulation feedback when we miss notes that have been played.
#define WANT_ACK true
#define TX_RETRY_DELAY_MULTIPLIER 2
#define TX_MAX_RETRIES 15

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


/******************
 * Implementation *
 ******************/

void setup()
{
  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = true;
  payload.widgetHeader.channel = 0;

  Serial.begin(31250);    
}


void loop() {

  static uint8_t midiMessage[3];
  static uint8_t midiByteCount;
  static bool gotStatusByte;
  
  uint8_t inByte;
  if (Serial.available()) {
    inByte = Serial.read();
    // If this is a status byte, reset the midiMessage buffer.
    if (inByte & 0x80) {
      gotStatusByte = true;
      midiByteCount = 0;
    }
    // MIDI messages that we care about are always 3 bytes long.
    if (midiByteCount < 3) {
      midiMessage[midiByteCount] = inByte;
      ++midiByteCount;
    }

    // When we've received a complete MIDI message, relay it immediately.
    if (midiByteCount == 3 && gotStatusByte) {
      midiByteCount = 0;
      gotStatusByte = false;

      // position holds the MIDI status byte.  Velocity holds data1
      // in the high-order byte and data2 in the low-order byte.
      payload.position = midiMessage[0];
      payload.velocity = ((uint16_t) midiMessage[1] << 8) | midiMessage[2];

      radio.write(&payload, sizeof(payload), !WANT_ACK);
    }
  }

}

