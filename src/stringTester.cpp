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

#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "Log.h"
#include "pixeltypes.h"

using namespace std;


Log logger;                     // this is the global Log object used everywhere

static string configFileName = "activeConfig.json";
static unsigned int stringNum;
static CRGB testColor;


void usage()
{
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Usage: stringTester [options] <string number or 0 for all> <red intensity> <green intensity> <blue intensity>\n");
    printf("\n");
    printf("Options:\n");
    printf("\n");
    printf("-c pathname, --config_file=pathname\n");
    printf("    Read the JSON configuration document from the file specified by pathname.\n");
    printf("    Default is \"%s\".\n", configFileName.c_str());
    printf("\n");
    printf("-h, --help\n");
    printf("    Print this help information.\n");
    printf("\n");
}


static void getCommandLineOptions(int argc, char* argv[])
{
    enum LongOnlyOption {
        unhandled = 0
    };

    int longOnlyOption = unhandled;
    static struct option longopts[] = {
        { "config_file",    required_argument,      NULL,            'c'            },
        { "help",           no_argument,            NULL,            'h'            },
        { NULL,             0,                      NULL,            0              }
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "c:h", longopts, NULL)) != -1) {
        switch (ch) {
            case 'c':
                configFileName = optarg;
                break;
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            case 0:
                switch (longOnlyOption) {
                    default:
                        fprintf(stderr, "Unhandled long option encountered.\n");
                        exit(EXIT_FAILURE);
                }                    
                break;
            default:
                // Invalid or unrecognized option message has already been printed.
                fprintf(stderr, "Use -h or --help for help.\n");
                exit(EXIT_FAILURE);
        }
    }

    argc -= optind;
    argv += optind;

    // Handle non-option args here.
    if (argc != 4) {
        fprintf(stderr, "Use -h or --help for help.\n");
        exit(EXIT_FAILURE);
    }
    stringNum = atoi(argv[0]);
    testColor.r = atoi(argv[1]);
    testColor.g = atoi(argv[2]);
    testColor.b = atoi(argv[3]);
}


int main(int argc, char **argv)
{
    getCommandLineOptions(argc, argv);

    logger.startLogging("stringTester", Log::LogTo::console);

    ConfigReader configReader;
    if (!configReader.loadConfiguration(configFileName)) {
        return(EXIT_FAILURE);
    }
    // We'll use the main patternController instance's configuration.
    json11::Json configObject;
    json11::Json instanceConfigObject;
    if (!ConfigReader::getJsonObject(configReader.getConfigObject(),
                                     "patternController",
                                     instanceConfigObject,
                                     " in " + configFileName + ".")) 
    {
        return(EXIT_FAILURE);
    }
    json11::Json commonConfigObject;
    if (ConfigReader::getJsonObject(configReader.getConfigObject(),
                                    "common",
                                    commonConfigObject))
    {
        configObject = ConfigReader::mergeConfigObjects(instanceConfigObject, commonConfigObject);
    }
    else
    {
        configObject = instanceConfigObject;
        logger.logMsg(LOG_WARNING, "%s does not have a common section.", configFileName.c_str());
    }

    string errMsgSuffix = " in " + configFileName + ".";
    unsigned int numberOfStrings;
    unsigned int numberOfPixelsPerString;
    string opcServerIpAddress;
    unsigned int opcServerPortNumber;
    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfStrings", numberOfStrings, errMsgSuffix)
        || !ConfigReader::getUnsignedIntValue(configObject, "numberOfPixelsPerString", numberOfPixelsPerString, errMsgSuffix)
        || !ConfigReader::getStringValue(configObject, "opcServerIpAddress", opcServerIpAddress, errMsgSuffix)
        || !ConfigReader::getUnsignedIntValue(configObject, "opcServerPortNumber", opcServerPortNumber, errMsgSuffix, 1024, 65535))
    {
        return(EXIT_FAILURE);
    }

    logger.logMsg(LOG_INFO, "numberOfStrings=%d, numberOfPixelsPerString=%d", numberOfStrings, numberOfPixelsPerString);
    logger.logMsg(LOG_INFO, "OPC server is at %s:%d", opcServerIpAddress.c_str(), opcServerPortNumber);

    if (stringNum > numberOfStrings) {
        logger.logMsg(LOG_ERR, "Invalid string number %d.  Valid string numbers are 1 - %d (or 0 for all).",
                      stringNum, numberOfStrings);
        return(EXIT_FAILURE);
    }

    if (stringNum == 0) {
        logger.logMsg(LOG_INFO, "Illuminating all strings with r=%d, g=%d, b=%d.", testColor.r, testColor.g, testColor.b);
    }
    else {
        logger.logMsg(LOG_INFO, "Illuminating string %d with r=%d, g=%d, b=%d.", stringNum, testColor.r, testColor.g, testColor.b);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(opcServerPortNumber);
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        logger.logMsg(LOG_ERR, "Couldn't connect to OPC server at %s:%d", opcServerIpAddress.c_str(), opcServerPortNumber);
        return(EXIT_FAILURE);
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
