/*****************************************************************
 *                                                               *
 * FourPlay Widget                                               *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, August 2016                               )'( *
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

#define SPINNAH
//#define FOURPLAY
//#define FOURPLAY_4_2
//#define FOURPLAY_4_3

#if defined(SPINNAH)
  #define WIDGET_ID 2
#elif defined(FOURPLAY)
  #define WIDGET_ID 9
#elif defined(FOURPLAY_4_2)
  #define WIDGET_ID 12
#elif defined(FOURPLAY_4_3)
  #define WIDGET_ID 13
#endif

#ifdef SPINNAH
  #define NUM_ENCODERS 1
  #define ACTIVE_TX_INTERVAL_MS 100L
  #define INACTIVE_TX_INTERVAL_MS 1000L
  //#define TX_FAILURE_LED_PIN 2
  #define ENCODER_0_A_PIN 2
  #define ENCODER_0_B_PIN 3
#else
  #define NUM_ENCODERS 4
  #define ACTIVE_TX_INTERVAL_MS 10L
  #define INACTIVE_TX_INTERVAL_MS 1000L
  //#define TX_FAILURE_LED_PIN 2
  #define ENCODER_0_A_PIN 2
  #define ENCODER_0_B_PIN 3
  #define ENCODER_1_A_PIN 4
  #define ENCODER_1_B_PIN 5
  #define ENCODER_2_A_PIN 6
  #define ENCODER_2_B_PIN 7
  #define ENCODER_3_A_PIN A0
  #define ENCODER_3_B_PIN A1
  //#define ENCODERS_VDD_PIN 8
  //#define ENCODER_ACTIVE_PIN 8
#endif

#define SPIN_ACTIVITY_DETECT_MS 50
#define SPIN_INACTIVITY_TIMEOUT_MS 500

#define RPM_UPDATE_INTERVAL_MS 250L

#if defined(SPINNAH) || defined(FOURPLAY)
  // bicycle wheels
  #define NUM_STEPS_PER_REV 36
#elif defined(FOURPLAY_4_2) || defined(FOURPLAY_4_3)
  // them cool little spoked wheels that someone left out back
  #define NUM_STEPS_PER_REV 20
#endif


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "1wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11, 13) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 5

#if defined(SPINNAH)
#define TX_RETRY_DELAY_MULTIPLIER 11
#elif defined(FOURPLAY)
#define TX_RETRY_DELAY_MULTIPLIER 7
#elif defined(FOURPLAY_4_2)
#define TX_RETRY_DELAY_MULTIPLIER 3
#elif defined(FOURPLAY_4_3)
#define TX_RETRY_DELAY_MULTIPLIER 7
#endif

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

PositionVelocityPayload payload;

static bool g_anyEncoderActive;
static bool g_encoderActive[NUM_ENCODERS];
static volatile uint8_t g_lastPortCEncoderStates;
static volatile uint8_t g_lastPortDEncoderStates;
static volatile int g_encoderValues[NUM_ENCODERS];
static uint32_t g_encoderRpms[NUM_ENCODERS];

const int8_t g_greyCodeToEncoderStepMap[] = {
       // last this
   0,  //  00   00
  -1,  //  00   01
   1,  //  00   10
   0,  //  00   11
   1,  //  01   00
   0,  //  01   01
   0,  //  01   10
  -1,  //  01   11
  -1,  //  10   00
   0,  //  10   01
   0,  //  10   10
   1,  //  10   11
   0,  //  11   00
   1,  //  11   01
  -1,  //  11   10
   0,  //  11   11
};

volatile uint8_t g_pincEncoderStates;


/******************
 * Implementation *
 ******************/

void setUpPinChangeInterrupt(uint8_t pin) 
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group 
}


