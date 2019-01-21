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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "Log.h"
#include "pixeltypes.h"

using namespace std;


Log logger;                     // this is the global Log object used everywhere

int main(int argc, char **argv)
{
    if (argc != 6) {
        cout << "Usage:  " << argv[0] << " <config file name> <string number or 0 for all> <red intensity> <green intensity> <blue intensity>" << endl;
        return 2;
    }
    string jsonFileName(argv[1]);
    unsigned int stringNum = atoi(argv[2]);
    CRGB testColor;
    testColor.r = atoi(argv[3]);
    testColor.g = atoi(argv[4]);
    testColor.b = atoi(argv[5]);

    logger.startLogging("stringTester", Log::LogTo::console);

    ConfigReader config;
    if (!config.readConfigurationFile(jsonFileName)) {
        return(EXIT_FAILURE);
    }

    unsigned int numberOfStrings = config.getNumberOfStrings();
    unsigned int numberOfPixelsPerString = config.getNumberOfPixelsPerString();
    cout << "numberOfStrings = " << numberOfStrings << endl;
    cout << "numberOfPixelsPerString = " << numberOfPixelsPerString << endl;

    if (stringNum > numberOfStrings) {
        cerr << "Invalid string number." << endl;
        return(EXIT_FAILURE);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(config.getOpcServerIpAddress().c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        cout << "SOMETHING'S FUCKY: couldn't connect to opc-server!" << endl;
        while (1);
    }

    uint8_t opcBuffer[numberOfStrings * numberOfPixelsPerString * 3 + 4];
    uint8_t* opcData = &opcBuffer[4];    // points to the data portion of opcBuffer

    // Set up the OPC header.
    opcBuffer[0] = 0;
    opcBuffer[1] = 0;
    opcBuffer[2] = numberOfStrings * numberOfPixelsPerString * 3 / 256;
    opcBuffer[3] = numberOfStrings * numberOfPixelsPerString * 3 % 256;

    // Set the pixel data.
    for (unsigned int col = 0; col < numberOfStrings; col++) {
        unsigned int colOffset = col * numberOfPixelsPerString * 3;
        CRGB pixelColor = (stringNum == 0 || col == stringNum - 1) ? testColor : CRGB::Black;
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            unsigned int pixelOffset = colOffset + row * 3;
            opcData[pixelOffset] = pixelColor.r;
            opcData[pixelOffset + 1] = pixelColor.g;
            opcData[pixelOffset + 2] = pixelColor.b;
        }
    }

    // Periodically send a message to the OPC server.
    while (1) {
        send(sock, opcBuffer, sizeof(opcBuffer), 0);
        usleep(20000);
    }

    logger.stopLogging();
}
