/*****************************************************
 *                                                   *
 * GardenSpinner Interactive Lamps Pattern Generator *
 *                                                   *
 * Ross Butler  November 2019                        *
 *                                                   *
 * based on OctoFlashy rev. 2019-11-09               *
 *****************************************************/


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

#ifndef ENABLE_DEBUG_PRINT
#include "DmxSimple.h"
#else
#include "printf.h"
#endif


/*********************************************
 * Implementation and Behavior Configuration *
 *********************************************/

#define IMU_INTERRUPT_PIN 2
#define DMX_OUTPUT_PIN 3
#define LAMP_TEST_PIN 8

#define LAMP_TEST_ACTIVE LOW
#define LAMP_TEST_INTENSITY 255

#define DMX_TX_INTERVAL_MS 33L

#define NUM_LAMPS 5

#define LAMP_MIN_INTENSITY 64
#define LAMP_MAX_INTENSITY 255


/***********************
 * Radio Configuration *
 ***********************/

// Possible data rates are RF24_250KBPS, RF24_1MBPS, or RF24_2MBPS (genuine Noric chips only).
#define DATA_RATE RF24_1MBPS

// Valid CRC length values are RF24_CRC_8, RF24_CRC_16, and RF24_CRC_DISABLED
#define CRC_LENGTH RF24_CRC_16

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
// Illumicone widgets use channel 84.  IBG widgets used channel 97 in 2018.
#define RF_CHANNEL 84

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

#define ACK_WIDGET_PACKETS false

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/*************************
 * Don't mess with these *
 *************************/

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


/**********************************************************
 * Widget Packet Header and Payload Structure Definitions *
 **********************************************************/

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

static float currentRotationAngle;

static int lampIntensities[NUM_LAMPS + 1];      // +1 because the last element is a virtual lamp that facilitates wraparound

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)


/******************
 * Implementation *
 ******************/

void initRadio()
{
  Serial.println(F("Initializing radio..."));    
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

  Serial.println(F("Radio initialized."));    
}


void initDmx()
{
#ifndef ENABLE_DEBUG_PRINT
  DmxSimple.usePin(DMX_OUTPUT_PIN);
  DmxSimple.maxChannel(DMX_NUM_CHANNELS);
#endif
}


void setup()
{
#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
  printf_begin();
  Serial.println(F("Debug print enabled."));    
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


bool handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, uint8_t payloadSize)
{
  if (payload->widgetHeader.id < 11 || payload->widgetHeader.id > 18) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got MeasurementVectorPayload payload from widget "));
    Serial.print(payload->widgetHeader.id);
    Serial.println(F(" but expected one from a flower widget (ids 11-18)."));
#endif
    return false;
  }

  uint8_t numExpectedValues = 7         // flower widgets send ypr, gyro xyz, and temperature
  uint16_t expectedPayloadSize = sizeof(WidgetHeader) + sizeof(int16_t) * numExpectedValues;
  if (payloadSize != expectedPayloadSize) {
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(F("got MeasurementVectorPayload from widget "));
    Serial.print(payload->widgetHeader.id);
    Serial.print(F(" with "));
    Serial.print(payloadSize);
    Serial.print(F(" bytes but expected "));
    Serial.print(expectedPayloadSize);
    Serial.println(F(" bytes."));    
#endif
    return false;
  }

  // Ignore payloads with all-zero data because the packet is probably
  // just a heartbeat while the widget is in standby mode.
  bool gotAllZeroData = true;
  for (uint8_t i = 0; gotAllZeroData && i < numExpectedValues; ++i) {
    gotAllZeroData = payload->measurements[0] == 0;
  }
  if (gotAllZeroData) {
    return false;
  }
  
  switch (payload->widgetHeader.id) {

    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      // yaw (0-3599 tenths of a degree) represents the rotation angle
      currentRotationAngle = ((float) payload->measurements[1]) / 10.0;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got pitch "));
      Serial.print(currentRotationAngle);
      Serial.println(F(" for lamp angle from widget 1"));
#endif
      break;
  }
  
  return true;
}


