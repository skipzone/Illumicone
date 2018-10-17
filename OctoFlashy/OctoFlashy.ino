/**************************************************
 *                                                *
 * OctoFlashy Interactive Lamps Pattern Generator *
 *                                                *
 * Ross Butler  September 2018                    *
 *                                                *
 **************************************************/


/***********
 * Options *
 ***********/

//#define USE_LOCAL_SENSOR
#define ENABLE_WATCHDOG
//#define ENABLE_DEBUG_PRINT


/************
 * Includes *
 ************/

#ifdef ENABLE_WATCHDOG
#include <avr/wdt.h>
#endif

#ifdef USE_LOCAL_SENSOR
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif
#else
#include <SPI.h>
#include "RF24.h"
#endif

#include "DmxSimple.h"


/*********************************
 * Payload Structure Definitions *
 *********************************/

union WidgetHeader {
  struct {
    uint8_t id       : 4;
    bool    isActive : 1;
    uint8_t channel  : 3;
  };
  uint8_t raw;
};

// pipe 0
struct StressTestPayload {
  WidgetHeader widgetHeader;
  uint32_t     payloadNum;
  uint32_t     numTxFailures;
};

// pipe 1
struct PositionVelocityPayload {
  WidgetHeader widgetHeader;
  int16_t      position;
  int16_t      velocity;
};

// pipe 2
struct MeasurementVectorPayload {
  WidgetHeader widgetHeader;
  int16_t      measurements[15];
};

// pipe 5
struct CustomPayload {
  WidgetHeader widgetHeader;
  uint8_t      buf[31];
};


/*****************
 * Configuration *
 *****************/

#define TEMPERATURE_SAMPLE_INTERVAL_MS 500L
#define DMX_TX_INTERVAL_MS 33L

#define IMU_INTERRUPT_PIN 2

#define NUM_MA_SETS 9
#define MA_LENGTH 8

// When we haven't retrieved packets from the MPU6050's DMP's FIFO fast enough,
// data corruption becomes likely even before the FIFO overflows.  We'll clear
// the FIFO when more than maxPacketsInFifoBeforeReset packets are in it.
constexpr uint8_t maxPacketsInFifoBeforeReset = 2;

constexpr double countsPerG = 8192.0;

// Restrict pitch and roll to avoid gimbal lock.
constexpr float maxPitchDegrees = 45;
constexpr float maxRollDegrees = 52;

constexpr int16_t minPpSoundForStrobe = 300;
constexpr int16_t maxPpSoundForStrobe = 500;
constexpr uint8_t minStrobeValue = 225;
constexpr uint8_t maxStrobeValue = 250;

#define NUM_COLORS_PER_LAMP 4
#define NUM_LAMPS 8
#define LAMP_MIN_INTENSITY 24

#define DMX_OUTPUT_PIN 3
#define DMX_NUM_CHANNELS_PER_LAMP 4
#define DMX_NUM_CHANNELS (NUM_LAMPS * DMX_NUM_CHANNELS_PER_LAMP)

// Let the scale8 function stolen from the FastLED library use assembly code if we're on an AVR chip.
#if defined(__AVR__)
#define SCALE8_AVRASM 1
#else
#define SCALE8_C 1
#endif
typedef uint8_t fract8;   ///< ANSI: unsigned short _Fract
#define LIB8STATIC_ALWAYS_INLINE __attribute__ ((always_inline)) static inline


/*********************************************
 * Radio Configuration Common To All Widgets *
 *********************************************/

// Possible data rates are RF24_250KBPS, RF24_1MBPS, or RF24_2MBPS (genuine Noric chips only).
#define DATA_RATE RF24_250KBPS

// Valid CRC length values are RF24_CRC_8, RF24_CRC_16, and RF24_CRC_DISABLED
#define CRC_LENGTH RF24_CRC_16

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
#define RF_CHANNEL 84

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

#define ACK_WIDGET_PACKETS false


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

