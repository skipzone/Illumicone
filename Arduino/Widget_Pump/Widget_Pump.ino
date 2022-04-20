/*****************************************************************
 *                                                               *
 * Naked's and Monty's Pump Widget with Air Valve Control        *
 * (inspired by Kayla's Plunger Widget)                          *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, January 2022                              )'( *
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
  // TODO:  maybe these should be init, asleep, and awake, with the inactive/active states expressed by WidgetState.
};

enum class WidgetState {
  init,
  sleepBleedoff,
  sleep,
  timerWake,
  wakeup,
  inactive,
  pressurizing,
  bleedoff,
  popoff,
  rest,
  restBleedoff
};


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 6

#define PRESSURE_SENSOR_SIGNAL_PIN A3
#define PIN_TX_FAILURE_LED 7
#define PIN_RADIO_CE 9
#define PIN_RADIO_CSN 10
// TODO:  the LEDs need to be on PWM pins
#define PIN_STATUS_RED_LED 3
#define PIN_STATUS_GREEN_LED 3
#define PIN_STATUS_BLUE_LED 3
// TODO:  the valve control relay and sensor power do not need a PWM pin
#define PIN_VALVE_CONTROL 99
#define PIN_VALVE_POWER_EN 99
#define PIN_SENSOR_POWER 99

#define STATUS_LED_ON HIGH
#define STATUS_LED_OFF LOW


//=-=-=-=-=-=-= vvv  old shit  vvv =-=-=-=-=-=-=
#define ACTIVE_TX_INTERVAL_MS 250L
#define INACTIVE_TX_INTERVAL_MS 60000L      // should be a multiple of ACTIVE_TX_INTERVAL_MS
#define PRESSURE_SAMPLE_INTERVAL_MS 10L

// In standby mode, we'll transmit a packet with zero-valued data approximately
// every STANDBY_TX_INTERVAL_S seconds.  Wake-ups occur at 8-second intervals, so
// STANDBY_TX_INTERVAL_S should be a multiple of 8 between 8 and 2040, inclusive.
#define STANDBY_TX_INTERVAL_S 64

// The processor is put to sleep when pressure above the activity threshold
// hasn't been detected for inactivityTimeoutForSleepMs ms.
static constexpr uint16_t activityDetectionThreshold = 100;
static constexpr uint32_t inactivityTimeoutForSleepMs = 60000L;

constexpr uint16_t activePressureThreshold = 120;
constexpr uint8_t numInactiveSendTries = 5; // when going inactive, transmit that fact this many times at the active rate

static constexpr uint32_t gatherMeasurementsIntervalMs = 25;  // 40 samples/s
//=-=-=-=-=-=-= ^^^  old shit  ^^^ =-=-=-=-=-=-=





// moving average length for averaging the pressure sensor and battery voltage measurements
static constexpr uint8_t maLength = 2;

// pressure and battery voltage
static constexpr uint8_t numMaSets = 2;


// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "2wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK true
#define TX_RETRY_DELAY_MULTIPLIER 13    // use 13 when getting acks
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
#define RF_POWER_LEVEL RF24_PA_HIGH


/***********
 * Globals *
 ***********/

static WidgetMode widgetMode = WidgetMode::init;

static RF24 radio(PIN_RADIO_CE, PIN_RADIO_CSN);   // also uses SPI0 bus (SCK on 13, MISO on 12, MOSI on 11)

static MeasurementVectorPayload payload;

static bool isActive;
static uint8_t wasActiveCountdown;

static int16_t maValues[numMaSets][maLength];
static int32_t maSums[numMaSets];
static uint8_t maNextSlotIdx[numMaSets];
static bool maSetFull[numMaSets];

static volatile bool gotSoundDetectedWakeupInterrupt;

static int32_t nextGatherMeasurementsMs;

static int32_t nextTxMs;
static uint32_t txInterval;

static uint32_t lastActiveMs;       // when activity was last detected
static uint32_t hangTimeEndsMs;     // when the current hang time ends; 0 means not set

static uint8_t stayAwakeCountdown;


/******************
 * Implementation *
 ******************/


//=-=-=-=-=-=-=-=-=-=-==-=-=

ISR(WDT_vect) {
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
}


// TODO:  Use a comparator to generate an interrupt when sound is detected.

void handleSoundDetectedWakeupInterrupt()
{
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
  gotSoundDetectedWakeupInterrupt = true;
}


