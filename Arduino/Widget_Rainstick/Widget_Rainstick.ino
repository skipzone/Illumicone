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


#include "I2Cdev.h"
#include "illumiconeWidget.h"

#include "MPU6050_6Axis_MotionApps20.h"

//#ifdef ENABLE_DEBUG_PRINT
// For some unkown reason, shit don't work without printf.
#include "printf.h"
//#endif

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high


// uncomment "OUTPUT_READABLE_QUATERNION" if you want to see the actual
// quaternion components in a [w, x, y, z] format (not best for parsing
// on a remote host such as Processing or something though)
//#define OUTPUT_READABLE_QUATERNION

// uncomment "OUTPUT_READABLE_EULER" if you want to see Euler angles
// (in degrees) calculated from the quaternions coming from the FIFO.
// Note that Euler angles suffer from gimbal lock (for more info, see
// http://en.wikipedia.org/wiki/Gimbal_lock)
//#define OUTPUT_READABLE_EULER

// uncomment "OUTPUT_READABLE_YAWPITCHROLL" if you want to see the yaw/
// pitch/roll angles (in degrees) calculated from the quaternions coming
// from the FIFO. Note this also requires gravity vector calculations.
// Also note that yaw/pitch/roll angles suffer from gimbal lock (for
// more info, see: http://en.wikipedia.org/wiki/Gimbal_lock)
#define OUTPUT_READABLE_YAWPITCHROLL

// uncomment "OUTPUT_READABLE_REALACCEL" if you want to see acceleration
// components with gravity removed. This acceleration reference frame is
// not compensated for orientation, so +X is always +X according to the
// sensor, just without the effects of gravity. If you want acceleration
// compensated for orientation, us OUTPUT_READABLE_WORLDACCEL instead.
//#define OUTPUT_READABLE_REALACCEL

// uncomment "OUTPUT_READABLE_WORLDACCEL" if you want to see acceleration
// components with gravity removed and adjusted for the world frame of
// reference (yaw is relative to initial orientation, since no magnetometer
// is present in this case). Could be quite handy in some cases.
//#define OUTPUT_READABLE_WORLDACCEL

// uncomment "OUTPUT_TEAPOT" if you want output that matches the
// format used for the InvenSense teapot demo
//#define OUTPUT_TEAPOT


// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion quat;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = { '$', 0x02, 0,0, 0,0, 0,0, 0,0, 0x00, 0x00, '\r', '\n' };


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 4
#define SOUND_SAMPLE_INTERVAL_MS 10L
#define ACTIVE_TX_INTERVAL_MS 500L
#define INACTIVE_TX_INTERVAL_MS 2000L
//#define TX_FAILURE_LED_PIN 2
#define MIC_SIGNAL_PIN A0
#define MIC_POWER_PIN 8

#define NUM_SOUND_VALUES_TO_SEND 3
#define NUM_MPU_VALUES_TO_SEND 4

#define MA_LENGTH 10
#define NUM_MA_SETS (NUM_SOUND_VALUES_TO_SEND + NUM_MPU_VALUES_TO_SEND)


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "2wdgt"

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 5

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

MeasurementVectorPayload payload;

static FILE lcdout = {0};

int16_t mpuMeasurementValues[NUM_MPU_VALUES_TO_SEND];
bool isActive;

static uint16_t minSoundSample = UINT16_MAX;
static uint16_t maxSoundSample;

static float maValues[NUM_MA_SETS][MA_LENGTH];
static float maSums[NUM_MA_SETS];
static uint8_t nextSlotIdx[NUM_MA_SETS];
static bool maSetFull[NUM_MA_SETS];


/******************
 * Implementation *
 ******************/

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
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
    // initialize device
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Initializing I2C devices..."));
#endif
    mpu.initialize();

    // verify connection
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
#endif

    // load and configure the DMP
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("Initializing DMP..."));
#endif
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("Enabling DMP..."));
#endif
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
#endif
        attachInterrupt(0, dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
#endif
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    }
    else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
#endif
    }
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
#endif

  pinMode(MIC_POWER_PIN, OUTPUT);
  digitalWrite(MIC_POWER_PIN, HIGH);

// For some unkown reason, shit don't work without printf.
//#ifdef ENABLE_DEBUG_PRINT
  printf_begin();
//#endif

  initI2c();
  initMpu();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;
}


void updateMovingAverage(uint8_t setIdx, float newValue)
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


float getMovingAverage(uint8_t setIdx)
{
  uint16_t avg;
  if (maSetFull[setIdx]) {
    avg = maSums[setIdx] / MA_LENGTH;
  }
  else {
    avg = nextSlotIdx[setIdx] > 0 ? maSums[setIdx] / nextSlotIdx[setIdx] : 0;
  }

  return avg;
}