// moving average variables
static int16_t maValues[NUM_MA_SETS][MA_LENGTH];
static int32_t maSums[NUM_MA_SETS];
static uint8_t nextSlotIdx[NUM_MA_SETS];
static bool maSetFull[NUM_MA_SETS];

#ifdef USE_LOCAL_SENSOR
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
static VectorInt16 aa;            // [x, y, z] accel sensor measurements
static VectorInt16 aaReal;        // [x, y, z] gravity-free accel sensor measurements
static int16_t rawAccel[3];
static int16_t gyro[3];
static float mpuTemperatureC;
static float mpuTemperatureF;

static volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
#endif

static int16_t avgMinSoundSample;
static int16_t avgMaxSoundSample;
static int16_t ppSoundSample;
static double avgYaw;
static double avgPitch;
static double avgRoll;
static double avgRealAccelX;
static double avgRealAccelY;
static double avgRealAccelZ;
static double avgGyroX;
static double avgGyroY;
static double avgGyroZ;

static int colorChannelIntensities[NUM_LAMPS][NUM_COLORS_PER_LAMP];

#ifndef USE_LOCAL_SENSOR
static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)
#endif


/******************
 * Implementation *
 ******************/

#ifdef USE_LOCAL_SENSOR
void dmpDataReady() {
    mpuInterrupt = true;
}
#endif


#ifndef USE_LOCAL_SENSOR
void initRadio()
{
  radio.begin();

  radio.setPALevel(RF_POWER_LEVEL);
  //radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
  radio.setDataRate(DATA_RATE);
  radio.setChannel(RF_CHANNEL);
  radio.setAutoAck(ACK_WIDGET_PACKETS);
  radio.enableDynamicPayloads();
  radio.setCRCLength(CRC_LENGTH);

  // Unlike widgetRcvr, we don't open pipe 0 here.
  for (uint8_t i = 0; i < numReadPipes; ++i) {
      radio.openReadingPipe(i, readPipeAddresses[i]);
  }

#ifdef ENABLE_DEBUG_PRINT 
  radio.printDetails();
#endif
  
  radio.startListening();
}
#endif


#ifdef USE_LOCAL_SENSOR
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
#endif


#ifdef USE_LOCAL_SENSOR
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
      //mpu.setXGyroOffset(220);
      //mpu.setYGyroOffset(76);
      //mpu.setZGyroOffset(-85);
      //mpu.setZAccelOffset(1788); // 1688 factory default for my test chip
  
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
#endif


void initDmx()
{
  DmxSimple.usePin(DMX_OUTPUT_PIN);
  DmxSimple.maxChannel(DMX_NUM_CHANNELS);
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
#endif

#ifdef USE_LOCAL_SENSOR
  initI2c();
  initMpu();
#else
  initRadio();
#endif
  initDmx();


  // Communication with the MPU6050 has proven to be problematic.
  // If we don't hear from the unit periodically, or if we don't
  // get good data for a while, we need the watchdog to reset us.
#ifdef ENABLE_WATCHDOG
  wdt_enable(WDTO_1S);     // enable the watchdog
#endif
}


