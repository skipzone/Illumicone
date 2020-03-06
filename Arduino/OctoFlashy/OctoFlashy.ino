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

// Restrict color and lamp selection angles to avoid gimbal lock.
constexpr float maxColorAngleDegrees = 45;
constexpr float maxLampAngleDegrees = 45;

constexpr int16_t minPpSoundForStrobe = 300;
constexpr int16_t maxPpSoundForStrobe = 500;
constexpr uint8_t minStrobeValue = 225;
constexpr uint8_t maxStrobeValue = 250;

#define NUM_LAMPS 8

#define LAMP_MIN_INTENSITY 64

// With RGB lamps, enable red-green-blue-red wraparound as widget value
// varies from one extreme to another.  (Without wraparound, the lamp
// color would be red at one extreme and blue at the other, with no way
// to go from blue through violet and magenta back to red.)
#define ENABLE_COLOR_WRAPAROUND

// The cheap-o RGB "PAR" lamps have a fourth DMX channel for strobe and intensity.
#define LAMP_HAS_CONTROL_CHANNEL

// With four-channel dimmers, it can be convenient to leave the fourth channel
// unused so that each dimmer controls one tri-color lamp arrangement.
//#define SKIP_DIMMER_FOURTH_CHANNEL


/***********************
 * Radio Configuration *
 ***********************/

// Possible data rates are RF24_250KBPS, RF24_1MBPS, or RF24_2MBPS (genuine Noric chips only).
#define DATA_RATE RF24_250KBPS

// Valid CRC length values are RF24_CRC_8, RF24_CRC_16, and RF24_CRC_DISABLED
#define CRC_LENGTH RF24_CRC_16

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
// Illumicone widgets use channel 84.  IBG widgets used channel 97 in 2018.
#define RF_CHANNEL 80

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

#ifdef ENABLE_COLOR_WRAPAROUND
  // With RGB lamps, we want to fade from red to green to blue then back to red as
  // the corresponding value from the widget goes from one extreme to the other.
  // To facilitate that, we track four color intensities, with both the first and
  // last representing the red intensity.
  #define NUM_COLORS_PER_LAMP 4
#else
  #define NUM_COLORS_PER_LAMP 3
#endif

#if defined(LAMP_HAS_CONTROL_CHANNEL) || defined(SKIP_DIMMER_FOURTH_CHANNEL)
#define DMX_NUM_CHANNELS_PER_LAMP 4
#else
#define DMX_NUM_CHANNELS_PER_LAMP 3
#endif
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
    uint8_t id       : 5;
    uint8_t channel  : 2;
    bool    isActive : 1;
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

static double currentColorAngle;
static double currentLampAngle;
static int16_t currentPpSound;

static int colorChannelIntensities[NUM_LAMPS][NUM_COLORS_PER_LAMP];

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
//    case 0:
//      // Vary yaw continuously between -180 and 180.
//      pmax = 180;
//      p = (payload->position * speedupFactor) % (pmax * 4);
//      p = abs(p);                                 // in Arduinolandia, abs is a macro
//      currentSomethingAngle = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
//      gotMeasurement = true;
//      break;
    case 1:
      // Vary color selection angle continuously between -maxColorAngleDegrees and maxColorAngleDegrees.
      pmax = maxColorAngleDegrees;
      p = (payload->position * speedupFactor) % (pmax * 4);
      p = abs(p);                                 // in Arduinolandia, abs is a macro
      currentColorAngle = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
      gotMeasurement = true;
      break;
    case 2:
      // Vary lamp selection angle continuously between -maxLampAngleDegrees and maxLampAngleDegrees.
      pmax = maxLampAngleDegrees;
      p = (payload->position * speedupFactor) % (pmax * 4);
      p = abs(p);                                 // in Arduinolandia, abs is a macro
      currentLampAngle = (p <= pmax * 2) ? p - pmax : pmax * 3 - p;
      gotMeasurement = true;
      break;
    case 3:
      currentPpSound = payload->velocity * speedupFactor;
      currentPpSound = abs(currentPpSound);
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
      currentPpSound = 0;
    }
  }

  return gotMeasurement;
}


