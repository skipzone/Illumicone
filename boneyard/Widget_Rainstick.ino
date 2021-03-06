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

//#define ENABLE_DEBUG_PRINT


#include <avr/sleep.h>
#include <avr/wdt.h>

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

enum class Mpu6050Mode {
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
#define SOUND_SAVE_INTERVAL_MS 50L      // same as 200 Hz IMU sample frequency so MA length works for both
#define ACTIVE_TX_INTERVAL_MS 200L
#define INACTIVE_TX_INTERVAL_MS 2000L

// In standby mode, we'll transmit a packet with zero-valued data
// approximately every STANDBY_TX_INTERVAL_S seconds.  Wake-ups
// occur at 8-second intervals, so STANDBY_TX_INTERVAL_S should
// be a multiple of 8 between 8 and 2040, inclusive.
#define STANDBY_TX_INTERVAL_S 64

// There must be at least YPR_MOTION_CHANGE_THRESHOLD tenths of a degree
// motion in yaw, pitch, or roll every MOTION_TIMEOUT_MS ms to keep us
// out of standby mode.
#define MOTION_TIMEOUT_MS 10000L
#define YPR_MOTION_CHANGE_THRESHOLD 1

// When we're not using the watchdog, we use the time elapsed since getting
// good data from the MPU-6050 to determine if we need to re-init the little
// bastard because he's quit working right.  MPU6050_ASSUMED_DEAD_MS should be
// less than MOTION_TIMEOUT_MS so that we re-init the MPU-6050 rather than
// putting it in cycle mode when we're not getting good data from it.
#define MPU6050_ASSUMED_DEAD_MS 3000

//#define TX_INDICATOR_LED_PIN 16
//#define TX_INDICATOR_LED_ON HIGH
//#define TX_INDICATOR_LED_OFF LOW
//#define IMU_NORMAL_INDICATOR_LED_PIN 16
//#define IMU_NORMAL_INDICATOR_LED_ON HIGH
//#define IMU_NORMAL_INDICATOR_LED_OFF LOW
#define IMU_INTERRUPT_PIN 2
// --- the real Rainstick ---
#define MIC_SIGNAL_PIN A0
#define MIC_POWER_PIN 8
// --- development breadboard ---
//#define MIC_SIGNAL_PIN A3
//#define MIC_POWER_PIN 4

// moving average lengthf for averaging sound and IMU measurements
#define MA_LENGTH 8

constexpr uint16_t activeSoundThreshold = 100;

// When we aren't retrieving packets from the MPU-6050's FIFO fast enough, FIFO overflow
// and resulting data corruption become likely.  To help prevent that, we'll clear the
// FIFO when more than maxPacketsInMpu6050FifoBeforeForcedClear packets are in it.
constexpr uint8_t maxPacketsInMpu6050FifoBeforeForcedClear = 2;

// ---------- radio configuration ----------

// Nwdgt, where N indicates the payload type (0: stress test; 1: position
// and velocity; 2: measurement vector; 3,4: undefined; 5: custom)
#define TX_PIPE_ADDRESS "2wdgt"

// Set WANT_ACK to false, TX_RETRY_DELAY_MULTIPLIER to 0, and TX_MAX_RETRIES
// to 0 for fire-and-forget.  To enable retries and delivery failure detection,
// set WANT_ACK to true.  The delay between retries is 250 us multiplied by
// TX_RETRY_DELAY_MULTIPLIER.  To help prevent repeated collisions, use 1, a
// prime number (2, 3, 5, 7, 11, 13), or 15 (the maximum) for TX_MAX_RETRIES.
#define WANT_ACK false
#define TX_RETRY_DELAY_MULTIPLIER 0     // use 5 when getting acks
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
#define RF_CHANNEL 84

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

static WidgetMode widgetMode = WidgetMode::init;
static Mpu6050Mode mpu6050Mode = Mpu6050Mode::init;

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static MeasurementVectorPayload payload;

static uint16_t minSoundSample = UINT16_MAX;
static uint16_t maxSoundSample;

// TODO:  Implement isImuActive for BoogieBoard.
// static bool isImuActive;
static bool isSoundActive;

static constexpr uint8_t maSlotYaw = 0;
static constexpr uint8_t maSlotPitch = 1;
static constexpr uint8_t maSlotRoll = 2;
static constexpr uint8_t maSlotGyroX = 3;
static constexpr uint8_t maSlotGyroY = 4;
static constexpr uint8_t maSlotGyroZ = 5;
static constexpr uint8_t maSlotAccelX = 6;
static constexpr uint8_t maSlotAccelY = 7;
static constexpr uint8_t maSlotAccelZ = 8;
static constexpr uint8_t maSlotLinearAccelX = 9;
static constexpr uint8_t maSlotLinearAccelY = 10;
static constexpr uint8_t maSlotLinearAccelZ = 11;
static constexpr uint8_t maSlotPpSound = 12;

static constexpr uint8_t firstYprMaSlot = maSlotYaw;
static constexpr uint8_t lastYprMaSlot = maSlotRoll;

static constexpr uint8_t numMaSets = 13;
static int16_t maValues[numMaSets][MA_LENGTH];
static int32_t maSums[numMaSets];
static uint8_t maNextSlotIdx[numMaSets];
static bool maSetFull[numMaSets];

static MPU6050 mpu6050;           // using default I2C address 0x68

// MPU-6050 FIFO read buffer
static uint16_t packetSize;       // expected DMP packet size (default is 42 bytes)
static uint8_t packetBuffer[42];  // must be at least as large as packet size returned by dmpGetFIFOPacketSize

static volatile bool gotMpu6050Interrupt;

static int32_t nextTxMs;
static int32_t lastSoundSampleMs;
static int32_t lastSoundSaveMs;
static int32_t lastSuccessfulMpu6050ReadMs;
static uint32_t lastMotionDetectedMs;

static uint32_t txInterval;

static uint8_t stayAwakeCountdown;


/******************
 * Implementation *
 ******************/

// TODO:  Move the moving average stuff to a library.
void clearMovingAverages();
void updateMovingAverage(uint8_t setIdx, int16_t newValue);
int16_t getMovingAverage(uint8_t setIdx);
bool detectMovingAverageChange(uint8_t setIdx, int16_t threshold);


ISR(WDT_vect) {
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
}


// top half of the MPU-6050 ISR (bottom half is processMpu6050Interrupt)
void handleMpu6050Interrupt() {
  if (widgetMode == WidgetMode::standby) {
    sleep_disable();
  }
  gotMpu6050Interrupt = true;
}


bool widgetWake()
{
  uint32_t now = millis();

  if (!gotMpu6050Interrupt) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Woken up by watchdog."));
#endif
    if (--stayAwakeCountdown == 0) {
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      // Let the pattern controller know we're still alive.
      // (The data sent will be all zeros because the averages
      // were cleared before we went to sleep.)
      sendMeasurements();
    }
    return false;
  }

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Woken up by MPU-6050."));
#endif

