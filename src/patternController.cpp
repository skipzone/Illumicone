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


#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <getopt.h>
//#include <netinet/in.h>
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
#include "hsv2rgb.h"
#include "illumiconeTypes.h"
#include "illumiconePixelUtility.h"
#include "illumiconeUtility.h"
#include "lib8tion.h"
#include "Log.h"
#include "Pattern.h"
#include "patternFactory.h"
#include "pixeltypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "widgetFactory.h"

using namespace std;


static const SchedulePeriod noPeriod = {false, "", 0, 0};
static constexpr unsigned int reinitializationSleepIntervalS = 1;

enum class OperatingState {
    startup,
    initializing,
    testPattern,
    active,
    inactive,
    idle,
    quiescent,
    shutoff,
    shutdown
};

enum class PatternBlendMethod {
    overlay,
    rgbAdd,
    rgbBlend,
    hsvBlend,
    hsvHueBlend
};

struct PatternState {
    Pattern* pattern;
    int priority;
    fract8 amountOfOverlay;
    int wantsDisplay;
};

Log logger;                     // this is the global Log object used everywhere
Log dataLogger;                 // this logs received messages to the _data log

// command line options and their defaults
static bool runAsDaemon = false;
static string configFileName = "activeConfig.json";
static string instanceName = "patternController";
static Log::LogTo logTo = Log::LogTo::file;

// configuration (except for widgets and patterns)
static ConfigReader configReader;
static json11::Json configObject;
static string lockFilePath;
static string logFilePath = ".";
static string pidFilePathName;
static unsigned int numberOfStrings;
static unsigned int numberOfPixelsPerString;
static vector<SchedulePeriod> shutoffPeriods;
static vector<SchedulePeriod> quiescentPeriods;
static vector<SchedulePeriod> overridePeriods;
static string patternBlendMethodStr;
static PatternBlendMethod patternBlendMethod;
static unsigned int patternRunLoopSleepIntervalUs;
static bool useTcpForOpcServer;
static string opcServerIpAddress;
static unsigned int opcServerPortNumber;
static unsigned int logRotationIntervalMinutes;
static int logRotationOffsetHour;
static int logRotationOffsetMinute;

// flags set by signals
static volatile bool gotExitSignal;
static volatile bool gotReinitSignal;
static volatile bool gotToggleTestPatternSignal;

static struct sockaddr_in opcServerSockaddr;
static int opcServerSocketFd;
static uint8_t* opcBuffer;      // points to the buffer used for sending messages to the OPC server
static ssize_t opcBufferSize;
static uint8_t* opcData;        // points to the data portion of opcBuffer

static map<WidgetId, Widget*> widgets;
static vector<PatternState*> patternStates;

static HsvConeStrings hsvFinalFrame;
static RgbConeStrings rgbFinalFrame;


static void usage();
static void getCommandLineOptions(int argc, char* argv[]);
static bool registerSignalHandler();
static void signalHandler(int signum);
static void daemonize();
static void logOperatingState(OperatingState currentOperatingState, const string& msg = "");


