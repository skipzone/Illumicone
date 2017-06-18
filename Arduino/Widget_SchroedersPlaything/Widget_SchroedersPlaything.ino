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
#include <MIDI.h>
#include "printf.h"

MIDI_CREATE_DEFAULT_INSTANCE();



/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 4   // change back to 5
//#define TX_INTERVAL_MS 1000L
//#define TX_FAILURE_LED_PIN 8


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "2wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 11

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

MeasurementVectorPayload payload;

static bool gotNoteOn;
static byte noteOnChannel;
static byte noteOnPitch;
static byte noteOnVelocity;
static bool gotNoteOff;
static byte noteOffChannel;
static byte noteOffPitch;
static byte noteOffVelocity;
static bool gotPitchBend;
static byte pitchBendChannel;
static byte pitchBend;
static bool gotProgramChange;
static byte programChangeChannel;
static byte programChangeNumber;


/******************
 * Implementation *
 ******************/

// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// http://arduinomidilib.fortyseveneffects.com/a00022.html


void handleNoteOn(byte in_channel, byte in_pitch, byte in_velocity)
{
  noteOnChannel = in_channel;
  noteOnPitch = in_pitch;
  noteOnVelocity = in_velocity;
  gotNoteOn = true;
  digitalWrite(LED_BUILTIN, HIGH);
}


void handleNoteOff(byte in_channel, byte in_pitch, byte in_velocity)
{
  noteOffChannel = in_channel;
  noteOffPitch = in_pitch;
  noteOffVelocity = in_velocity;
  gotNoteOff = true;
  digitalWrite(LED_BUILTIN, LOW);
}


void handlePitchBend(byte in_channel, byte in_pitchBend)
{
  pitchBendChannel = in_channel;
  pitchBend = in_pitchBend;
  gotPitchBend = true;
  digitalWrite(LED_BUILTIN, LOW);
}


void handleProgramChange(byte in_channel, byte in_number)
{
  programChangeChannel = in_channel;
  programChangeNumber = in_number;
  gotProgramChange = true;
  digitalWrite(LED_BUILTIN, LOW);
}


void setup()
{

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = true;
  payload.widgetHeader.channel = 0;

  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleProgramChange(handleProgramChange);

  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {

  static int32_t lastTxMs;

  uint32_t now = millis();

  // Call MIDI.read the fastest you can for real-time performance.
  MIDI.read();

  if (gotNoteOn || gotNoteOff || gotPitchBend || gotProgramChange) {

    for (int i = 0; i < 15; ++i) {
      payload.measurements[i] = 0;
    }

    if (gotNoteOn) {
      gotNoteOn = false;
      payload.measurements[0] = 1;
      payload.measurements[1] = noteOnPitch;
      payload.measurements[2] = noteOnVelocity;
    }

    if (gotNoteOff) {
      gotNoteOff = false;
      payload.measurements[0] = 2;
      payload.measurements[1] = noteOffPitch;
      payload.measurements[2] = noteOffVelocity;
    }

    if (gotPitchBend) {
      gotPitchBend = false;
      payload.measurements[0] = 3;
      payload.measurements[1] = pitchBend;
      payload.measurements[2] = 0;
    }

    if (gotProgramChange) {
      gotProgramChange = false;
      payload.measurements[0] = 4;
      payload.measurements[1] = programChangeNumber;
      payload.measurements[2] = 0;
    }

    if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * 3)) {
#ifdef TX_FAILURE_LED_PIN
      digitalWrite(TX_FAILURE_LED_PIN, HIGH);
#endif
    }
    else {
#ifdef TX_FAILURE_LED_PIN
      digitalWrite(TX_FAILURE_LED_PIN, LOW);
#endif
    }
    
  }

//  if (now - lastTxMs >= TX_INTERVAL_MS) {
//    lastTxMs = now;
//    gotNoteOn = true;
//    noteOnPitch = 1;
//    noteOnVelocity = 2;
//  }

}

