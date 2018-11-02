/*****************************************************************
 *                                                               *
 * Rainstick Widget                                              *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, February 2017                             )'( *
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


// The watchdog doesn't work correctly on Pro Mini boards.  See
// https://andreasrohner.at/posts/Electronics/How-to-make-the-Watchdog-Timer-work-on-an-Arduino-Pro-Mini-by-replacing-the-bootloader/
//#define ENABLE_WATCHDOG

//#define ENABLE_DEBUG_PRINT


#include <avr/sleep.h>
#ifdef ENABLE_WATCHDOG
#include <avr/wdt.h>
#endif

#include "I2Cdev.h"

#include "illumiconeWidget.h"

#include "MPU6050_6Axis_MotionApps20.h"

#ifdef ENABLE_DEBUG_PRINT
#include "printf.h"
#endif

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif


/****************************
 * Constants and Data Types *
 ****************************/

enum class MpuMode {
  init,
  cycle,
  normal
};

enum class WidgetMode {
  init,
  standby,
  inactive,
  active
};


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 4

#define SOUND_SAMPLE_INTERVAL_MS 10L
#define SOUND_SAMPLE_SAVE_INTERVAL_MS 50L   // same as 200 Hz IMU sample frequency so MA length works for both
#define ACTIVE_TX_INTERVAL_MS 100L
#define INACTIVE_TX_INTERVAL_MS 500L
#define STANDBY_TX_INTERVAL_MS 5000L

// There must be at least YPR_MOTION_CHANGE_THRESHOLD tenths of a degree
// motion in yaw, pitch, or roll every MOTION_TIMEOUT_MS ms to keep us
// out of standby mode.
#define MOTION_TIMEOUT_MS 5000L
#define YPR_MOTION_CHANGE_THRESHOLD 1

// When we're not using the watchdog, we use the time elapsed since getting
// good data from the MPU-6050 to determine if we need to re-init the little
// bastard because he's quit working right.  MPU_ASSUMED_DEAD_MS should be
// less than MOTION_TIMEOUT_MS so that we re-init the MPU-6050 rather than
// putting it in cycle mode when we're not getting good data from it.
#define MPU_ASSUMED_DEAD_MS 3000

#define TX_INDICATOR_LED_PIN LED_BUILTIN
#define TX_INDICATOR_LED_ON HIGH
#define TX_INDICATOR_LED_OFF LOW
#define IMU_INTERRUPT_PIN 2
// --- the real Rainstick ---
//#define MIC_SIGNAL_PIN A0
//#define MIC_POWER_PIN 8
// --- development breadboard ---
#define MIC_SIGNAL_PIN A3
#define MIC_POWER_PIN 4

#define NUM_SOUND_VALUES_TO_SEND 3
#define NUM_MPU_VALUES_TO_SEND 6
#define NUM_MA_SETS (NUM_SOUND_VALUES_TO_SEND + NUM_MPU_VALUES_TO_SEND)
#define MA_LENGTH 8

constexpr uint16_t activeSoundThreshold = 100;

// When we haven't retrieved packets from the MPU6050's FIFO fast enough data,
// corruption becomes likely even before the FIFO overflows.  We'll clear the
// FIFO when more than maxPacketsInFifoBeforeForcedClear packets are in it.
constexpr uint8_t maxPacketsInFifoBeforeForcedClear = 2;


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "2wdgt"

#define WANT_ACK false

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11, 13) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 0 // 5

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 0  // 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

// standby/active control
static WidgetMode widgetMode = WidgetMode::init;
static MpuMode mpuMode = MpuMode::init;
static uint32_t lastMotionDetectedMs;

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static MeasurementVectorPayload payload;

static uint16_t minSoundSample = UINT16_MAX;
static uint16_t maxSoundSample;

static int16_t avgMinSoundSample;
static int16_t avgMaxSoundSample;
static int16_t ppSoundSample;

static int16_t maValues[NUM_MA_SETS][MA_LENGTH];
static int32_t maSums[NUM_MA_SETS];
static uint8_t maNextSlotIdx[NUM_MA_SETS];
static bool maSetFull[NUM_MA_SETS];

static MPU6050 mpu;               // using default I2C address 0x68

// MPU FIFO read buffer
static uint16_t packetSize;       // expected DMP packet size (default is 42 bytes)
static uint8_t packetBuffer[42];  // must be at least as large as packet size returned by dmpGetFIFOPacketSize

// orientation/motion vars
static Quaternion quat;           // [w, x, y, z] quaternion container
static VectorFloat gravity;       // [x, y, z] gravity vector
static float ypr[3];              // [yaw, pitch, roll] yaw/pitch/roll container
static int16_t gyro[3];