void usage()
{
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Usage: patternController [options]\n");
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
                        printf("%s last modified on %s, compiled on %s %s\n", __BASE_FILE__, __TIMESTAMP__, __DATE__, __TIME__);
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


static bool registerSignalHandler()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    bool succeeded = true;

    sa.sa_handler = signalHandler;
    if (sigaction(SIGHUP, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGINT, &sa, NULL) < 0) succeeded = false;    // ^C
    if (sigaction(SIGQUIT, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) succeeded = false;   // TCP socket sends this when disconnected
    if (sigaction(SIGTERM, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) succeeded = false;
    if (sigaction(SIGUSR2, &sa, NULL) < 0) succeeded = false;

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) succeeded = false;

    if (!succeeded) {
        logger.logMsg(LOG_ERR, "Registration of a handler for one or more signals failed.");
    }

    return succeeded;
}


static void signalHandler(int signum)
{
    logger.logMsg(LOG_NOTICE, "Got signal %d.", signum);
    switch(signum) {

        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            logger.logMsg(LOG_NOTICE, "Setting exit flag.");
            gotExitSignal = true;
            break;

        case SIGPIPE:
            logger.logMsg(LOG_ERR, "Got SIGPIPE.  Connection to OPC server probably lost.  Setting exit flag.");
            gotExitSignal = true;
            break;

        case SIGUSR1:
            logger.logMsg(LOG_NOTICE, "Setting reinitialize flag.");
            gotReinitSignal = true;
            break;

        case SIGUSR2:
            logger.logMsg(LOG_NOTICE, "Setting test pattern toggle flag.");
            gotToggleTestPatternSignal = true;
            break;

        default:
            logger.logMsg(LOG_WARNING, "Signal %d is not supported.", signum);
    }
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


void dumpOpcBuffer(size_t numStringsToPrint)
{
    logger.logMsg(LOG_DEBUG,
        "opcBuffer header:  "
        + to_string((int) opcBuffer[0]) + " "
        + to_string((int) opcBuffer[1]) + " "
        + to_string((int) opcBuffer[2]) + " "
        + to_string((int) opcBuffer[3]));

    if (numStringsToPrint > 0) {
        for (size_t iPixel = 0; iPixel < numberOfPixelsPerString; ++iPixel) {
            stringstream sstr;
            sstr << setfill(' ') << setw(3) << iPixel << ":";
            for (size_t iString = 0; iString < numStringsToPrint; ++iString) {
                size_t pixelOffset = iString * numberOfPixelsPerString * 3 + iPixel * 3;
                sstr << "  "
                    << setfill(' ') << setw(3) << (int) opcData[pixelOffset] << ","
                    << setfill(' ') << setw(3) << (int) opcData[pixelOffset + 1] << ","
                    << setfill(' ') << setw(3) << (int) opcData[pixelOffset + 2];
            }
            logger.logMsg(LOG_DEBUG, sstr.str());
        }
    }
}


bool sendOpcMessage()
{
    // TODO 2/5/2018 ross:  Need to throttle the messaging so that things like turnOnSafetlyLights don't hammer opc-server.

    for (unsigned int col = 0; col < numberOfStrings; col++) {
        unsigned int colOffset = col * numberOfPixelsPerString * 3;
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            unsigned int pixelOffset = colOffset + row * 3;
            opcData[pixelOffset] = rgbFinalFrame[col][row].r;
            opcData[pixelOffset + 1] = rgbFinalFrame[col][row].g;
            opcData[pixelOffset + 2] = rgbFinalFrame[col][row].b;
        }
    }

    //dumpOpcBuffer(2);

    if (useTcpForOpcServer) {
        //logger.logMsg(LOG_DEBUG, "sending message to OPC server via TCP...");
        if (send(opcServerSocketFd, opcBuffer, opcBufferSize, 0) == -1) {
            logger.logMsg(LOG_ERR, errno, "Failed to send message to OPC server via TCP.");
            return false;
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
            return false;
        }
        if (bytesSentCount != opcBufferSize) {
            logger.logMsg(LOG_ERR,
                   "UDP payload size is " + to_string(opcBufferSize)
                   + ", but " + to_string(bytesSentCount) + " bytes were sent to OPC server.");
            return false;
        }
        //logger.logMsg(LOG_DEBUG, "Sent " to_string(bytesSentCount) + " byte payload via UDP.");
    }

    return true;
}


void turnOffAllPixels()
{
    fillSolid(rgbFinalFrame, RgbPixel::Black);
    sendOpcMessage();
}


void turnOnSafetyLights()
{
    fillSolid(rgbFinalFrame, RgbPixel::Black);
    for (auto&& stringPixels : rgbFinalFrame) {
        // TODO ross 7/22/2017:  get the safety color from config
        stringPixels[0] = RgbPixel::Magenta;
        stringPixels[numberOfPixelsPerString - 1] = RgbPixel::Magenta;
    }
    sendOpcMessage();
}


void setAllPixelsToQuiescentColor(SchedulePeriod& selectedSchedulePeriod)
{
    RgbPixel quiescentColor = RgbPixel::Navy;
    string errMsgSuffix =
        " in " + selectedSchedulePeriod.description
        + " quiescent period configuration.  Using default instead.";
    string rgbStr;
    ConfigReader::getRgbPixelValue(selectedSchedulePeriod.periodConfigObj,
                                   "quiescentColor",
                                   rgbStr,
                                   quiescentColor,
                                   errMsgSuffix,
                                   true,
                                   RgbPixel::Navy);

    fillSolid(rgbFinalFrame, quiescentColor);
    sendOpcMessage();
}


void displayTestPattern()
{
    // As an aid for identifying strings that intermittenly don't respond to new
    // data but are illuminated nevertheless, we'll alternate between the odd
    // and even illumination pixels with each frame.  If a string isn't
    // responding consistently, every other illumination pixel will be off or
    // perhaps flickering.  Note that the average intensity will be half of that
    // specified by areaIlluminationColorStr.
    static int areaIlluminationPixelToggle;

    // TODO 2/5/2018 ross:  get the colors from config
    string areaIlluminationColorStr = "255,255,255";        // effectively half-intensity white due to 50% duty cycle
    string stringPosition1IndicatorColorStr = "0,255,0";    // full-intensity green
    string stringPosition5IndicatorColorStr = "0,255,255";  // full-intensity cyan
    string stringPosition10IndicatorColorStr = "255,0,255"; // full-intensity magenta
    CRGB areaIlluminationColor;
    CRGB stringPosition1IndicatorColor;
    CRGB stringPosition5IndicatorColor;
    CRGB stringPosition10IndicatorColor;
    stringToRgbPixel(areaIlluminationColorStr, areaIlluminationColor);
    stringToRgbPixel(stringPosition1IndicatorColorStr, stringPosition1IndicatorColor);
    stringToRgbPixel(stringPosition5IndicatorColorStr, stringPosition5IndicatorColor);
    stringToRgbPixel(stringPosition10IndicatorColorStr, stringPosition10IndicatorColor);

    fillSolid(rgbFinalFrame, RgbPixel::Black);

    // On each string and from the bottom up, turn on the quantity of pixels
    // that corresponds to the string's position.  Also, turn on the top part of
    // the cone for area illumination.
    for (size_t iString = 0; iString < numberOfStrings; ++iString) {

        // It is possible to have more strings than pixels per string (like,
        // if we expand Mini-Cone from 12 to 24 strings).  So, we'll make the
        // number of illuminated string position pixels wrap around to 1.
        int numStringPositionPixels = iString % numberOfPixelsPerString + 1;

        // The string position pixels are illuminated from the bottom up.
        int iPixel = numberOfPixelsPerString - 1;
        for (int i = 0; i < numStringPositionPixels; ++i) {
            rgbFinalFrame[iString][iPixel]
                = (i + 1) % 10 == 0 ? stringPosition10IndicatorColor
                : (i + 1) % 5 == 0 ? stringPosition5IndicatorColor
                : stringPosition1IndicatorColor;
            --iPixel;
        }

        // Use the part of the cone above the string
        // position indicators for area illumination.
        for (iPixel = numberOfPixelsPerString - numberOfStrings - 1; iPixel >= 0; --iPixel) {
            if (iPixel % 2 == areaIlluminationPixelToggle) {
                rgbFinalFrame[iString][iPixel] = areaIlluminationColor;
            }
        }
    }

    areaIlluminationPixelToggle = !areaIlluminationPixelToggle;

    sendOpcMessage();
}


void printSchedulePeriods(const string& scheduleDescription, const vector<SchedulePeriod>& schedulePeriods)
{
    logger.logMsg(LOG_INFO, scheduleDescription + ":");

    for (auto&& schedulePeriod : schedulePeriods) {
        string msg;
        int msgPriority = LOG_INFO;

        struct tm tmStartTime;
        struct tm tmEndTime;
        localtime_r(&schedulePeriod.startTime, &tmStartTime);
        localtime_r(&schedulePeriod.endTime, &tmEndTime);
        char startTimeBuf[20];
        char endTimeBuf[20];
        msg = "    " + schedulePeriod.description + ":  ";
        string dateTimeFormat;
        if (schedulePeriod.isDaily) {
            dateTimeFormat = "%H:%M:%S";
            msg += "daily, ";
        }
        else {
            dateTimeFormat = "%Y-%m-%d %H:%M:%S";
        }
        if (strftime(startTimeBuf, sizeof(startTimeBuf), dateTimeFormat.c_str(), &tmStartTime) != 0
            && strftime(endTimeBuf, sizeof(endTimeBuf), dateTimeFormat.c_str(), &tmEndTime) != 0)
        {
            msg += string(startTimeBuf) + " - " + string(endTimeBuf);
        }
        else
        {
            msg += "is shit!";
            msgPriority = LOG_ERR;
        }

        logger.logMsg(msgPriority, msg);
    }
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

    string errMsgSuffix = " in the " + instanceName + " or common section of " + configFileName + ".";

    lockFilePath.clear();
    ConfigReader::getStringValue(configObject, "lockFilePath", lockFilePath);
    if (lockFilePath.empty()) {
        logger.logMsg(LOG_WARNING, "There is no lock file path" + errMsgSuffix);
    }

    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfStrings", numberOfStrings, errMsgSuffix)) return false;
    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfPixelsPerString", numberOfPixelsPerString, errMsgSuffix)) return false;

    shutoffPeriods.clear();
    quiescentPeriods.clear();
    overridePeriods.clear();
    if (!ConfigReader::getSchedulePeriods(configObject, "shutoffPeriods", shutoffPeriods)
        || !ConfigReader::getSchedulePeriods(configObject, "quiescentPeriods", quiescentPeriods)
        || !ConfigReader::getSchedulePeriods(configObject, "overridePeriods", overridePeriods))
    {
        logger.logMsg(LOG_ERR, "shutoffPeriods, quiescentPeriods, and/or overridePeriods is missing or invalid" + errMsgSuffix);
        return false;
    }

    if (!ConfigReader::getStringValue(configObject, "patternBlendMethod", patternBlendMethodStr, errMsgSuffix)) return false;
    if (patternBlendMethodStr == "overlay") {
        patternBlendMethod = PatternBlendMethod::overlay;
    }
    else if (patternBlendMethodStr == "rgbAdd") {
        patternBlendMethod = PatternBlendMethod::rgbAdd;
    }
    else if (patternBlendMethodStr == "rgbBlend") {
        patternBlendMethod = PatternBlendMethod::rgbBlend;
    }
    else if (patternBlendMethodStr == "hsvBlend") {
        patternBlendMethod = PatternBlendMethod::hsvBlend;
    }
    else if (patternBlendMethodStr == "hsvHueBlend") {
        patternBlendMethod = PatternBlendMethod::hsvHueBlend;
    }
    else {
        logger.logMsg(LOG_ERR, "Unrecognized patternBlendMethod \"" + patternBlendMethodStr + "\"" + errMsgSuffix);
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(configObject, "patternRunLoopSleepIntervalUs", patternRunLoopSleepIntervalUs, errMsgSuffix, 1)) return false;

    if (!ConfigReader::getBoolValue(configObject, "useTcpForOpcServer", useTcpForOpcServer, errMsgSuffix)) return false;
    if (!ConfigReader::getStringValue(configObject, "opcServerIpAddress", opcServerIpAddress, errMsgSuffix)) return false;
    if (!ConfigReader::getUnsignedIntValue(configObject, "opcServerPortNumber", opcServerPortNumber, errMsgSuffix, 1024, 65535)) return false;

    if (!ConfigReader::getUnsignedIntValue(configObject, "logRotationIntervalMinutes", logRotationIntervalMinutes, errMsgSuffix, 1)) return false;
    if (!ConfigReader::getIntValue(configObject, "logRotationOffsetHour", logRotationOffsetHour, errMsgSuffix, 0, 23)) return false;
    if (!ConfigReader::getIntValue(configObject, "logRotationOffsetMinute", logRotationOffsetMinute, errMsgSuffix, 0, 59)) return false;

    return true;
}


bool initOpcBuffer()
{
    // Allocate the buffer that will be used to build and send the Open Pixel Control (OPC) messages.
    opcBufferSize = numberOfStrings * numberOfPixelsPerString * 3 + 4;
    opcBuffer = new uint8_t[opcBufferSize];
    if (opcBuffer == nullptr) {
        logger.logMsg(LOG_ERR, "Unable to allocate an OPC buffer of size " + to_string(opcBufferSize));
        return false;
    }

    // Set up the header now because it won't change.
    opcBuffer[0] = 0;   // channel
    opcBuffer[1] = 0;   // command
    opcBuffer[2] = numberOfStrings * numberOfPixelsPerString * 3 / 256; // data length high byte
    opcBuffer[3] = numberOfStrings * numberOfPixelsPerString * 3 % 256; // data length low byte

    // The data portion of the OPC message starts just after the header.
    opcData = &opcBuffer[4];

    return true;
}


void freeOpcBuffer()
{
    delete [] opcBuffer;
    opcBuffer = nullptr;
    opcBufferSize = 0;
    opcData = nullptr;
}


void loadWidgetConfig(const json11::Json& widgetConfigObject)
{
    string widgetName = widgetConfigObject["name"].string_value();
    if (widgetName.empty()) {
        logger.logMsg(LOG_ERR, "Widget configuration has no name:  " + widgetConfigObject.dump());
        return;
    }
    if (!widgetConfigObject["enabled"].bool_value()) {
        logger.logMsg(LOG_INFO, widgetName + " is disabled.");
        return;
    }
    WidgetId widgetId = stringToWidgetId(widgetName);
    if (widgetId == WidgetId::invalid) {
        logger.logMsg(LOG_ERR, "Widget configuration has invalid name:  " + widgetConfigObject.dump());
        return;
    }
    if (widgets.find(widgetId) != widgets.end()) {
        logger.logMsg(LOG_ERR, widgetName + " appears multiple times.  This configuration ignored:  " + widgetConfigObject.dump());
        return;
    }
    Widget* newWidget = widgetFactory(widgetId);
    if (newWidget == nullptr) {
        logger.logMsg(LOG_ERR, "Unable to instantiate Widget object for " + widgetName);
        return;
    }
    if (!newWidget->init(widgetConfigObject, configObject)) {
        logger.logMsg(LOG_ERR, "Unable to initialize Widget object for " + widgetName);
        return;
    }
    logger.logMsg(LOG_INFO, widgetName + " initialized.");
    widgets[widgetId] = newWidget;
}


void initWidgets()
{
    logger.logMsg(LOG_INFO, "Initializing widgets...");

    for (auto& widgetConfigObject : configObject["widgets"].array_items()) {

        if (widgetConfigObject["widgets"].is_array()) {
            for (auto& nestedWidgetConfigObject : widgetConfigObject["widgets"].array_items()) {
                loadWidgetConfig(nestedWidgetConfigObject);
            }
        }
        else {
            loadWidgetConfig(widgetConfigObject);
        }
    }
}


void tearDownWidgets()
{
    logger.logMsg(LOG_INFO, "Tearing down widgets...");
    for (auto&& widget : widgets) {
        logger.logMsg(LOG_DEBUG, "deleting " + widgetIdToString(widget.first));
        delete widget.second;
    }
    widgets.clear();
}


void loadPatternConfig(const json11::Json& patternConfigObject)
{
    //logger.logMsg(LOG_DEBUG, "patternConfigObject:  " + patternConfigObject.dump());
    string patternName = patternConfigObject["name"].string_value();
    if (patternName.empty()) {
        logger.logMsg(LOG_ERR, "Pattern configuration has no name:  " + patternConfigObject.dump());
        return;
    }
    if (!patternConfigObject["enabled"].bool_value()) {
        logger.logMsg(LOG_INFO, patternName + " is disabled.");
        return;
    }
    string patternClassName = patternConfigObject["patternClassName"].string_value();
    if (patternClassName.empty()) {
        logger.logMsg(LOG_ERR, "Pattern configuration does not have a pattern class name:  " + patternConfigObject.dump());
        return;
    }
    Pattern* newPattern = patternFactory(patternClassName, patternName);
    if (newPattern == nullptr) {
        logger.logMsg(LOG_ERR,
                "Unable to instantiate " + patternClassName + " object for " + patternName
                + ".  (Is the pattern class name correct?)");
        return;
    }
    if (!newPattern->init(patternConfigObject, configObject, widgets)) {
        logger.logMsg(LOG_ERR, "Unable to initialize Pattern object for " + patternName);
        delete newPattern;
        return;
    }
    logger.logMsg(LOG_INFO, patternName + " initialized.");

    PatternState* newPatternState = new PatternState;
    newPatternState->pattern = newPattern;
    patternStates.emplace_back(newPatternState);
}


void initPatterns()
{
    logger.logMsg(LOG_INFO, "Initializing patterns...");

    //logger.logMsg(LOG_DEBUG, "configObject[\"patterns\"] has %ld elements", configObject["patterns"].array_items().size());
    //logger.logMsg(LOG_DEBUG, "configObject[\"patterns\"]:  " + configObject["patterns"].dump());
    for (auto& patternConfigObject : configObject["patterns"].array_items()) {
        if (patternConfigObject["patterns"].is_array()) {
            for (auto& nestedPatternConfigObject : patternConfigObject["patterns"].array_items()) {
                loadPatternConfig(nestedPatternConfigObject);
            }
        }
        else {
            loadPatternConfig(patternConfigObject);
        }
    }
}


void tearDownPatterns()
{
    logger.logMsg(LOG_INFO, "Tearing down patterns...");
    for (auto&& patternState : patternStates) {
        delete patternState->pattern;
        patternState->pattern = nullptr;
    }
    patternStates.clear();
}


bool timeIsInPeriod(
    time_t now,
    const vector<SchedulePeriod>& schedulePeriods,
    SchedulePeriod& selectedSchedulePeriod)
{
    // For daily events, we need a tm structure containing the current
    // time so that we can set a daily event's date to today.
    struct tm result1;
    struct tm tmNowTime = *localtime_r(&now, &result1);

    for (auto&& schedulePeriod : schedulePeriods) {
        if (schedulePeriod.isDaily) {
            // Modify the start and end times so that they occur today.
            struct tm result2;
            struct tm tmStartTimeToday = *localtime_r(&schedulePeriod.startTime, &result2);
            struct tm result3;
            struct tm tmEndTimeToday = *localtime_r(&schedulePeriod.endTime, &result3);
            tmStartTimeToday.tm_year = tmEndTimeToday.tm_year = tmNowTime.tm_year;
            tmStartTimeToday.tm_mon = tmEndTimeToday.tm_mon = tmNowTime.tm_mon;
            tmStartTimeToday.tm_mday = tmEndTimeToday.tm_mday = tmNowTime.tm_mday;
            tmStartTimeToday.tm_isdst = tmEndTimeToday.tm_isdst = tmNowTime.tm_isdst;
            time_t startTimeToday = mktime(&tmStartTimeToday);
            time_t endTimeToday = mktime(&tmEndTimeToday);
            //logger.logMsg(LOG_DEBUG, "desc=" + schedulePeriod.description + ", now=" + to_string(now) + ", startTime="
            //                  + to_string(startTimeToday) + ", endTime=" + to_string(endTimeToday));
            // Periods that span midnight have an end time that is numerically less
            // than the start time (which actually occurs on the previous day).
            if (endTimeToday < startTimeToday) {
                if (now >= startTimeToday || now <= endTimeToday) {
                    selectedSchedulePeriod = schedulePeriod;
                    return true;
                }
            }
            else {
                if (now >= startTimeToday && now <= endTimeToday) {
                    selectedSchedulePeriod = schedulePeriod;
                    return true;
                }
            }
        }
        else {
            // This is a one-time event.
            //logger.logMsg(LOG_DEBUG, "desc=" + schedulePeriod.description + ", now=" + to_string(now) + ", startTime="
            //                  + to_string(schedulePeriod.startTime) + ", endTime=" + to_string(schedulePeriod.endTime));
            if (now >= schedulePeriod.startTime && now <= schedulePeriod.endTime) {
                selectedSchedulePeriod = schedulePeriod;
                return true;
            }
        }
    }

    return false;
}


bool doInitialization()
{
    // If the config file is really a symbolic link,
    // add the link target to the file name we'll log.
    string configFileNameAndTarget = configFileName;
    char buf[512];
    int count = readlink(configFileName.c_str(), buf, sizeof(buf));
    if (count >= 0 && (unsigned int) count < sizeof(buf)) {
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
    logger.logMsg(LOG_INFO, "numberOfStrings = " + to_string(numberOfStrings));
    logger.logMsg(LOG_INFO, "numberOfPixelsPerString = " + to_string(numberOfPixelsPerString));
    logger.logMsg(LOG_INFO, "pattern blend method is " + patternBlendMethodStr);
    printSchedulePeriods("Shutoff periods", shutoffPeriods);
    printSchedulePeriods("Quiescent periods", quiescentPeriods);
    printSchedulePeriods("Override periods", overridePeriods);

    logger.setAutoLogRotation(logRotationIntervalMinutes, logRotationOffsetHour, logRotationOffsetMinute);

    dataLogger.startLogging(instanceName + "_data", Log::LogTo::file, logFilePath);
    dataLogger.setAutoLogRotation(logRotationIntervalMinutes, logRotationOffsetHour, logRotationOffsetMinute);

    if (!allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(hsvFinalFrame, numberOfStrings, numberOfPixelsPerString)) {
        logger.logMsg(LOG_ERR, "Unable to allocate pixels for hsvFinalFrame.");
        return false;
    }

    if (!allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(rgbFinalFrame, numberOfStrings, numberOfPixelsPerString)) {
        logger.logMsg(LOG_ERR, "Unable to allocate pixels for rgbFinalFrame.");
        return false;
    }

    if (!initOpcBuffer()) {
        return false;
    }

    // Open communications with OPC server.
    if (useTcpForOpcServer) {
        if (!openOpcServerTcpConnection()) {
            return false;
        }
    }
    else {
        if (!openUdpPortForOpcServer()) {
            return false;
        }
    }

    initWidgets();
    initPatterns();

    logger.logMsg(LOG_INFO, "Initialization done.  Start doing shit!");

    return true;
}


bool doTeardown()
{
    logger.logMsg(LOG_INFO, "Starting teardown.");

    tearDownPatterns();
    tearDownWidgets();

    if (useTcpForOpcServer) {
        if (!closeOpcServerTcpConnection()) {
            return false;
        }
    }
    else {
        if (!closeUdpPortForOpcServer()) {
            return false;
        }
    }

    freeOpcBuffer();

    freeConePixels<HsvConeStrings, HsvPixel>(hsvFinalFrame);
    freeConePixels<RgbConeStrings, RgbPixel>(rgbFinalFrame);

    dataLogger.stopLogging();

    logger.logMsg(LOG_INFO, "Teardown done.");

    return true;
}


void doPatterns()
{
    static bool doIdlePattern;
    static time_t timeWentInactive;

    // Let all the patterns update their shit.
    bool anyPatternIsActive = false;
    int minPriority = INT_MAX;
    int maxPriority = INT_MIN;
    for (auto&& patternState : patternStates) {
        //logger.logMsg(LOG_DEBUG, "calling update for " + patternState->pattern->getName());
        patternState->wantsDisplay = patternState->pattern->update();
        patternState->priority = patternState->pattern->priority;
        patternState->amountOfOverlay = (int) patternState->pattern->opacity * 255 / 100;
        anyPatternIsActive |= patternState->wantsDisplay;
        minPriority = min(patternState->priority, minPriority);
        maxPriority = max(patternState->priority, maxPriority);
        //logger.logMsg(LOG_DEBUG, patternState->pattern->getName() + ":  priority=" + to_string(patternState->priority)
        //                  + ", opacity=" + to_string(patternState->pattern->opacity)
        //                  + ", amountOfOverlay=" + to_string(patternState->amountOfOverlay));
    }

    clearAllPixels(rgbFinalFrame);
    clearAllPixels(hsvFinalFrame);

    if (anyPatternIsActive) {
        logOperatingState(OperatingState::active);

        bool anyPixelIsOn = false;

        // Layer the patterns into the final frame in reverse priority order
        // (i.e., lowest priority first, highest priority last).  Note that
        // a lower priority value denotes higher priority (0 is highest).
        for (int priority = maxPriority; priority >= minPriority; --priority) {
            for (auto&& patternState : patternStates) {
                if (patternState->wantsDisplay && patternState->priority == priority) {
                    //logger.logMsg(LOG_DEBUG, patternState->pattern->getName() + " wants display.");
                    for (unsigned int col = 0; col < numberOfStrings; col++) {
                        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {

                            // Ignore the pixel if it is transparent.
                            if (patternState->pattern->usesHsvModel) {
                                if (!patternState->pattern->coneStrings[col][row]) {
                                    continue;
                                }
                            }
                            else {
                                if (!patternState->pattern->pixelArray[col][row]) {
                                    continue;
                                }
                            }

                            // Convert the pixel to the required color model, if necessary.
                            RgbPixel rgbPatPixel;
                            HsvPixel hsvPatPixel;
                            switch (patternBlendMethod) {
                                case PatternBlendMethod::overlay:
                                case PatternBlendMethod::rgbAdd:
                                case PatternBlendMethod::rgbBlend:
                                    if (patternState->pattern->usesHsvModel) {
                                        hsv2rgb_rainbow(patternState->pattern->coneStrings[col][row], rgbPatPixel);
                                    }
                                    else {
                                        rgbPatPixel = patternState->pattern->pixelArray[col][row];
                                    }
                                    break;
                                case PatternBlendMethod::hsvBlend:
                                case PatternBlendMethod::hsvHueBlend:
                                    if (patternState->pattern->usesHsvModel) {
                                        hsvPatPixel = patternState->pattern->coneStrings[col][row];
                                    }
                                    else {
                                        rgb2hsv(patternState->pattern->pixelArray[col][row], hsvPatPixel);
                                    }
                                    break;
                            }

                            // Blend the pattern's pixel into the final frame if it isn't transparent.
                            anyPixelIsOn = true;
                            switch (patternBlendMethod) {
                                case PatternBlendMethod::overlay:
                                    // A higher priority pattern overrides a lower priority pattern.
                                    rgbFinalFrame[col][row] = rgbPatPixel;
                                    break;
                                case PatternBlendMethod::rgbAdd:
                                    // Saturating addition ignores priority, but it looks better than nblend of rgb.
                                    rgbFinalFrame[col][row] += rgbPatPixel;
                                    break;
                                case PatternBlendMethod::rgbBlend:
                                    // Blending rgb tends to make things look dark when combined.
                                    if (rgbFinalFrame[col][row]) {
                                        nblend(rgbFinalFrame[col][row], rgbPatPixel, patternState->amountOfOverlay);
                                    }
                                    else {
                                        rgbFinalFrame[col][row] = rgbPatPixel;
                                    }
                                    break;
                                case PatternBlendMethod::hsvBlend:
                                case PatternBlendMethod::hsvHueBlend:
                                    HsvPixel ffPixel = hsvFinalFrame[col][row];
                                    if (hsvFinalFrame[col][row]) {
                                        nblend(hsvFinalFrame[col][row], hsvPatPixel, patternState->amountOfOverlay);
                                        if (patternBlendMethod == PatternBlendMethod::hsvHueBlend) {
                                            hsvFinalFrame[col][row].s = max(ffPixel.s, hsvPatPixel.s);
                                            hsvFinalFrame[col][row].v = max(ffPixel.v, hsvPatPixel.v);
                                        }
                                    }
                                    else {
                                        hsvFinalFrame[col][row] = hsvPatPixel;
                                    }
                                    break;
                            }

                        }
                    }
                }
            }
        }

        if (anyPixelIsOn) {
            //logger.logMsg(LOG_DEBUG, "a pixel is on");
            if (patternBlendMethod == PatternBlendMethod::hsvBlend || patternBlendMethod == PatternBlendMethod::hsvHueBlend) {
                hsv2rgb(hsvFinalFrame, rgbFinalFrame);
            }
            sendOpcMessage();
        }
        else {
            // Don't allow the cone to go completely dark.
            turnOnSafetyLights();
        }
        doIdlePattern = false;
        timeWentInactive = 0;
    }
    else {
        if (!doIdlePattern) {
            logOperatingState(OperatingState::inactive);
            // Turn on the safety lights until the idle pattern takes over.
            turnOnSafetyLights();
            if (timeWentInactive == 0) {
                time(&timeWentInactive);
            }
            else {
                time_t now;
                time(&now);
                // TODO 7/22/2017 ross:  replace the magic number 3 with the idle timeout
                if (now - timeWentInactive > 3) {
                    logOperatingState(OperatingState::idle);
                    doIdlePattern = true;
                }
            }
        }
        // When doIdlePattern is true, we stop sending messages to the OPC server.
        // The server will display its own rainbow-like pattern when it hasn't
        // received a message for a few seconds.
    }
}


void logOperatingState(OperatingState currentOperatingState, const string& msg)
{
    static OperatingState lastOperatingState = OperatingState::startup;

    if (currentOperatingState != lastOperatingState) {
        lastOperatingState = currentOperatingState;
        switch (currentOperatingState) {
            case OperatingState::startup:
                break;
            case OperatingState::initializing:
                logger.logMsg(LOG_INFO, "Initializing.  " + msg);
                break;
            case OperatingState::testPattern:
                logger.logMsg(LOG_INFO, "Displaying test pattern.");
                break;
            case OperatingState::active:
                logger.logMsg(LOG_INFO, "Going active.");
                break;
            case OperatingState::inactive:
                logger.logMsg(LOG_INFO, "Going inactive.");
                break;
            case OperatingState::idle:
                logger.logMsg(LOG_INFO, "Going idle.");
                break;
            case OperatingState::quiescent:
                logger.logMsg(LOG_INFO, "Starting \"" + msg + "\" quiescent period.");
                break;
            case OperatingState::shutoff:
                logger.logMsg(LOG_INFO, "Starting \"" + msg + "\" shutoff period.");
                break;
            case OperatingState::shutdown:
                logger.logMsg(LOG_INFO, "Shutting down.  " + msg);
                break;
        }
    }
}


int main(int argc, char **argv)
{
    getCommandLineOptions(argc, argv);

    // We can start logging to the system log before daemonizing.
    if (logTo == Log::LogTo::systemLog) {
        logger.startLogging(instanceName, Log::LogTo::systemLog);
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
        logger.startLogging(instanceName, Log::LogTo::console);
    }
    else if (logTo == Log::LogTo::file) {
        logger.startLogging(instanceName, Log::LogTo::file, logFilePath);
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

    logOperatingState(OperatingState::initializing, "---------- patternController starting ----------");

    if (!doInitialization()) {
        return(EXIT_FAILURE);
    }
    bool displayingTestPattern = false;
    time_t lastPeriodCheckTime = 0;
    SchedulePeriod selectedSchedulePeriod = noPeriod;
    SchedulePeriod lastSelectedSchedulePeriod = noPeriod;

/*
//=-=-=-=-=-=-=-=-=
    RgbPixel rgb(255, 255, 0);
    //RgbPixel rgb(196, 138, 0);
    cout << "r=" << (int) rgb.r << " g=" << (int) rgb.g << " b=" << (int) rgb.b << endl;
    HsvPixel hsv;
    rgb2hsv(rgb, hsv);
    cout << "h=" << (int) hsv.h << " s=" << (int) hsv.s << " v=" << (int) hsv.v << endl;
    hsv2rgb_rainbow(hsv, rgb);
    cout << "r=" << (int) rgb.r << " g=" << (int) rgb.g << " b=" << (int) rgb.b << endl;
    return EXIT_SUCCESS;
//=-=-=-=-=-=-=-=-=
*/

    // ----- run loop -----
    bool inOverridePeriod = false;
    while (!gotExitSignal) {

        usleep(patternRunLoopSleepIntervalUs);

        if (gotReinitSignal) {
            gotReinitSignal = false;
            logOperatingState(OperatingState::initializing, "---------- Reinitializing... ----------");
            turnOnSafetyLights();
            if (!doTeardown()) {
                logOperatingState(OperatingState::shutdown, "Exiting due to error during teardown.");
                return(EXIT_FAILURE);
            }
            // Sleep for a little bit because reconnecting to
            // the OPC server immediately sometimes fails.
            logger.logMsg(LOG_INFO, "Sleeping for " + to_string(reinitializationSleepIntervalS) + " seconds.");
            sleep(reinitializationSleepIntervalS);
            if (!readConfig() || !doInitialization()) {
                logOperatingState(OperatingState::shutdown, "Exiting due to error during reinitialization.");
                return(EXIT_FAILURE);
            }
            displayingTestPattern = false;
            lastPeriodCheckTime = 0;
            selectedSchedulePeriod = noPeriod;
            lastSelectedSchedulePeriod = noPeriod;
        }

        if (gotToggleTestPatternSignal) {
            gotToggleTestPatternSignal = false;
            displayingTestPattern = !displayingTestPattern;
            // TODO:  add an operation state for test pattern
            logger.logMsg(LOG_INFO, string("Turning test pattern ") + (displayingTestPattern ? "on." : "off."));
            if (!displayingTestPattern) {
                // Turn on the safety lights as an indication
                // that we're no longer in test-pattern mode.
                turnOnSafetyLights();
            }
        }

        // Give the widgets a chance to update their simulated measurements.
        //logger.logMsg(LOG_DEBUG, "Updating simulated measurements.");
        for (auto&& widget : widgets) {
            widget.second->updateSimulatedMeasurements();
        }

        if (displayingTestPattern) {
            logOperatingState(OperatingState::testPattern);
            displayTestPattern();
            continue;
        }

        // Once per second, check if we're in a schedule period.
        time_t now;
        time(&now);
        if (now > lastPeriodCheckTime) {
            lastPeriodCheckTime = now;

            // An override period overrides both shutoff and quiescent periods,
            // forcing the cone to be on.  Basically, it is an exception to
            // the regular schedule.
            inOverridePeriod = false;
            if (timeIsInPeriod(now, overridePeriods, selectedSchedulePeriod)) {
                inOverridePeriod = true;
            }
            else if (timeIsInPeriod(now, shutoffPeriods, selectedSchedulePeriod)) {
                logOperatingState(OperatingState::shutoff, selectedSchedulePeriod.description);
                turnOffAllPixels();
            }
            else if (timeIsInPeriod(now, quiescentPeriods, selectedSchedulePeriod)) {
                logOperatingState(OperatingState::quiescent, selectedSchedulePeriod.description);
                setAllPixelsToQuiescentColor(selectedSchedulePeriod);
            }
            else {
                selectedSchedulePeriod = noPeriod;
            }

            if (selectedSchedulePeriod != lastSelectedSchedulePeriod) {
                if (lastSelectedSchedulePeriod != noPeriod) {
                    logger.logMsg(LOG_INFO, string("Leaving ") + lastSelectedSchedulePeriod.description + string(" period."));
                }
                if (selectedSchedulePeriod != noPeriod) {
                    logger.logMsg(LOG_INFO, string("Entering ") + selectedSchedulePeriod.description + string(" period."));
                }
                lastSelectedSchedulePeriod = selectedSchedulePeriod;
            }
        }

        if (selectedSchedulePeriod == noPeriod || inOverridePeriod) {
            doPatterns();
        }
    }

    logOperatingState(OperatingState::shutdown, "---------- Exiting... ----------");
    turnOnSafetyLights();
    if (!doTeardown()) {
        return(EXIT_FAILURE);
    }
    logger.stopLogging();
    return EXIT_SUCCESS;
}

