#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "illumiconeTypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"

using namespace std;

static struct sockaddr_in server;
static int sock;
static int n;
static uint8_t opcArray[NUM_STRINGS * PIXELS_PER_STRING * 3 + 4];

bool setupConnection()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr("192.168.7.2");
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Couldn't connect.\n");
        while (1);
    }

    return true;
}

void dumpPacket(uint8_t *opcArray)
{
    int i, ii;
    uint8_t *pixels;

    cout << "opcArray[0]: " << unsigned(opcArray[0]) << endl;
    cout << "opcArray[1]: " << unsigned(opcArray[1]) << endl;
    cout << "opcArray[2]: " << unsigned(opcArray[2]) << endl;
    cout << "opcArray[3]: " << unsigned(opcArray[3]) << endl;

    pixels = &opcArray[4];

    for (i = 0; i < 2 * PIXELS_PER_STRING * 3; i++) {
        cout << unsigned(pixels[i]) << " " << unsigned(pixels[i+1]) << " " << unsigned(pixels[i+2]) << " " << endl;
    }
}

int sendPacket(std::vector<std::vector<opc_pixel_t>> &pixelArray)
{
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
            pixels[col*PIXELS_PER_STRING*3 + row * 3 + 0] = pixelArray[col][row].r;
            pixels[col*PIXELS_PER_STRING*3 + row * 3 + 1] = pixelArray[col][row].g;
            pixels[col*PIXELS_PER_STRING*3 + row * 3 + 2] = pixelArray[col][row].b;
        }
    }
//    pixels[0*PIXELS_PER_STRING + 0 * 3 + 0] = 128;
//    pixels[0*PIXELS_PER_STRING + 1 * 3 + 1] = 128;
//    pixels[0*PIXELS_PER_STRING + 2 * 3 + 2] = 128;
//
//    pixels[1*PIXELS_PER_STRING * 3 + 13 * 3 + 0] = 128;
//    pixels[2*PIXELS_PER_STRING * 3 + 14 * 3 + 1] = 128;
//    pixels[3*PIXELS_PER_STRING * 3 + 15 * 3 + 2] = 128;


    // send over network connection

//    dumpPacket(opcArray);
    n = send(sock, opcArray, sizeof(opcArray), 0);

    return n;
}

int main(void)
{
    RgbVerticalPattern rgbPattern;
    int num_channels[2] = {3, 5};
    int i, num_bytes;

    cout << "Pattern initialization!\n";

    setupConnection();
    cout << "NUM_STRINGS: " << NUM_STRINGS << endl;
    cout << "PIXELS_PER_STRING: " << PIXELS_PER_STRING << endl;
    rgbPattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    rgbPattern.initWidgets(1, num_channels[0]);

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

    while (true) {
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

        num_bytes = sendPacket(rgbPattern.pixelArray);
        cout << num_bytes << " sent!" << endl;

        usleep(50000);
    }
}