static volatile bool gotMpuInterrupt;

static int32_t nextTxMs;
static int32_t lastSoundSampleMs;
static int32_t lastSoundSampleSaveMs;
static int32_t lastSuccessfulMpuReadMs;

static uint32_t txInterval;


/******************
 * Implementation *
 ******************/

// TODO:  Move the moving average stuff to a library.
void clearMovingAverages();
void updateMovingAverage(uint8_t setIdx, int16_t newValue);
int16_t getMovingAverage(uint8_t setIdx);
bool detectMovingAverageChange(uint8_t setIdx, int16_t threshold);


// top half of the MPU ISR (bottom half is processMpuInterrupt)
void handleMpuInterrupt() {

  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }

  gotMpuInterrupt = true;
}


void widgetWake()
{
  // Reinitialize all the time trackers because the ms timer has been off.
  // Time in this little world has stood still while time in the default
  // world marched on, so we need transmit and gather data ASAP.
  uint32_t now = millis();
  nextTxMs = now;
  lastSoundSampleMs = now;
  lastSoundSampleSaveMs = now;
  lastSuccessfulMpuReadMs = now;

  clearMovingAverages();
}


void widgetSleep()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // We don't want interrupts duing the timing-critical stuff below,
  // and we don't want the ISR to disable sleep before we go to sleep.
  noInterrupts();

  // The interrupt is already attached because it is the interrupt the MPU uses.

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
  switch (newMode) {

    case WidgetMode::standby:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Widget mode changing to standby."));
#endif
      // widgetSleep returns after we sleep then wake up.
      widgetSleep();
      widgetWake();
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


void setMpuMode(MpuMode newMode, uint32_t now)
{
  switch (newMode) {

    case MpuMode::cycle:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Setting mpuMode to cycle..."));
#endif
      mpu.setDMPEnabled(false);
      // TODO:  motion detection should be configurable
      // Set up motion detection.
      mpu.setMotionDetectionThreshold(1);         // unit is 2mg
      mpu.setMotionDetectionCounterDecrement(1);
      mpu.setMotionDetectionDuration(1);          // unit is ms
      // Put MPU-6050 in cycle mode.
      mpuMode = MpuMode::cycle;
      // TODO:  wake frequency should be configurable
      mpu.setWakeFrequency(0);                    // 0 = 1.25 Hz, 1 = 2.5 Hz, 2 - 5 Hz, 3 = 10 Hz
      mpu.setWakeCycleEnabled(true);
      mpu.setIntMotionEnabled(true);
      break;

    case MpuMode::normal:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Setting mpuMode to normal..."));
#endif
      mpu.setIntMotionEnabled(false);
      mpu.setWakeCycleEnabled(false);
      clearMovingAverages();
      mpuMode = MpuMode::normal;
      mpu.setDMPEnabled(true);
      lastMotionDetectedMs = now;
      lastSuccessfulMpuReadMs = now;
      break;

    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("*** Invalid newMode in setMpuMode"));
#endif
      break;
  }
}


void initI2c()
{
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("initI2c:  I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE"));
#endif
  Wire.begin();
  TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("initI2c:  I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE"));
#endif
  Fastwire::setup(400, true);
#endif
}


void initMpu()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Initializing MPU6050..."));
#endif
  mpu.initialize();

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Testing MPU6050 connection..."));
#endif
  if (mpu.testConnection()) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("MPU6050 connection successful.  Initializing DMP..."));
#endif
    uint8_t devStatus = mpu.dmpInitialize();
    if (devStatus == 0) {

      // supply your own gyro offsets here, scaled for min sensitivity
      // TODO 2/28/2018 ross:  What do we do about this?  Every widget could be different.
      //mpu.setXGyroOffset(220);
      //mpu.setYGyroOffset(76);
      //mpu.setZGyroOffset(-85);
      //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Enabling interrupt..."));
#endif
      attachInterrupt(digitalPinToInterrupt(IMU_INTERRUPT_PIN), handleMpuInterrupt, RISING);

      // Get expected DMP packet size, and make sure packetBuffer is large enough.
      packetSize = mpu.dmpGetFIFOPacketSize();
      if (sizeof(packetBuffer) >= packetSize) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("DMP ready."));
#endif
        setMpuMode(MpuMode::normal, millis());
      }
      else {
        Serial.print(F("*** FIFO packet size "));
        Serial.print(packetSize);
        Serial.print(F(" is larger than packetBuffer size "));
        Serial.println(sizeof(packetBuffer));
      }
    }
    else {
      // Well, shit.
      // 1 = initial memory load failed (most likely)
      // 2 = DMP configuration updates failed
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("*** DMP Initialization failed.  devStatus="));
      Serial.println(devStatus);
