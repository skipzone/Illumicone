/*****************************************************************
 *                                                               *
 * Widget Controller Low-Power Evaluation                        *
 *                                                               *
 * Platform:  various Arduinos                                   *
 *                                                               *
 * by Ross Butler, November 2019                             )'( *
 *                                                               *
 * based on Widget_FourPlay                                      *
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


/***********
 * Options *
 ***********/

//#define ENABLE_RADIO
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


/*****************
 * Configuration *
 *****************/

//#define DISABLE_ADC
//#define DISABLE_UART

constexpr uint32_t shortOpStateChangeIntervalMs = 10000;
constexpr uint32_t longOpStateChangeIntervalMs = 60000;

constexpr uint8_t interruptPin = 3;

constexpr uint8_t pinLed1 = LED_BUILTIN;    // 0 to disable
constexpr uint8_t led1On = HIGH;
constexpr uint8_t led1Off = LOW;

constexpr uint8_t pinLed2Pwm = 5;           // 0 to disable
constexpr uint8_t led2LowIntensity = 0;
constexpr uint8_t led2HighIntensity = 255;

#define WIDGET_ID 0

#define ACTIVE_TX_INTERVAL_MS 50L
#define INACTIVE_TX_INTERVAL_MS 500L
// In standby mode, we'll transmit a packet with zero-valued data approximately
// every STANDBY_TX_INTERVAL_S seconds.  Wake-ups occur at 8-second intervals, so
// STANDBY_TX_INTERVAL_S should be a multiple of 8 between 8 and 2040, inclusive.
#define STANDBY_TX_INTERVAL_S 24

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "2wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK FALSE
#define TX_RETRY_DELAY_MULTIPLIER 0     // use widget-specific values below when getting acks
#define TX_MAX_RETRIES 0                // use 15 when getting acks

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


/***********************
 * Types and Constants *
 ***********************/

enum class OperatingState {
  WIDGET_ACTIVE_START,
  WIDGET_ACTIVE,
  WIDGET_INACTIVE_LED1_OFF_START,
  WIDGET_INACTIVE_LED1_OFF,
  WIDGET_INACTIVE_LED1_ON_START,
  WIDGET_INACTIVE_LED1_ON,
  WIDGET_INACTIVE_LED2_25PCT_START,
  WIDGET_INACTIVE_LED2_25PCT,
  WIDGET_INACTIVE_LED2_50PCT,
  WIDGET_INACTIVE_LED2_75PCT,
  WIDGET_INACTIVE_LED2_100PCT,
  WIDGET_STANDBY_START,
  WIDGET_STANDBY,
};

enum class WidgetMode {
  init,
  standby,
  inactive,
  active
};


/***********
 * Globals *
 ***********/

static OperatingState opState = OperatingState::WIDGET_ACTIVE_START;
static WidgetMode widgetMode = WidgetMode::init;

#ifdef ENABLE_RADIO
static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)
#endif

static MeasurementVectorPayload payload;

static volatile bool gotPinInterrupt;

static int32_t nextOpStateChangeMs;
static int32_t remainingOpStateChangeMs;
static int32_t nextTxMs;
static uint32_t txInterval;

static uint8_t stayAwakeCountdown;


/*********************************
 * Interrupt Setup and Servicing *
 *********************************/

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

  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
    return;
  }
}


// Service pin change interrupt for A0 - A5.
ISR(PCINT1_vect)
{
  gotPinInterrupt = true;

  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
    return;
  }
}


/***********
 * Helpers *
 ***********/

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
  nextTxMs = now;
  nextOpStateChangeMs = now + remainingOpStateChangeMs;

  return true;
}


