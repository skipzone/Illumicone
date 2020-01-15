/*****************************************************************
 *                                                               *
 * FourPlay, Spinnah, and TriObelisk Widgets                     *
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


/************
 * Includes *
 ************/

#include <avr/sleep.h>
#include <avr/wdt.h>

#include "illumiconeWidget.h"

#ifdef ENABLE_DEBUG_PRINT
#include "printf.h"
#endif


/****************************
 * Constants and Data Types *
 ****************************/

enum class WidgetMode {
  init,
  standby,
  inactive,
  active
};


/************************
 * Widget Configuration *
 ************************/

#define SPINNAH
//#define FOURPLAY_4_2
//#define FOURPLAY_4_3

#if defined(SPINNAH)
  #define WIDGET_ID 2
#elif defined(FOURPLAY_4_2)
  #define WIDGET_ID 8
#elif defined(FOURPLAY_4_3)
  #define WIDGET_ID 9
#endif

#if defined(SPINNAH)
  #define NUM_ENCODERS 1
  #define ACTIVE_TX_INTERVAL_MS 100L
  #define INACTIVE_TX_INTERVAL_MS 1000L
  #define INACTIVITY_TIMEOUT_FOR_SLEEP_MS 10000L
  //#define TX_FAILURE_LED_PIN 2
  //#define TX_FAILURE_LED_ON HIGH
  //#define TX_FAILURE_LED_OFF LOW
  #define ENCODER_0_A_PIN 2
  #define ENCODER_0_B_PIN 3
#elif defined(FOURPLAY_4_2) || defined(FOURPLAY_4_3)
  #define NUM_ENCODERS 4
  #define ACTIVE_TX_INTERVAL_MS 10L
  #define INACTIVE_TX_INTERVAL_MS 2000L
  #define INACTIVITY_TIMEOUT_FOR_SLEEP_MS 10000L
  //#define TX_FAILURE_LED_PIN 2
  //#define TX_FAILURE_LED_ON HIGH
  //#define TX_FAILURE_LED_OFF LOW
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

#if defined(SPINNAH)
  // bicycle wheels
  #define NUM_STEPS_PER_REV 36
#elif defined(FOURPLAY_4_2) || defined(FOURPLAY_4_3)
  // them cool little spoked wheels that someone left out back
  #define NUM_STEPS_PER_REV 20
#endif

// In standby mode, we'll transmit a packet with zero-valued data approximately
// every STANDBY_TX_INTERVAL_S seconds.  Wake-ups occur at 8-second intervals, so
// STANDBY_TX_INTERVAL_S should be a multiple of 8 between 8 and 2040, inclusive.
#define STANDBY_TX_INTERVAL_S 64

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
//#define TX_RETRY_DELAY_MULTIPLIER 0     // use widget-specific values below when getting acks
#define TX_MAX_RETRIES 15               // use 15 when getting acks
// Use these when getting acks:
#if defined(SPINNAH)
#define TX_RETRY_DELAY_MULTIPLIER 11
#elif defined(FOURPLAY_4_2)
#define TX_RETRY_DELAY_MULTIPLIER 7
#elif defined(FOURPLAY_4_3)
#define TX_RETRY_DELAY_MULTIPLIER 4
#endif

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

static WidgetMode widgetMode = WidgetMode::init;

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static PositionVelocityPayload payload;

static bool g_anyEncoderActive;
static bool g_encoderActive[NUM_ENCODERS];
static volatile uint8_t g_lastPortCEncoderStates;
static volatile uint8_t g_lastPortDEncoderStates;
static volatile int g_encoderValues[NUM_ENCODERS];
static uint32_t g_encoderRpms[NUM_ENCODERS];

static const int8_t g_greyCodeToEncoderStepMap[] = {
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

static volatile uint8_t g_pincEncoderStates;

static volatile bool gotPinInterrupt;

static int32_t nextTxMs;
static uint32_t txInterval;

static uint32_t lastActiveMs;

static uint8_t stayAwakeCountdown;


/******************
 * Implementation *
 ******************/

void setUpPinChangeInterrupt(uint8_t pin) 
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group 
}


