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

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <RF24/RF24.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconeWidgetTypes.h"
#include "Log.h"
#include "WidgetId.h"

using namespace std;


Log logger;

static ConfigReader config;
static string lockFilePath;
static string patconIpAddress;
static unsigned int radioPollingLoopSleepIntervalUs;
static unsigned int widgetPortNumberBase;

static struct sockaddr_in widgetSockAddr[16];
static int widgetSock[16];



/***********************
 * Radio Configuration *
 ***********************/

#define RF24_SPI_DEV

#ifdef RF24_SPI_DEV
// RF24 radio(<ce_pin>, <a>*10+<b>) for spi device at /dev/spidev<a>.<b>
// See http://pi.gadgetoid.com/pinout
RF24 radio(25, 0);
#else
// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939
// and the related enumerations for pin information.  (That's some pretty useful shit right there, Maynard.)
// Raspberry Pi B+:  CE connected to GPIO25 on J8-22, CSN connected to CE0
#error oh shit!
RF24 radio(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);
#endif

// We're using dynamic payload size, but we still need to know what the largest can be.
constexpr uint8_t maxPayloadSize = 32;

constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
constexpr uint8_t rfChannel = 84;

// Probably no need to ever set auto acknowledgement to false because the sender
// can control whether or not acks are sent by using the NO_ACK bit.  Set
// autoAck false to prevent a misconfigured widget from creating unnecessary
// radio traffic (and to prevent any widgets expectig acks from working).
// TODO:  Set autoAck true after all widgets have been reprogrammed with
//        firmware that can send the NO_ACK big.  (As of 18 Dec. 2018, none
//        of them work right in that regard.)
constexpr bool autoAck = true;

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
constexpr rf24_pa_dbm_e rfPowerLevel = RF24_PA_MAX;

// RF24_250KBPS or RF24_1MBPS
constexpr rf24_datarate_e dataRate = RF24_1MBPS;

constexpr uint8_t txRetryDelayMultiplier = 15;  // 250 us additional delay multiplier (0-15)
constexpr uint8_t txMaxRetries = 15;            // max retries (0-15)

// RF24_CRC_DISABLED, RF24_CRC_8, or RF24_CRC_16
constexpr rf24_crclength_e crcLength = RF24_CRC_16;


/*********************
 * UDP Communication *
 *********************/