// scale8 function shamelessly stolen from FastLED library.
//
///  scale one byte by a second one, which is treated as
///  the numerator of a fraction whose denominator is 256
///  In other words, it computes i * (scale / 256)
///  4 clocks AVR with MUL, 2 clocks ARM
LIB8STATIC_ALWAYS_INLINE uint8_t scale8(uint8_t i, fract8 scale)
{
#if SCALE8_C == 1
    return (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;
#elif SCALE8_AVRASM == 1
#if defined(LIB8_ATTINY)
    uint8_t work=i;
    uint8_t cnt=0x80;
    asm volatile(
        "  inc %[scale]                 \n\t"
        "  breq DONE_%=                 \n\t"
        "  clr %[work]                  \n\t"
        "LOOP_%=:                       \n\t"
        /*"  sbrc %[scale], 0             \n\t"
        "  add %[work], %[i]            \n\t"
        "  ror %[work]                  \n\t"
        "  lsr %[scale]                 \n\t"
        "  clc                          \n\t"*/
        "  sbrc %[scale], 0             \n\t"
        "  add %[work], %[i]            \n\t"
        "  ror %[work]                  \n\t"
        "  lsr %[scale]                 \n\t"
        "  lsr %[cnt]                   \n\t"
        "brcc LOOP_%=                   \n\t"
        "DONE_%=:                       \n\t"
        : [work] "+r" (work), [cnt] "+r" (cnt)
        : [scale] "r" (scale), [i] "r" (i)
        :
      );
    return work;
#else
    asm volatile(
        // Multiply 8-bit i * 8-bit scale, giving 16-bit r1,r0
        "mul %0, %1          \n\t"
        // Add i to r0, possibly setting the carry flag
        "add r0, %0         \n\t"
        // load the immediate 0 into i (note, this does _not_ touch any flags)
        "ldi %0, 0x00       \n\t"
        // walk and chew gum at the same time
        "adc %0, r1          \n\t"
         "clr __zero_reg__    \n\t"

         : "+a" (i)      /* writes to i */
         : "a"  (scale)  /* uses scale */
         : "r0", "r1"    /* clobbers r0, r1 */ );

    /* Return the result */
    return i;
#endif
#else
#error "No implementation for scale8 available."
#endif
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


#ifdef USE_LOCAL_SENSOR
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
#endif


#ifdef USE_LOCAL_SENSOR
void gatherMotionMeasurements()
{
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
//#ifdef ENABLE_DEBUG_PRINT
//    // Careful:  We might not be able to keep up if this debug print is enabled.
//    Serial.print(F("Got packet from fifo.  fifoCount now "));
//    Serial.println(fifoCount);
//#endif

    mpu.dmpGetGyro(gyro, packetBuffer);
    mpu.dmpGetQuaternion(&quat, packetBuffer);
    mpu.dmpGetGravity(&gravity, &quat);
    mpu.dmpGetYawPitchRoll(ypr, &quat, &gravity);
    mpu.dmpGetAccel(&aa, packetBuffer);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

    updateMovingAverage(0, ypr[0] * (float) 1800 / M_PI);
    updateMovingAverage(1, ypr[1] * (float) 1800 / M_PI);
    updateMovingAverage(2, ypr[2] * (float) 1800 / M_PI);
    updateMovingAverage(3, aaReal.x);
    updateMovingAverage(4, aaReal.y);
    updateMovingAverage(5, aaReal.z);
//    updateMovingAverage(3, aa.x);
//    updateMovingAverage(4, aa.y);
//    updateMovingAverage(5, aa.z);
    updateMovingAverage(6, gyro[0]);
    updateMovingAverage(7, gyro[1]);
    updateMovingAverage(8, gyro[2]);

#ifdef ENABLE_WATCHDOG
    // If we're here and we got non-zero data, communication
    // with the MPU6050 is probably working, so kick the dog.
    bool gotNonzeroData = false;
    for (uint8_t i = 0; i < 3; ++i) {
      if (ypr[i] > 0.001 || gyro[i] != 0) {
        gotNonzeroData = true;
        break;
      }
    }
    if (gotNonzeroData) {
      wdt_reset();
    }
#endif

#ifdef ENABLE_DEBUG_PRINT
      // Careful:  We might not be able to keep up if this debug print is enabled.
    Serial.print("ypr:  ");
    Serial.print(ypr[0]);
    Serial.print(", ");
    Serial.print(ypr[1]);
    Serial.print(", ");
    Serial.print(ypr[2]);
    Serial.print("    aaReal:  ");
    Serial.print(aaReal.x);
    Serial.print(", ");
    Serial.print(aaReal.y);
    Serial.print(", ");
    Serial.print(aaReal.z);
    Serial.print("    gyro:  ");
    Serial.print(gyro[0]);
    Serial.print(", ");
    Serial.print(gyro[1]);
    Serial.print(", ");
    Serial.println(gyro[2]);
#endif
  }
}
#endif


#ifdef USE_LOCAL_SENSOR
void gatherTemperatureMeasurement()
{
  int16_t rawTemperature = mpu.getTemperature();
  mpuTemperatureC = (float) rawTemperature / 340.0 + 36.53;
  mpuTemperatureF = mpuTemperatureC * 9.0/5.0 + 32.0;
}
#endif


void getAverageMeasurements()
{
  avgMinSoundSample = 0;
  avgMaxSoundSample = 0;
  ppSoundSample = 0;
  avgYaw = getMovingAverage(0) / 10.0;
  avgPitch = getMovingAverage(1) / 10.0;
  avgRoll = getMovingAverage(2) / 10.0;
  avgRealAccelX = getMovingAverage(3) / countsPerG;
  avgRealAccelY = getMovingAverage(4) / countsPerG;
  avgRealAccelZ = getMovingAverage(5) / countsPerG;
  avgGyroX = getMovingAverage(6);
  avgGyroY = getMovingAverage(7);
  avgGyroZ = getMovingAverage(8);

#ifdef ENABLE_DEBUG_PRINT
  Serial.print(F("avgYaw="));
  Serial.print(avgYaw);
  Serial.print(F(" avgPitch="));
  Serial.print(avgPitch);
  Serial.print(F(" avgRoll="));
  Serial.print(avgRoll);
  Serial.print(F(" avgRealAccelX="));
  Serial.print(avgRealAccelX);
  Serial.print(F(" avgRealAccelY="));
  Serial.print(avgRealAccelY);
  Serial.print(F(" avgRealAccelZ="));
  Serial.print(avgRealAccelZ);
  Serial.print(F(" avgGyroX="));
  Serial.print(avgGyroX);
  Serial.print(F(" avgGyroY="));
  Serial.print(avgGyroY);
  Serial.print(F(" avgGyroZ="));
  Serial.println(avgGyroZ);
#endif
}


#ifndef USE_LOCAL_SENSOR

//void handleStressTestPayload(const StressTestPayload* payload, uint8_t payloadSize)
//{
//}


bool handlePositionVelocityPayload(const PositionVelocityPayload* payload, uint8_t payloadSize)
{
  constexpr uint16_t expectedPayloadSize = sizeof(PositionVelocityPayload);
  if (payloadSize != expectedPayloadSize) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got PositionVelocityPayload with "));
    Serial.print(payloadSize);
    Serial.print(F(" bytes but expected "));
    Serial.print(expectedPayloadSize);
    Serial.println(F(" bytes."));    
#endif
    return false;
  }

  if (payload->widgetHeader.id != 2       // Spinnah
      && payload->widgetHeader.id != 6    // TriObelisk
      && payload->widgetHeader.id != 9    // FourPlay
      && payload->widgetHeader.id != 10   // FourPlay-4-2
      && payload->widgetHeader.id != 11   // FourPlay-4-3
     )
  {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got PositionVelocityPayload payload from widget "));
    Serial.print(payload->widgetHeader.id);
    Serial.println(F(" but expected one from widget 2, 6, 9, 10, or 11."));
#endif
    return false;
  }

  bool gotMeasurement = false;
  int16_t p;
  int16_t pmax;
  constexpr int16_t speedupFactor = 4;
  switch (payload->widgetHeader.channel) {
    case 0:
//      // Vary yaw continuously between -180 and 180.
//      pmax = 180;
//      p = (payload->position * speedupFactor) % (pmax * 4);
//      p = abs(p);                                 // in Arduinolandia, abs is a macro
//      avgYaw = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
      avgYaw = 0;   // TODO:  enable above code and remove this if we start using yaw
      gotMeasurement = true;
      break;
    case 1:
      // Vary pitch continuously between -maxPitchDegrees and maxPitchDegrees.
      pmax = maxPitchDegrees;
      p = (payload->position * speedupFactor) % (pmax * 4);
      p = abs(p);                                 // in Arduinolandia, abs is a macro
      avgPitch = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
      gotMeasurement = true;
      break;
    case 2:
      // Vary roll continuously between -maxRollDegrees and maxRollDegrees.
      pmax = maxRollDegrees;
      p = (payload->position * speedupFactor) % (pmax * 4);
      p = abs(p);                                 // in Arduinolandia, abs is a macro
      avgRoll = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
      gotMeasurement = true;
      break;
    case 3:
      ppSoundSample = payload->velocity * speedupFactor;
      ppSoundSample = abs(ppSoundSample);
      gotMeasurement = true;
      break;
    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got PositionVelocityPayload payload from widget "));
      Serial.print(payload->widgetHeader.id);
      Serial.print(F(" for unsupported channel "));
      Serial.println(payload->widgetHeader.channel);
#endif
      break;
  }

  // Clear the measurements we cannot get from this widget.
  if (gotMeasurement) {
    if (payload->widgetHeader.id == 2       // Spinnah
        || payload->widgetHeader.id == 6    // TriObelisk
       )
    {
      ppSoundSample = 0;
    }
    avgMinSoundSample = 0;
    avgMaxSoundSample = 0;
    avgRealAccelX = 0.0;
    avgRealAccelY = 0.0;
    avgRealAccelZ = 0.0;
    avgGyroX = 0.0;
    avgGyroY = 0.0;
    avgGyroZ = 0.0;
  }

  return gotMeasurement;
}