  // Reinitialize all the time trackers because the ms timer has been off.
  // Time in this little world has stood still while time in the default
  // world marched on, so we need transmit and gather data ASAP.
  nextTxMs = now;
  lastSoundSampleMs = now;
  lastSoundSaveMs = now;
  lastMotionDetectedMs = now;
  lastSuccessfulMpu6050ReadMs = now;

  isSoundActive = false;

  return true;
}


void widgetSleep()
{
  // After we wake up, the averages will be stale.  Clear them now
  // so that the periodic transmissions caused by watchdog wakeup
  // will send zero values.
  clearMovingAverages();
  
  // We don't want interrupts duing the timing-critical stuff below,
  // and we don't want the ISR to disable sleep before we go to sleep.
  noInterrupts();

  // Ignore an IMU interrupt that hasn't been serviced yet.
  gotMpu6050Interrupt = false;

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
      digitalWrite(MIC_POWER_PIN, LOW);
      setMpu6050Mode(Mpu6050Mode::cycle, now);
      wdt_reset();
      stayAwakeCountdown = STANDBY_TX_INTERVAL_S / 8;
      stayAwake = false;
      while (!stayAwake) {
        // widgetSleep returns after we sleep then wake up.
        widgetSleep();
        stayAwake = widgetWake();
      }
      digitalWrite(MIC_POWER_PIN, HIGH);
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
      digitalWrite(MIC_POWER_PIN, HIGH);
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
      digitalWrite(MIC_POWER_PIN, HIGH);
      widgetMode = WidgetMode::active;
      break;

    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("*** Invalid newMode in setWidgetMode"));