#endif
    }
  }
  else {
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("MPU6050 connection failed."));
#endif
  }

  setWidgetMode(WidgetMode::inactive, millis());
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

  pinMode(TX_INDICATOR_LED_PIN, OUTPUT);
  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_OFF);

  initI2c();
  initMpu();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL, WANT_ACK);

  pinMode(MIC_POWER_PIN, OUTPUT);
  digitalWrite(MIC_POWER_PIN, HIGH);

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

  // Communication with the MPU6050 has proven to be problematic.
  // If we don't hear from the unit periodically, or if we don't
  // get good data for a while, we need the watchdog to reset us.
#ifdef ENABLE_WATCHDOG
  wdt_enable(WDTO_1S);     // enable the watchdog
#endif
}


void clearMovingAverages()
{
  for (uint8_t i = 0; i < NUM_MA_SETS; ++i) {
    for (uint8_t j = 0; j < MA_LENGTH; ++j) {
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
  if (maNextSlotIdx[setIdx] >= MA_LENGTH) {
     maSetFull[setIdx] = true;
     maNextSlotIdx[setIdx] = 0;
  }
}


int16_t getMovingAverage(uint8_t setIdx)
{
  int32_t avg;
  if (maSetFull[setIdx]) {
    avg = maSums[setIdx] / (int32_t) MA_LENGTH;
  }
  else {
    avg = maNextSlotIdx[setIdx] > 0 ? (int32_t) maSums[setIdx] / (int32_t) maNextSlotIdx[setIdx] : 0;
  }

//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("getMovingAverage(");
//  Serial.print(setIdx);
//  Serial.print("):  ");
//  for (uint8_t i = 0; i < (maSetFull[setIdx] ? MA_LENGTH : maNextSlotIdx[setIdx]); ++i) {
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
    latestSlotIdx = MA_LENGTH - 1;
  }
  else {
    latestSlotIdx = maNextSlotIdx[setIdx] - 1;
  }
  int16_t diff = maValues[setIdx][maNextSlotIdx[setIdx]] - maValues[setIdx][latestSlotIdx];
  return abs(diff) > threshold ? true : false;
}


void clearMpuFifo()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Clearing FIFO..."));
#endif

  uint16_t fifoCount = mpu.getFIFOCount();
  while (fifoCount != 0) {
    uint8_t readLength = fifoCount >= packetSize ? packetSize : fifoCount;
    mpu.getFIFOBytes(packetBuffer, readLength);
    //fifoCount = mpu.getFIFOCount();
    fifoCount -= readLength;
  }

#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Cleared FIFO."));
#endif
}


void gatherMotionMeasurements(uint32_t now)
{
  uint16_t fifoCount = mpu.getFIFOCount();
  while (fifoCount >= packetSize) {
    mpu.getFIFOBytes(packetBuffer, packetSize);
    fifoCount -= packetSize;

//#ifdef ENABLE_DEBUG_PRINT
//    // Careful:  We might not be able to keep up if this debug print is enabled.
//    Serial.print(F("Got packet from fifo.  fifoCount now "));
//    Serial.println(fifoCount);
//#endif

    mpu.dmpGetGyro(gyro, packetBuffer);
    mpu.dmpGetQuaternion(&quat, packetBuffer);
    mpu.dmpGetGravity(&gravity, &quat);
    mpu.dmpGetYawPitchRoll(ypr, &quat, &gravity);

    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND    , ypr[0] * (float) 1800 / M_PI);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 1, ypr[1] * (float) 1800 / M_PI);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 2, ypr[2] * (float) 1800 / M_PI);

    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 3, gyro[0]);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 4, gyro[1]);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 5, gyro[2]);

    // If we're here and we got non-zero data, communication
    // with the MPU6050 is probably working.
    bool gotNonzeroData = false;
    for (uint8_t i = 0; i < 3; ++i) {
      if (ypr[i] > 0.001 || gyro[i] != 0) {
        gotNonzeroData = true;
        break;
      }
    }
    if (gotNonzeroData) {
#ifdef ENABLE_WATCHDOG
      wdt_reset();
#else
      lastSuccessfulMpuReadMs = now;
#endif
    }

  // If there was sufficient motion, keep us out of standby mode for now.
  for (uint8_t i = NUM_SOUND_VALUES_TO_SEND; i <= NUM_SOUND_VALUES_TO_SEND + 2; ++i) {
    if (detectMovingAverageChange(i, YPR_MOTION_CHANGE_THRESHOLD)) {
      lastMotionDetectedMs = now;
      break;
    }
  }