bool handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, uint8_t payloadSize)
{
  constexpr uint16_t expectedPayloadSize = sizeof(WidgetHeader) + sizeof(int16_t) * 9;
  if (payloadSize != expectedPayloadSize) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got MeasurementVectorPayload with "));
    Serial.print(payloadSize);
    Serial.print(F(" bytes but expected "));
    Serial.print(expectedPayloadSize);
    Serial.println(F(" bytes."));    
#endif
    return false;
  }

  if (payload->widgetHeader.id != 4) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got MeasurementVectorPayload payload from widget "));
    Serial.print(payload->widgetHeader.id);
    Serial.println(F(" but expected one from widget 4 (Rainstick)."));
#endif
    return false;
  }

  avgMinSoundSample = payload->measurements[0];
  avgMaxSoundSample = payload->measurements[1];
  ppSoundSample = payload->measurements[2];
  avgYaw = payload->measurements[3] / 10.0;
  avgPitch = payload->measurements[4] / 10.0;
  avgRoll = payload->measurements[5] / 10.0;
  avgRealAccelX = 0.0;
  avgRealAccelY = 0.0;
  avgRealAccelZ = 0.0;
  avgGyroX = payload->measurements[6];
  avgGyroY = payload->measurements[7];
  avgGyroZ = payload->measurements[8];

  return true;
}