#endif
      break;
  }
}


void setMpu6050Mode(Mpu6050Mode newMode, uint32_t now)
{
  switch (newMode) {

    case Mpu6050Mode::cycle:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Setting mpu6050Mode to cycle..."));
#endif
      mpu6050.setDMPEnabled(false);
      // TODO:  motion detection should be configurable
      // Set up motion detection.
      mpu6050.setMotionDetectionThreshold(1);         // unit is 2mg
      mpu6050.setMotionDetectionCounterDecrement(1);
      mpu6050.setMotionDetectionDuration(1);          // unit is ms
      // Put MPU-6050 in cycle mode.
      mpu6050Mode = Mpu6050Mode::cycle;
      // TODO:  wake frequency should be configurable
      mpu6050.setWakeFrequency(0);                    // 0 = 1.25 Hz, 1 = 2.5 Hz, 2 - 5 Hz, 3 = 10 Hz
      mpu6050.setWakeCycleEnabled(true);
      mpu6050.setIntMotionEnabled(true);
#ifdef IMU_NORMAL_INDICATOR_LED_PIN
      digitalWrite(IMU_NORMAL_INDICATOR_LED_PIN, IMU_NORMAL_INDICATOR_LED_OFF);
#endif
      break;

    case Mpu6050Mode::normal:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Setting mpu6050Mode to normal..."));
#endif
      mpu6050.setIntMotionEnabled(false);
      mpu6050.setWakeCycleEnabled(false);
      clearMpu6050Fifo();
      mpu6050Mode = Mpu6050Mode::normal;
      mpu6050.setDMPEnabled(true);
      lastMotionDetectedMs = now;
      lastSuccessfulMpu6050ReadMs = now;
#ifdef IMU_NORMAL_INDICATOR_LED_PIN
      digitalWrite(IMU_NORMAL_INDICATOR_LED_PIN, IMU_NORMAL_INDICATOR_LED_ON);
#endif
      break;

    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("*** Invalid newMode in setMpu6050Mode"));
#endif
#ifdef IMU_NORMAL_INDICATOR_LED_PIN
      digitalWrite(IMU_NORMAL_INDICATOR_LED_PIN, IMU_NORMAL_INDICATOR_LED_OFF);
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


void initMpu6050()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Initializing MPU6050..."));
#endif
  mpu6050.initialize();

#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Testing MPU6050 connection..."));
#endif
  if (mpu6050.testConnection()) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("MPU6050 connection successful.  Initializing DMP..."));
#endif
    uint8_t devStatus = mpu6050.dmpInitialize();
    if (devStatus == 0) {

      // supply your own gyro offsets here, scaled for min sensitivity
      // TODO 2/28/2018 ross:  What do we do about this?  Every widget could be different.
      //mpu6050.setXGyroOffset(220);
      //mpu6050.setYGyroOffset(76);
      //mpu6050.setZGyroOffset(-85);
      //mpu6050.setZAccelOffset(1788); // 1688 factory default for my test chip

#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Enabling interrupt..."));
#endif
      attachInterrupt(digitalPinToInterrupt(IMU_INTERRUPT_PIN), handleMpu6050Interrupt, RISING);

      // Get expected DMP packet size, and make sure packetBuffer is large enough.
      packetSize = mpu6050.dmpGetFIFOPacketSize();
      if (sizeof(packetBuffer) >= packetSize) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("DMP ready."));
#endif
        setMpu6050Mode(Mpu6050Mode::normal, millis());
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
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
#endif

#ifdef TX_INDICATOR_LED_PIN
  pinMode(TX_INDICATOR_LED_PIN, OUTPUT);
  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_OFF);
