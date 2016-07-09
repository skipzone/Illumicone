/*****************************************************************
 *                                                               *
 * Illumicone Widget Radio Functions                             *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
 *                                                               *
 *****************************************************************/

#include "illumiconeWidget.h"


void configureRadio(
    RF24&         radio,
    const char*   writePipeAddress,
    uint8_t       txRetryDelayMultiplier,
    uint8_t       txMaxRetries,
    rf24_pa_dbm_e rfPowerLevel)
{
  radio.begin();

  radio.setPALevel(rfPowerLevel);
  radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
  radio.setDataRate(DATA_RATE);
  radio.setChannel(RF_CHANNEL);
  radio.setAutoAck(1);
  radio.enableDynamicPayloads();
  radio.setCRCLength(CRC_LENGTH);

  radio.openWritingPipe((const uint8_t*) writePipeAddress);
  
  radio.printDetails();
  
  // Widgets only transmit data.
  radio.stopListening();
}