bool openUdpPort(WidgetId widgetId)
{
    unsigned int widgetIdNumber = widgetIdToInt(widgetId);
    unsigned int portNumber = widgetPortNumberBase + widgetIdNumber;

    logger.logMsg(LOG_INFO, "Creating and binding socket for %s.", widgetIdToString(intToWidgetId(widgetIdNumber)).c_str());

    memset(&widgetSockAddr[widgetIdNumber], 0, sizeof(struct sockaddr_in));

    widgetSockAddr[widgetIdNumber].sin_family = AF_INET;
    widgetSockAddr[widgetIdNumber].sin_addr.s_addr = htonl(INADDR_ANY);
    widgetSockAddr[widgetIdNumber].sin_port = htons(0);

    if ((widgetSock[widgetIdNumber] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        logger.logMsg(LOG_ERR, errno, "Failed to create socket for %s.", widgetIdToString(intToWidgetId(widgetIdNumber)).c_str());
        return false;
    }

    if (::bind(widgetSock[widgetIdNumber], (struct sockaddr *) &widgetSockAddr[widgetIdNumber], sizeof(struct sockaddr_in)) < 0) {
        logger.logMsg(LOG_ERR, errno, "bind failed for %s.", widgetIdToString(intToWidgetId(widgetIdNumber)).c_str());
        return false;
    }

    logger.logMsg(LOG_INFO, "Setting address to %s:%d", patconIpAddress.c_str(), portNumber);

    inet_pton(AF_INET, patconIpAddress.c_str(), &widgetSockAddr[widgetIdNumber].sin_addr.s_addr);
    widgetSockAddr[widgetIdNumber].sin_port = htons(portNumber);

    return true;
}


bool sendUdp(const UdpPayload& payload)
{
    ssize_t bytesSentCount = sendto(widgetSock[payload.id],
                                    &payload,
                                    sizeof(payload),
                                    0,
                                    (struct sockaddr *) &widgetSockAddr[payload.id],
                                    sizeof(struct sockaddr_in));

    if (bytesSentCount != sizeof(payload)) {
        logger.logMsg(LOG_ERR, "UPD payload size is %d, but %d bytes were sent.", sizeof(payload), bytesSentCount);
        return false;
    }
    //logger.logMsg(LOG_DEBUG, "Sent %d-byte payload via UDP.", bytesSentCount);

    return true;
}


/********************
 * Payload Handlers *
 ********************/

void handleContortOMaticTouchDataPayload(const ContortOMaticTouchDataPayload* payload)
{
    // Send the pad-is-touched bitfield as a position measurement on channel 0.
    UdpPayload udpPayload;
    udpPayload.id       = payload->widgetHeader.id;
    udpPayload.channel  = 0;
    udpPayload.isActive = payload->widgetHeader.isActive;
    udpPayload.position = payload->padIsTouchedBitfield;
    udpPayload.velocity = 0;
    sendUdp(udpPayload);
}


void handleContortOMaticCalibrationDataPayload(const ContortOMaticCalibrationDataPayload* payload)
{
    // The pattern isn't interested in the calibration data.
    // We just log it here for future reference.
    unsigned int padNumOffset = payload->setNum == 0 ? 0 : 8;
    stringstream sstr;
    for (unsigned int i = 0; i < 8; ++i) {
        sstr << " " << to_string(i + padNumOffset) << ":" << payload->capSenseReferenceValues[i];
    }
    logger.logMsg(LOG_INFO, "Cap sense reference values: %s", sstr.str().c_str());
}


void handleContortOMaticPayload(const CustomPayload* payload, unsigned int payloadSize)
{
    switch (payload->widgetHeader.channel) {          // channel is actually payload subtype
        case 0:
            if (payloadSize != sizeof(ContortOMaticTouchDataPayload)) {
                logger.logMsg(LOG_ERR,
                              "Got ContortOMaticTouchDataPayload payload with size %d, but size %d was expected.",
                              payloadSize, sizeof(ContortOMaticTouchDataPayload));
                return;
            }
            handleContortOMaticTouchDataPayload(reinterpret_cast<const ContortOMaticTouchDataPayload*>(payload));
            break;

        case 1:
            if (payloadSize != sizeof(ContortOMaticCalibrationDataPayload)) {
                logger.logMsg(LOG_ERR,
                              "Got ContortOMaticCalibrationDataPayload payload with size %d, but size %d was expected.",
                              payloadSize, sizeof(ContortOMaticCalibrationDataPayload));
                return;
            }
            handleContortOMaticCalibrationDataPayload(reinterpret_cast<const ContortOMaticCalibrationDataPayload*>(payload));
            break;

        default:
            logger.logMsg(LOG_ERR,
                          "Got ContortOMatic payload on channel %d, but there is no payload subtype assigned to that channel.",
                          payload->widgetHeader.channel);
    }
}


void handleStressTestPayload(const StressTestPayload* payload, unsigned int payloadSize)
{
    if (payloadSize != sizeof(StressTestPayload)) {
        logger.logMsg(LOG_ERR,
                      "Got StressTestPayload payload with size %d, but size %d was expected.",
                      payloadSize, sizeof(StressTestPayload));
        return;
    }

    logger.logMsg(LOG_INFO,
                  "stest: id=%d a=%d ch=%d seq=%d fails=%d(%d%%)",
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  payload->payloadNum,
                  payload->numTxFailures,
                  payload->numTxFailures * 100 / payload->payloadNum);

    UdpPayload udpPayload;
    udpPayload.id       = payload->widgetHeader.id;
    udpPayload.channel  = payload->widgetHeader.channel;
    udpPayload.isActive = payload->widgetHeader.isActive;
    udpPayload.position = payload->payloadNum;
    udpPayload.velocity = payload->numTxFailures;

    sendUdp(udpPayload);
}


void handlePositionVelocityPayload(const PositionVelocityPayload* payload, unsigned int payloadSize)
{
    if (payloadSize != sizeof(PositionVelocityPayload)) {
        logger.logMsg(LOG_ERR,
                      "Got PositionVelocityPayload payload with size %d, but size %d was expected.",
                      payloadSize, sizeof(PositionVelocityPayload));
        return;
    }

    logger.logMsg(LOG_INFO,
                  "pv: id=%d a=%d ch=%d p=%d v=%d",
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  payload->position,
                  payload->velocity);

    UdpPayload udpPayload;
    udpPayload.id       = payload->widgetHeader.id;
    udpPayload.channel  = payload->widgetHeader.channel;
    udpPayload.isActive = payload->widgetHeader.isActive;
    udpPayload.position = payload->position;
    udpPayload.velocity = payload->velocity;

    sendUdp(udpPayload);
}


void handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, unsigned int payloadSize)
{
    if (payloadSize < (1 + sizeof(int16_t))) {
        logger.logMsg(LOG_ERR, "Got MeasurementVectorPayload without any data.");
        return;
    }
    // TODO 8/3/2017 ross:  might be a good idea to make sure payloadSize is odd

    unsigned int numMeasurements = (payloadSize - 1) / sizeof(int16_t);

    stringstream sstr;
    for (unsigned int i = 0; i < numMeasurements; ++i) {
        sstr << " " << setfill(' ') << setw(6) << payload->measurements[i];
    }
    logger.logMsg(LOG_INFO,
                  "mvec: id=%d a=%d ch=%d n=%d %s",
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  numMeasurements,
                  sstr.str().c_str());

    // Map the measurements to position measurements on the channel
    // corresponding to the measurement's position in the array.
    for (unsigned int i = 0; i < numMeasurements; ++i) {
        UdpPayload udpPayload;
        udpPayload.id       = payload->widgetHeader.id;
        udpPayload.channel  = i;
        udpPayload.isActive = payload->widgetHeader.isActive;
        udpPayload.position = payload->measurements[i];
        udpPayload.velocity = 0;

        sendUdp(udpPayload);
    }
}