#endif
#ifdef IMU_NORMAL_INDICATOR_LED_PIN
  pinMode(IMU_NORMAL_INDICATOR_LED_PIN, OUTPUT);
  digitalWrite(IMU_NORMAL_INDICATOR_LED_PIN, IMU_NORMAL_INDICATOR_LED_OFF);
#endif
  pinMode(MIC_POWER_PIN, OUTPUT);
  digitalWrite(MIC_POWER_PIN, LOW);

  initI2c();
  initMpu6050();

  configureRadio(radio, TX_PIPE_ADDRESS, WANT_ACK, TX_RETRY_DELAY_MULTIPLIER,
                 TX_MAX_RETRIES, CRC_LENGTH, RF_POWER_LEVEL, DATA_RATE,
                 RF_CHANNEL);

  // Set the watchdog for interrupt only (no system reset) and an 8s interval.
  // We have to turn off interrupts because the changes to the control register
  // must be done within four clock cycles of setting WDCE (change-enable bit).
  noInterrupts();
  _WD_CONTROL_REG = (1 << WDCE) | (1 << WDE);
  _WD_CONTROL_REG = (1 << WDIE) | (0 << WDE) | (1 << WDP3) | (1 << WDP0);
///  wdt_enable(WDTO_8S);     // enable the watchdog
  interrupts();

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

  setWidgetMode(WidgetMode::inactive, millis());
}


