/*****************************************************************
 *                                                               *
 * nRF24 Stress Test Widget                                      *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, June 2016                                 )'( *
 *                                                               *
 *****************************************************************/

#include "illumiconeWidget.h"
#include "printf.h"


/************************
 * Widget Configuration *
 ************************/

#define WIDGET_ID 1
#define TX_INTERVAL_MS 20L
#define STATS_PRINT_INTERVAL_MS 1000L
//#define LED_PIN 2


/***************************************
 * Widget-Specific Radio Configuration *
 ***************************************/

// Nwdgt, where N indicates the pipe number (0-6) and payload type (0: stress test;
// 1: position & velocity; 2: measurement vector; 3,4: undefined; 5: custom
#define TX_PIPE_ADDRESS "0wdgt"       // 0 for tx stress

// Delay between retries is 250 us multiplied by the delay multiplier.  To help
// prevent repeated collisions, use a prime number (2, 3, 5, 7, 11) or 15 (the max).
#define TX_RETRY_DELAY_MULTIPLIER 3

// Max. retries can be 0 to 15.
#define TX_MAX_RETRIES 15

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
#define RF_POWER_LEVEL RF24_PA_MAX


/***********
 * Globals *
 ***********/

RF24 radio(9, 10);    // CE on pin 9, CSN on pin 10, also uses SPI bus (SCK on 13, MISO on 12, MOSI on 11)

StressTestPayload payload;


/******************
 * Implementation *
 ******************/

void setup()
{
  Serial.begin(57600);
  printf_begin();

  configureRadio(radio, TX_PIPE_ADDRESS, TX_RETRY_DELAY_MULTIPLIER, TX_MAX_RETRIES, RF_POWER_LEVEL);
  
  payload.widgetHeader.id = WIDGET_ID;
  payload.widgetHeader.isActive = false;
  payload.widgetHeader.channel = 0;
}


void loop() {

  static int32_t lastTxMs;
  static int32_t lastStatsPrintMs;
  static int32_t lastStatsPrintPayloadNum;

  uint32_t now = millis();
  if (now - lastTxMs >= TX_INTERVAL_MS) {

    ++payload.payloadNum;
    ++payload.widgetHeader.channel;
    payload.widgetHeader.isActive = !payload.widgetHeader.isActive;

    //  Send the time.  This will block until complete.
    if (!radio.write(&payload, sizeof(payload))) {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, HIGH);
#endif
      ++payload.numTxFailures;
      //Serial.print(F("radio.write failed for "));
      //Serial.println(payload.payloadNum);
    }
    else {
#ifdef LED_PIN      
      digitalWrite(LED_PIN, LOW);
#endif
    }
    
    lastTxMs = now;
  }

  now = millis();
  uint32_t statsIntervalMs = now - lastStatsPrintMs;
  if (statsIntervalMs >= STATS_PRINT_INTERVAL_MS) {
    uint32_t sendRate = (payload.payloadNum - lastStatsPrintPayloadNum) * 1000L / statsIntervalMs;
    uint32_t pctFail = payload.numTxFailures * 100L / payload.payloadNum;
    Serial.print(F("---------- "));
    Serial.print(payload.payloadNum);
    Serial.print(F(" sent, "));
    Serial.print(payload.numTxFailures);
    Serial.print(F(" failures ("));
    Serial.print(pctFail);
    Serial.print(F("%), "));
    Serial.print(sendRate);
    Serial.print(F("/s over last "));
    Serial.print(statsIntervalMs);
    Serial.println(F(" ms"));
    lastStatsPrintMs = now;
    lastStatsPrintPayloadNum = payload.payloadNum;
  }

}

