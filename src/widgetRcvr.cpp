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
#include <getopt.h>
#include <netinet/in.h>
#include <RF24/RF24.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconeWidgetTypes.h"
#include "Log.h"
#include "WidgetId.h"

using namespace std;


// ---------- constants ----------

constexpr unsigned int reinitializationSleepIntervalS = 1;

// We're using dynamic payload size, but we still need to know what the largest can be.
constexpr uint8_t maxPayloadSize = 32;

constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

constexpr int maxRadios = 2;


// ---------- command line options and their defaults ----------

static bool runAsDaemon = false;
static string configFileName = "activeConfig.json";
static string instanceName = "widgetRcvr";
static Log::LogTo logTo = Log::LogTo::redirect;     // we use redirect because radio.printDetails writes to stdout


// ---------- configuration ----------

static ConfigReader configReader;
static json11::Json configObject;
static string lockFilePath;
static string logFilePath = ".";
static std::string pidFilePathName;
static string patconIpAddress;
static unsigned int numRadios;
static unsigned int radioPollingLoopSleepIntervalUs;
static unsigned int widgetPortNumberBase;
static unsigned int logRotationIntervalMinutes;
static int logRotationOffsetHour;
static int logRotationOffsetMinute;

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
static uint8_t rfChannel[maxRadios];

// Probably no need to ever set auto acknowledgement to false because the sender
// can control whether or not acks are sent by using the NO_ACK bit.
static bool autoAck[maxRadios];

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
static rf24_pa_dbm_e rfPowerLevel[maxRadios];
static string rfPowerLevelStr[maxRadios];

// RF24_250KBPS or RF24_1MBPS
static rf24_datarate_e dataRate[maxRadios];
static string dataRateStr[maxRadios];

static uint8_t txRetryDelayMultiplier[maxRadios];   // 250 us additional delay multiplier (0-15)
static uint8_t txMaxRetries[maxRadios];             // max retries (0-15)

// RF24_CRC_DISABLED, RF24_CRC_8, or RF24_CRC_16 (the only sane choice)
static rf24_crclength_e crcLength[maxRadios];
static string crcLengthStr[maxRadios];


// ---------- globals ----------

Log logger;                     // this is the global Log object used everywhere
Log dataLogger;                 // this logs received messages to the _data log

// flags set by signals
static volatile bool gotExitSignal;
static volatile bool gotReinitSignal;

static struct sockaddr_in widgetSockAddr[maxWidgets];
static int widgetSock[maxWidgets];

RF24* radio[maxRadios];

// radio-specific statistics and state data
// TODO 9/30/2019 ross:  this stuff really should be in an array of struct radioStatistics
static time_t lastDataReceivedTime[maxRadios];
static time_t noDataReceivedMessageIntervalS[maxRadios];
static time_t noDataReceivedMessageTime[maxRadios];
static int zeroLengthPacketCount[maxRadios];
static bool lastPacketWasZeroLength[maxRadios];