void gatherMeasurements()
{
  unsigned long now = millis();

    // reset interrupt flag and get INT_STATUS byte
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();

    // get current FIFO count
    fifoCount = mpu.getFIFOCount();

    // check for overflow (this should never happen unless our code is too inefficient)
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        // reset so we can continue cleanly
        mpu.resetFIFO();
#ifdef ENABLE_DEBUG_PRINT
        Serial.println(F("FIFO overflow!"));
#endif

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
    }
    else if (mpuIntStatus & 0x02) {
        // wait for correct available data length, should be a VERY short wait
        while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

        // read a packet from FIFO
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        
        // track FIFO count here in case there is > 1 packet available
        // (this lets us immediately read more without waiting for an interrupt)
        fifoCount -= packetSize;

        #ifdef OUTPUT_READABLE_QUATERNION
            // display quaternion values in easy matrix form: w x y z
            mpu.dmpGetQuaternion(&quat, fifoBuffer);
#ifdef ENABLE_DEBUG_PRINT
            Serial.print("quat\t");
            Serial.print(quat.w);
            Serial.print("\t");
            Serial.print(quat.x);
            Serial.print("\t");
            Serial.print(quat.y);
            Serial.print("\t");
            Serial.println(quat.z);
#endif
        #endif

        #ifdef OUTPUT_READABLE_EULER
            // display Euler angles in degrees
            mpu.dmpGetQuaternion(&quat, fifoBuffer);
            mpu.dmpGetEuler(euler, &quat);
#ifdef ENABLE_DEBUG_PRINT
            Serial.print("euler\t");
            Serial.print(euler[0] * 180/M_PI);
            Serial.print("\t");
            Serial.print(euler[1] * 180/M_PI);
            Serial.print("\t");
            Serial.println(euler[2] * 180/M_PI);
#endif
        #endif

#ifdef OUTPUT_READABLE_YAWPITCHROLL
    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&quat, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &quat);
    mpu.dmpGetYawPitchRoll(ypr, &quat, &gravity);

    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND, ypr[0] * 180/M_PI);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 1, ypr[1] * 180/M_PI);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 2, ypr[2] * 180/M_PI);
    updateMovingAverage(NUM_SOUND_VALUES_TO_SEND + 3, 0);

//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print("ypr\t");
//    Serial.print(mpuMeasurementValues[0]);
//    Serial.print("\t");
//    Serial.print(mpuMeasurementValues[1]);
//    Serial.print("\t");
//    Serial.println(mpuMeasurementValues[2]);
//#endif
#endif

        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            mpu.dmpGetQuaternion(&quat, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &quat);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
#ifdef ENABLE_DEBUG_PRINT
            Serial.print("areal\t");
            Serial.print(aaReal.x);
            Serial.print("\t");
            Serial.print(aaReal.y);
            Serial.print("\t");
            Serial.println(aaReal.z);
#endif
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            mpu.dmpGetQuaternion(&quat, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &quat);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &quat);
#ifdef ENABLE_DEBUG_PRINT
            Serial.print("aworld\t");
            Serial.print(aaWorld.x);
            Serial.print("\t");
            Serial.print(aaWorld.y);
            Serial.print("\t");
            Serial.println(aaWorld.z);
#endif
        #endif
    
        #ifdef OUTPUT_TEAPOT
            // display quaternion values in InvenSense Teapot demo format:
            teapotPacket[2] = fifoBuffer[0];
            teapotPacket[3] = fifoBuffer[1];
            teapotPacket[4] = fifoBuffer[4];
            teapotPacket[5] = fifoBuffer[5];
            teapotPacket[6] = fifoBuffer[8];
            teapotPacket[7] = fifoBuffer[9];
            teapotPacket[8] = fifoBuffer[12];
            teapotPacket[9] = fifoBuffer[13];
#ifdef ENABLE_DEBUG_PRINT
            Serial.write(teapotPacket, 14);
#endif
            teapotPacket[11]++; // packetCount, loops at 0xFF on purpose
        #endif
    }

  isActive = true;

//#ifdef ENABLE_DEBUG_PRINT
//  for (int i = 0; i < NUM_MPU_VALUES_TO_SEND; ++i) {
//    Serial.print(i);
//    Serial.print(",");
//    Serial.println(mpuMeasurementValues[i]);
//  }
//#endif
    
}


void sendMeasurements()
{
  updateMovingAverage(0, minSoundSample);
  updateMovingAverage(1, maxSoundSample);
  minSoundSample = UINT16_MAX;
  maxSoundSample = 0;

  payload.measurements[0] = getMovingAverage(0);
  payload.measurements[1] = getMovingAverage(1);
  payload.measurements[2] = getMovingAverage(1) - getMovingAverage(0);
  
//  updateMovingAverage(2, getMovingAverage(1) - getMovingAverage(0));

  for (int i = 0, j = NUM_SOUND_VALUES_TO_SEND; i < NUM_MPU_VALUES_TO_SEND; ++i, ++j) {
      payload.measurements[j] = getMovingAverage(j);
  }

  payload.widgetHeader.isActive = isActive;

#ifdef ENABLE_DEBUG_PRINT
  for (int i = 0; i < NUM_MPU_VALUES_TO_SEND + NUM_SOUND_VALUES_TO_SEND; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println(payload.measurements[i]);
  }
#endif

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * (NUM_MPU_VALUES_TO_SEND + NUM_SOUND_VALUES_TO_SEND))) {
#ifdef LED_PIN      
    digitalWrite(LED_PIN, HIGH);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write failed."));
#endif
  }
  else {
#ifdef LED_PIN      
    digitalWrite(LED_PIN, LOW);
#endif
#ifdef ENABLE_DEBUG_PRINT
    Serial.println(F("radio.write succeeded."));
#endif
  }

}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastSoundSampleMs;

  uint32_t now = millis();

  if (dmpReady && (mpuInterrupt || fifoCount >= packetSize)) {
    gatherMeasurements();
  }

  if (now - lastSoundSampleMs >= SOUND_SAMPLE_INTERVAL_MS) {
    unsigned int soundSample = analogRead(MIC_SIGNAL_PIN);
    if (soundSample < minSoundSample) {
      minSoundSample = soundSample;
    }
    if (soundSample > maxSoundSample) {
      maxSoundSample = soundSample;
    }
  }

  if (now - lastTxMs >= (isActive ? ACTIVE_TX_INTERVAL_MS : INACTIVE_TX_INTERVAL_MS)) {
    sendMeasurements();
    lastTxMs = now;
  }

}

