#pragma once
#include <cstdint>

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
