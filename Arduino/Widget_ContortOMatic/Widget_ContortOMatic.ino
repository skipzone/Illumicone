/*****************************************************************
 *                                                               *
 * Twister Widget                                                *
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

#include <ADCTouch.h>
#include "illumiconeWidget.h"
#include <LiquidCrystal.h>
#include "printf.h"




/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 10
#define ACTIVE_TX_INTERVAL_MS 50000L
#define INACTIVE_TX_INTERVAL_MS 100000L
//#define TX_FAILURE_LED_PIN 2

constexpr uint32_t gatherMeasurementsIntervalMs = 333;

constexpr uint8_t numCapSensePins = 2;
constexpr uint8_t capSensePins[numCapSensePins] = {A2, A3};
constexpr int capSenseNumSamplesForRef = 500;
constexpr int capSenseNumSamples = 10;
constexpr int capSenseThresholds[numCapSensePins] = {750, 750};

constexpr uint8_t greenLedPin = 3;
constexpr uint8_t greenLedStateOff = 1;
constexpr uint8_t greenLedStateOn = 0;

#define LCD_RS_PIN A1
#define LCD_E_PIN  A0
#define LCD_D4_PIN 5
#define LCD_D5_PIN 6
#define LCD_D6_PIN 7
#define LCD_D7_PIN 8
#define LCD_NUM_COLS 20
#define LCD_NUM_ROWS 2

#define NUM_VALUES_TO_SEND 2

#define ENABLE_DEBUG_PRINT
#define ENABLE_LCD


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

LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);
static FILE lcdout = {0};

static int capSenseRef[numCapSensePins];
static bool padIsTouched[numCapSensePins];
bool isActive;


/******************
 * Implementation *
 ******************/

static int lcd_putchar(char ch, FILE* stream)
{
  lcd.write(ch);
  return (0);
}


void initLcd()
{
  lcd.begin(LCD_NUM_COLS, LCD_NUM_ROWS);

  fdev_setup_stream(&lcdout, lcd_putchar, NULL, _FDEV_SETUP_WRITE);

  lcd.setCursor(0, 0);  // col, row
  lcd.print(F("                    "));
  lcd.setCursor(0, 1);
  lcd.print(F("                    "));
}


void setup()
{
  pinMode(greenLedPin, OUTPUT);

#ifdef ENABLE_DEBUG_PRINT
  Serial.begin(115200);
#endif

#if defined(ENABLE_DEBUG_PRINT) || defined(ENABLE_LCD)
  printf_begin();
#endif

#ifdef ENABLE_LCD
  initLcd();
#endif

//          // Create reference value to account for stray
//          // capacitance and capacitance of the pad.
//          capSenseRef[stepNum] = ADCTouch.read(capSensePins[stepNum], capSenseNumSamplesForRef);

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;
}


void gatherMeasurements()
{
  unsigned long now = millis();
  isActive = true;

  for (uint8_t i = 0; i < numCapSensePins; ++i) {
    int value = ADCTouch.read(capSensePins[i], capSenseNumSamples) - capSenseRef[i];
#ifdef ENABLE_DEBUG_PRINT
    Serial.print(i);
    Serial.print(":");
    Serial.println(value);
#endif
    padIsTouched[i] = value > capSenseThresholds[i];
  }

#ifdef ENABLE_DEBUG_PRINT
  for (int i = 0; i < NUM_VALUES_TO_SEND; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.print(capSenseRef[i]);
    Serial.print(", ");
    Serial.println(padIsTouched[i]);
  }
#endif

  digitalWrite(greenLedPin, padIsTouched[0] ? greenLedStateOn : greenLedStateOff);

}


void sendMeasurements()
{
  for (int i = 0; i < NUM_VALUES_TO_SEND; ++i) {
      payload.measurements[i] = padIsTouched[i];
  }

  payload.widgetHeader.isActive = isActive;

  if (!radio.write(&payload, sizeof(WidgetHeader) + sizeof(int16_t) * NUM_VALUES_TO_SEND)) {
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
  static uint32_t lastGatherMeasurementsMs;

  uint32_t now = millis();

  if ((int32_t) (now - lastGatherMeasurementsMs) >= 0) {
    lastGatherMeasurementsMs += gatherMeasurementsIntervalMs;
    gatherMeasurements();
  }

  if (now - lastTxMs >= (isActive ? ACTIVE_TX_INTERVAL_MS : INACTIVE_TX_INTERVAL_MS)) {
    sendMeasurements();
    lastTxMs = now;
  }

}