void pollRadio()
{
#ifdef ENABLE_WATCHDOG
  wdt_reset();
#endif

  uint8_t pipeNum;
  if (!radio.available(&pipeNum)) {
    return;
  }

  constexpr uint8_t maxPayloadSize = 32 + sizeof(WidgetHeader);
  uint8_t payload[maxPayloadSize];
  uint8_t payloadSize = radio.getDynamicPayloadSize();
  if (payloadSize > maxPayloadSize) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got message on pipe "));
    Serial.print(pipeNum);
    Serial.print(F(" with payload size "));
    Serial.print(payloadSize);
    Serial.print(F(" but maximum payload size is "));
    Serial.println(maxPayloadSize);
#endif
    return;
  }
  radio.read(payload, payloadSize);

  bool gotMeasurements = false;
  switch(pipeNum) {
//    case 0:
//        gotMeasurements = handleStressTestPayload((StressTestPayload*) payload, payloadSize);
//        break;
    case 1:
        gotMeasurements = handlePositionVelocityPayload((PositionVelocityPayload*) payload, payloadSize);
        break;
    case 2:
        gotMeasurements = handleMeasurementVectorPayload((MeasurementVectorPayload*) payload, payloadSize);
        break;
    default:
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got message on unsupported pipe "));
      Serial.println(pipeNum);
#endif
      break;
  }