void clearMovingAverages()
{
  for (uint8_t i = 0; i < numMaSets; ++i) {
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


void clearMpu6050Fifo()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.println(F("Clearing FIFO..."));
#endif

  uint16_t fifoCount = mpu6050.getFIFOCount();
  while (fifoCount != 0) {
    uint8_t readLength = fifoCount >= packetSize ? packetSize : fifoCount;
    mpu6050.getFIFOBytes(packetBuffer, readLength);
    fifoCount -= readLength;
  }

#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Cleared FIFO."));
#endif
}


void gatherMotionMeasurements(uint32_t now)
{
  uint16_t fifoCount = mpu6050.getFIFOCount();
  while (fifoCount >= packetSize) {
    mpu6050.getFIFOBytes(packetBuffer, packetSize);
    fifoCount -= packetSize;

//#ifdef ENABLE_DEBUG_PRINT
//    // Careful:  We might not be able to keep up if this debug print is enabled.
//    Serial.print(F("Got packet from fifo.  fifoCount now "));
//    Serial.println(fifoCount);
//#endif

    VectorInt16 gyro;
    Quaternion quat;            // [w, x, y, z] quaternion container
    VectorFloat gravity;        // [x, y, z] gravity vector
    float ypr[3];               // [yaw, pitch, roll] yaw/pitch/roll container
    VectorInt16 accel;          // [x, y, z] accel sensor measurements
    VectorInt16 linearAccel;    // [x, y, z] gravity-free accel sensor measurements

    mpu6050.dmpGetGyro(&gyro, packetBuffer);
    mpu6050.dmpGetQuaternion(&quat, packetBuffer);
    mpu6050.dmpGetGravity(&gravity, &quat);
    mpu6050.dmpGetYawPitchRoll(ypr, &quat, &gravity);
    mpu6050.dmpGetAccel(&accel, packetBuffer);
    mpu6050.dmpGetLinearAccel(&linearAccel, &accel, &gravity);

    // The unit for yaw, pitch, and roll measurements is tenths of a degree.
    updateMovingAverage(maSlotYaw  , ypr[0] * (float) 1800 / M_PI);
    updateMovingAverage(maSlotPitch, ypr[1] * (float) 1800 / M_PI);
    updateMovingAverage(maSlotRoll , ypr[2] * (float) 1800 / M_PI);

    updateMovingAverage(maSlotGyroX, gyro.x);
    updateMovingAverage(maSlotGyroY, gyro.y);
    updateMovingAverage(maSlotGyroZ, gyro.z);

    updateMovingAverage(maSlotAccelX, accel.x);
    updateMovingAverage(maSlotAccelY, accel.y);
    updateMovingAverage(maSlotAccelZ, accel.z);

    updateMovingAverage(maSlotLinearAccelX, linearAccel.x);
    updateMovingAverage(maSlotLinearAccelY, linearAccel.y);
    updateMovingAverage(maSlotLinearAccelZ, linearAccel.z);

    // If we got any non-zero quaternion or gyro data (which come directly
    // from the packet), communication with the MPU-6050 is probably working.
    if (quat.w != 0 || quat.x != 0 || quat.y != 0 || quat.z != 0
        || gyro.x != 0 || gyro.y != 0 || gyro.z != 0)
    {
      lastSuccessfulMpu6050ReadMs = now;
    }

  // If there was sufficient motion, keep us out of standby mode for now.
  for (uint8_t i = firstYprMaSlot; i < lastYprMaSlot; ++i) {
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


void processMpu6050Interrupt(uint32_t now)
{
  static bool needClearMpu6050Fifo;
  uint16_t fifoCount;

  uint8_t mpu6050IntStatus = mpu6050.getIntStatus();
//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("0x");
//  Serial.print((int) mpu6050IntStatus, HEX);
//  Serial.print(" ");
//#endif

  // Frequently, the interrupt status is zero by the time we're processing
  // the interrupt.  Why that happens has not yet been determined.  If it
  // is zero, there's nothing we can do because we don't know what the
  // interrupt was for.
  if (mpu6050IntStatus == 0) {
    return;
  }

  switch (mpu6050Mode) {

    case Mpu6050Mode::init:
      // Don't do anything because the MPU isn't ready yet.
      break;

    case Mpu6050Mode::cycle:
      if (mpu6050IntStatus & 0x40) {
        setMpu6050Mode(Mpu6050Mode::normal, now);
      }
      break;

    case Mpu6050Mode::normal:

      fifoCount = mpu6050.getFIFOCount();

      // Check if FIFO overflowed.  If it did, clear it, then wait for another
      // data-ready interrupt and clear the FIFO again so that we are back in
      // sync (i.e., any partial packets have been cleared out).  Based on some
      // brief testing, it appears that bits 0 and 1 always appear set together.
      if (mpu6050IntStatus & 0x10) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("*** FIFO overflow!  fifoCount is "));
        Serial.println(fifoCount);
#endif
        clearMpu6050Fifo();
        needClearMpu6050Fifo = true;    // clear it again after next interrupt to get in sync
        return;
      }

      // The MPU6050 register map document says that bit 0 indicates data ready and bit 1
      // is reserved.  However, the I2C data analyzer dump from Jeff Rowberg found at
      // https://www.i2cdevlib.com/tools/analyzer/1 shows that bit 0 indicates raw data
      // ready and bit 1 indicates DMP data ready.
      if (!(mpu6050IntStatus & 0x02)) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("Got interrupt but not for data ready.  mpu6050IntStatus=0x"));
        Serial.print((int) mpu6050IntStatus, HEX);
        Serial.print(F(", fifoCount="));
        Serial.println(fifoCount);
#endif
        return;
      }

      if (needClearMpu6050Fifo) {
        needClearMpu6050Fifo = false;
        clearMpu6050Fifo();
        return;
      }

      // If we've missed retrieving more than a few packets in time, the FIFO may
      // overflow (if it hasn't already), causing data corruption.  We need to
      // clear it now to avoid getting bad data.
      if (fifoCount > packetSize * maxPacketsInMpu6050FifoBeforeForcedClear) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("*** Missed too many packets.  fifoCount="));
        Serial.println(fifoCount);
#endif
        clearMpu6050Fifo();
        return;
      }

      // If the FIFO length is not a multiple of the packet size, there is a partial
      // packet in the FIFO, either due to the FIFO being filled right now or due to some
      // sort of FIFO corruption.  We need to clear the FIFO to avoid getting bad data.
      if (fifoCount % packetSize != 0) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("*** Partial packet in FIFO.  fifoCount="));
        Serial.println(fifoCount);
