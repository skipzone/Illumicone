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

#ifdef ENABLE_DEBUG_PRINT 
  radio.printDetails();
#endif
  
  // Widgets only transmit data.
  radio.stopListening();
}


