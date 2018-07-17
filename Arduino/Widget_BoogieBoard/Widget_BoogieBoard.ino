/*****************************************************************
 *                                                               *
 * Boogie Board Widget                                           *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, June 2018                                 )'( *
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


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 7

#define ACTIVE_TX_INTERVAL_MS 50L
#define INACTIVE_TX_INTERVAL_MS 2000L       // should be a multiple of ACTIVE_TX_INTERVAL_MS

//#define TX_FAILURE_LED_PIN 2
#define IMU_INTERRUPT_PIN 2

#define NUM_MPU_VALUES_TO_SEND 6

#define MA_LENGTH 8
#define NUM_MA_SETS (NUM_MPU_VALUES_TO_SEND)

constexpr uint16_t activeAccelerationThreshold = 10;
constexpr uint32_t activityTimeoutMs = 5000L;

// When we haven't retrieved packets from the MPU6050's DMP's FIFO fast enough,
// data corruption becomes likely even before the FIFO overflows.  We'll clear
// the FIFO when more than maxPacketsInFifoBeforeReset packets are in it.
constexpr uint8_t maxPacketsInFifoBeforeReset = 2;


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "2wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11, 13) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 3

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

static MeasurementVectorPayload payload;

static bool isActive;
static bool wasActive;

static uint16_t minSoundSample = UINT16_MAX;
static uint16_t maxSoundSample;

static int16_t avgMinSoundSample;
static int16_t avgMaxSoundSample;
static int16_t ppSoundSample;

static int16_t maValues[NUM_MA_SETS][MA_LENGTH];
static int32_t maSums[NUM_MA_SETS];
static uint8_t nextSlotIdx[NUM_MA_SETS];
static bool maSetFull[NUM_MA_SETS];

static MPU6050 mpu;               // using default I2C address 0x68

// MPU control/status vars
static bool dmpReady = false;     // set true if DMP init was successful
static uint8_t devStatus;         // return status after each device operation (0 = success, !0 = error)
static uint16_t packetSize;       // expected DMP packet size (default is 42 bytes)
static uint8_t packetBuffer[42];  // must be at least as large as packet size returned by dmpGetFIFOPacketSize

// orientation/motion vars
static Quaternion quat;           // [w, x, y, z] quaternion container
static VectorFloat gravity;       // [x, y, z] gravity vector
static float ypr[3];              // [yaw, pitch, roll] yaw/pitch/roll container
static int16_t gyro[3];

static volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high


/******************
 * Implementation *
 ******************/

void dmpDataReady() {
    mpuInterrupt = true;
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
    devStatus = mpu.dmpInitialize();
    if (devStatus == 0) {
  
      // supply your own gyro offsets here, scaled for min sensitivity
      // TODO 2/28/2018 ross:  What do we do about this?  Every widget could be different.
      mpu.setXGyroOffset(220);
      mpu.setYGyroOffset(76);
      mpu.setZGyroOffset(-85);
      mpu.setZAccelOffset(1788); // 1688 factory default for my test chip
  
      // turn on the DMP, now that it's ready
#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Enabling DMP..."));
#endif
      mpu.setDMPEnabled(true);

#ifdef ENABLE_DEBUG_PRINT
      Serial.println(F("Enabling interrupt..."));
#endif
      attachInterrupt(digitalPinToInterrupt(IMU_INTERRUPT_PIN), dmpDataReady, RISING);

      // Get expected DMP packet size, and make sure packetBuffer is large enough.
      packetSize = mpu.dmpGetFIFOPacketSize();
      if (sizeof(packetBuffer) >= packetSize) {
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("DMP ready."));
#endif
        dmpReady = true;
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
  Serial.begin(57600);
  printf_begin();
#endif

  initI2c();
  initMpu();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);

  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;

  // Communication with the MPU6050 has proven to be problematic.
  // If we don't hear from the unit periodically, or if we don't
  // get good data for a while, we need the watchdog to reset us.
  wdt_enable(WDTO_1S);     // enable the watchdog
}


void updateMovingAverage(uint8_t setIdx, int16_t newValue)
{
  maSums[setIdx] -= maValues[setIdx][nextSlotIdx[setIdx]];
  maSums[setIdx] += newValue;
  maValues[setIdx][nextSlotIdx[setIdx]] = newValue;

  ++nextSlotIdx[setIdx];
  if (nextSlotIdx[setIdx] >= MA_LENGTH) {
     maSetFull[setIdx] = true;
     nextSlotIdx[setIdx] = 0;
  }
}