bool widgetWake()
{
  // Returns true if the widget should stay awake
  // or false if it should continue to sleep.

  uint32_t now = millis();

  if (!gotSoundDetectedWakeupInterrupt) {
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

  gotSoundDetectedWakeupInterrupt = false;

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

  gotSoundDetectedWakeupInterrupt = false;

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


void setWidgetMode(WidgetMode newMode, uint32_t now)
{
  bool stayAwake;

  switch (newMode) {

    case WidgetMode::standby:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to standby."));
#endif
      digitalWrite(PIN_SOUND_ACTIVE_LED, SOUND_ACTIVE_LED_OFF);
      digitalWrite(PIN_MSGEQ7_VDD, LOW);    // Turn off the MSGEQ7.
      wdt_reset();
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      stayAwake = false;
      while (!stayAwake) {
        // widgetSleep sets widgetMode to standby at the appropriate
        // point.  It returns after we sleep then wake up.
        widgetSleep();
        stayAwake = widgetWake();
      }
      digitalWrite(PIN_MSGEQ7_VDD, HIGH);   // Turn on the MSGEQ7.
      widgetMode = WidgetMode::inactive;
      break;

    case WidgetMode::inactive:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to inactive."));
#endif
      digitalWrite(PIN_SOUND_ACTIVE_LED, SOUND_ACTIVE_LED_OFF);
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
      digitalWrite(PIN_SOUND_ACTIVE_LED, SOUND_ACTIVE_LED_ON);
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


void clearMovingAverages()
{
  for (uint8_t i = 0; i < numMaSets; ++i) {
    for (uint8_t j = 0; j < maLength; ++j) {
      maValues[i][j] = 0;
    }
    maSums[i] = 0;
    maNextSlotIdx[i] = 0;
    maSetFull[i] = false;
  }
}


void updateMovingAverage(uint8_t setIdx, int16_t newValue)
{
  maSums[setIdx] -= maValues[setIdx][maNextSlotIdx[setIdx]];
  maSums[setIdx] += newValue;
  maValues[setIdx][maNextSlotIdx[setIdx]] = newValue;

  ++maNextSlotIdx[setIdx];
  if (maNextSlotIdx[setIdx] >= maLength) {
     maSetFull[setIdx] = true;
     maNextSlotIdx[setIdx] = 0;
  }
}


int16_t getMovingAverage(uint8_t setIdx)
{
  int32_t avg;
  if (maSetFull[setIdx]) {
    avg = maSums[setIdx] / (int32_t) maLength;
  }
  else {
    avg = maNextSlotIdx[setIdx] > 0 ? (int32_t) maSums[setIdx] / (int32_t) maNextSlotIdx[setIdx] : 0;
  }

//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("getMovingAverage(");
//  Serial.print(setIdx);
//  Serial.print("):  ");
//  for (uint8_t i = 0; i < (maSetFull[setIdx] ? maLength : maNextSlotIdx[setIdx]); ++i) {
//    Serial.print(i);
//    Serial.print(":");
//    Serial.print(maValues[setIdx][i]);
//    Serial.print("  ");
//  }
//  Serial.print(maSums[setIdx]);
//  Serial.print("  ");
//  Serial.println(avg);
//#endif

  return avg;
}


bool detectMovingAverageChange(uint8_t setIdx, int16_t threshold)
{
  uint8_t latestSlotIdx;
  if (maNextSlotIdx[setIdx] == 0) {
    if (!maSetFull[setIdx]) {
      return false;
    }
    latestSlotIdx = maLength - 1;
  }
  else {
    latestSlotIdx = maNextSlotIdx[setIdx] - 1;
  }
  int16_t diff = maValues[setIdx][maNextSlotIdx[setIdx]] - maValues[setIdx][latestSlotIdx];
  return abs(diff) > threshold ? true : false;
}


void gatherMeasurements(uint32_t now)
{
  msgeq7.reset();
  msgeq7.read();

  bool anyBandIsActive = false;
  for (int i = 0; i < numMsgeq7Bands; ++i) {
    updateMovingAverage(i, msgeq7.get(i));
    if (getMovingAverage(i) > activityDetectionThreshold) {
      anyBandIsActive = true;
    }
  }
  if (anyBandIsActive) {
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

#ifdef TX_INDICATOR_LED_PIN
  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_ON);
#endif

  for (int i = 0; i < numMaSets; ++i) {
    payload.measurements[i] = getMovingAverage(i);
  }
  payload.widgetHeader.isActive = (widgetMode == WidgetMode::active);

//#ifdef ENABLE_DEBUG_PRINT
//  for (int i = 0; i < numMaSets; ++i) {
//    Serial.print(i);
//    Serial.print(":  ");
//    Serial.println(payload.measurements[i]);
//  }
//#endif

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * numMaSets, !WANT_ACK)) {
#ifdef PIN_TX_FAILURE_LED
    digitalWrite(PIN_TX_FAILURE_LED, TX_FAILURE_LED_ON);
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("tx failed."));
#endif
#endif
  }
  else {
#ifdef PIN_TX_FAILURE_LED
    digitalWrite(PIN_TX_FAILURE_LED, TX_FAILURE_LED_OFF);
#endif
  }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef PIN_SOUND_DETECTED_WAKEUP
  pinMode(PIN_SOUND_DETECTED_WAKEUP, INPUT);
#endif

#ifdef PIN_SOUND_ACTIVE_LED
  pinMode(PIN_SOUND_ACTIVE_LED, OUTPUT);
  digitalWrite(PIN_SOUND_ACTIVE_LED, SOUND_ACTIVE_LED_OFF);
#endif

#ifdef PIN_TX_FAILURE_LED
  pinMode(PIN_TX_FAILURE_LED, OUTPUT);
  digitalWrite(PIN_TX_FAILURE_LED, TX_FAILURE_LED_OFF);
#endif

#ifdef PIN_MSGEQ7_VDD
  pinMode(PIN_MSGEQ7_VDD, OUTPUT);
  digitalWrite(PIN_MSGEQ7_VDD, HIGH);
#endif

  msgeq7.begin();

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
  payload.widgetHeader.channel = 0;

  setWidgetMode(WidgetMode::inactive, millis());

  // Set up and turn on the pin-change interrupts last.
#ifdef PIN_SOUND_DETECTED_WAKEUP
  pinMode(PIN_SOUND_DETECTED_WAKEUP, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SOUND_DETECTED_WAKEUP),
                  handleSoundDetectedWakeupInterrupt,
                  SOUND_DETECTED_INTERRUPT_MODE);
#endif
}