void widgetSleep(uint32_t now)
{
  // Keep track of how long we have left in this operating state because
  // we're going to lose the millisecond counter when we sleep so we'll
  // need to recompute nextOpStateChangeMs when we awaken.
  remainingOpStateChangeMs = nextOpStateChangeMs - now;

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
        widgetSleep(now);
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


/******************
 * Initialization *
 ******************/

void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef DISABLE_ADC
  ADCSRA = 0;
  power_adc_disable(); // ADC converter
#endif

#if defined(DISABLE_UART) && !defined(ENABLE_DEBUG_PRINT)
  power_usart0_disable();
#endif

  pinMode(interruptPin, INPUT); 
  digitalWrite(interruptPin, HIGH);

  if (pinLed1 != 0) {
    pinMode(pinLed1, OUTPUT);
    digitalWrite(pinLed1, led1Off);
  }

  if (pinLed2Pwm != 0) {
    pinMode(pinLed2Pwm, OUTPUT);
    analogWrite(pinLed2Pwm, map(0, 0, 100, led2LowIntensity, led2HighIntensity));
  }

#ifdef ENABLE_RADIO
  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
#endif
  
  // Set the watchdog for interrupt only (no system reset) and an 8s interval.
  // We have to turn off interrupts because the changes to the control register
  // must be done within four clock cycles of setting WDCE (change-enable bit).
  noInterrupts();
  _WD_CONTROL_REG = (1 << WDCE) | (1 << WDE);
  _WD_CONTROL_REG = (1 << WDIE) | (0 << WDE) | (1 << WDP3) | (1 << WDP0);
  interrupts();

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.channel = 9;
  payload.widgetHeader.isActive = false;
  for (uint8_t i = 0; i < 15; ++i) {
    payload.measurements[i] = 0xa5a5;
  }

  // Set up and turn on the pin-change interrupts last.
  setUpPinChangeInterrupt(interruptPin);
}


void sendMeasurements(uint32_t now)
{
#ifdef ENABLE_RADIO
  if (!radio.write(&payload, sizeof(payload), !WANT_ACK)) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("tx failed."));
#endif      
  }
#endif
}


void loop()
{
  static int32_t lastOpStateChangeMs;
  static bool doTx;

  uint32_t now = millis();

  switch(opState) {

    case OperatingState::WIDGET_ACTIVE_START:
      doTx = true;
      setWidgetMode(WidgetMode::active, now);
      opState = OperatingState::WIDGET_ACTIVE;
      nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      break;

    case OperatingState::WIDGET_ACTIVE:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        opState = OperatingState::WIDGET_INACTIVE_LED1_OFF_START;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED1_OFF_START:
      setWidgetMode(WidgetMode::inactive, now);
      opState = OperatingState::WIDGET_INACTIVE_LED1_OFF;
      nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      break;

    case OperatingState::WIDGET_INACTIVE_LED1_OFF:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        opState = OperatingState::WIDGET_INACTIVE_LED1_ON_START;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED1_ON_START:
      if (pinLed1 != 0) {
        digitalWrite(pinLed1, led1On);
        opState = OperatingState::WIDGET_INACTIVE_LED1_ON;
        nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      }
      else {
        opState = OperatingState::WIDGET_INACTIVE_LED2_25PCT_START;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED1_ON:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        digitalWrite(pinLed1, led1Off);
        opState = OperatingState::WIDGET_INACTIVE_LED2_25PCT_START;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED2_25PCT_START:
      if (pinLed2Pwm != 0) {
        analogWrite(pinLed2Pwm, map(25, 0, 100, led2LowIntensity, led2HighIntensity));
        nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
        opState = OperatingState::WIDGET_INACTIVE_LED2_25PCT;
      }
      else {
        opState = OperatingState::WIDGET_STANDBY_START;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED2_25PCT:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        analogWrite(pinLed2Pwm, map(50, 0, 100, led2LowIntensity, led2HighIntensity));
        opState = OperatingState::WIDGET_INACTIVE_LED2_50PCT;
        nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED2_50PCT:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        analogWrite(pinLed2Pwm, map(75, 0, 100, led2LowIntensity, led2HighIntensity));
        opState = OperatingState::WIDGET_INACTIVE_LED2_75PCT;
        nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED2_75PCT:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        analogWrite(pinLed2Pwm, map(100, 0, 100, led2LowIntensity, led2HighIntensity));
        opState = OperatingState::WIDGET_INACTIVE_LED2_100PCT;
        nextOpStateChangeMs = now + shortOpStateChangeIntervalMs;
      }
      break;

    case OperatingState::WIDGET_INACTIVE_LED2_100PCT:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        analogWrite(pinLed2Pwm, map(0, 0, 100, led2LowIntensity, led2HighIntensity));
        opState = OperatingState::WIDGET_STANDBY_START;
      }
      break;

    case OperatingState::WIDGET_STANDBY_START:
      doTx = false;
      setWidgetMode(WidgetMode::standby, now);
      opState = OperatingState::WIDGET_STANDBY;
      nextOpStateChangeMs = now + longOpStateChangeIntervalMs;
      break;

    case OperatingState::WIDGET_STANDBY:
      if ((int32_t) (now - nextOpStateChangeMs) >= 0) {
        opState = OperatingState::WIDGET_ACTIVE_START;
      }
      break;
  }

  if (doTx && (int32_t) (now - nextTxMs) >= 0) {
    nextTxMs = now + txInterval;
    sendMeasurements(now);
  }

}
