/*****************************************************************
 *                                                               *
 * ScotchHop Widget Units                                        *
 *                                                               *
 * Platform:  Polulu A-Star 328PB                                *
 *                                                               *
 * by Ross Butler, June 2025                                 )'( *
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

enum class StatusLedId {
  stepSwitchActive,
  txOk,
  txFailure,
  count
};

struct StatusLed {
  uint8_t pin;
  uint8_t pwm;
  uint32_t flashMs;
};
typedef struct StatusLed StatusLed_t;

enum class StatusLedState {
  on,
  off,
  flash
};

union StepId {
  struct {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
  };
  uint8_t id;
};


/************************
 * Widget Configuration *
 ************************/

static constexpr StatusLed_t statusLeds[static_cast<int>(StatusLedId::count)] = {
  {5, 128, 10},     // green for step switch active
  {6, 128, 10},     // blue for TX ok
  {3, 128, 10L} };  // red for TX failure

#define PIN_STEP_SWITCH 2
#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
// The radio also uses SPI0 bus MOSI on 11, MISO on 12, and SCK on 13
#define PIN_STEP_ID_0 14  // A0
#define PIN_STEP_ID_1 15  // A1
#define PIN_STEP_ID_2 16  // A2
#define PIN_STEP_ID_3 17  // A3

#define STEP_SWITCH_ACTIVE_STATE LOW
#define STEP_SWITCH_INTERRUPT_MODE RISING

#define WIDGET_ID 18

#define ACTIVE_TX_INTERVAL_MS 100L
#define INACTIVE_TX_INTERVAL_MS 500L

// When standby mode is enabled, the processor will be put to sleep after
// the widget has been inactive for inactivityTimeoutForSleepMs ms.
#define ENABLE_STANDBY_MODE

// In standby mode, we'll transmit a packet with zero-valued data approximately
// every STANDBY_TX_INTERVAL_S seconds.  Wake-ups occur at 8-second intervals, so
// STANDBY_TX_INTERVAL_S should be a multiple of 8 between 8 and 2040, inclusive.
#define STANDBY_TX_INTERVAL_S 64

// The processor is put to sleep when step switch activity
// hasn't been detected for inactivityTimeoutForSleepMs ms.
static constexpr uint32_t inactivityTimeoutForSleepMs = 180000L;

// Before changing from active to inactive mode, we'll
// wait for the hang time after activity ceases.
static constexpr uint32_t activityHangTimeMs = 5000L;

static constexpr uint32_t gatherMeasurementsIntervalMs = 25;  // 40 samples/s

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "1wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for
// TX_RETRY_DELAY_MULTIPLIER.  15 is the maximum value for TX_MAX_RETRIES.
#define WANT_ACK true
#define TX_MAX_RETRIES 15
// Use this when getting acks:
#define TX_RETRY_DELAY_MULTIPLIER 15
// Use this when not getting acks:
//#define TX_RETRY_DELAY_MULTIPLIER 0

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
#define RF_POWER_LEVEL RF24_PA_HIGH


/***********
 * Globals *
 ***********/

static WidgetMode widgetMode = WidgetMode::init;

static RF24 radio(PIN_RADIO_CE, PIN_RADIO_CSN);   // also uses SPI0 bus (SCK on 13, MISO on 12, MOSI on 11)

static StepId stepId;

static bool stepSwitchIsActive;

static PositionVelocityPayload payload;

static volatile bool gotStepSwitchWakeupInterrupt;

static int32_t nextGatherMeasurementsMs;

static int32_t nextTxMs;
static uint32_t txInterval;

static uint32_t lastActiveMs;       // when activity was last detected
static uint32_t hangTimeEndsMs;     // when the current hang time ends; 0 means not set

static uint8_t stayAwakeCountdown;


/******************
 * Implementation *
 ******************/

ISR(WDT_vect)
{
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
}


void handleStepSwitchWakeupInterrupt()
{
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
  gotStepSwitchWakeupInterrupt = true;
}


