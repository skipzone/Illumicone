/*****************************************************
 *                                                   *
 * GardenSpinner Interactive Lamps Pattern Generator *
 *                                                   *
 * Ross Butler  November 2019                        *
 *                                                   *
 * based on OctoFlashy rev. 2019-11-09               *
 *****************************************************/

// TODO:  turn on built-in LED when widget is active
// TODO:  turn on all lamps when widget is inactive
// TODO:  fade lights down when widget becomes active, fade up when it goes inactive
// TODO:  auto-inactive timeout

/***********
 * Options *
 ***********/

#define ENABLE_WATCHDOG
//#define ENABLE_DEBUG_PRINT
//#define ENABLE_SIMULATED_MEASUREMENTS
#define USE_ROSE_GARDEN_2019_MAPPING


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

// TODO:  change to 20 for the garden
#define DMX_NUM_CHANNELS 27
#define DMX_TX_INTERVAL_MS 33L

#define NUM_LAMPS 4

#define LAMP_MIN_INTENSITY 64
#define LAMP_MAX_INTENSITY 255
//#define ENABLE_GAMMA_CORRECTION

#define SIMULATED_MEASUREMENT_UPDATE_INTERVAL_MS 15
#define SIMULATED_MEASUREMENT_STEP 1


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
// Illumicone widgets use channel 97.
#define RF_CHANNEL 97

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

// Probably no need to ever set auto acknowledgement to false because the sender
// can control whether or not acks are sent by using the NO_ACK bit.
#define ACK_WIDGET_PACKETS true

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX

// 250 us additional delay multiplier (0-15)
#define TX_RETRY_DELAY_MULTIPLIER 15

// max retries (0-15)
#define TX_MAX_RETRIES 15


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


/***********************
 * Types and Constants *
 ***********************/
// gamma correction for human-eye perception of WS2812 RGB LED brightness
// (from http://rgb-123.com/ws2812-color-output/ on 3 April 2014).
// Hopefully, will work well for single-color LED light strings.
uint8_t g_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
    2,  2,  2,  3,  3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,  5,
    6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 11, 11,
   11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
   19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28,
   29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
   40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54,
   55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
   71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
   90, 91, 93, 94, 95, 96, 98, 99,100,102,103,104,106,107,109,110,
  111,113,114,116,117,119,120,121,123,124,126,128,129,131,132,134,
  135,137,138,140,142,143,145,146,148,150,151,153,155,157,158,160,
  162,163,165,167,169,170,172,174,176,178,179,181,183,185,187,189,
  191,193,194,196,198,200,202,204,206,208,210,212,214,216,218,220,
  222,224,227,229,231,233,235,237,239,241,244,246,248,250,252,255
};

#ifdef ENABLE_GAMMA_CORRECTION
  #define GAMMA(x) (g_gamma[x])
#else
  #define GAMMA(x) (x)
#endif


/***********
 * Globals *
 ***********/

static bool widgetIsActive;
static float currentRotationAngle;

static uint8_t lampIntensities[NUM_LAMPS + 1];      // +1 because the last element is a virtual lamp that facilitates wraparound

static RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)


/******************
 * Implementation *
 ******************/