#ifdef ENABLE_DEBUG_PRINT
  if (gotMeasurements) {
    Serial.print(F("avgMinSoundSample="));
    Serial.print(avgMinSoundSample);
    Serial.print(F(" avgMaxSoundSample="));
    Serial.print(avgMaxSoundSample);
    Serial.print(F(" ppSoundSample="));
    Serial.print(ppSoundSample);
    Serial.print(F(" avgYaw="));
    Serial.print(avgYaw);
    Serial.print(F(" avgPitch="));
    Serial.print(avgPitch);
    Serial.print(F(" avgRoll="));
    Serial.print(avgRoll);
    Serial.print(F(" avgGyroX="));
    Serial.print(avgGyroX);
    Serial.print(F(" avgGyroY="));
    Serial.print(avgGyroY);
    Serial.print(F(" avgGyroZ="));
    Serial.println(avgGyroZ);
  }
#endif
}
#endif


void sendDmx()
{
  uint8_t dmxChannelValues[DMX_NUM_CHANNELS + 1];

  uint16_t dmxChannelNum = 1;
  for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {

    // If the sound is sufficiently loud, flash all the lamps instead of setting their intensities.
    if (ppSoundSample >= minPpSoundForStrobe) {
      uint8_t strobeValue = map(constrain(ppSoundSample, minPpSoundForStrobe, minPpSoundForStrobe),
                                minPpSoundForStrobe, maxPpSoundForStrobe, minStrobeValue, maxStrobeValue);
      dmxChannelValues[dmxChannelNum++] = strobeValue;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("strobeValue="));
      Serial.print(strobeValue);
      Serial.print(F(" ppSoundSample="));
      Serial.print(ppSoundSample);
      Serial.print(F(" minPpSoundForStrobe="));
      Serial.print(minPpSoundForStrobe);
      Serial.print(F(" maxPpSoundForStrobe="));
      Serial.print(maxPpSoundForStrobe);
      Serial.print(F(" minStrobeValue="));
      Serial.print(minStrobeValue);
      Serial.print(F(" maxStrobeValue="));
      Serial.println(maxStrobeValue);
#endif
   }
    else {
      // Set the lamp to full brightness because we control its
      // overall brightness by way of the individual colors.
      dmxChannelValues[dmxChannelNum++] = 127;
    }

    if (NUM_COLORS_PER_LAMP == 3) {
      for (uint8_t colorIdx = 0; colorIdx < NUM_COLORS_PER_LAMP; ++colorIdx) {
        dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][colorIdx];
      }
    }
    else if (NUM_COLORS_PER_LAMP == 4) {
      // The first color (probably red) appears before and after the other
      // two so that we can go all the way around the color wheel.
      dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][0] + colorChannelIntensities[lampIdx][3];
      dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][1];
      dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][2];      
    }
  }

  // Send zeros to any unused channels.
  while (dmxChannelNum <= DMX_NUM_CHANNELS) {
    dmxChannelValues[dmxChannelNum++] = 0;
  }

  // Transmit the DMX channel values.
  for (dmxChannelNum = 1; dmxChannelNum <= DMX_NUM_CHANNELS; ++dmxChannelNum) {
    DmxSimple.write(dmxChannelNum, dmxChannelValues[dmxChannelNum]);
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("sent DMX ch "));
//    Serial.print(dmxChannelNum);
//    Serial.print(F(": "));
//    Serial.println(dmxChannelValues[dmxChannelNum]);    
//#endif
  }
}