//#ifdef ENABLE_DEBUG_PRINT
//      // Careful:  We might not be able to keep up if this debug print is enabled.
//    Serial.print("ypr:  ");
//    Serial.print(ypr[0]);
//    Serial.print(", ");
//    Serial.print(ypr[1]);
//    Serial.print(", ");
//    Serial.print(ypr[2]);
//    Serial.print("    gyro:  ");
//    Serial.print(gyro[0]);
//    Serial.print(", ");
//    Serial.print(gyro[1]);
//    Serial.print(", ");
//    Serial.println(gyro[2]);
//#endif
  }
}


void processMpuInterrupt(uint32_t now)
{
  static bool needClearMpuFifo;
  uint16_t fifoCount;

  uint8_t mpuIntStatus = mpu.getIntStatus();
//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("0x");
//  Serial.print((int) mpuIntStatus, HEX);
//  Serial.print(" ");
//#endif

  // Frequently, the interrupt status is zero by the time we're processing
  // the interrupt.  Why that happens has not yet been determined.  If it
  // is zero, there's nothing we can do because we don't know what the
  // interrupt was for.
  if (mpuIntStatus == 0) {
    return;
  }

  switch (mpuMode) {

    case MpuMode::init:
      // Don't do anything because the MPU isn't ready yet.
      break;

    case MpuMode::cycle:
      if (mpuIntStatus & 0x40) {
        setMpuMode(MpuMode::normal, now);
        clearMpuFifo();
      }
      break;

    case MpuMode::normal:

      fifoCount = mpu.getFIFOCount();

      // Check if FIFO overflowed.  If it did, clear it, then wait for another
      // data-ready interrupt and clear the FIFO again so that we are back in
      // sync (i.e., any partial packets have been cleared out).  Based on some
      // brief testing, it appears that bits 0 and 1 always appear set together.
      if (mpuIntStatus & 0x10) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("*** FIFO overflow!  fifoCount is "));
        Serial.println(fifoCount);
#endif
        clearMpuFifo();
        needClearMpuFifo = true;    // clear it again after next interrupt to get in sync
        return;
      }

      // The MPU6050 register map document says that bit 0 indicates data ready and bit 1
      // is reserved.  However, the I2C data analyzer dump from Jeff Rowberg found at
      // https://www.i2cdevlib.com/tools/analyzer/1 shows that bit 0 indicates raw data
      // ready and bit 1 indicates DMP data ready.
      if (!(mpuIntStatus & 0x02)) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("Got interrupt but not for data ready.  mpuIntStatus=0x"));
        Serial.print((int) mpuIntStatus, HEX);
        Serial.print(F(", fifoCount="));
        Serial.println(fifoCount);
#endif
        return;
      }

      if (needClearMpuFifo) {
        needClearMpuFifo = false;
        clearMpuFifo();
        return;
      }

      // If we've missed retrieving more than a few packets in time, the FIFO has
      // probably already overflowed (even though we haven't gotten that interrupt)
      // or otherwise become corrupted.  We need to reset it to avoid getting bad data.
      if (fifoCount > packetSize * maxPacketsInFifoBeforeForcedClear) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("*** Missed too many packets.  fifoCount="));
        Serial.println(fifoCount);
#endif
        clearMpuFifo();
        return;
      }

      // If the FIFO length is not a multiple of the packet size, there is a partial
      // packet in the FIFO, either due to the FIFO being filled right now or due to some
      // sort of FIFO corruption.  We need to clear the FIFO to avoid getting bad data.
      if (fifoCount % packetSize != 0) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("////////// *** Partial packet in FIFO.  fifoCount="));
        Serial.println(fifoCount);
#endif
        clearMpuFifo();
        needClearMpuFifo = true;    // clear it again after next interrupt to get in sync
        return;
      }

//#ifdef ENABLE_DEBUG_PRINT
//      Serial.print(F("Got data ready interrupt 0x"));
//      Serial.print((int) mpuIntStatus, HEX);
//      Serial.print(F(", fifoCount="));
//      Serial.println(fifoCount);
//#endif
      gatherMotionMeasurements(now);
      break;
  }
}


