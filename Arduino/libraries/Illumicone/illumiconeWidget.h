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


// ----- standard payloads -----

// pipe 0
struct StressTestPayload {
  WidgetHeader widgetHeader;
  uint32_t     payloadNum;
  uint32_t     numTxFailures;
  uint32_t     lastTxUs;
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


// ----- custom payloads (always sent on pipe 5) -----

// channel 0 carries touch data payloads
struct ContortOMaticTouchDataPayload {
    WidgetHeader widgetHeader;
    uint16_t padIsTouchedBitfield;
};

// channel 1 carries calibration data payloads
struct ContortOMaticCalibrationDataPayload {
    WidgetHeader widgetHeader;
    uint8_t setNum;                         // 0 for pads 0-7, 1 for pads 8-15
    uint16_t capSenseReferenceValues[8];
};


