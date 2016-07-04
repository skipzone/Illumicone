#include <iostream>
#include <vector>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "illumiconeTypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "SolidBlackPattern.h"
#include "QuadSlicePattern.h"

using namespace std;

static struct sockaddr_in server;
static int sock;
static int n;
static uint8_t opcArray[NUM_STRINGS * PIXELS_PER_STRING * 3 + 4];

bool setupConnection()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr(OPC_SERVER_ADDR);
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        cout << "SOMETHING'S FUCKY: couldn't connect to opc-server!" << endl;
        while (1);
    }

    return true;
}

void dumpPacket(uint8_t *opcArray)
{
    int i;
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

void printInit(Pattern *pattern)
{
    cout << "Initialized " << pattern->name << "!" << endl;
    cout << "    priority: " << pattern->priority << endl;
    cout << "    pixelArray size X: " << pattern->pixelArray.size() << endl;
    cout << "    pixelArray size Y: " << pattern->pixelArray[0].size() << endl;
    cout << "    widgets size: " << pattern->widgets.size() << endl;
    for (auto&& widget:pattern->widgets) {
        cout << "        " << widget->numChannels << endl;
    }
}

/*
 * Build the OPC packet based on the values in the pixelArray for a pattern, and
 * its priority.
 *
 * This will do some sort of math based on its parameters to determine how to update
 * the final array.  The final array will be stored separately as a 2d opc_pixel_t
 * vector, probably globally like opcArray.
 *
 * We might have two to provide for double buffering of some sort.
 *
 * Ideally this would be called for each pattern that has updates before sending
 * the final packet over the network, like:
 *      buildPacket(finalFrame1, rgbPattern.pixelArray, rgbPattern.priority);
 *      buildPacket(finalFrame1, solidBlackPattern.pixelArray, solidBlackPattern.priority);
 *      ..
 *      ..
 *
 *      sendPacket(finalFrame1);
 *
 * Call this in reverse priority for best results, i.e. call with priority 3 pattern, then with
 * 2, then 1, then 0.
 */
bool buildFrame(
        std::vector<std::vector<opc_pixel_t>> &finalFrame,
        std::vector<std::vector<opc_pixel_t>> &pixelArray,
        int priority)
{
    int col;
    int row;

    switch (priority) {
        case 0:
            //
            // SolidBlackPattern
            //
            for (col = 0; col < NUM_STRINGS; col++) {
                for (row = 0; row < PIXELS_PER_STRING; row++) {
                    finalFrame[col][row] = pixelArray[col][row];
                }
            }
            break;

        case 1:
            //
            // RgbVerticalPattern
            //
            for (col = 0; col < NUM_STRINGS; col++) {
                for (row = 0; row < PIXELS_PER_STRING; row++) {
                    // only update the value of the final frame if the pixel
                    // contains non-zero values (is on)
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;
            
        case 2:
            //
            // QuadSlicePattern
            //
            for (col = 0; col < NUM_STRINGS; col++) {
                for (row = 0; row < PIXELS_PER_STRING; row++) {
                    // only update the value of the final frame if the pixel
                    // contains non-zero values (is on)
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

        default:
            cout << "SOMETHING'S FUCKY : no case for priority " << priority << endl;
    }

    return true;
}

int sendFrame(std::vector<std::vector<opc_pixel_t>> &finalFrame)
{
    uint8_t *pixels;

    int col;
    int row;

    opcArray[0] = 0;
    opcArray[1] = 0;
    opcArray[2] = NUM_STRINGS * PIXELS_PER_STRING * 3 / 256;
    opcArray[3] = NUM_STRINGS * PIXELS_PER_STRING * 3 % 256;

    pixels = &opcArray[4];

    for (col = 0; col < NUM_STRINGS; col++) {
        for (row = 0; row < PIXELS_PER_STRING; row++) {
            pixels[col*PIXELS_PER_STRING*3 + row*3 + 0] = finalFrame[col][row].r;
            pixels[col*PIXELS_PER_STRING*3 + row*3 + 1] = finalFrame[col][row].g;
            pixels[col*PIXELS_PER_STRING*3 + row*3 + 2] = finalFrame[col][row].b;
        }
    }

//    dumpPacket(opcArray);
    // send over network connection
    n = send(sock, opcArray, sizeof(opcArray), 0);

    return n;
}

int main(void)
{
    RgbVerticalPattern rgbVerticalPattern;
    SolidBlackPattern solidBlackPattern;
    QuadSlicePattern quadSlicePattern;

    vector<vector<opc_pixel_t>> finalFrame1;
    vector<vector<opc_pixel_t>> finalFrame2;

    // to be filled in from a config file somewhere
    int numChannels[3] = {3, 1, 4};
    int numBytes;

    finalFrame1.resize(NUM_STRINGS, std::vector<opc_pixel_t>(PIXELS_PER_STRING));
    finalFrame2.resize(NUM_STRINGS, std::vector<opc_pixel_t>(PIXELS_PER_STRING));

    cout << "Pattern initialization!\n";

    // open socket, connect with opc-server
    setupConnection();
    cout << "NUM_STRINGS: " << NUM_STRINGS << endl;
    cout << "PIXELS_PER_STRING: " << PIXELS_PER_STRING << endl;
    rgbVerticalPattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    rgbVerticalPattern.initWidgets(1, numChannels[0]);
    printInit(&rgbVerticalPattern);

    solidBlackPattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    solidBlackPattern.initWidgets(1, numChannels[1]);
    printInit(&solidBlackPattern);

    quadSlicePattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    quadSlicePattern.initWidgets(1, numChannels[2]);
    printInit(&quadSlicePattern);

    while (true) {
        rgbVerticalPattern.update();
        solidBlackPattern.update();
        quadSlicePattern.update();

        if (quadSlicePattern.isActive) {
            buildFrame(finalFrame1, quadSlicePattern.pixelArray, quadSlicePattern.priority);
        }
        
        if (rgbVerticalPattern.isActive) {
            buildFrame(finalFrame1, rgbVerticalPattern.pixelArray, rgbVerticalPattern.priority);
        }

        if (solidBlackPattern.isActive) {
            buildFrame(finalFrame1, solidBlackPattern.pixelArray, solidBlackPattern.priority);
        }

        numBytes = sendFrame(finalFrame1);

        usleep(500000);
    }
}