void pollRadio()
{
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
#ifdef ENABLE_DEBUG_PRINT
  Serial.print(F("got message on pipe "));
  Serial.println(pipeNum);
#endif

  radio.read(payload, payloadSize);

  bool gotMeasurements = false;
  switch(pipeNum) {
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
    Serial.print(F(" currentRotationAngle="));
    Serial.print(currentRotationAngle);
  }
#endif
}


void sendDmx()
{
  // Allocate the array one larger than the number of channels
  // to facilitate 1-based indexing.  Element zero is not used.
  uint8_t dmxChannelValues[DMX_NUM_CHANNELS + 1];

  for (uint8_t i = 1; i <= DMX_NUM_CHANNELS; dmxChannelValues[i++] = 0);

  dmxChannelValue[ 1] = lampIntensities[0] + lampIntensities[5];    // add virtual lamp for wraparound
  dmxChannelValue[ 4] = lampIntensities[1];
  dmxChannelValue[ 7] = lampIntensities[2];
  dmxChannelValue[10] = lampIntensities[3];

  // Transmit the DMX channel values.
#ifndef ENABLE_DEBUG_PRINT
  for (dmxChannelNum = 1; dmxChannelNum <= DMX_NUM_CHANNELS; ++dmxChannelNum) {
    DmxSimple.write(dmxChannelNum, dmxChannelValues[dmxChannelNum]);
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("sent DMX ch "));
//    Serial.print(dmxChannelNum);
//    Serial.print(F(": "));
//    Serial.println(dmxChannelValues[dmxChannelNum]);    
//#endif
  }
#endif
}


void updateLamps()
{
#ifdef LAMP_TEST_PIN
  if (digitalRead(LAMP_TEST_PIN) == LAMP_TEST_ACTIVE) {
    for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {
      lampIntensities[lampIdx] = LAMP_TEST_INTENSITY;
    }
    lampIntensities[lampIdx] = 0;       // turn off the virtual lamp because no wraparound needed
#ifndef ENABLE_DEBUG_PRINT
    sendDmx();
#endif
    return;
  }
#endif

  constexpr float lampRotationStepAngle = 360.0 / (float) (NUM_LAMPS - 1);

  for (uint8_t i = 0; i <= NUM_LAMPS; ++i) {
    lampIntensities[i] = LAMP_MIN_INTENSITY;
  }

  // Fade across the lamps based on lamp selection angle.  The trick here is
  // that there are no more than two lamps illuminated at a time.  Fading across
  // pairs of lamps in succession gives the illusion of movement.  currentRotationAngle is
  // supposed to be 0 to 359.9 degrees.  If the widget sent crap data, we will
  // just pretend the angle is 0.
  int fadeDownLamp = currentRotationAngle >= 0 && currentRotationAngle < 360 ? currentRotationAngle / lampRotationStepAngle : 0;
  int fadeUpLamp = fadeDownLamp + 1;
// TODO:  the math says we shouldn't need this as long as the widget doesn't send shit data
//  // Fix up fadeDownLamp if measurement is maxLampAngleDegrees degrees or more.
//  if (fadeDownLamp >= NUM_LAMPS - 1) {
//      fadeDownLamp = NUM_LAMPS - 2;
//  }
  lampIntensities[fadeDownLamp] =
    map(currentRotationAngle,
        lampRotationStepAngle * fadeDownLamp, lampRotationStepAngle * (fadeUpLamp),
        LAMP_MAX_INTENSITY, LAMP_MIN_INTENSITY);
  lampIntensities[fadeUpLamp] =
    map(currentRotationAngle,
        lampRotationStepAngle * fadeDownLamp, lampRotationStepAngle * (fadeUpLamp),
        LAMP_MIN_INTENSITY, LAMP_MAX_INTENSITY);

  sendDmx();
}


void loop()
{
  static int32_t lastDmxTxMs;

  uint32_t now = millis();

  pollRadio();

  if (now - lastDmxTxMs >= DMX_TX_INTERVAL_MS) {
    lastDmxTxMs = now;
    updateLamps();
  }

#ifdef ENABLE_WATCHDOG
  wdt_reset();
#endif
}