ISR(WDT_vect) {
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
}


// Service pin change interrupt for D0 - D7.
ISR(PCINT2_vect)
{
  gotPinInterrupt = true;

  uint8_t encoderStates = PIND >> 2;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastPortDEncoderStates;

  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
    return;
  }

  for (uint8_t i = 0; i < 3; ++i) {  // TODO: replace magic number 3
    uint8_t idx = ((lastStates & 0b11) << 2) | (curStates & 0b11);
    g_encoderValues[i] += g_greyCodeToEncoderStepMap[idx];
    curStates >>= 2;
    lastStates >>= 2;
  }

  g_lastPortDEncoderStates = encoderStates;
}


// Service pin change interrupt for A0 - A5.
ISR(PCINT1_vect)
{
  gotPinInterrupt = true;

  uint8_t encoderStates = PINC;
  uint8_t curStates = encoderStates;
  uint8_t lastStates = g_lastPortCEncoderStates;

  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
    return;
  }

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


bool widgetWake()
{
  uint32_t now = millis();

  if (!gotPinInterrupt) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Woken up by watchdog."));
#endif
    if (--stayAwakeCountdown == 0) {
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      // Let the pattern controller know we're still alive.
      sendMeasurements(now);
    }
    return false;
  }

  gotPinInterrupt = false;

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Woken up by pin interrupt."));
#endif

  // Reinitialize all the time trackers because the ms timer has been off.
  // Time in this little world has stood still while time in the default
  // world marched on, so we need transmit and gather data ASAP.
  nextTxMs = now;
  lastActiveMs = now;

  return true;
}


void widgetSleep()
{
  // We don't want interrupts duing the timing-critical stuff below,
  // and we don't want the ISR to disable sleep before we go to sleep.
  noInterrupts();

  gotPinInterrupt = false;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // The interrupt is already attached because it is the interrupt the IMU uses.

  // Turn off brown-out enable in software.  BODS must be set to
  // one and BODSE must be set to zero within four clock cycles.
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles.
  MCUCR = bit (BODS);

  // Standby mode tells the ISR to disable sleep after we wake up.
  widgetMode = WidgetMode::standby;

  // sleep_cpu is guarnteed to be called because the processor always
  // executes the next instruction after interrupts are enabled.
  interrupts();
  sleep_cpu();
}


void setWidgetMode(WidgetMode newMode, uint32_t now)
{
  bool stayAwake;
  
  switch (newMode) {

    case WidgetMode::standby:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to standby."));
#endif
      wdt_reset();
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      stayAwake = false;
      while (!stayAwake) {
        // widgetSleep returns after we sleep then wake up.
        widgetSleep();
        stayAwake = widgetWake();
      }
      widgetMode = WidgetMode::inactive;
      break;

    case WidgetMode::inactive:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to inactive."));
#endif
      // The change in tx interval will become effective after the next
      // transmission, which needs to happen at the shorter active interval
      // so that the pattern controller quicly knows we've gone inactive.
      txInterval = INACTIVE_TX_INTERVAL_MS;
      widgetMode = WidgetMode::inactive;
      break;

    case WidgetMode::active:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to active."));
#endif
      txInterval = ACTIVE_TX_INTERVAL_MS;
      // The next transmission needs to be as soon as practical so that
      // the pattern controller quickly knows that we're active again.
      if ((int32_t) (nextTxMs - now) > ACTIVE_TX_INTERVAL_MS) {
        nextTxMs = now + ACTIVE_TX_INTERVAL_MS;
      }
      widgetMode = WidgetMode::active;
      break;

    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("*** Invalid newMode in setWidgetMode"));
#endif
      break;
  }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef TX_FAILURE_LED_PIN      
  pinMode(TX_FAILURE_LED_PIN, OUTPUT);
  digitalWrite(TX_FAILURE_LED_PIN, TX_FAILURE_LED_OFF);
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

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
  // Set the watchdog for interrupt only (no system reset) and an 8s interval.
  // We have to turn off interrupts because the changes to the control register
  // must be done within four clock cycles of setting WDCE (change-enable bit).
  noInterrupts();
  _WD_CONTROL_REG = (1 << WDCE) | (1 << WDE);
  _WD_CONTROL_REG = (1 << WDIE) | (0 << WDE) | (1 << WDP3) | (1 << WDP0);
  interrupts();

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

  setWidgetMode(WidgetMode::inactive, millis());
}


