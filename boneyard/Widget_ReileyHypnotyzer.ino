/*****************************************************************
 *                                                               *
 * Reiley's Hypnotyzer Widget                                    *
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

#define WIDGET_ID 2
#define NUM_CHANNELS 1
#define ACTIVE_TX_INTERVAL_MS 200L
#define INACTIVE_TX_INTERVAL_MS 1000L
//#define TX_FAILURE_LED_PIN 2
#define ENCODER_0_A_PIN 2
#define ENCODER_0_B_PIN 3
//#define ENCODERS_VDD_PIN 8
//#define ENCODER_ACTIVE_PIN 8

#define SPIN_ACTIVITY_DETECT_MS 50
#define SPIN_INACTIVITY_TIMEOUT_MS 500

#define NUM_ENCODERS 1
#define NUM_STEPS_PER_REV 18

#define ENABLE_DEBUG_PRINT


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "1wdgt"

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

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

PositionVelocityPayload payload;

bool g_anyEncoderActive;
bool g_encoderActive[NUM_ENCODERS];
volatile uint8_t g_lastEncoderStates = 0;
volatile int g_encoderValues[] = {0};
uint32_t g_encoderRpms[NUM_ENCODERS];

const int8_t g_greyCodeToEncoderStepMap[] = {
       // this is not really a Grey code
       // last this
   0,  //  00   00
   1,  //  00   01
   1,  //  00   10
   1,  //  00   11
   1,  //  01   00
   0,  //  01   01
   1,  //  01   10
   1,  //  01   11
   1,  //  10   00
   1,  //  10   01
   0,  //  10   10
   1,  //  10   11
   1,  //  11   00
   1,  //  11   01
   1,  //  11   10
   0,  //  11   11
};


/******************
 * Implementation *
 ******************/

void setUpPinChangeInterrupt(uint8_t pin) 
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group 
}


// Service pin change interrupt for A0 - A5.
//ISR (PCINT1_vect)
// Service pin change interrupt for D0 - D7.
ISR (PCINT2_vect)
{
//  uint8_t encoderStates = PINC;
  uint8_t encoderStates = PIND >> 2;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastEncoderStates;

  for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
    uint8_t idx = ((lastStates & 0b11) << 2) | (curStates & 0b11);
    g_encoderValues[i] += g_greyCodeToEncoderStepMap[idx];
    curStates >>= 2;
    lastStates >>= 2;
  }

  g_lastEncoderStates = encoderStates;
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(57600);
  printf_begin();
#endif

  pinMode(ENCODER_0_A_PIN, INPUT); 
  pinMode(ENCODER_0_B_PIN, INPUT);
#ifdef ENCODERS_VDD_PIN
  pinMode(ENCODERS_VDD_PIN, OUTPUT);
#endif
#ifdef ENCODER_ACTIVE_PIN
  pinMode(ENCODER_ACTIVE_PIN, OUTPUT);
#endif

  // Turn on pullups.
  digitalWrite(ENCODER_0_A_PIN, HIGH);
  digitalWrite(ENCODER_0_B_PIN, HIGH);
  
  // Initially, turn on power to the encoders and set the active indicator low.
#ifdef ENCODERS_VDD_PIN
  digitalWrite(ENCODERS_VDD_PIN, HIGH);
#endif
#ifdef ENCODER_ACTIVE_PIN
  digitalWrite(ENCODER_ACTIVE_PIN, LOW);
#endif

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;

  g_anyEncoderActive = false;
  for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
    g_encoderActive[i] = false;
  }

  // Set up and turn on the pin-change interrupts last.
  setUpPinChangeInterrupt(ENCODER_0_A_PIN);
  setUpPinChangeInterrupt(ENCODER_0_B_PIN);
}


void gatherMeasurements()
{
  static int32_t lastEncoderValues[NUM_ENCODERS] = {0};
  static uint32_t lastEncoderInactiveMs[NUM_ENCODERS] = {0};
  static uint32_t lastEncoderChangeMs[NUM_ENCODERS] = {0};

  unsigned long now = millis();

  for (int i = 0; i < NUM_ENCODERS; ++i) {
    bool encoderChanged = false;
    int thisEncoderValue = g_encoderValues[i];
    if (thisEncoderValue != lastEncoderValues[i]) {
      encoderChanged = true;

      uint32_t actualIntervalMs = lastEncoderChangeMs[i] != 0 ? now - lastEncoderChangeMs[i] : 0;
      uint32_t delta = thisEncoderValue - lastEncoderValues[i];

      int32_t encoderStepsPerSecond = 0;
      int32_t encoderRpm = 0;
      if (delta != 0 && actualIntervalMs != 0) {
        encoderStepsPerSecond = delta * 1000L / actualIntervalMs;
        g_encoderRpms[i] = encoderStepsPerSecond * 60L / NUM_STEPS_PER_REV;
      }

      lastEncoderValues[i] = thisEncoderValue;
      lastEncoderChangeMs[i] = now;
#ifdef ENABLE_DEBUG_PRINT
      for (int i = 0; i < NUM_ENCODERS; ++i) {
        Serial.print(i);
        Serial.print(",");
        Serial.print(g_encoderActive[i]);
        Serial.print(",");
        Serial.print(g_encoderValues[i]);
        Serial.print(",");
        Serial.println(g_encoderRpms[i]);
      }
#endif
    }

    if (!g_encoderActive[i]) {
      if (!encoderChanged && now - lastEncoderChangeMs[i] > SPIN_INACTIVITY_TIMEOUT_MS) {
        lastEncoderInactiveMs[i] = now;
      }
      else {
        if (now - lastEncoderInactiveMs[i] > SPIN_ACTIVITY_DETECT_MS) {
          g_encoderActive[i] = true;
          for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
            lastEncoderValues[i] = g_encoderValues[i];
          }
        }
      }
    }
    else {
      if (!encoderChanged) {
        if (now - lastEncoderChangeMs[i] > SPIN_INACTIVITY_TIMEOUT_MS) {
          g_encoderActive[i] = false;
          lastEncoderChangeMs[i] = 0;
        }
      }
    }
  }

  g_anyEncoderActive = false;
  for (int i = 0; i < NUM_ENCODERS; g_anyEncoderActive |= g_encoderActive[i++]);
#ifdef ENCODER_ACTIVE_PIN
  digitalWrite(ENCODER_ACTIVE_PIN, g_anyEncoderActive);
#endif
    
}


void sendMeasurements()
{
  for (int i = 0; i < NUM_ENCODERS; ++i) {
    
    payload.widgetHeader.channel = i;
    payload.widgetHeader.isActive = g_encoderActive[i];
    payload.position = g_encoderValues[i];
    payload.velocity = g_encoderRpms[i];
    
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
  }
}


void loop() {

  static int32_t lastTxMs;

  gatherMeasurements();

  uint32_t now = millis();
  if (now - lastTxMs >= (g_anyEncoderActive ? ACTIVE_TX_INTERVAL_MS : INACTIVE_TX_INTERVAL_MS)) {
    sendMeasurements();
    lastTxMs = now;
  }

}

