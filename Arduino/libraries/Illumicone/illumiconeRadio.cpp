/*****************************************************************
 *                                                               *
 * Illumicone Widget Radio Functions                             *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
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

#include "illumiconeWidget.h"


void configureRadio(
  RF24&             radio,
  const char*       writePipeAddress,
  bool              wantAcks,
  uint8_t           txRetryDelayMultiplier,
  uint8_t           txMaxRetries,
  rf24_crclength_e  crcLength,
  rf24_pa_dbm_e     rfPowerLevel,
  rf24_datarate_e   dataRate,
  uint8_t           channel)
{
  radio.begin();

  radio.setPALevel(rfPowerLevel);
  radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
  radio.setDataRate(dataRate);
  radio.setChannel(channel);
  radio.enableDynamicAck();         // allow sending payloads with or without ack request
  radio.enableDynamicPayloads();
  radio.setCRCLength(crcLength);

  radio.openWritingPipe((const uint8_t*) writePipeAddress);

#ifdef ENABLE_DEBUG_PRINT 
  radio.printDetails();
#endif
  
  // Widgets only transmit data.
  radio.stopListening();
}