void loop()
{
// =-=-=-= TODO:  make sure signed/unsigned next-event stuff works.  Maybe put that shit in macros.

  static int32_t lastTxMs;

  uint32_t now = millis();

  if ((int32_t) (now - nextGatherMeasurementsMs) >= 0) {
    // TODO:  really should be nextGatherMeasurementsMs += gatherMeasurementsIntervalMs to avoid drift
    nextGatherMeasurementsMs = now + gatherMeasurementsIntervalMs;
    gatherMeasurements(now);
  }

  if ((int32_t) (now - nextTxMs) >= 0) {
    // TODO:  really should be nextTxMs += txInterval to avoid drift
    nextTxMs = now + txInterval;
    sendMeasurements(now);
  }

#ifdef ENABLE_STANDBY_MODE
  if (widgetMode == WidgetMode::inactive
      && now - lastActiveMs >= inactivityTimeoutForSleepMs)
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Going standby because no sound above activiy threshold was detected from "));
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

//=-=-=-=-=-=-=-=-=-=-==-=-=


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(57600);
  printf_begin();
#endif

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastSampleMs;
  static uint16_t numSamples;
  static int32_t pressureMeasmtSum;

  uint32_t now = millis();

  if (now - lastSampleMs >= PRESSURE_SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    // If we're inactive, don't average because we want to react as fast as
    // possible to pumping.  Hopefully, noise won't poke above the threshold.
    if (!isActive) {
      numSamples = 1;
      pressureMeasmtSum = analogRead(PRESSURE_SENSOR_SIGNAL_PIN);
    }
    else {
      ++numSamples;
      pressureMeasmtSum += analogRead(PRESSURE_SENSOR_SIGNAL_PIN);
    }
  }

  if (numSamples > 0 && now - lastTxMs >= ACTIVE_TX_INTERVAL_MS) {
    int16_t avgPressure = pressureMeasmtSum / numSamples;
    isActive = avgPressure > activePressureThreshold;
    if (isActive || wasActiveCountdown > 0 || now - lastTxMs >= INACTIVE_TX_INTERVAL_MS) {
      lastTxMs = now;

      payload.position = avgPressure;
      payload.velocity = numSamples;
      payload.widgetHeader.isActive = isActive;
      radio.write(&payload, sizeof(payload), !WANT_ACK);

      numSamples = 0;
      pressureMeasmtSum = 0L;

      if (isActive) {
        wasActiveCountdown = numInactiveSendTries;
      }
      else {
        if (wasActiveCountdown > 0) {
          --wasActiveCountdown;
        }
      }
    }
  }

}