#endif
        clearMpu6050Fifo();
        needClearMpu6050Fifo = true;    // clear it again after next interrupt to get in sync
        return;
      }

//#ifdef ENABLE_DEBUG_PRINT
//      Serial.print(F("Got data ready interrupt 0x"));
//      Serial.print((int) mpu6050IntStatus, HEX);
//      Serial.print(F(", fifoCount="));
//      Serial.println(fifoCount);
//#endif
      gatherMotionMeasurements(now);
      break;
  }
}


void sampleSound()
{
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


void saveSoundPeakToPeak(uint32_t now)
{
  int16_t pp = maxSoundSample - minSoundSample;
  updateMovingAverage(maSlotPpSound, pp);

  int16_t avgPp = getMovingAverage(maSlotPpSound);
  isSoundActive = avgPp > activeSoundThreshold;

//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("minSoundSample="));
//    Serial.print(minSoundSample);
//    Serial.print(F(" maxSoundSample="));
//    Serial.println(maxSoundSample);
//    Serial.print(F(" pp="));
//    Serial.println(pp);
//    Serial.print(F(" avgPp="));
//    Serial.println(avgPp);
//#endif

  maxSoundSample = 0;
  minSoundSample = UINT16_MAX;
}


void sendMeasurements()
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

  payload.widgetHeader.isActive = widgetMode == WidgetMode::active;

#ifdef ENABLE_DEBUG_PRINT
  for (int i = 0; i < numMaSets; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(payload.measurements[i]);
  }
#endif

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * numMaSets)) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }

#ifdef TX_INDICATOR_LED_PIN
  digitalWrite(TX_INDICATOR_LED_PIN, TX_INDICATOR_LED_OFF);
#endif
}


void loop()
{
  uint32_t now = millis();

  // We need to reset the IMU if we are awake and haven't received any
  // data from it for a while (because it has probably gone out to lunch).
  if (now - lastSuccessfulMpu6050ReadMs >= MPU6050_ASSUMED_DEAD_MS) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("///// *** MPU-6050 assumed dead.  Reinitializing..."));
#endif
    mpu6050Mode = Mpu6050Mode::init;
    initMpu6050();
    // lastMotionDetectedMs ends up with the current value returned by
    // millis() when initMpu6050 calls setMpu6050Mode.  Since initializaing the
    // MPU-6050 likely took a millisecond or more, we need to fix the
    // value of lastMotionDetectedMs so that it isn't in the future.
    lastMotionDetectedMs = now;
  }

  if (gotMpu6050Interrupt) {
    gotMpu6050Interrupt = false;
    processMpu6050Interrupt(now);
  }

  if (now - lastSoundSampleMs >= SOUND_SAMPLE_INTERVAL_MS) {
    lastSoundSampleMs = now;
    sampleSound();
  }

  if (now - lastSoundSaveMs >= SOUND_SAVE_INTERVAL_MS) {
    lastSoundSaveMs = now;
    saveSoundPeakToPeak(now);
  }

  // TODO:  activity indicator depends on which actual widget this is so make configurable
  if (isSoundActive) {
    if (widgetMode == WidgetMode::inactive) {
      setWidgetMode(WidgetMode::active, now);
    }
  }
  else {
    if (widgetMode == WidgetMode::active) {
      setWidgetMode(WidgetMode::inactive, now);
    }
  }

  if ((int32_t) (now - nextTxMs) >= 0) {
    nextTxMs = now + txInterval;
    sendMeasurements();
  }

  if (widgetMode == WidgetMode::inactive
      && mpu6050Mode == Mpu6050Mode::normal
      && now - lastMotionDetectedMs >= MOTION_TIMEOUT_MS)
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Going standby because no motion from "));
    Serial.print(lastMotionDetectedMs);
    Serial.print(F(" to "));
    Serial.println(now);
#endif
    setWidgetMode(WidgetMode::standby, now);
    // Setting the widget mode to standby will put the processor to sleep.
    // It wakes when it gets a motion detection interrupt from the MPU-6050.
    // Sometime after that, setWidgetMode returns, and execution resumes here.
    // The world may be a different place then.  Just thought you should know.
  }
}