// Service pin change interrupt for D0 - D7.
ISR (PCINT2_vect)
{
  uint8_t encoderStates = PIND >> 2;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastPortDEncoderStates;

  for (uint8_t i = 0; i < 3; ++i) {  // TODO: replace magic number 3
    uint8_t idx = ((lastStates & 0b11) << 2) | (curStates & 0b11);
    g_encoderValues[i] += g_greyCodeToEncoderStepMap[idx];
    curStates >>= 2;
    lastStates >>= 2;
  }

  g_lastPortDEncoderStates = encoderStates;
}


// Service pin change interrupt for A0 - A5.
ISR (PCINT1_vect)
{
  uint8_t encoderStates = PINC;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastPortCEncoderStates;

#ifdef ENABLE_DEBUG_PRINT
  g_pincEncoderStates = encoderStates;
#endif

  for (uint8_t i = 3; i < 4; ++i) {  // TODO: replace magic number for range
    uint8_t idx = ((lastStates & 0b11) << 2) | (curStates & 0b11);
    g_encoderValues[i] += g_greyCodeToEncoderStepMap[idx];
    curStates >>= 2;
    lastStates >>= 2;
  }

  g_lastPortCEncoderStates = encoderStates;
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(57600);
  printf_begin();
#endif

#ifdef ENCODER_0_A_PIN
  pinMode(ENCODER_0_A_PIN, INPUT); 
  pinMode(ENCODER_0_B_PIN, INPUT);
  digitalWrite(ENCODER_0_A_PIN, HIGH);
  digitalWrite(ENCODER_0_B_PIN, HIGH);
#endif
#ifdef ENCODER_1_A_PIN
  pinMode(ENCODER_1_A_PIN, INPUT); 
  pinMode(ENCODER_1_B_PIN, INPUT);
  digitalWrite(ENCODER_1_A_PIN, HIGH);
  digitalWrite(ENCODER_1_B_PIN, HIGH);
#endif
#ifdef ENCODER_2_A_PIN
  pinMode(ENCODER_2_A_PIN, INPUT); 
  pinMode(ENCODER_2_B_PIN, INPUT);
  digitalWrite(ENCODER_2_A_PIN, HIGH);
  digitalWrite(ENCODER_2_B_PIN, HIGH);
#endif
#ifdef ENCODER_3_A_PIN
  pinMode(ENCODER_3_A_PIN, INPUT); 
  pinMode(ENCODER_3_B_PIN, INPUT);
  digitalWrite(ENCODER_3_A_PIN, HIGH);
  digitalWrite(ENCODER_3_B_PIN, HIGH);
#endif

  // Initially, turn on power to the encoders and set the active indicator low.
#ifdef ENCODERS_VDD_PIN
  pinMode(ENCODERS_VDD_PIN, OUTPUT);
  digitalWrite(ENCODERS_VDD_PIN, HIGH);
#endif
#ifdef ENCODER_ACTIVE_PIN
  pinMode(ENCODER_ACTIVE_PIN, OUTPUT);
  digitalWrite(ENCODER_ACTIVE_PIN, LOW);
#endif

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;

  g_anyEncoderActive = false;
  for (uint8_t i = 0; i < NUM_ENCODERS; ++i) {
    g_encoderValues[i] = 0;
    g_encoderActive[i] = false;
  }

  // Set up and turn on the pin-change interrupts last.
#ifdef ENCODER_0_A_PIN
  setUpPinChangeInterrupt(ENCODER_0_A_PIN);
  setUpPinChangeInterrupt(ENCODER_0_B_PIN);
#endif
#ifdef ENCODER_1_A_PIN
  setUpPinChangeInterrupt(ENCODER_1_A_PIN);
  setUpPinChangeInterrupt(ENCODER_1_B_PIN);
#endif
#ifdef ENCODER_2_A_PIN
  setUpPinChangeInterrupt(ENCODER_2_A_PIN);
  setUpPinChangeInterrupt(ENCODER_2_B_PIN);
#endif
#ifdef ENCODER_3_A_PIN
  setUpPinChangeInterrupt(ENCODER_3_A_PIN);
  setUpPinChangeInterrupt(ENCODER_3_B_PIN);
#endif
}