//bool handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, uint8_t payloadSize)
bool handleMeasurementVectorPayload(MeasurementVectorPayload* payload, uint8_t payloadSize)
{
  if (payload->widgetHeader.id == 12) payload->widgetHeader.id = 3;  // Pretend that Baton is Rainstick
  
//  if (payload->widgetHeader.id < 1 || payload->widgetHeader.id > 4) {
//#ifdef ENABLE_DEBUG_PRINT
//    Serial.print(F("got MeasurementVectorPayload payload from widget "));
//    Serial.print(payload->widgetHeader.id);
//    Serial.println(F(" but expected one from widgets 1 (Tilt-1), 2 (Tilt-2), 3 (Tilt-Test), or 4 (Rainstick)."));
//#endif
//    return false;
//  }

// TODO:  Enable the next two lines and remove the third when Rainstick is running current firmware.
  // Rainstick sends everything the tilt widgets send plus a peak-to-peak sound value.
  uint8_t numExpectedValues;
  switch(payload->widgetHeader.id) {
    case 30:
    case 31:
      numExpectedValues = 4;
      break;
    default:
      numExpectedValues = 0;
      break;
  }
//  numExpectedValues = payload->widgetHeader.id <= 3 ? 13 : 14;
//  uint8_t numExpectedValues = 13;
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
    gotAllZeroData = payload->measurements[i] == 0;
  }
  if (gotAllZeroData) {
    return false;
  }
  
  switch (payload->widgetHeader.id) {

    case 1:
      // Tilt-1's pitch is the lamp selection angle.
      currentLampAngle = payload->measurements[1] / 10.0;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got pitch "));
      Serial.print(currentLampAngle);
      Serial.println(F(" for lamp angle from widget 1"));
#endif
      break;

    case 2:
      // Tilt-2's pitch is the color selection angle.
      currentColorAngle = payload->measurements[1] / 10.0;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got pitch "));
      Serial.print(currentColorAngle);
      Serial.println(F(" for color angle from widget 2"));
#endif
      break;

    case 3:
      // Tilt-Test's pitch is the lamp selection angle, and its roll is the color selection angle.
      currentLampAngle = payload->measurements[1] / 10.0;
      currentColorAngle = payload->measurements[2] / 10.0;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got pitch "));
      Serial.print(currentLampAngle);
      Serial.print(F(" for lamp angle and roll "));
      Serial.print(currentColorAngle);
      Serial.println(F(" for color angle from widget 2"));
#endif
      break;

    case 4:
    // TODO:  fix the damn comment below
      // Rainstick's pitch is the color selection angle, and its pitch is the color selection angle.
      currentColorAngle = payload->measurements[1] / 10.0;
      currentLampAngle = payload->measurements[2] / 10.0;
      currentPpSound = payload->measurements[13];
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got pitch "));
      Serial.print(currentColorAngle);
      Serial.print(F(" for color angle, roll "));
      Serial.print(currentLampAngle);
      Serial.print(F(" for lamp angle, and p-p sound "));
      Serial.println(currentPpSound);
#endif
    break;
  
    case 30:
    case 31:
      // Theremin's measurement 1 is the lamp selection angle, and measurement 2 is the color selection angle.
      // (Measurment 0 is the pattern number, which we don't use here.)
      currentLampAngle = map(payload->measurements[1], 0, 1023, -maxLampAngleDegrees, maxLampAngleDegrees);
      currentColorAngle = map(payload->measurements[2], 0, 1023, -maxColorAngleDegrees, maxColorAngleDegrees);
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("got measmt "));
      Serial.print(payload->measurements[1]);
      Serial.print(F(" for lamp angle "));
      Serial.print(currentLampAngle);
      Serial.print(F(" and measmt "));
      Serial.print(payload->measurements[2]);
      Serial.print(F(" for color angle "));
      Serial.print(currentColorAngle);
      Serial.println(F(" from widget 30"));
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
    Serial.print(F(" currentColorAngle="));
    Serial.print(currentColorAngle);
    Serial.print(F(" currentLampAngle="));
    Serial.print(currentLampAngle);
    Serial.print(F(" currentPpSound="));
    Serial.println(currentPpSound);
  }
#endif
}


