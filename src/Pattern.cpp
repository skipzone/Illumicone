#include <iostream>
#include <vector>
#include <unistd.h>

#include "illumiconeTypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"

using namespace std;

void dumpPacket(uint8_t *opcArray)
{
    int i, ii;
    uint8_t *pixels;

    cout << "opcArray[0]: " << opcArray[0] << endl;
    cout << "opcArray[1]: " << opcArray[1] << endl;
    cout << "opcArray[2]: " << opcArray[2] << endl;
    cout << "opcArray[3]: " << opcArray[3] << endl;

    pixels = &opcArray[4];

    for (i = 0; i < 2 * PIXELS_PER_STRING * 3; i++) {
        cout << unsigned(pixels[i]) << " " << unsigned(pixels[i+1]) << " " << unsigned(pixels[i+2]) << " " << endl;
    }
}

bool sendPacket(std::vector<std::vector<opc_pixel_t>> &pixelArray)
{
    uint8_t opcArray[NUM_STRINGS * PIXELS_PER_STRING * 3 + 4];
    uint8_t *pixels;

    int col;
    int row;

    cout << "sentPacket!" << endl;

    opcArray[0] = 0;
    opcArray[1] = 0;
    opcArray[2] = NUM_STRINGS * PIXELS_PER_STRING * 3 / 256;
    opcArray[3] = NUM_STRINGS * PIXELS_PER_STRING * 3 % 256;

    pixels = &opcArray[4];

    for (col = 0; col < NUM_STRINGS; col++) {
        for (row = 0; row < PIXELS_PER_STRING; row++) {
            pixels[col*PIXELS_PER_STRING + row * 3 + 0] = pixelArray[col][row].r;
            pixels[col*PIXELS_PER_STRING + row * 3 + 1] = pixelArray[col][row].g;
            pixels[col*PIXELS_PER_STRING + row * 3 + 2] = pixelArray[col][row].b;
        }
    }

    // send over network connection

    dumpPacket(opcArray);
}

int main(void)
{
    RgbVerticalPattern rgbPattern;
    int pattern[2] = {3, 5};
    int i;

    cout << "Pattern initialization!\n";

    rgbPattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    rgbPattern.initWidgets(1, pattern[0]);

    // initialize channels
    // NEED emplace_back or something...
    // put an "init channels" in widget to emplace_back channels
    for (auto channel:rgbPattern.widgets[0]->channels) {
        cout << "initialize channel!" << endl;
        i++;
    }

    cout << "Number of channels: " << rgbPattern.widgets[0]->channels.size() << endl;

    for (auto channel:rgbPattern.widgets[0]->channels) {
        cout << "Channel: " << channel.number << endl;
    }

    rgbPattern.update();

    cout << "rgbPattern pixel array size X: " << rgbPattern.pixelArray.size() << endl;
    cout << "rgbPattern pixel array size Y: " << rgbPattern.pixelArray[0].size() << endl;
    cout << "rgbPattern widgets size: " << rgbPattern.widgets.size() << endl;

//    cout << "Pixel 0:" << endl;
//    for (auto pixel:rgbPattern.pixelArray[0]) {
//        cout << "" << pixel.r << " " << pixel.g << " " << pixel.b << endl;
//    }
//
//    cout << "\n\n\nPixel 1:" << endl;
//    for (auto pixel:rgbPattern.pixelArray[1]) {
//        cout << "" << pixel.r << " " << pixel.g << " " << pixel.b << endl;
//    }

    sendPacket(rgbPattern.pixelArray);
}
