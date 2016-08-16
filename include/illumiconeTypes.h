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

#include <cstdint>
#include <string>

#define NUM_STRINGS 4
#define PIXELS_PER_STRING 16
#define OPC_SERVER_ADDR "192.168.0.14"
// david's home addr for the sever
//#define OPC_SERVER_ADDR "192.168.0.10"
// type definitions for Illumicone

//
// structure to hold Open Pixel Control pixel data
//
typedef struct _opc_pixel_ {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} opc_pixel_t;


static const std::string patconIpAddress = "192.168.69.101";
constexpr static unsigned int widgetPortNumberBase = 4200;


/*******************
 * Widget Payloads *
 *******************/

#pragma pack(push)
#pragma pack(1)

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

// UDP to the pattern controller
struct UdpPayload {
    uint8_t id;
    uint8_t channel;
    uint8_t isActive;
    int16_t position;
    int16_t velocity;
};

#pragma pack(pop)