void updateLamps()
{
  constexpr float colorAngleStep = maxPitchDegrees * 2 / (NUM_COLORS_PER_LAMP - 1);
  constexpr float lampAngleStep = maxRollDegrees * 2 / (NUM_LAMPS - 1);

  int colorIntensities[NUM_COLORS_PER_LAMP];
  for (uint8_t i = 0; i < NUM_COLORS_PER_LAMP; ++i) {
    colorIntensities[i] = 0;
  }

  int lampIntensities[NUM_LAMPS];
  for (uint8_t i = 0; i < NUM_LAMPS; ++i) {
    lampIntensities[i] = LAMP_MIN_INTENSITY;
  }

//  for (uint8_t i = 0; i < NUM_LAMPS; ++i) {
//    for (uint8_t j = 0; j < NUM_COLORS_PER_LAMP; ++j) {
//      colorChannelIntensities[i][j] = 0;
//    }
//  }

#ifdef USE_LOCAL_SENSOR
  getAverageMeasurements();
#endif

  // Normalize and restrict pitch and roll to [0, max___Degrees*2] degrees.
  float normalizedPitch;
  if (avgPitch <= - maxPitchDegrees) {
    normalizedPitch = 0;
  }
  else if (avgPitch >= maxPitchDegrees) {
    normalizedPitch = maxPitchDegrees * 2.0;
  }
  else {
    normalizedPitch = avgPitch + maxPitchDegrees;
  }
  float normalizedRoll;
  if (avgRoll <= - maxRollDegrees) {
    normalizedRoll = 0;
  }
  else if (avgRoll >= maxRollDegrees) {
    normalizedRoll = maxRollDegrees * 2.0;
  }
  else {
    normalizedRoll = avgRoll + maxRollDegrees;
  }

  // Fade across the colors based on pitch angle.
  int colorSection = floor(normalizedPitch / colorAngleStep);
  // Fix up colorSection if measurement is maxPitchDegrees degrees or more.
  if (colorSection >= NUM_COLORS_PER_LAMP - 1) {
      colorSection = NUM_COLORS_PER_LAMP - 2;
  }
  colorIntensities[colorSection] =
    map(normalizedPitch, colorAngleStep * colorSection, colorAngleStep * (colorSection + 1), 255, 0);
  colorIntensities[colorSection + 1] =
    map(normalizedPitch, colorAngleStep * colorSection, colorAngleStep * (colorSection + 1), 0, 255);

  // Fade across the lamps based on roll angle.
  int lampSection = floor(normalizedRoll / lampAngleStep);
  // Fix up lampSection if measurement is maxRollDegrees degrees or more.
  if (lampSection >= NUM_LAMPS - 1) {
      lampSection = NUM_LAMPS - 2;
  }
  lampIntensities[lampSection] =
    map(normalizedRoll, lampAngleStep * lampSection, lampAngleStep * (lampSection + 1), 255, LAMP_MIN_INTENSITY);
  lampIntensities[lampSection + 1] =
    map(normalizedRoll, lampAngleStep * lampSection, lampAngleStep * (lampSection + 1), LAMP_MIN_INTENSITY, 255);

  // For each lamp, scale the intensity of its colors by the corresponding color intensity.
  for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {
    for (uint8_t colorIdx = 0; colorIdx < NUM_COLORS_PER_LAMP; ++colorIdx) {
      colorChannelIntensities[lampIdx][colorIdx] = scale8(lampIntensities[lampIdx], colorIntensities[colorIdx]);
    }
  }

  sendDmx();
}


void loop()
{
  static int32_t lastTemperatureSampleMs;
  static int32_t lastDmxTxMs;

  uint32_t now = millis();

#ifdef USE_LOCAL_SENSOR
  if (dmpReady && mpuInterrupt) {
    gatherMotionMeasurements();
  }

  if (now - lastTemperatureSampleMs >= TEMPERATURE_SAMPLE_INTERVAL_MS) {
    lastTemperatureSampleMs = now;
    gatherTemperatureMeasurement();
  }
#else
  pollRadio();
#endif

  if (now - lastDmxTxMs >= DMX_TX_INTERVAL_MS) {
    lastDmxTxMs = now;
    updateLamps();
  }
}

