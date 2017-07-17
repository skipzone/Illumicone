/*****************************************************************
 *                                                               *
 * Illumicone Widget Common Includes and Definitions             *
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

#pragma once

#include <SPI.h>
#include "RF24.h"
#include "illumiconeRadio.h"


/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Eye (Ray)
 *  2:  Shirley's Web (Reiley)
 *  3:  Bells (Ray)
 *  4:  Rainstick (Dr. Naked)
 *  5:  Schroeder's Plaything (Ross)
 *  6:  TriObelisk (Phyxx and Dr. Naked)
 *  7:  Squawk Box (James)
 *  8:  Plunger (Monte, based on Kayla's original)
 *  9:  Contort-O-Matic (Ray and Mishi)
 * 10:  FourPlay-4-2
 * 11:  FourPlay-4-3
 * 12:  Buck Norris
 * 13:  unassigned
 * 14:  unassigned
 * 15:  unassigned
 *
 * For stress tests, widget ids are reused as needed because stress-test
 * payloads are handled separately from all other types of payloads.
 */


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