void handleCustomPayload(const CustomPayload* payload, unsigned int payloadSize)
{
    if (payloadSize < 2) {
        logger.logMsg(LOG_ERR, "Got CustomPayload without any data.");
        return;
    }

    unsigned int bufLen = payloadSize - 1;

    stringstream sstr;
    for (unsigned int i = 0; i < bufLen; ++i) {
        sstr << " 0x" << hex << (int) payload->buf[i];
    }
    logger.logMsg(LOG_INFO,
                  "custom: id=%d a=%d ch=%d bufLen=%d %s",
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  bufLen,
                  sstr.str().c_str());

    switch (intToWidgetId(payload->widgetHeader.id)) {
        case WidgetId::contortOMatic:
            handleContortOMaticPayload(payload, payloadSize);
            break;
        default:
            logger.logMsg(LOG_ERR, "There is no payload handler defined for widget id %d.", payload->widgetHeader.id);
    }
}


/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

bool readConfig(const string& configFileName)
{
    if (!config.readConfigurationFile(configFileName)) {
        return false;
    }

    lockFilePath = config.getLockFilePath("widgetRcvr");
    if (lockFilePath.empty()) {
        logger.logMsg(LOG_WARNING, "There is no lock file name for widgetRcvr in configuration.");
    }

    patconIpAddress = config.getPatconIpAddress();
    if (patconIpAddress.empty()) {
        return false;
    }

    radioPollingLoopSleepIntervalUs = config.getRadioPollingLoopSleepIntervalUs();

    widgetPortNumberBase = config.getWidgetPortNumberBase();
    if (widgetPortNumberBase == 0) {
        return false;
    }

    return true;
}


bool openUdpPorts()
{
    bool retval = 
           openUdpPort(WidgetId::eye)
        && openUdpPort(WidgetId::spinnah)
        && openUdpPort(WidgetId::bells)
        && openUdpPort(WidgetId::rainstick)
        && openUdpPort(WidgetId::schroedersPlaything)
        && openUdpPort(WidgetId::triObelisk)
        && openUdpPort(WidgetId::boogieBoard)
        && openUdpPort(WidgetId::pump)
        && openUdpPort(WidgetId::contortOMatic)
        && openUdpPort(WidgetId::fourPlay42)
        && openUdpPort(WidgetId::fourPlay43)
        && openUdpPort(WidgetId::baton);

    return retval;
}


bool configureRadio()
{
    if (!radio.begin()) {
        logger.logMsg(LOG_ERR, "radio.begin failed.");
        return false;
    }

    radio.setPALevel(rfPowerLevel);
    radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
    radio.setDataRate(dataRate);
    radio.setChannel(rfChannel);
    radio.setAutoAck(autoAck);
    radio.enableDynamicPayloads();
    radio.setCRCLength(crcLength);

    for (uint8_t i = 0; i < numReadPipes; ++i) {
        radio.openReadingPipe(i, readPipeAddresses[i]);
    }

    logger.logMsg(LOG_INFO, "Radio configuration details:");
    radio.printDetails();

    return true;
}