bool widgetWake()
{
  // Returns true if the widget should stay awake
  // or false if it should continue to sleep.

  uint32_t now = millis();

  if (!gotStepSwitchWakeupInterrupt) {
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

  gotStepSwitchWakeupInterrupt = false;

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Woken up by pin interrupt."));
#endif

  // Reinitialize all the time trackers because the ms timer has been off.
  // Time in this little world has stood still while time in the default
  // world marched on, so we need transmit and gather data ASAP.
  nextTxMs = now;
  nextGatherMeasurementsMs = now;
  lastActiveMs = now;
  hangTimeEndsMs = 0;

  return true;
}


void widgetSleep()
{
  // We don't want interrupts duing the timing-critical stuff below,
  // and we don't want the ISR to disable sleep before we go to sleep.
  noInterrupts();

  gotStepSwitchWakeupInterrupt = false;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // The sound-detected wakeup interrupt is already attached.

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


void setLedState(StatusLedId id, StatusLedState state)
{
  uint8_t idx = static_cast<int>(id);

  switch (state) {
    case StatusLedState::on:
      analogWrite(statusLeds[idx].pin, statusLeds[idx].pwm);
      break;
    case StatusLedState::off:
      digitalWrite(statusLeds[idx].pin, LOW);
      break;
    case StatusLedState::flash:
      // TODO:  when not in standby mode, turn on the LED, then set a one-shot timer to turn it off
      digitalWrite(statusLeds[idx].pin, HIGH);
      delay(statusLeds[idx].flashMs);
      digitalWrite(statusLeds[idx].pin, LOW);
      break;
    default:
      break;
  }
}


void setWidgetMode(WidgetMode newMode, uint32_t now)
{
  bool stayAwake;

  switch (newMode) {

    case WidgetMode::standby:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to standby."));
#endif
      setLedState(StatusLedId::txFailure, StatusLedState::off);
      wdt_reset();
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      stayAwake = false;
      while (!stayAwake) {
        // widgetSleep sets widgetMode to standby at the appropriate
        // point.  It returns after we sleep then wake up.
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
      // so that the pattern controller quickly knows we've gone inactive.
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


void gatherMeasurements(uint32_t now)
{
  stepSwitchIsActive = digitalRead(PIN_STEP_SWITCH) == STEP_SWITCH_ACTIVE_STATE;

   setLedState(StatusLedId::stepSwitchActive, stepSwitchIsActive ? StatusLedState::on : StatusLedState::off);

  if (stepSwitchIsActive) {
    // Go active immediately if we're inactive.
    lastActiveMs = now;
    hangTimeEndsMs = 0;
    if (widgetMode == WidgetMode::inactive) {
      setWidgetMode(WidgetMode::active, now);
    }
  }
  // Activity has stopped.  Change to inactive
  // mode at the end of the hang-time period.
  else if (widgetMode == WidgetMode::active) {
    if (hangTimeEndsMs == 0) {
      hangTimeEndsMs = now + activityHangTimeMs;
    }
    if ((int32_t) (now - hangTimeEndsMs) >= 0) {
      setWidgetMode(WidgetMode::inactive, now);
    }
  }
}


void sendMeasurements(uint32_t now)
{
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("send"));
#endif

  setLedState(StatusLedId::txFailure, StatusLedState::off);

  payload.velocity = stepSwitchIsActive;
  payload.widgetHeader.isActive = (widgetMode == WidgetMode::active);

//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("active: ");
//  Serial.println(stepSwitchIsActive);
//#endif

  if (!radio.write(&payload, sizeof(payload), !WANT_ACK)) {
    // Turning the txFailure LED on solid while in standby mode isn't visible
    // because it gets turned off almost right away.  So, flash it.
    setLedState(StatusLedId::txFailure, widgetMode != WidgetMode::standby ? StatusLedState::on : StatusLedState::flash);
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("tx failed"));
#endif
  }
  else {
    setLedState(StatusLedId::txOk, StatusLedState::flash);
  }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

  for (uint8_t i = 0; i < static_cast<int>(StatusLedId::count); i++) {
    pinMode(statusLeds[i].pin, OUTPUT);
  }

  // Read stepId set by jumpers on D14-D17.
  pinMode(PIN_STEP_ID_0, INPUT_PULLUP);
  pinMode(PIN_STEP_ID_1, INPUT_PULLUP);
  pinMode(PIN_STEP_ID_2, INPUT_PULLUP);
  pinMode(PIN_STEP_ID_3, INPUT_PULLUP);
  stepId.bit0 = !digitalRead(PIN_STEP_ID_0);
  stepId.bit1 = !digitalRead(PIN_STEP_ID_1);
  stepId.bit2 = !digitalRead(PIN_STEP_ID_2);
  stepId.bit3 = !digitalRead(PIN_STEP_ID_3);
  pinMode(PIN_STEP_ID_0, INPUT);
  pinMode(PIN_STEP_ID_1, INPUT);
  pinMode(PIN_STEP_ID_2, INPUT);
  pinMode(PIN_STEP_ID_3, INPUT);

  setLedState(StatusLedId::txFailure, StatusLedState::on); // We'll leave the red LED on solid if radio config fails.
  if (!configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                      TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                      RF_CHANNEL)) {
    while (true);
  }
  setLedState(StatusLedId::txFailure, StatusLedState::off);

  // Set the watchdog for interrupt only (no system reset) and an 8s interval.
  // We have to turn off interrupts because the changes to the control register
  // must be done within four clock cycles of setting WDCE (change-enable bit).
  noInterrupts();
  _WD_CONTROL_REG = (1 << WDCE) | (1 << WDE);
  _WD_CONTROL_REG = (1 << WDIE) | (0 << WDE) | (1 << WDP3) | (1 << WDP0);
  interrupts();

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.channel = 0;
  payload.position = stepId.id;

  setWidgetMode(WidgetMode::inactive, millis());

  // Set up and turn on the pin-change interrupts last.
  pinMode(PIN_STEP_SWITCH, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_STEP_SWITCH),
                  handleStepSwitchWakeupInterrupt,
                  STEP_SWITCH_INTERRUPT_MODE);
}


void loop() {

  static int32_t lastTxMs;

  uint32_t now = millis();

  if ((int32_t) (now - nextGatherMeasurementsMs) >= 0) {
    nextGatherMeasurementsMs += gatherMeasurementsIntervalMs;
    gatherMeasurements(now);
  }

  if ((int32_t) (now - nextTxMs) >= 0) {
    nextTxMs += txInterval;
    sendMeasurements(now);
  }

#ifdef ENABLE_STANDBY_MODE
  if (widgetMode == WidgetMode::inactive
      && now - lastActiveMs >= inactivityTimeoutForSleepMs)
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Going standby because no step activiy was detected from "));
    Serial.print(lastActiveMs);
    Serial.print(F(" to "));
    Serial.println(now);
#endif
    setWidgetMode(WidgetMode::standby, now);
    // Setting the widget mode to standby will put the processor to sleep.
    // When it wakes due to a pin interupt, execution eventually resumes here.
  }
#endif
}
