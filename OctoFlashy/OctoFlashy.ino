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

#define ENABLE_WATCHDOG
//#define ENABLE_DEBUG_PRINT


/************
 * Includes *
 ************/

#ifdef ENABLE_WATCHDOG
#include <avr/wdt.h>
#endif

#include <SPI.h>
#include "RF24.h"

#include "DmxSimple.h"


/*****************
 * Configuration *
 *****************/

#define LAMP_TEST_PIN 8
#define LAMP_TEST_ACTIVE LOW
#define LAMP_TEST_INTENSITY 255

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
#define NUM_LAMPS 9
#define LAMP_MIN_INTENSITY 24

#define DMX_OUTPUT_PIN 3
#define DMX_NUM_CHANNELS_PER_LAMP 3
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


/***********
 * Globals *
 ***********/

// moving average variables
static int16_t maValues[NUM_MA_SETS][MA_LENGTH];
static int32_t maSums[NUM_MA_SETS];
static uint8_t nextSlotIdx[NUM_MA_SETS];
static bool maSetFull[NUM_MA_SETS];

static int16_t avgPpSound;
static double avgYaw;
static double avgPitch;
static double avgRoll;
static double avgLinearAccelX;
static double avgLinearAccelY;
static double avgLinearAccelZ;
static double avgGyroX;
static double avgGyroY;
static double avgGyroZ;

static int colorChannelIntensities[NUM_LAMPS][NUM_COLORS_PER_LAMP];

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)


/******************
 * Implementation *
 ******************/

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

  initRadio();
  initDmx();

#ifdef LAMP_TEST_PIN
#if LAMP_TEST_ACTIVE == LOW
  pinMode(LAMP_TEST_PIN, INPUT_PULLUP);
#else
  pinMode(LAMP_TEST_PIN, INPUT);
#endif
#endif

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


void getAverageMeasurements()
{
  avgPpSound = 0;
  avgYaw = getMovingAverage(0) / 10.0;
  avgPitch = getMovingAverage(1) / 10.0;
  avgRoll = getMovingAverage(2) / 10.0;
  avgLinearAccelX = getMovingAverage(3) / countsPerG;
  avgLinearAccelY = getMovingAverage(4) / countsPerG;
  avgLinearAccelZ = getMovingAverage(5) / countsPerG;
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
  Serial.print(F(" avgLinearAccelX="));
  Serial.print(avgLinearAccelX);
  Serial.print(F(" avgLinearAccelY="));
  Serial.print(avgLinearAccelY);
  Serial.print(F(" avgLinearAccelZ="));
  Serial.print(avgLinearAccelZ);
  Serial.print(F(" avgGyroX="));
  Serial.print(avgGyroX);
  Serial.print(F(" avgGyroY="));
  Serial.print(avgGyroY);
  Serial.print(F(" avgGyroZ="));
  Serial.println(avgGyroZ);
#endif
}


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
      avgPpSound = payload->velocity * speedupFactor;
      avgPpSound = abs(avgPpSound);
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
      avgPpSound = 0;
    }
    avgLinearAccelX = 0.0;
    avgLinearAccelY = 0.0;
    avgLinearAccelZ = 0.0;
    avgGyroX = 0.0;
    avgGyroY = 0.0;
    avgGyroZ = 0.0;
  }

  return gotMeasurement;
}


bool handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, uint8_t payloadSize)
{
  constexpr uint8_t numExpectedValues = 13;
  
  constexpr uint16_t expectedPayloadSize = sizeof(WidgetHeader) + sizeof(int16_t) * numExpectedValues;
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

  if (payload->widgetHeader.id != 1
      && payload->widgetHeader.id != 2
      && payload->widgetHeader.id != 3) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got MeasurementVectorPayload payload from widget "));
    Serial.print(payload->widgetHeader.id);
    Serial.println(F(" but expected one from widgets 1 (Tilt1), 2 (Tilt2), or 3 (Tilt spare)."));
#endif
    return false;
  }

  // Ignore payloads with all-zero data because the packet is probably
  // just a heartbeat while the widget is in standby mode.
  bool gotAllZeroData = true;
  for (uint8_t i = 0; gotAllZeroData && i < numExpectedValues; ++i) {
    gotAllZeroData = payload->measurements[0] == 0;
  }
  if (!gotAllZeroData) {
    avgYaw = payload->measurements[0] / 10.0;
    // TODO:  hack - swap pitch and roll for Tilt1 so that its pitch appears as roll to control which lamp is brightest
    if (payload->widgetHeader.id == 1) {
      avgRoll = payload->measurements[1] / 10.0;
      avgPitch = payload->measurements[2] / 10.0;
    }
    else {
      avgPitch = payload->measurements[1] / 10.0;
      avgRoll = payload->measurements[2] / 10.0;
    }
    avgGyroX = payload->measurements[3];
    avgGyroY = payload->measurements[4];
    avgGyroZ = payload->measurements[5];
    avgLinearAccelX = payload->measurements[9];
    avgLinearAccelY = payload->measurements[10];
    avgLinearAccelZ = payload->measurements[11];
    avgPpSound = payload->measurements[12];
  }
  
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
    Serial.print(F(" avgPpSound="));
    Serial.print(avgPpSound);
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


void sendDmx()
{
  uint8_t dmxChannelValues[DMX_NUM_CHANNELS + 1];

  uint16_t dmxChannelNum = 1;
  for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {

#if DMX_NUM_CHANNELS_PER_LAMP == 4
    // If the sound is sufficiently loud, flash all the lamps instead of setting their intensities.
    if (avgPpSound >= minPpSoundForStrobe) {
      uint8_t strobeValue = map(constrain(avgPpSound, minPpSoundForStrobe, minPpSoundForStrobe),
                                minPpSoundForStrobe, maxPpSoundForStrobe, minStrobeValue, maxStrobeValue);
      dmxChannelValues[dmxChannelNum++] = strobeValue;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("strobeValue="));
      Serial.print(strobeValue);
      Serial.print(F(" avgPpSound="));
      Serial.print(avgPpSound);
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
#endif

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
#ifdef LAMP_TEST_PIN
  if (digitalRead(LAMP_TEST_PIN) == LAMP_TEST_ACTIVE) {
    for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {
      for (uint8_t colorIdx = 0; colorIdx < NUM_COLORS_PER_LAMP; ++colorIdx) {
        colorChannelIntensities[lampIdx][colorIdx] = LAMP_TEST_INTENSITY;
      }
    }
    sendDmx();
    return;
  }
#endif

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

  pollRadio();

  if (now - lastDmxTxMs >= DMX_TX_INTERVAL_MS) {
    lastDmxTxMs = now;
    updateLamps();
  }
}