void gatherMeasurements(uint32_t now)
{
  static int32_t lastEncoderValues[NUM_ENCODERS];
  static uint32_t lastEncoderInactiveMs[NUM_ENCODERS];
  static uint32_t lastEncoderChangeMs[NUM_ENCODERS];

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
  for (int i = 0; i < NUM_ENCODERS; ++i) {
    if (g_encoderActive[i]) {
      g_anyEncoderActive = true;
      break;
    }
  }
#ifdef ENCODER_ACTIVE_PIN
  digitalWrite(ENCODER_ACTIVE_PIN, g_anyEncoderActive);
#endif
  if (g_anyEncoderActive) {
    lastActiveMs = now;
    if (widgetMode == WidgetMode::inactive) {
      setWidgetMode(WidgetMode::active, now);
    }
  }
  else if (!g_anyEncoderActive && widgetMode == WidgetMode::active) {
    setWidgetMode(WidgetMode::inactive, now);
  }


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


void sendMeasurements(uint32_t now)
{
  static int32_t lastEncoderValues[NUM_ENCODERS];
  static uint32_t lastRpmUpdateMs[NUM_ENCODERS];
  static int32_t encoderSteps[NUM_ENCODERS];
  static int16_t encoderRpms[NUM_ENCODERS];

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
    
//#ifdef ENABLE_DEBUG_PRINT
//  if (i == 2 && g_encoderActive[0]) printf("%d:  payload.position=%d, payload.velocity=%d\n", i, payload.position, payload.velocity);
//#endif
  
    if (!radio.write(&payload, sizeof(payload), !WANT_ACK)) {
#ifdef TX_FAILURE_LED_PIN      
      digitalWrite(TX_FAILURE_LED_PIN, TX_FAILURE_LED_ON);
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("tx failed."));
#endif      
#endif
    }
    else {
#ifdef TX_FAILURE_LED_PIN
      digitalWrite(TX_FAILURE_LED_PIN, TX_FAILURE_LED_OFF);
#endif
    }
  }
}


void loop() {

  static int32_t lastTxMs;

  uint32_t now = millis();

  gatherMeasurements(now);

//  if (now - lastTxMs >= (g_anyEncoderActive ? ACTIVE_TX_INTERVAL_MS : INACTIVE_TX_INTERVAL_MS)) {
//
//#ifdef ENABLE_DEBUG_PRINT
////    printf("g_pincEncoderStates=%x\n", g_pincEncoderStates);
//#endif
//    
//    sendMeasurements();
//    lastTxMs = now;
//  }
  if ((int32_t) (now - nextTxMs) >= 0) {
    nextTxMs = now + txInterval;
    sendMeasurements(now);
  }

  if (widgetMode == WidgetMode::inactive
      && now - lastActiveMs >= INACTIVITY_TIMEOUT_FOR_SLEEP_MS)
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Going standby because no motion from "));
    Serial.print(lastActiveMs);
    Serial.print(F(" to "));
    Serial.println(now);
#endif
    setWidgetMode(WidgetMode::standby, now);
    // Setting the widget mode to standby will put the processor to sleep.
    // When it wakes due to a pin interupt, execution eventually resumes here.
  }
}