void usage()
{
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Usage: widgegRcvr [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("\n");
    printf("-c pathname, --config_file=pathname\n");
    printf("    Read the JSON configuration document from the file specified by pathname.\n");
    printf("    Default is \"%s\".\n", configFileName.c_str());
    printf("\n");
    printf("-d, --daemon\n");
    printf("    Run as a daemon.\n");
    printf("\n");
    printf("-h, --help\n");
    printf("    Print this help information.\n");
    printf("\n");
    printf("-i name, --instance_name=name\n");
    printf("    Use name as the unique name of this instance.  The name is used in log file\n");
    printf("    names and as the message source identifier when logging to the system log.\n");
    printf("    It also specifies the JSON configuration document section (object) to be\n");
    printf("    used.  Default is \"%s\".\n", instanceName.c_str());
    printf("\n");
    printf("-l path, --log_path=path\n");
    printf("    Place log files in the directory specified by path.  Default is \"%s\".\n", logFilePath.c_str());
    printf("\n");
    printf("--log_to_console\n");
    printf("    Send all log messages to the console.\n");
    printf("\n");
    printf("--log_to_syslog\n");
    printf("    Send all log messages to the system log.  On Linux, the messages should go\n");
    printf("    to /var/log/messages.  On macOS, use this command to see the messages:\n");
    printf("    log stream --info --debug --predicate 'sender == \"<instance_name>\"' --style syslog\n");
    printf("\n");
    printf("--pidfile=pathname\n");
    printf("    When running as a daemon, write the process's PID to the file specified by\n");
    printf("    pathname.\n");
    printf("\n");
    printf("--version\n");
    printf("    Print version information and exit.\n");
    printf("\n");
}


static void getCommandLineOptions(int argc, char* argv[])
{
    enum LongOnlyOption {
        unhandled = 0,
        log_to_console,
        log_to_syslog,
        pid_file,
        version
    };

    int longOnlyOption = unhandled;
    static struct option longopts[] = {
        { "config_file",    required_argument,      NULL,            'c'            },
        { "daemon",         no_argument,            NULL,            'd'            },
        { "help",           no_argument,            NULL,            'h'            },
        { "instance_name",  required_argument,      NULL,            'i'            },
        { "log_path",       required_argument,      NULL,            'l'            },
        { "log_to_console", no_argument,            &longOnlyOption, log_to_console },
        { "log_to_syslog",  no_argument,            &longOnlyOption, log_to_syslog  },
        { "pidfile",        required_argument,      &longOnlyOption, pid_file       },
        { "version",        no_argument,            &longOnlyOption, version        },
        { NULL,             0,                      NULL,            0              }
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "c:dhi:l:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'c':
                configFileName = optarg;
                break;
            case 'd':
                runAsDaemon = true;
                break;
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            case 'i':
                instanceName = optarg;
                break;
            case 'l':
                logFilePath = optarg;
                break;
            case 0:
                switch (longOnlyOption) {
                    case log_to_console:
                        logTo = Log::LogTo::console;
                        break;
                    case log_to_syslog:
                        logTo = Log::LogTo::systemLog;
                        break;
                    case pid_file:
                        pidFilePathName = optarg;
                        break;
                    case version:
                        printf("%s last modified on %s, compiled on %s %s\n",
                               __BASE_FILE__, __TIMESTAMP__, __DATE__, __TIME__);
                        exit(EXIT_SUCCESS);
                        break;
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
    // TODO:  Handle non-option args here.
}


static void signalHandler(int signum)
{
    logger.logMsg(LOG_NOTICE, "Got signal %d.", signum);
    switch(signum) {

        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
        case SIGHUP:
            logger.logMsg(LOG_NOTICE, "Setting exit flag.");
            gotExitSignal = true;
            break;

        case SIGUSR1:
            logger.logMsg(LOG_NOTICE, "Setting reinitialize flag.");
            gotReinitSignal = true;
            break;

        case SIGUSR2:
            logger.logMsg(LOG_NOTICE, "SIGUSR2 ignored.");
            break;

        default:
            logger.logMsg(LOG_WARNING, "Signal %d is not supported.", signum);
    }
}


static bool registerSignalHandler()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    bool succeeded = true;

    sa.sa_handler = signalHandler;
    if (sigaction(SIGINT, &sa, NULL) < 0) succeeded = false;    // ^C
    if (sigaction(SIGTERM, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGQUIT, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGHUP, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGUSR2, &sa, NULL) < 0) succeeded = false;

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) succeeded = false;

    if (!succeeded) {
        logger.logMsg(LOG_ERR, "Registration of a handler for one or more signals failed.");
    }

    return succeeded;
}


void daemonize()
{
    pid_t pid;
    int fd;

    // Become an orphaned child of the init process.
    pid = fork();
    if (pid < 0) {
        logger.logMsg(LOG_CRIT, errno, "First fork failed.");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent process exits.
        exit(EXIT_SUCCESS);
    }

    // Detach from controlling terminal by putting this
    // process in a new process group and session.
    if (setsid() < 0) {
        logger.logMsg(LOG_CRIT, errno, "New session creation failed.");
        exit(EXIT_FAILURE);
    }

    // Ensure that the daemon cannot re-acquire a terminal.
    pid = fork();
    if (pid < 0) {
        logger.logMsg(LOG_CRIT, errno, "Second fork failed.");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent process exits.
        exit(EXIT_SUCCESS);
    }

    // Close all open files.
    for (fd = getdtablesize() - 1; fd >= 0; --fd) {
        close(fd);
    }

    // Something might try to use stdin, stdout, or stderr.
    // So, re-open them, but point them at /dev/null.
    fd = open("/dev/null", O_RDWR);     // first open descriptor is stdin
    if (fd < 0) {
        logger.logMsg(LOG_CRIT, errno, "Unable to open stdin");
        exit(EXIT_FAILURE);
    }
    dup(fd);    // second open descriptor is stdout
    dup(fd);    // third open descriptor is stderr

    // File permissions will need to be specified in the corresponding open() call.
    umask(0);

    // Make the root directory the current working directory because it is always there.
    chdir("/");

    // Write our PID to the PID file.
    if (!pidFilePathName.empty()) {
        std::ofstream ofs;
        ofs.open(pidFilePathName.c_str(), std::ios_base::out | std::ios_base::trunc);
        if (ofs.good()) {
            pid_t myPid = getpid();
            ofs << myPid << std::endl;
            ofs.close();
            logger.logMsg(LOG_INFO, "Wrote PID %d to %s", myPid, pidFilePathName.c_str());
        }
        else {
            logger.logMsg(LOG_CRIT, errno, "Unable to create PID file %s", pidFilePathName.c_str());
            exit(EXIT_FAILURE);
        }
    }
}



/*********************
 * UDP Communication *
 *********************/

bool openUdpPort(WidgetId widgetId)
{
    unsigned int widgetIdNumber = widgetIdToInt(widgetId);
    unsigned int portNumber = widgetPortNumberBase + widgetIdNumber;

    logger.logMsg(LOG_INFO, "Creating and binding socket for %s.", widgetIdToString(widgetId).c_str());

    memset(&widgetSockAddr[widgetIdNumber], 0, sizeof(struct sockaddr_in));

    widgetSockAddr[widgetIdNumber].sin_family = AF_INET;
    widgetSockAddr[widgetIdNumber].sin_addr.s_addr = htonl(INADDR_ANY);
    widgetSockAddr[widgetIdNumber].sin_port = htons(0);

    if ((widgetSock[widgetIdNumber] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        logger.logMsg(LOG_ERR, errno, "Failed to create socket for %s.", widgetIdToString(widgetId).c_str());
        return false;
    }

    if (::bind(widgetSock[widgetIdNumber],
               (struct sockaddr *) &widgetSockAddr[widgetIdNumber],
               sizeof(struct sockaddr_in)) < 0)
    {
        logger.logMsg(LOG_ERR, errno, "bind failed for %s.", widgetIdToString(widgetId).c_str());
        return false;
    }

    logger.logMsg(LOG_INFO, "Setting address to %s:%d", patconIpAddress.c_str(), portNumber);

    inet_pton(AF_INET, patconIpAddress.c_str(), &widgetSockAddr[widgetIdNumber].sin_addr.s_addr);
    widgetSockAddr[widgetIdNumber].sin_port = htons(portNumber);

    return true;
}


bool closeUdpPort(WidgetId widgetId)
{
    unsigned int widgetIdNumber = widgetIdToInt(widgetId);

    logger.logMsg(LOG_INFO, "Closing UDP port for %s...", widgetIdToString(widgetId).c_str());
    // TODO 2/3/2018 ross:  make sure this implementation is correct
    if (close(widgetSock[widgetIdNumber]) != 0) {
        logger.logMsg(LOG_ERR, errno, "Unable to close UDP port for %s.",
                      widgetIdToString(widgetId).c_str());
        return false;
    }
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

void handleStressTestPayload(const StressTestPayload* payload, unsigned int payloadSize, unsigned int radioIdx)
{
    static int lastPayloadNum[maxRadios];

    if (payloadSize != sizeof(StressTestPayload)) {
        logger.logMsg(LOG_ERR,
                      "Got StressTestPayload payload on radio %d with size %d, but size %d was expected.",
                      radioIdx, payloadSize, sizeof(StressTestPayload));
        return;
    }

    dataLogger.logMsg(LOG_INFO,
                  "stest: r=%d id=%d a=%d ch=%d seq=%d lastTxUs=%d fails=%d(%d%%)",
                  radioIdx,
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  payload->payloadNum,
                  payload->lastTxUs,
                  payload->numTxFailures,
                  payload->numTxFailures * 100 / payload->payloadNum);

    int payloadNumChange = (int) payload->payloadNum - lastPayloadNum[radioIdx];
    if (payloadNumChange != 1) {
        logger.logMsg(LOG_WARNING, "radio %d stest message gap:  %d", radioIdx, payloadNumChange);
    }
    lastPayloadNum[radioIdx] = payload->payloadNum;

    if (payload->widgetHeader.id != 0) {
        UdpPayload udpPayload;
        udpPayload.id       = payload->widgetHeader.id;
        udpPayload.channel  = payload->widgetHeader.channel;
        udpPayload.isActive = payload->widgetHeader.isActive;
        udpPayload.position = payload->payloadNum;
        udpPayload.velocity = 0;

        sendUdp(udpPayload);
    }
}


void handlePositionVelocityPayload(
    const PositionVelocityPayload* payload,
    unsigned int payloadSize,
    unsigned int radioIdx)
{
    if (payloadSize != sizeof(PositionVelocityPayload)) {
        logger.logMsg(LOG_ERR,
                      "Got PositionVelocityPayload payload on radio %d with size %d, but size %d was expected.",
                      radioIdx, payloadSize, sizeof(PositionVelocityPayload));
        return;
    }

    dataLogger.logMsg(LOG_INFO,
                  "pv: r=%d id=%d a=%d ch=%d p=%d v=%d",
                  radioIdx,
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


void handleMeasurementVectorPayload(
    const MeasurementVectorPayload* payload,
    unsigned int payloadSize,
    unsigned int radioIdx)
{
    if (payloadSize < (1 + sizeof(int16_t))) {
        logger.logMsg(LOG_ERR, "Got MeasurementVectorPayload on radio %d without any data.", radioIdx);
        return;
    }
    // TODO 8/3/2017 ross:  might be a good idea to make sure payloadSize is odd

    unsigned int numMeasurements = (payloadSize - 1) / sizeof(int16_t);

    stringstream sstr;
    for (unsigned int i = 0; i < numMeasurements; ++i) {
        sstr << " " << setfill(' ') << setw(6) << payload->measurements[i];
    }
    dataLogger.logMsg(LOG_INFO,
                  "mvec: r=%d id=%d a=%d ch=%d n=%d %s",
                  radioIdx,
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  numMeasurements,
                  sstr.str().c_str());

    // Map each measurement to a position measurement on the channel
    // corresponding to the measurement's position in the array.  The
    // channel number in the header is ignored.
    for (unsigned int i = 0; i < numMeasurements; ++i) {
        UdpPayload udpPayload;
        udpPayload.id       = payload->widgetHeader.id;
        udpPayload.channel  = i;
        udpPayload.isActive = payload->widgetHeader.isActive;
        udpPayload.position = payload->measurements[i];
        udpPayload.velocity = payload->measurements[i];

        sendUdp(udpPayload);
    }
}


void handleCustomPayload(const CustomPayload* payload, unsigned int payloadSize, unsigned int radioIdx)
{
    if (payloadSize < 2) {
        logger.logMsg(LOG_ERR, "Got CustomPayload on radio %d without any data.", radioIdx);
        return;
    }

    unsigned int bufLen = payloadSize - 1;

    stringstream sstr;
    for (unsigned int i = 0; i < bufLen; ++i) {
        sstr << " 0x" << hex << (int) payload->buf[i];
    }
    dataLogger.logMsg(LOG_INFO,
                  "custom: r=%d id=%d a=%d ch=%d bufLen=%d %s",
                  radioIdx,
                  payload->widgetHeader.id,
                  payload->widgetHeader.isActive,
                  payload->widgetHeader.channel,
                  bufLen,
                  sstr.str().c_str());

    switch (intToWidgetId(payload->widgetHeader.id)) {
        // At present, there are no widgets that send custom payloads.  When
        // one is added, call the handler for its specific payload type here.
        // Example:
        //case WidgetId::contortOMatic:
        //    handleContortOMaticPayload(payload, payloadSize);
        //    break;
        default:
            logger.logMsg(LOG_ERR, "There is no payload handler defined for widget id %d.", payload->widgetHeader.id);
    }
}



/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

bool loadRadioConfig(const json11::Json& radioConfigObject, unsigned int radioIdx, const string& errMsgSuffix)
{
    //logger.logMsg(LOG_DEBUG, "radioConfigObject:  " + radioConfigObject.dump());

    if (radioIdx >= maxRadios) {
        logger.logMsg(LOG_ERR, "Too many radios configured.  Maximum is %d.", maxRadios);
        return false;
    }

    bool successful = true;
    int i;

    if (!ConfigReader::getIntValue(radioConfigObject, "rfChannel", i, errMsgSuffix, 1, 125)) {
        successful = false;
    }
    else {
        rfChannel[radioIdx] = i;
    }

    if (!ConfigReader::getBoolValue(radioConfigObject, "autoAck", autoAck[radioIdx], errMsgSuffix)) {
        successful = false;
    }

    if (!ConfigReader::getStringValue(radioConfigObject, "rfPowerLevel", rfPowerLevelStr[radioIdx], errMsgSuffix)) {
        successful = false;
    }
    else if (rfPowerLevelStr[radioIdx] == "RF24_PA_MIN") {
        rfPowerLevel[radioIdx] = RF24_PA_MIN;
    }
    else if (rfPowerLevelStr[radioIdx] == "RF24_PA_LOW") {
        rfPowerLevel[radioIdx] = RF24_PA_LOW;
    }
    else if (rfPowerLevelStr[radioIdx] == "RF24_PA_HIGH") {
        rfPowerLevel[radioIdx] = RF24_PA_HIGH;
    }
    else if (rfPowerLevelStr[radioIdx] == "RF24_PA_MAX") {
        rfPowerLevel[radioIdx] = RF24_PA_MAX;
    }
    else {
        logger.logMsg(LOG_ERR,
                      "Invalid rfPowerLevel value \"%s\".  Valid values are"
                      " RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, and RF24_PA_MAX.",
                      rfPowerLevelStr[radioIdx].c_str());
        successful = false;
    }

    if (!ConfigReader::getStringValue(radioConfigObject, "dataRate", dataRateStr[radioIdx], errMsgSuffix)) {
        successful = false;
    }
    else if (dataRateStr[radioIdx] == "RF24_250KBPS") {
        dataRate[radioIdx] = RF24_250KBPS;
    }
    else if (dataRateStr[radioIdx] == "RF24_1MBPS") {
        dataRate[radioIdx] = RF24_1MBPS;
    }
    else {
        logger.logMsg(LOG_ERR,
                      "Invalid dataRate value \"%s\".  Valid values are RF24_250KBPS and RF24_1MBPS.",
                      dataRateStr[radioIdx].c_str());
        successful = false;
    }

    if (!ConfigReader::getIntValue(radioConfigObject, "txRetryDelayMultiplier", i, errMsgSuffix, 0, 15)) {
        successful = false;
    }
    else {
        txRetryDelayMultiplier[radioIdx] = i;
    }

    if (!ConfigReader::getIntValue(radioConfigObject, "txMaxRetries", i, errMsgSuffix, 1, 15)) {
        successful = false;
    }
    else {
        txMaxRetries[radioIdx] = i;
    }

    if (!ConfigReader::getStringValue(radioConfigObject, "crcLength", crcLengthStr[radioIdx], errMsgSuffix)) {
        successful = false;
    }
    else if (crcLengthStr[radioIdx] == "RF24_CRC_DISABLED") {
        crcLength[radioIdx] = RF24_CRC_DISABLED;
    }
    else if (crcLengthStr[radioIdx] == "RF24_CRC_8") {
        crcLength[radioIdx] = RF24_CRC_8;
    }
    else if (crcLengthStr[radioIdx] == "RF24_CRC_16") {
        crcLength[radioIdx] = RF24_CRC_16;
    }
    else {
        logger.logMsg(LOG_ERR,
                      "Invalid dataRate value \"%s\".  Valid values are RF24_CRC_DISABLED, RF24_CRC_8, and RF24_CRC_16.",
                      crcLengthStr[radioIdx].c_str());
        successful = false;
    }

    return successful;
}


bool readConfig()
{
    // Read the configuration file, and merge the instance-specific and common
    // configurations into a single configuration in configObject.  The
    // instance-specific configuration has priority (i.e., items in the common
    // configuration will not override the same items in the instance-specific
    // configuration).
    if (!configReader.loadConfiguration(configFileName)) {
        return false;
    }
    json11::Json instanceConfigObject;
    if (!ConfigReader::getJsonObject(configReader.getConfigObject(),
                                     instanceName,
                                     instanceConfigObject,
                                     " in " + configFileName + "."))
    {
        return false;
    }
    //logger.logMsg(LOG_DEBUG, "instanceConfigObject = " + instanceConfigObject.dump());
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
    //logger.logMsg(LOG_DEBUG, "configObject = " + configObject.dump());

    bool successful = true;
    string errMsgSuffix = " in the " + instanceName + " or common section of " + configFileName + ".";

    lockFilePath.clear();
    ConfigReader::getStringValue(configObject, "lockFilePath", lockFilePath);
    if (lockFilePath.empty()) {
        logger.logMsg(LOG_WARNING, "There is no lock file path" + errMsgSuffix);
    }

    if (!ConfigReader::getStringValue(configObject, "patconIpAddress", patconIpAddress, errMsgSuffix)) {
        successful = false;
    }

    if (!ConfigReader::getUnsignedIntValue(configObject,
                                           "radioPollingLoopSleepIntervalUs", radioPollingLoopSleepIntervalUs,
                                           errMsgSuffix, 1))
    {
        successful = false;
    }

    if (!ConfigReader::getUnsignedIntValue(configObject,
                                           "widgetPortNumberBase", widgetPortNumberBase,
                                           errMsgSuffix, 1024, 65535))
    {
        successful = false;
    }

    numRadios = 0;
    for (auto& radioConfigObject : configObject["radios"].array_items()) {
        if (radioConfigObject["radios"].is_array()) {
            for (auto& nestedRadioConfigObject : radioConfigObject["radios"].array_items()) {
                if (loadRadioConfig(nestedRadioConfigObject, numRadios, errMsgSuffix)) {
                    ++numRadios;
                }
                else {
                    successful = false;
                }
            }
        }
        else {
            if (loadRadioConfig(radioConfigObject, numRadios, errMsgSuffix)) {
                ++numRadios;
            }
            else {
                successful = false;
            }
        }
    }
    if (numRadios == 0) {
        logger.logMsg(LOG_ERR, "No radios are configured.");
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(configObject,
                                           "logRotationIntervalMinutes", logRotationIntervalMinutes,
                                           errMsgSuffix, 1))
    {
        return false;
    }

    if (!ConfigReader::getIntValue(configObject,
                                   "logRotationOffsetHour", logRotationOffsetHour,
                                   errMsgSuffix, 0, 23))
    {
        return false;
    }

    if (!ConfigReader::getIntValue(configObject,
                                   "logRotationOffsetMinute", logRotationOffsetMinute,
                                   errMsgSuffix, 0, 59))
    {
        return false;
    }

    return successful;
}


bool openUdpPorts()
{
    bool retval = true;
    for (int i = 0; i < maxWidgets; ++i) {
        WidgetId widgetId = intToWidgetId(i);
        if (widgetId != WidgetId::reserved && widgetId != WidgetId::invalid) {
            retval &= openUdpPort(widgetId);
        }
    }
    return retval;
}


bool closeUdpPorts()
{
    bool retval = true;
    for (int i = 0; i < maxWidgets; ++i) {
        WidgetId widgetId = intToWidgetId(i);
        if (widgetId != WidgetId::reserved && widgetId != WidgetId::invalid) {
            retval &= closeUdpPort(widgetId);
        }
    }
    return retval;
}


bool configureRadios()
{
    for (unsigned int radioIdx = 0; radioIdx < numRadios; ++radioIdx) {

        logger.logMsg(LOG_DEBUG, "Instantiating RF24 object for radio %d.", radioIdx);
        switch(radioIdx) {
            // RF24 constructor takes CE GPIO, A*10+B for SPI device at /dev/spidevA.B
            case 0:
                radio[radioIdx] = new RF24(25, 0);      // CE on GPIO 25 (J8 pin 22), SPI device spidev0.0
                break;
            case 1:
                radio[radioIdx] = new RF24(24, 1);      // CE on GPIO 24 (J8 pin 18), SPI device spidev0.1
                break;
            default:
                logger.logMsg(LOG_ERR, "No SPI interface defined for radio %d.", radioIdx);
                return false;
        }

        logger.logMsg(LOG_DEBUG, "Calling radio[%d]->begin.", radioIdx);
        if (!radio[radioIdx]->begin()) {
            logger.logMsg(LOG_ERR, "radio[%d]->begin failed.", radioIdx);
            return false;
        }

        logger.logMsg(LOG_DEBUG, "Setting configuration of radio %d.", radioIdx);
        radio[radioIdx]->setPALevel(rfPowerLevel[radioIdx]);
        radio[radioIdx]->setRetries(txRetryDelayMultiplier[radioIdx], txMaxRetries[radioIdx]);
        radio[radioIdx]->setDataRate(dataRate[radioIdx]);
        radio[radioIdx]->setChannel(rfChannel[radioIdx]);
        radio[radioIdx]->setAutoAck(autoAck[radioIdx]);
        radio[radioIdx]->enableDynamicPayloads();
        radio[radioIdx]->setCRCLength(crcLength[radioIdx]);

        for (uint8_t i = 0; i < numReadPipes; ++i) {
            logger.logMsg(LOG_DEBUG, "Opening read pipe %d for radio %d.", i, radioIdx);
            radio[radioIdx]->openReadingPipe(i, readPipeAddresses[i]);
        }

        logger.logMsg(LOG_INFO, "Radio %d configuration details:", radioIdx);
        radio[radioIdx]->printDetails();

        logger.logMsg(LOG_DEBUG, "Calling radio[%d]->startListening.", radioIdx);
        radio[radioIdx]->startListening();
        logger.logMsg(LOG_INFO, "Now listening for widget data on radio %d.", radioIdx);
    }

    return true;
}


bool shutDownRadios()
{
    for (unsigned int radioIdx = 0; radioIdx < numRadios; ++radioIdx) {

        for (uint8_t i = 0; i < numReadPipes; ++i) {
            radio[radioIdx]->closeReadingPipe(i);
        }

        // TODO:  Maybe someday power it off here (and also power it on in configureRadios).

        delete radio[radioIdx];
        radio[radioIdx] = nullptr;
    }

    return true;
}


bool doInitialization()
{
    // We print the configuration here rather than in readConfig because readConfig
    // can be called before we know if we're the only instance and can run.  When
    // using the one-minute restart approach via crontab, printing anything in
    // readConfig would result in writing a lot of unnecessary crap to the log.

    // If the config file is really a symbolic link,
    // add the link target to the file name we'll log.
    string configFileNameAndTarget = configFileName;
    char buf[512];
    int count = readlink(configFileName.c_str(), buf, sizeof(buf));
    if (count >= 0) {
        buf[count] = '\0';
        configFileNameAndTarget += string(" -> ") + buf;
    }

    logger.logMsg(LOG_INFO, "Starting initialization.");
    logger.logMsg(LOG_INFO, "instanceName = " + instanceName);
    logger.logMsg(LOG_INFO, "configFileName = " + configFileNameAndTarget);
    logger.logMsg(LOG_INFO, "lockFilePath = " + lockFilePath);
    logger.logMsg(LOG_INFO, "logFilePath = " + logFilePath);
    logger.logMsg(LOG_INFO, "logRotationIntervalMinutes = %d", logRotationIntervalMinutes);
    logger.logMsg(LOG_INFO, "logRotationOffsetHour = %d", logRotationOffsetHour);
    logger.logMsg(LOG_INFO, "logRotationOffsetMinute = %d", logRotationOffsetMinute);
    logger.logMsg(LOG_INFO, "patconIpAddress = " + patconIpAddress);
    logger.logMsg(LOG_INFO, "radioPollingLoopSleepIntervalUs = %u", radioPollingLoopSleepIntervalUs);
    logger.logMsg(LOG_INFO, "widgetPortNumberBase = %u", widgetPortNumberBase);

    for (int i = 0; i < maxRadios; ++i) {
        logger.logMsg(LOG_INFO, "Radio %d", i);
        logger.logMsg(LOG_INFO, "  rfChannel = %d", rfChannel[i]);
        logger.logMsg(LOG_INFO, "  autoAck = %s", autoAck[i] ? "true" : "false");
        logger.logMsg(LOG_INFO, "  rfPowerLevel = " + rfPowerLevelStr[i]);
        logger.logMsg(LOG_INFO, "  dataRate = " + dataRateStr[i]);
        logger.logMsg(LOG_INFO, "  txRetryDelayMultiplier = %d", txRetryDelayMultiplier[i]);
        logger.logMsg(LOG_INFO, "  txMaxRetries = %d", txMaxRetries[i]);
        logger.logMsg(LOG_INFO, "  crcLength = " + crcLengthStr[i]);
    }

    logger.setAutoLogRotation(logRotationIntervalMinutes, logRotationOffsetHour, logRotationOffsetMinute);

    dataLogger.startLogging(instanceName + "_data", Log::LogTo::file, logFilePath);
    dataLogger.setAutoLogRotation(logRotationIntervalMinutes, logRotationOffsetHour, logRotationOffsetMinute);

    if (!openUdpPorts()) {
        return false;
    }

    if (!configureRadios()) {
        return false;
    }

    logger.logMsg(LOG_INFO, "Initialization done.");

    return true;
}


bool doTeardown()
{
    logger.logMsg(LOG_INFO, "Starting teardown.");

    if (!closeUdpPorts()) {
        return false;
    }

    if (!shutDownRadios()) {
        return false;
    }

    dataLogger.stopLogging();

    logger.logMsg(LOG_INFO, "Teardown done.");

    return true;
}


void initRadioStatistics()
{
    for (unsigned int radioIdx = 0; radioIdx < numRadios; ++radioIdx) {
        time(&lastDataReceivedTime[radioIdx]);
        noDataReceivedMessageIntervalS[radioIdx] = 2;
        noDataReceivedMessageTime[radioIdx] = 0;
        zeroLengthPacketCount[radioIdx] = 0;
        lastPacketWasZeroLength[radioIdx] = false;
    }
}


bool pollRadio(unsigned int radioIdx)
{
    bool dataReceived = false;
    uint8_t pipeNum;

    while (radio[radioIdx]->available(&pipeNum)) {

        dataReceived = true;

        unsigned int payloadSize = radio[radioIdx]->getDynamicPayloadSize();
        if (payloadSize == 0) {
            // The zero-length packet burst problem makes the log file massive.
            // To prevent that until we can find and fix the cause, we'll log
            // one message when the first zero-length packet is received and
            // another with the zero-length packet count when a non-zero-length
            // packet is finally received.
            if (!lastPacketWasZeroLength[radioIdx]) {
                lastPacketWasZeroLength[radioIdx] = true;
                zeroLengthPacketCount[radioIdx] = 1;
                logger.logMsg(LOG_ERR, "Got invalid packet (payloadSize = 0) on radio %d.", radioIdx);
            }
            else {
                ++zeroLengthPacketCount[radioIdx];
            }
            continue;
        }
        if (lastPacketWasZeroLength[radioIdx]) {
            lastPacketWasZeroLength[radioIdx] = false;
            logger.logMsg(LOG_ERR,
                          "%d zero-length packets were received on radio %d.",
                          zeroLengthPacketCount[radioIdx], radioIdx);
        }
        if (payloadSize > maxPayloadSize) {
            logger.logMsg(LOG_ERR, "Got unsupported payload size %d on radio %d.", payloadSize, radioIdx);
            // RF24 is supposed to do a Flush_RX command and return 0 for
            // the size if an invalid payload length is detected.  It
            // apparently didn't do that.  Who knows what we're supposed
            // to do now.  We'll try turning receive off and on to clear
            // the rx buffers and start over.
            radio[radioIdx]->stopListening();
            delay(100);
            radio[radioIdx]->startListening();
            continue;
        }

        uint8_t payload[payloadSize];
        radio[radioIdx]->read(payload, payloadSize);

        stringstream sstr;

        switch(pipeNum) {

            case 0:
                handleStressTestPayload((StressTestPayload*) payload, payloadSize, radioIdx);
                break;

            case 1:
                handlePositionVelocityPayload((PositionVelocityPayload*) payload, payloadSize, radioIdx);
                break;

            case 2:
                handleMeasurementVectorPayload((MeasurementVectorPayload*) payload, payloadSize, radioIdx);
                break;

            case 3:
            case 4:
                for (unsigned int i = 0; i < maxPayloadSize; ++i) {
                    sstr << " 0x" << hex << (int) payload[i];
                }
                logger.logMsg(LOG_ERR,
                       "Got payload with size %d via unsupported pipe %d on radio %d.  Contents: %s",
                       payloadSize, (int) pipeNum, radioIdx, sstr.str().c_str());
                break;

            case 5:
                handleCustomPayload((CustomPayload*) payload, payloadSize, radioIdx);
                break;

            default:
                for (unsigned int i = 0; i < maxPayloadSize; ++i) {
                    sstr << " 0x" << hex << (int) payload[i];
                }
                logger.logMsg(LOG_ERR,
                       "pipeNum on radio %d is %d, which should never happen!  Payload contents: %s",
                       radioIdx, (int) pipeNum, sstr.str().c_str());
        }
    }

    if (dataReceived) {
        time(&lastDataReceivedTime[radioIdx]);
        noDataReceivedMessageIntervalS[radioIdx] = 2;
    }
    else {
        time_t now;
        time(&now);
        if (now != noDataReceivedMessageTime[radioIdx]) {
            time_t noDataReceivedIntervalS = now - lastDataReceivedTime[radioIdx];
            if (noDataReceivedIntervalS >= noDataReceivedMessageIntervalS[radioIdx]
                && noDataReceivedIntervalS % noDataReceivedMessageIntervalS[radioIdx] == 0)
            {
                logger.logMsg(LOG_INFO,
                              "No widget data received on radio %d for %ld seconds.",
                              radioIdx, noDataReceivedIntervalS);
                noDataReceivedMessageTime[radioIdx] = now;
                if (noDataReceivedMessageIntervalS[radioIdx] <= 32) {
                    noDataReceivedMessageIntervalS[radioIdx] *= 2;
                }
            }
        }
    }

    return dataReceived;
}


bool runLoop()
{
    initRadioStatistics();

    while (!gotExitSignal) {

        if (gotReinitSignal) {
            gotReinitSignal = false;
            logger.logMsg(LOG_INFO, "---------- Reinitializing... ----------");
            if (!doTeardown()) {
                return false;
            }
            // Sleep for a little bit because reconnecting to
            // the OPC server immediately sometimes fails.
            logger.logMsg(LOG_INFO, "Sleeping for " + to_string(reinitializationSleepIntervalS) + " seconds.");
            sleep(reinitializationSleepIntervalS);
            if (!readConfig() || !doInitialization()) {
                return false;
            }
            initRadioStatistics();
        }

        bool dataReceived = false;
        for (unsigned int radioIdx = 0; radioIdx < numRadios; ++radioIdx) {
            dataReceived |= pollRadio(radioIdx);
        }
        if (!dataReceived) {
            // There are no payloads to process, so give other threads a chance to
            // run.  Also, don't hammmer on the SPI interface too hard (and drive
            // up cpu usage) by polling for data too often.
            usleep(radioPollingLoopSleepIntervalUs);
        }
    }

    return true;
}


int main(int argc, char** argv)
{
    getCommandLineOptions(argc, argv);

    // We can start logging to the system log before daemonizing.
    if (logTo == Log::LogTo::systemLog) {
        logger.startLogging(instanceName, logTo);
    }

    if (runAsDaemon) {
        logger.logMsg(LOG_INFO, "Daemonizing...");
        daemonize();
        logger.logMsg(LOG_NOTICE, "Now running as a daemon.");
    }

    // Because daemonizing closes all files, we have to wait until after we're
    // running as a daemon before we can log to a file.  (Logging to the console
    // when running as a daemon ends up logging to the bit bucket.)
    if (logTo == Log::LogTo::console) {
        logger.startLogging(instanceName, logTo);
    }
    else if (logTo == Log::LogTo::redirect) {
        logger.startLogging(instanceName, logTo, logFilePath);
    }

    if (!registerSignalHandler()) {
        return(EXIT_FAILURE);
    }

    if (!readConfig()) {
        return(EXIT_FAILURE);
    }

    // Make sure this is the only instance running if a
    // place to put the lock file has been specified.
    if (!lockFilePath.empty()) {
        string lockFilePathName = lockFilePath + "/" + instanceName + ".lock";
        bool logIfLocked = logTo == Log::LogTo::console || runAsDaemon;
        if (acquireProcessLock(lockFilePathName, logIfLocked) < 0) {
            return(EXIT_FAILURE);
        }
    }

    logger.logMsg(LOG_INFO, "---------- widgetRcvr starting ----------");

    if (!doInitialization()) {
        return(EXIT_FAILURE);
    }

    if (!runLoop()) {
        return(EXIT_FAILURE);
    }

    logger.logMsg(LOG_INFO, "---------- Exiting... ----------");

    if (!doTeardown()) {
        return(EXIT_FAILURE);
    }

    logger.stopLogging();

    return EXIT_SUCCESS;
}