void gatherMeasurements()
{
  static int32_t lastEncoderValues[NUM_ENCODERS];
  static uint32_t lastEncoderInactiveMs[NUM_ENCODERS];
  static uint32_t lastEncoderChangeMs[NUM_ENCODERS];

  unsigned long now = millis();

  for (int i = 0; i < NUM_ENCODERS; ++i) {
    bool encoderChanged = false;
    int thisEncoderValue = g_encoderValues[i];
    if (thisEncoderValue != lastEncoderValues[i]) {
      encoderChanged = true;
      lastEncoderValues[i] = thisEncoderValue;
      lastEncoderChangeMs[i] = now;
    }

    if (!g_encoderActive[i]) {
      if (!encoderChanged && now - lastEncoderChangeMs[i] > SPIN_INACTIVITY_TIMEOUT_MS) {
        lastEncoderInactiveMs[i] = now;
      }
      else {
        if (now - lastEncoderInactiveMs[i] > SPIN_ACTIVITY_DETECT_MS) {
          g_encoderActive[i] = true;
          lastEncoderValues[i] = g_encoderValues[i];
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

#ifdef ENABLE_DEBUG_PRINT
//  for (int i = 0; i < NUM_ENCODERS; ++i) {
//    Serial.print(i);
//    Serial.print(",");
//    Serial.print(g_encoderActive[i]);
//    Serial.print(",");
//    Serial.print(g_encoderValues[i]);
//    Serial.print(",");
//    Serial.println(g_encoderRpms[i]);
//  }
#endif
    
}


void sendMeasurements()
{
  static int32_t lastEncoderValues[NUM_ENCODERS];
  static uint32_t lastRpmUpdateMs[NUM_ENCODERS];
  static int32_t encoderSteps[NUM_ENCODERS];
  static int16_t encoderRpms[NUM_ENCODERS];
  uint32_t now = millis();

  for (int i = 0; i < NUM_ENCODERS; ++i) {

    // TODO:  change this crap to use a moving average
    if (g_encoderActive[i]) {
      encoderSteps[i] += (g_encoderValues[i] - lastEncoderValues[i]);
      int32_t rpmIntervalMs = now - lastRpmUpdateMs[i];
      if (rpmIntervalMs >= RPM_UPDATE_INTERVAL_MS) {
        lastRpmUpdateMs[i] = now;
        if (encoderSteps[i] != 0) {
          int32_t encoderStepsPerMinute = (encoderSteps[i] * 60000L) / rpmIntervalMs;
          encoderRpms[i] = encoderStepsPerMinute / NUM_STEPS_PER_REV;
#ifdef ENABLE_DEBUG_PRINT
          printf("%d: encoderSteps[i]=%ld, encoderStepsPerMinute=%ld, rpmIntervalMs=%ld, encoderRpms[i]=%d\n",
                 i, encoderSteps[i], encoderStepsPerMinute, rpmIntervalMs, encoderRpms[i]);
#endif
          encoderSteps[i] = 0;
        }
        else {
          encoderRpms[i] = 0;
        }
      }
    }
    else {
      lastRpmUpdateMs[i] = now;
      encoderSteps[i] = 0;
      encoderRpms[i] = 0;
    }
    lastEncoderValues[i] = g_encoderValues[i];

    payload.widgetHeader.channel = i;
    payload.widgetHeader.isActive = g_encoderActive[i];
    payload.position = g_encoderValues[i];
    payload.velocity = encoderRpms[i];
    
#ifdef ENABLE_DEBUG_PRINT
  if (i == 2 && g_encoderActive[0]) printf("%d:  payload.position=%d, payload.velocity=%d\n", i, payload.position, payload.velocity);
#endif
  
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

#ifdef ENABLE_DEBUG_PRINT
//    printf("g_pincEncoderStates=%x\n", g_pincEncoderStates);
#endif
    
    sendMeasurements();
    lastTxMs = now;
  }

}