void sampleSound()
{
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.println(F("sampleSound"));
//#endif
  uint16_t soundSample = analogRead(MIC_SIGNAL_PIN);
  if (soundSample < minSoundSample) {
    minSoundSample = soundSample;
  }
  if (soundSample > maxSoundSample) {
    maxSoundSample = soundSample;
  }
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("soundSample="));
//    Serial.print(soundSample);
//    Serial.print(F(" minSoundSample="));
//    Serial.print(minSoundSample);
//    Serial.print(F(" maxSoundSample="));
//    Serial.println(maxSoundSample);
//#endif
}


void saveSoundSample(uint32_t now)
{
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("saveSoundSample:  minSoundSample="));
//    Serial.print(minSoundSample);
//    Serial.print(F(" maxSoundSample="));
//    Serial.println(maxSoundSample);
//#endif

  updateMovingAverage(0, minSoundSample);
  updateMovingAverage(1, maxSoundSample);

  minSoundSample = UINT16_MAX;
  maxSoundSample = 0;

  avgMinSoundSample = getMovingAverage(0);
  avgMaxSoundSample = getMovingAverage(1);
  ppSoundSample = avgMaxSoundSample - avgMinSoundSample;
  if (ppSoundSample > activeSoundThreshold) {
    if (widgetMode == WidgetMode::inactive) {
      setWidgetMode(WidgetMode::active, now);
    }
  }
  else {
    if (widgetMode == WidgetMode::active) {
      setWidgetMode(WidgetMode::inactive, now);
    }
  }

//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("avgMinSoundSample="));
//    Serial.print(avgMinSoundSample);
//    Serial.print(F(" avgMaxSoundSample="));
//    Serial.println(avgMaxSoundSample);
//    Serial.print(F(" ppSoundSample="));
//    Serial.println(ppSoundSample);
//#endif
}


void sendMeasurements()
{
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("send"));
#endif

  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_ON);

  payload.measurements[0] = avgMinSoundSample;
  payload.measurements[1] = avgMaxSoundSample;
  payload.measurements[2] = ppSoundSample;

  // Place the MPU average values after the sound values.
  for (int i = 0, j = NUM_SOUND_VALUES_TO_SEND; i < NUM_MPU_VALUES_TO_SEND; ++i, ++j) {
    payload.measurements[j] = getMovingAverage(j);
  }

  payload.widgetHeader.isActive = widgetMode == WidgetMode::active;

#ifdef ENABLE_DEBUG_PRINT
  for (int i = 0; i < NUM_MPU_VALUES_TO_SEND + NUM_SOUND_VALUES_TO_SEND; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(payload.measurements[i]);
  }
#endif

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * (NUM_MPU_VALUES_TO_SEND + NUM_SOUND_VALUES_TO_SEND))) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }

  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_OFF);
}


void loop()
{
  static bool wasActive;

  uint32_t now = millis();

#ifdef ENABLE_WATCHDOG
  // We don't get data periodically from the MPU when it
  // is in cycle mode, so we need to kick the dog here.
  if (mpuMode == MpuMode::cycle) {
    wdt_reset();
  }
#else
  // If we're not using the watchdog, we need to reset the MPU
  // if we haven't received any data from it for a while
  // (because it has probably gone out to lunch).
  if (mpuMode == MpuMode::normal && now - lastSuccessfulMpuReadMs >= MPU_ASSUMED_DEAD_MS) {
    mpuMode = MpuMode::init;
    initMpu();
  }
#endif

  if (gotMpuInterrupt) {
    gotMpuInterrupt = false;
    processMpuInterrupt(now);
  }

  if (now - lastSoundSampleMs >= SOUND_SAMPLE_INTERVAL_MS) {
    lastSoundSampleMs = now;
    sampleSound();
  }

  if (now - lastSoundSampleSaveMs >= SOUND_SAMPLE_SAVE_INTERVAL_MS) {
    lastSoundSampleSaveMs = now;
    saveSoundSample(now);
  }

  if ((int32_t) (now - nextTxMs) >= 0) {
    nextTxMs = now + txInterval;
    sendMeasurements();
  }

  if (widgetMode == WidgetMode::inactive
      && mpuMode == MpuMode::normal
      && now - lastMotionDetectedMs >= MOTION_TIMEOUT_MS)
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Going standby because no motion from "));
    Serial.print(lastMotionDetectedMs);
    Serial.print(F(" to "));
    Serial.println(now);
#endif
    setMpuMode(MpuMode::cycle, now);
    setWidgetMode(WidgetMode::standby, now);
    // Setting the widget mode to standby will put the processor to sleep.
    // It wakes when it gets a motion detection interrupt from the MPU.
    // Sometime after that, setWidgetMode returns, and execution resumes here.
    // The world may be a different place then.  Just thought you should know.
  }
}