void runLoop()
{
    time_t lastDataReceivedTime;
    time(&lastDataReceivedTime);
    time_t noDataReceivedMessageIntervalS = 2;
    time_t noDataReceivedMessageTime = 0;

    while (1) {

        uint8_t pipeNum;
        while(radio.available(&pipeNum)) {

            time(&lastDataReceivedTime);
            noDataReceivedMessageIntervalS = 2;

            unsigned int payloadSize = radio.getDynamicPayloadSize();
            if (payloadSize == 0) {
                logger.logMsg(LOG_ERR, "Got invalid packet (payloadSize = 0).");
                continue;
            }
            if (payloadSize > maxPayloadSize) {
                logger.logMsg(LOG_ERR, "Got unsupported payload size " + to_string(payloadSize));
                // RF24 is supposed to do a Flush_RX command and return 0 for
                // the size if an invalid payload length is detected.  It
                // apparently didn't do that.  Who knows what we're supposed
                // to do now.  We'll try turning receive off and on to clear
                // the rx buffers and start over.
                radio.stopListening();
                delay(100);
                radio.startListening();
                continue;
            }

            uint8_t payload[payloadSize];
            radio.read(payload, payloadSize);

            stringstream sstr;

            switch(pipeNum) {

                case 0:
                    handleStressTestPayload((StressTestPayload*) payload, payloadSize);
                    break;

                case 1:
                    handlePositionVelocityPayload((PositionVelocityPayload*) payload, payloadSize);
                    break;

                case 2:
                    handleMeasurementVectorPayload((MeasurementVectorPayload*) payload, payloadSize);
                    break;

                case 3:
                case 4:
                    for (unsigned int i = 0; i < maxPayloadSize; ++i) {
                        sstr << " 0x" << hex << (int) payload[i];
                    }
                    logger.logMsg(LOG_ERR,
                           "Got payload with size " + to_string(payloadSize)
                           + " via unsupported pipe " + to_string((int) pipeNum)
                           + ".  Contents: " + sstr.str());
                    break;

                case 5:
                    handleCustomPayload((CustomPayload*) payload, payloadSize);
                    break;

                default:
                    for (unsigned int i = 0; i < maxPayloadSize; ++i) {
                        sstr << " 0x" << hex << (int) payload[i];
                    }
                    logger.logMsg(LOG_ERR,
                           "pipeNum is " + to_string((int) pipeNum)
                           + ", which should never happen!  Payload contents: " + sstr.str());
            }
        }

        time_t now;
        time(&now);
        if (now != noDataReceivedMessageTime) {
            time_t noDataReceivedIntervalS = now - lastDataReceivedTime;
            if (noDataReceivedIntervalS >= noDataReceivedMessageIntervalS
                && noDataReceivedIntervalS % noDataReceivedMessageIntervalS == 0)
            {
                logger.logMsg(LOG_INFO, "No widget data received for " + to_string(noDataReceivedIntervalS) + " seconds.");
                noDataReceivedMessageTime = now;
                if (noDataReceivedMessageIntervalS <= 32) {
                    noDataReceivedMessageIntervalS *= 2;
                }
            }
        }

        // There are no payloads to process, so give other threads a chance to
        // run.  Also, don't hammmer on the SPI interface too hard (and drive
        // up cpu usage) by polling for data too often.
        usleep(radioPollingLoopSleepIntervalUs);
    }
}


int main(int argc, char** argv)
{
    // Read configuration from the JSON file specified on the command line.
    if (argc != 2) {
        cout << "Usage:  " << argv[0] << " <configFileName>" << endl;
        return 2;
    }
    string configFileName(argv[1]);
    if (!readConfig(configFileName)) {
        return(EXIT_FAILURE);
    }

    // Make sure this is the only instance running.
    if (!lockFilePath.empty() && acquireProcessLock(lockFilePath) < 0) {
        exit(EXIT_FAILURE);
    }

    if (!logger.startLogging("widgetRcvr", Log::LogTo::redirect)) {
        exit(EXIT_FAILURE);
    }

    logger.logMsg(LOG_INFO, "---------- widgetRcvr starting ----------");

    // If the config file is really a symbolic link,
    // add the link target to the file name we'll log.
    string configFileNameAndTarget = configFileName;
    char buf[512];
    int count = readlink(configFileName.c_str(), buf, sizeof(buf));
    if (count >= 0) {
        buf[count] = '\0';
        configFileNameAndTarget += string(" -> ") + buf;
    }
    logger.logMsg(LOG_INFO, "configFileName = " + configFileNameAndTarget);
    logger.logMsg(LOG_INFO, "lockFilePath = " + lockFilePath);
    logger.logMsg(LOG_INFO, "patconIpAddress = " + patconIpAddress);
    logger.logMsg(LOG_INFO, "radioPollingLoopSleepIntervalUs = " + to_string(radioPollingLoopSleepIntervalUs));
    logger.logMsg(LOG_INFO, "widgetPortNumberBase = " + to_string(widgetPortNumberBase));

    if (!openUdpPorts()) {
        exit(EXIT_FAILURE);
    }

    if (!configureRadio()) {
        exit(EXIT_FAILURE);
    }

    radio.startListening();
    logger.logMsg(LOG_INFO, "Now listening for widget data.");

//    runLoop();
    
    logger.stopLogging();

    return 0;
}

