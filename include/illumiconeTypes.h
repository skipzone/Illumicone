#pragma once

#include <cstdint>
#include <string>

#define NUM_STRINGS 12
#define PIXELS_PER_STRING 20
#define OPC_SERVER_ADDR "192.168.69.100"
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