void initRadio()
{
  Serial.println(F("Initializing radio..."));    
  radio.begin();

  radio.setPALevel(RF_POWER_LEVEL);
  radio.setRetries(TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES);
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

  uint8_t numExpectedValues = 7;          // flower widgets send ypr, gyro xyz, and temperature
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
    widgetIsActive = false;
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
      if (payload->widgetHeader.isActive) {
        widgetIsActive = true;
        // yaw (0-3599 tenths of a degree) represents the rotation angle
        currentRotationAngle = ((float) payload->measurements[0]) / 10.0;
#ifdef ENABLE_DEBUG_PRINT
        Serial.print(F("got yaw "));
        Serial.print(currentRotationAngle);
        Serial.println(F(" for rotation angle from widget 1"));
#endif
      }
      else {
        widgetIsActive = false;
      }
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

#ifdef USE_ROSE_GARDEN_2019_MAPPING

  // yellow, outer ring (1, 18, 17, 2)
  dmxChannelValues[ 1] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[18] = GAMMA(lampIntensities[1]);
  dmxChannelValues[17] = GAMMA(lampIntensities[2]);
  dmxChannelValues[ 2] = GAMMA(lampIntensities[3]);

  // pink, inside ring (6, 14, 13, 5)
  dmxChannelValues[13] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[ 5] = GAMMA(lampIntensities[1]);
  dmxChannelValues[ 6] = GAMMA(lampIntensities[2]);
  dmxChannelValues[14] = GAMMA(lampIntensities[3]);

  // white, pink, yellow center tree
  dmxChannelValues[ 9] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[10] = GAMMA(lampIntensities[1]);
  dmxChannelValues[12] = GAMMA(lampIntensities[2]);

#else

  dmxChannelValues[ 1] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[ 4] = GAMMA(lampIntensities[1]);
  dmxChannelValues[ 7] = GAMMA(lampIntensities[2]);
  dmxChannelValues[10] = GAMMA(lampIntensities[3]);

  dmxChannelValues[26] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[23] = GAMMA(lampIntensities[1]);
  dmxChannelValues[20] = GAMMA(lampIntensities[2]);
  dmxChannelValues[17] = GAMMA(lampIntensities[3]);

  dmxChannelValues[13] = GAMMA(lampIntensities[0] + lampIntensities[4]);    // add virtual lamp for wraparound
  dmxChannelValues[14] = GAMMA(lampIntensities[1]);
  dmxChannelValues[15] = GAMMA(lampIntensities[2]);

#endif

  // Transmit the DMX channel values.
#ifndef ENABLE_DEBUG_PRINT
  for (uint8_t dmxChannelNum = 1; dmxChannelNum <= DMX_NUM_CHANNELS; ++dmxChannelNum) {
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
    lampIntensities[NUM_LAMPS] = 0;     // turn off the virtual lamp because no wraparound needed
    sendDmx();
    return;
  }
#endif

  if (!widgetIsActive) {
    uint8_t lampIdx;
    for (lampIdx = 0; lampIdx < NUM_LAMPS; ++lampIdx) {
      lampIntensities[lampIdx] = LAMP_MAX_INTENSITY;
    }
    lampIntensities[lampIdx] = 0;       // turn off the virtual lamp because no wraparound needed
    sendDmx();
    return;
  }

  constexpr float lampRotationStepAngle = 360.0 / (float) NUM_LAMPS;

  for (uint8_t lampIdx = 0; lampIdx <= NUM_LAMPS - 1; ++lampIdx) {
    lampIntensities[lampIdx] = LAMP_MIN_INTENSITY;
  }
  // The first lamp already has the minimum intensity, so the virtual lamp doesn't need it.
  lampIntensities[NUM_LAMPS] = 0;

  // Fade across the lamps based on lamp selection angle.  The trick here is
  // that there are no more than two lamps illuminated at a time.  Fading across
  // pairs of lamps in succession gives the illusion of movement.  currentRotationAngle is
  // supposed to be 0 to 359.9 degrees.  If the widget sent crap data, we will
  // just pretend the angle is 0.
  int fadeDownLamp = currentRotationAngle >= 0 && currentRotationAngle < 360 ? currentRotationAngle / lampRotationStepAngle : 0;
  int fadeUpLamp = fadeDownLamp + 1;
  lampIntensities[fadeDownLamp] =
    map(currentRotationAngle,
        lampRotationStepAngle * fadeDownLamp, lampRotationStepAngle * (fadeUpLamp),
        LAMP_MAX_INTENSITY, LAMP_MIN_INTENSITY);
  lampIntensities[fadeUpLamp] =
    map(currentRotationAngle,
        lampRotationStepAngle * fadeDownLamp, lampRotationStepAngle * (fadeUpLamp),
        LAMP_MIN_INTENSITY, LAMP_MAX_INTENSITY);
  // When the virtual lamp is fading up, we're actually wrapping around to
  // the first lamp, so prevent the minimum intensity from affecting it.
  if (fadeUpLamp == NUM_LAMPS) {
    lampIntensities[0] = 0;
  }

  sendDmx();
}


void loop()
{
  static int32_t lastDmxTxMs;

  uint32_t now = millis();

#ifdef ENABLE_SIMULATED_MEASUREMENTS
  static int32_t lastMeasmtIncMs;
  if (now - lastMeasmtIncMs >= SIMULATED_MEASUREMENT_UPDATE_INTERVAL_MS) {
    lastMeasmtIncMs = now;
    currentRotationAngle += SIMULATED_MEASUREMENT_STEP;
    if (currentRotationAngle >= 360.0) {
      currentRotationAngle -= 360.0;
    }
    widgetIsActive = true;
  }
#endif

  pollRadio();

  if (now - lastDmxTxMs >= DMX_TX_INTERVAL_MS) {
    lastDmxTxMs = now;
    updateLamps();
  }

#ifdef ENABLE_WATCHDOG
  wdt_reset();
#endif
}
