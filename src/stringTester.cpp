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
static bool useTcpForOpcServer;
static string opcServerIpAddress;
static unsigned int opcServerPortNumber;
static struct sockaddr_in opcServerSockaddr;
static int opcServerSocketFd;


bool openOpcServerTcpConnection()
{
    logger.logMsg(LOG_INFO, "Connecting to OPC server at " + opcServerIpAddress + ":" + to_string(opcServerPortNumber) + "...");

    opcServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (opcServerSocketFd == -1) {
        logger.logMsg(LOG_ERR, errno, "Failed to create socket for OPC server.");
        return false;
    }

    opcServerSockaddr.sin_family = AF_INET;
    opcServerSockaddr.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    opcServerSockaddr.sin_port = htons(opcServerPortNumber);

    if (connect(opcServerSocketFd, (struct sockaddr *) &opcServerSockaddr, sizeof(opcServerSockaddr)) == -1) {
        logger.logMsg(LOG_ERR, errno, "Unable to connect to opc-server.");
        return false;
    }

    logger.logMsg(LOG_INFO, "Connected.");

    return true;
}


bool closeOpcServerTcpConnection()
{
    logger.logMsg(LOG_INFO, "Disconnecting from OPC server...");
    ///if (disconnectx(opcServerSocketFd, SAE_ASSOCID_ANY, SAE_CONNID_ANY) != 0) {
    if (close(opcServerSocketFd) != 0) {
        logger.logMsg(LOG_ERR, errno, "Unable to close connection to opc-server.");
        return false;
    }
    return true;
}


bool openUdpPortForOpcServer()
{
    logger.logMsg(LOG_INFO, "Creating and binding socket for OPC server at " + opcServerIpAddress + ":" + to_string(opcServerPortNumber) + "...");

    memset(&opcServerSockaddr, 0, sizeof(struct sockaddr_in));

    opcServerSockaddr.sin_family = AF_INET;
    opcServerSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    opcServerSockaddr.sin_port = htons(0);

    if ((opcServerSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        logger.logMsg(LOG_ERR, errno, "Failed to create socket for OPC server.");
        return false;
    }

    if (::bind(opcServerSocketFd, (struct sockaddr *) &opcServerSockaddr, sizeof(struct sockaddr_in)) == -1) {
        logger.logMsg(LOG_ERR, errno, "bind failed for OPC server.");
        return false;
    }

    logger.logMsg(LOG_INFO, "Setting address to " + opcServerIpAddress + ":" + to_string(opcServerPortNumber) + ".");

    inet_pton(AF_INET, opcServerIpAddress.c_str(), &opcServerSockaddr.sin_addr.s_addr);
    opcServerSockaddr.sin_port = htons(opcServerPortNumber);

    return true;
}


bool closeUdpPortForOpcServer()
{
    // TODO 2/3/2018 ross:  make sure this implementation is correct
    logger.logMsg(LOG_INFO, "Closing UDP port for OPC server...");
    if (close(opcServerSocketFd) != 0) {
        logger.logMsg(LOG_ERR, errno, "Unable to close UDP port for opc-server.");
        return false;
    }
    return true;
}


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


static void getCommandLineOptions(int argc, char *argv[])
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
    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfStrings", numberOfStrings, errMsgSuffix)
        || !ConfigReader::getUnsignedIntValue(configObject, "numberOfPixelsPerString", numberOfPixelsPerString, errMsgSuffix)
        || !ConfigReader::getBoolValue(configObject, "useTcpForOpcServer", useTcpForOpcServer, errMsgSuffix)
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

    // Open communications with OPC server.
    if (useTcpForOpcServer) {
        if (!openOpcServerTcpConnection()) {
            return(EXIT_FAILURE);
        }
    }
    else {
        if (!openUdpPortForOpcServer()) {
            return(EXIT_FAILURE);
        }
    }

#ifdef NO_COMPILE
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(opcServerPortNumber);
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        logger.logMsg(LOG_ERR, "Couldn't connect to OPC server at %s:%d", opcServerIpAddress.c_str(), opcServerPortNumber);
        return(EXIT_FAILURE);
    }
#endif

    ssize_t opcBufferSize = numberOfStrings * numberOfPixelsPerString * 3 + 4;
    uint8_t *opcBuffer = new uint8_t[opcBufferSize];
    if (opcBuffer == nullptr) {
        logger.logMsg(LOG_ERR, "Unable to allocate an OPC buffer of size " + to_string(opcBufferSize));
        return(EXIT_FAILURE);
    }

    // Set up the OPC header.
    opcBuffer[0] = 0;
    opcBuffer[1] = 0;
    opcBuffer[2] = numberOfStrings * numberOfPixelsPerString * 3 / 256;
    opcBuffer[3] = numberOfStrings * numberOfPixelsPerString * 3 % 256;

    uint8_t *opcData = &opcBuffer[4];    // points to the data portion of opcBuffer

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
        if (useTcpForOpcServer) {
            //logger.logMsg(LOG_DEBUG, "sending message to OPC server via TCP...");
            if (send(opcServerSocketFd, opcBuffer, opcBufferSize, 0) == -1) {
                logger.logMsg(LOG_ERR, errno, "Failed to send message to OPC server via TCP.");
            }
            //logger.logMsg(LOG_DEBUG, "sent message to OPC server via TCP.");
        }
        else {
            //logger.logMsg(LOG_DEBUG, "sending message to OPC server via UDP...");
            // TODO 2/3/2018 ross:  modify to not block if no message space is available to hold the message
            ssize_t bytesSentCount = sendto(opcServerSocketFd,
                                            opcBuffer,
                                            opcBufferSize,
                                            0,
                                            (struct sockaddr *) &opcServerSockaddr,
                                            sizeof(struct sockaddr_in));
            if (bytesSentCount == -1) {
                logger.logMsg(LOG_ERR, errno, "Failed to send message to OPC server via UDP.");
            }
            if (bytesSentCount != opcBufferSize) {
                logger.logMsg(LOG_ERR,
                       "UDP payload size is " + to_string(opcBufferSize)
                       + ", but " + to_string(bytesSentCount) + " bytes were sent to OPC server.");
            }
            //logger.logMsg(LOG_DEBUG, "Sent " to_string(bytesSentCount) + " byte payload via UDP.");
        }
        usleep(20000);
    }

    // We never get here.

    delete [] opcBuffer;
    opcBuffer = nullptr;
    opcBufferSize = 0;
    opcData = nullptr;

    logger.stopLogging();
}