void sendDmx()
{
  uint8_t dmxChannelValues[DMX_NUM_CHANNELS + 1];

  uint16_t dmxChannelNum = 1;
  for (uint8_t lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {

#if defined(LAMP_HAS_CONTROL_CHANNEL)
    // If the sound is sufficiently loud, flash all the lamps instead of setting their intensities.
    if (currentPpSound >= minPpSoundForStrobe) {
      uint8_t strobeValue = map(constrain(currentPpSound, minPpSoundForStrobe, minPpSoundForStrobe),
                                minPpSoundForStrobe, maxPpSoundForStrobe, minStrobeValue, maxStrobeValue);
      dmxChannelValues[dmxChannelNum++] = strobeValue;
#ifdef ENABLE_DEBUG_PRINT
      Serial.print(F("strobeValue="));
      Serial.print(strobeValue);
      Serial.print(F(" currentPpSound="));
      Serial.print(currentPpSound);
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

#if (NUM_COLORS_PER_LAMP == 4)
    dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][0] + colorChannelIntensities[lampIdx][3];
#else
    dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][0];
#endif
    dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][1];
    dmxChannelValues[dmxChannelNum++] = colorChannelIntensities[lampIdx][2];      

#ifdef SKIP_DIMMER_FOURTH_CHANNEL
    dmxChannelValues[dmxChannelNum++] = 0;
#endif
  }

  // Send zeros to any unused channels.
  while (dmxChannelNum <= DMX_NUM_CHANNELS) {
    dmxChannelValues[dmxChannelNum++] = 0;
  }

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
      for (uint8_t colorIdx = 0; colorIdx < 3; ++colorIdx) {
        colorChannelIntensities[lampIdx][colorIdx] = LAMP_TEST_INTENSITY;
      }
    }
#ifndef ENABLE_DEBUG_PRINT
    sendDmx();
#endif
    return;
  }
#endif

  constexpr float colorAngleStep = maxColorAngleDegrees * 2 / (NUM_COLORS_PER_LAMP - 1);
  constexpr float lampAngleStep = maxLampAngleDegrees * 2 / (NUM_LAMPS - 1);

  int colorIntensities[NUM_COLORS_PER_LAMP];
  for (uint8_t i = 0; i < NUM_COLORS_PER_LAMP; ++i) {
    colorIntensities[i] = 0;
  }

  int lampIntensities[NUM_LAMPS];
  for (uint8_t i = 0; i < NUM_LAMPS; ++i) {
    lampIntensities[i] = LAMP_MIN_INTENSITY;
  }

  // Normalize and restrict color and lamp selection angles to [0, max___Degrees*2] degrees.
  float normalizedColorAngle;
  if (currentColorAngle <= - maxColorAngleDegrees) {
    normalizedColorAngle = 0;
  }
  else if (currentColorAngle >= maxColorAngleDegrees) {
    normalizedColorAngle = maxColorAngleDegrees * 2.0;
  }
  else {
    normalizedColorAngle = currentColorAngle + maxColorAngleDegrees;
  }
  float normalizedLampAngle;
  if (currentLampAngle <= - maxLampAngleDegrees) {
    normalizedLampAngle = 0;
  }
  else if (currentLampAngle >= maxLampAngleDegrees) {
    normalizedLampAngle = maxLampAngleDegrees * 2.0;
  }
  else {
    normalizedLampAngle = currentLampAngle + maxLampAngleDegrees;
  }

  // Fade across the colors based on color selection angle.
  int colorSection = floor(normalizedColorAngle / colorAngleStep);
  // Fix up colorSection if measurement is maxColorAngleDegrees degrees or more.
  if (colorSection >= NUM_COLORS_PER_LAMP - 1) {
      colorSection = NUM_COLORS_PER_LAMP - 2;
  }
  colorIntensities[colorSection] =
    map(normalizedColorAngle, colorAngleStep * colorSection, colorAngleStep * (colorSection + 1), 255, 0);
  colorIntensities[colorSection + 1] =
    map(normalizedColorAngle, colorAngleStep * colorSection, colorAngleStep * (colorSection + 1), 0, 255);

  // Fade across the lamps based on lamp selection angle.
  int lampSection = floor(normalizedLampAngle / lampAngleStep);
  // Fix up lampSection if measurement is maxLampAngleDegrees degrees or more.
  if (lampSection >= NUM_LAMPS - 1) {
      lampSection = NUM_LAMPS - 2;
  }
  lampIntensities[lampSection] =
    map(normalizedLampAngle, lampAngleStep * lampSection, lampAngleStep * (lampSection + 1), 255, LAMP_MIN_INTENSITY);
  lampIntensities[lampSection + 1] =
    map(normalizedLampAngle, lampAngleStep * lampSection, lampAngleStep * (lampSection + 1), LAMP_MIN_INTENSITY, 255);

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