int16_t getMovingAverage(uint8_t setIdx)
{
  int32_t avg;
  if (maSetFull[setIdx]) {
    avg = maSums[setIdx] / (int32_t) MA_LENGTH;
  }
  else {
    avg = nextSlotIdx[setIdx] > 0 ? (int32_t) maSums[setIdx] / (int32_t) nextSlotIdx[setIdx] : 0;
  }

//#ifdef ENABLE_DEBUG_PRINT
//  Serial.print("getMovingAverage(");
//  Serial.print(setIdx);
//  Serial.print("):  ");
//  for (uint8_t i = 0; i < (maSetFull[setIdx] ? MA_LENGTH : nextSlotIdx[setIdx]); ++i) {
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


void resetDmpFifo()
{
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Resetting DMP FIFO..."));
#endif
    mpu.resetFIFO();
    while (true) {
      if (mpuInterrupt) {
        mpuInterrupt = false;
        uint8_t mpuIntStatus = mpu.getIntStatus();
        if (mpuIntStatus & 0x01) {
          mpu.resetFIFO();
          break;
        }
      }
    }
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Cleared FIFO and re-sync'd."));
#endif
}


void gatherMotionMeasurements(uint32_t now)
{
  static uint32_t inactiveStartMs;

//#ifdef ENABLE_DEBUG_PRINT
//    Serial.println(F("gather motion"));
//#endif

  mpuInterrupt = false;
  uint8_t mpuIntStatus = mpu.getIntStatus();
  uint16_t fifoCount = mpu.getFIFOCount();

  // Check if FIFO overflowed.  If it did, clear it, then wait for another
  // data-ready interrupt and clear the FIFO again so that we are back in
  // sync (i.e., any partial packets have been cleared out).  Based on some
  // brief testing, it appears that bits 0 and 1 always appear set together.
  if (mpuIntStatus & 0x10) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("*** FIFO overflow!  fifoCount is "));
    Serial.println(fifoCount);
#endif
    resetDmpFifo();
    return;
  }

  // The MPU6050 register map document says that bit 0 indicates data ready and bit 1
  // is reserved.  However, the I2C data analyzer dump from Jeff Rowberg found at
  // https://www.i2cdevlib.com/tools/analyzer/1 shows that bit 0 indicates raw data
  // ready and bit 1 indicates DMP data ready.
  if (!(mpuIntStatus & 0x02)) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("Got interrupt but not for data ready.  mpuIntStatus="));
    Serial.print((int) mpuIntStatus);
    Serial.print(F(", fifoCount="));
    Serial.println(fifoCount);
#endif
    return;
  }

  // If we've missed retrieving more than a few packets in time, the FIFO has
  // probably already overflowed (even though we haven't gotten that interrupt)
  // or otherwise become corrupted.  We need to reset it to avoid getting bad data.
  if (fifoCount > packetSize * maxPacketsInFifoBeforeReset) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("*** Missed too many packets.  fifoCount="));
    Serial.println(fifoCount);
#endif
    resetDmpFifo();
    return;
  }

  // If the FIFO length is not a multiple of the packet size, there is a partial
  // packet in the FIFO, either due to the FIFO being filled right now or due to
  // some sort of FIFO corruption.  We need to reset it to avoid getting bad data.
  if (fifoCount % packetSize != 0) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("////////// *** Partial packet in FIFO.  fifoCount="));
    Serial.println(fifoCount);
#endif
    resetDmpFifo();
    return;
  }

  while (fifoCount >= packetSize) {
    fifoCount -= packetSize;
    mpu.getFIFOBytes(packetBuffer, packetSize);
#ifdef ENABLE_DEBUG_PRINT
    // Careful:  We might not be able to keep up if this debug print is enabled.
    Serial.print(F("Got packet from fifo.  fifoCount now "));
    Serial.println(fifoCount);
#endif

    mpu.dmpGetGyro(gyro, packetBuffer);
    mpu.dmpGetQuaternion(&quat, packetBuffer);
    mpu.dmpGetGravity(&gravity, &quat);
    mpu.dmpGetYawPitchRoll(ypr, &quat, &gravity);

    updateMovingAverage(0, ypr[0] * (float) 18000 / M_PI);
    updateMovingAverage(1, ypr[1] * (float) 18000 / M_PI);
    updateMovingAverage(2, ypr[2] * (float) 18000 / M_PI);
    updateMovingAverage(3, gyro[0]);
    updateMovingAverage(4, gyro[1]);
    updateMovingAverage(5, gyro[2]);

    // If we're here and we got non-zero data, communication
    // with the MPU6050 is probably working, so kick the dog.
    bool gotNonzeroData = false;
    for (uint8_t i = 0; i < 3; ++i) {
      if (ypr[i] > 0.001 || gyro != 0) {
        gotNonzeroData = true;
        break;
      }
    }
    if (gotNonzeroData) {
      wdt_reset();
    }

  if (abs(gyro[0]) > activeAccelerationThreshold) {
    isActive = true;
    inactiveStartMs = 0;
  }
  else {
    if (inactiveStartMs == 0) {
      inactiveStartMs = now;
    }
    else if (now - inactiveStartMs >= activityTimeoutMs) {
      isActive = false;
    }
  }

#ifdef ENABLE_DEBUG_PRINT
      // Careful:  We might not be able to keep up if this debug print is enabled.
    Serial.print("ypr:  ");
    Serial.print(ypr[0]);
    Serial.print(", ");
    Serial.print(ypr[1]);
    Serial.print(", ");
    Serial.print(ypr[2]);
    Serial.print("    gyro:  ");
    Serial.print(gyro[0]);
    Serial.print(", ");
    Serial.print(gyro[1]);
    Serial.print(", ");
    Serial.println(gyro[2]);
#endif
  }

}


void sendMeasurements()
{
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("send"));
#endif

  for (int i = 0; i < NUM_MPU_VALUES_TO_SEND; ++i) {
      payload.measurements[i] = getMovingAverage(i);
  }

  payload.widgetHeader.isActive = isActive;

#ifdef ENABLE_DEBUG_PRINT
  for (int i = 0; i < NUM_MPU_VALUES_TO_SEND; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(payload.measurements[i]);
  }
#endif

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * NUM_MPU_VALUES_TO_SEND)) {
#ifdef TX_FAILURE_LED_PIN      
    digitalWrite(TX_FAILURE_LED_PIN, HIGH);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
#ifdef TX_FAILURE_LED_PIN      
    digitalWrite(TX_FAILURE_LED_PIN, LOW);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }

}


void loop() {

  static int32_t lastTxMs;

  uint32_t now = millis();

  if (dmpReady && mpuInterrupt) {
    gatherMotionMeasurements(now);
  }

  if (now - lastTxMs >= ACTIVE_TX_INTERVAL_MS) {
    if (isActive || wasActive || now - lastTxMs >= INACTIVE_TX_INTERVAL_MS) {
      lastTxMs = now;
      sendMeasurements();
      wasActive = isActive;
    }
  }

}

