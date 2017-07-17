#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>


#include <arpa/inet.h>
///#include <netdb.h>
#include <netinet/in.h>
#include <RF24/RF24.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "illumiconeTypes.h"
#include "WidgetId.h"


using namespace std;


static struct sockaddr_in widgetSockAddr[16];
static int widgetSock[16];

constexpr char lockFilePath[] = "/tmp/widgetRcvr.lock";

// TODO 7/9/2017 ross:  Get this from the JSON config.
//static const string patconIpAddress = "192.168.69.101";
static const string patconIpAddress = "192.168.69.12";  // can't get the goddamn static ip address to work on ic-patcon



/***********************
 * Radio Configuration *
 ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939
// and the related enumerations for pin information.  (That's some pretty useful shit right there, Maynard.)
// Raspberry Pi B+:  CE connected to GPIO25 on J8-22, CSN connected to CE0
RF24 radio(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// We're using dynamic payload size, but we still need to know what the largest can be.
constexpr uint8_t maxPayloadSize = 32;

constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
constexpr uint8_t rfChannel = 84;

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
constexpr rf24_pa_dbm_e rfPowerLevel = RF24_PA_MAX;

// RF24_250KBPS or RF24_1MBPS
constexpr rf24_datarate_e dataRate = RF24_250KBPS;

constexpr uint8_t txRetryDelayMultiplier = 15;  // 250 us additional delay multiplier (0-15)
constexpr uint8_t txMaxRetries = 15;            // max retries (0-15)

// RF24_CRC_DISABLED, RF24_CRC_8, or RF24_CRC_16
constexpr rf24_crclength_e crcLength = RF24_CRC_16;


/***********
 * Helpers *
 ***********/

const string getTimestamp()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    int ms = epochMs.count() % 1000;
    time_t now = epochMs.count() / 1000;

    struct tm tmStruct = *localtime(&now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);

    stringstream sstr;
    sstr << buf << "." << setfill('0') << setw(3) << ms << ":  ";

    string str = sstr.str();
    return str;
}


/*********************
 * UDP Communication *
 *********************/

bool openUdpPort(WidgetId widgetId)
{
    // TODO 6/12/2017 ross:  Get this value from config.
    constexpr static unsigned int widgetPortNumberBase = 4200;

    unsigned int widgetIdNumber = widgetIdToInt(widgetId);
    unsigned int portNumber = widgetPortNumberBase + widgetIdNumber;

    cout << "Creating and binding socket for widget " << widgetIdNumber << endl;

    memset(&widgetSockAddr[widgetIdNumber], 0, sizeof(struct sockaddr_in));

    widgetSockAddr[widgetIdNumber].sin_family = AF_INET;
    widgetSockAddr[widgetIdNumber].sin_addr.s_addr = htonl(INADDR_ANY);
    widgetSockAddr[widgetIdNumber].sin_port = htons(0);

    if ((widgetSock[widgetIdNumber] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create socket");
        return false;
    }

    if (::bind(widgetSock[widgetIdNumber], (struct sockaddr *) &widgetSockAddr[widgetIdNumber], sizeof(struct sockaddr_in)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    cout << "Setting address to " << patconIpAddress << ":" << portNumber << endl;

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
        cerr << getTimestamp()
            << "UPD payload size is " << sizeof(payload)
            << " but " << bytesSentCount
            << " bytes were sent."
            << endl;
        return false;
    }
    //cout << "Sent " << bytesSentCount << " byte payload via UDP" << endl;

    return true;
}


/********************
 * Payload Handlers *
 ********************/

void handleStressTestPayload(const StressTestPayload* payload, unsigned int payloadSize)
{
    if (payloadSize != sizeof(StressTestPayload)) {
        cerr << getTimestamp()
            << "Got StressTestPayload payload with size " << payloadSize
            << " but size " << sizeof(StressTestPayload)
            << " was expected." << endl;
        return;
    }

    cout << getTimestamp() << "Got stress test payload;"
        << " Id = " << (int) payload->widgetHeader.id
        << ", " << string(payload->widgetHeader.isActive ? "active  " : "inactive")
        << ", ch = " << (int) payload->widgetHeader.channel
        << ", seq = " << payload->payloadNum
        << ", fails = " << payload->numTxFailures
        << " (" << payload->numTxFailures * 100 / payload->payloadNum << "% )"
        << endl;

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
        cerr << getTimestamp()
            << "Got PositionVelocityPayload payload with size " << payloadSize
            << " but size " << sizeof(PositionVelocityPayload)
            << " was expected." << endl;
        return;
    }

    cout << getTimestamp() << "Got position+velocity payload;"
        << " Id = " << (int) payload->widgetHeader.id
        << ", " << string(payload->widgetHeader.isActive ? "active  " : "inactive")
        << ", ch = " << (int) payload->widgetHeader.channel
        << ", position = " << payload->position
        << ", velocity = " << payload->velocity
        << endl;

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
        cerr << getTimestamp()
            << "Got MeasurementVectorPayload without any data." << endl;
        return;
    }

    int numMeasurements = (payloadSize - 1) / sizeof(int16_t);

    cout << getTimestamp() << "Got measurement vector payload;"
        << " Id = " << (int) payload->widgetHeader.id
        << ", " << string(payload->widgetHeader.isActive ? "active  " : "inactive")
        << ", ch = " << (int) payload->widgetHeader.channel
        << ", numMeasurements = " << numMeasurements
        << endl;

    cout << "Measurements:" << endl;
    for (int i = 0; i < numMeasurements; ++i) {
        cout << setfill(' ') << setw(6) << payload->measurements[i] << endl;
    }

    // The rainstick widget sends 7 position measurements.  We'll map them to
    // channels 0 through 6.
    if (payload->widgetHeader.id == widgetIdToInt(WidgetId::rainstick)) {
        for (unsigned int i = 0; i < 7; ++i) {
            UdpPayload udpPayload;
            udpPayload.id       = payload->widgetHeader.id;
            udpPayload.channel  = i;
            udpPayload.isActive = payload->widgetHeader.isActive;
            udpPayload.position = payload->measurements[i];
            udpPayload.velocity = 0;
            sendUdp(udpPayload);
        }
    }
}


void handleCustomPayload(const CustomPayload* payload, unsigned int payloadSize)
{
    if (payloadSize < 2) {
        cerr << getTimestamp()
            << "Got CustomPayload without any data." << endl;
        return;
    }

    int bufLen = payloadSize - 1;

    cout << getTimestamp() << "Got custom payload;"
        << " Id = " << (int) payload->widgetHeader.id
        << ", " << string(payload->widgetHeader.isActive ? "active  " : "inactive")
        << ", ch = " << (int) payload->widgetHeader.channel
        << ", bufLen = " << bufLen
        << endl;

    cout << "Contents:" << endl;
    for (int i = 0; i < bufLen; ++i) {
        cout << hex << (int) payload->buf[i] << " ";
    }
    cout << endl;
}


/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

int acquireProcessLock()
{
    int fd = open(lockFilePath, O_CREAT);
    if (fd >= 0) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            return fd;
        }
        else {
            if (errno == EWOULDBLOCK) {
                // Another process has the file locked.
                return -1;
            }
            else {
                fprintf(stderr, "Unable to lock %s.  Error %d:  %s\n", lockFilePath, errno, strerror(errno));
                return -1;
            }
        }
    }
    else {
        fprintf(stderr, "Unable to create or open %s.  Error %d:  %s\n", lockFilePath, errno, strerror(errno));
        return -1;
    }
}


void configureRadio()
{
    radio.begin();

    radio.setPALevel(rfPowerLevel);
    radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
    radio.setDataRate(dataRate);
    radio.setChannel(rfChannel);
    radio.setAutoAck(true);
    radio.enableDynamicPayloads();
    radio.setCRCLength(crcLength);

    for (uint8_t i = 0; i < numReadPipes; ++i) {
        radio.openReadingPipe(i, readPipeAddresses[i]);
    }

    radio.printDetails();
}


void runLoop()
{

    while (1) {

        uint8_t pipeNum;
        while(radio.available(&pipeNum)) {

            unsigned int payloadSize = radio.getDynamicPayloadSize();
            if (payloadSize == 0) {
                cerr << getTimestamp() << "Got invalid packet (payloadSize = 0)." << endl;
                continue;
            }
            if (payloadSize > maxPayloadSize) {
                cerr << getTimestamp() << "Got unsupported payload size " << payloadSize << endl;
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
                    cerr << getTimestamp()
                        << "Got payload with size " << payloadSize
                        << " via unsupported pipe " << (int) pipeNum
                        << ".  Contents:" << endl;
                    for (int i = 0; i < maxPayloadSize; ++i) {
                        cerr << hex << (int) payload[i] << " ";
                    }
                    cerr << endl;
                    break;

                case 5:
                    handleCustomPayload((CustomPayload*) payload, payloadSize);
                    break;

                default:
                    cerr << getTimestamp()
                        << "pipeNum is " << (int) pipeNum 
                        << ", which should never happen!  Payload contents:" << endl;
                    for (int i = 0; i < maxPayloadSize; ++i) {
                        cerr << hex << (int) payload[i] << " ";
                    }
                    cerr << endl;
            }
        }

        // There are no payloads to process, so give other threads a chance to run.
        this_thread::yield();
    }
}


int main(int argc, char** argv)
{
    if (acquireProcessLock() < 0) {
        exit(EXIT_FAILURE);
    }
    cout << getTimestamp() << "---------- widgetRcvr starting ----------" << endl;

    openUdpPort(WidgetId::eye);
    openUdpPort(WidgetId::shirleysWeb);
    openUdpPort(WidgetId::bells);
    openUdpPort(WidgetId::rainstick);
    openUdpPort(WidgetId::schroedersPlaything);
    openUdpPort(WidgetId::triObelisk);
    openUdpPort(WidgetId::squawkBox);
    openUdpPort(WidgetId::plunger);
    openUdpPort(WidgetId::contortOMatic);
    openUdpPort(WidgetId::fourPlay42);
    openUdpPort(WidgetId::fourPlay43);
    openUdpPort(WidgetId::buckNorris);

    configureRadio();

    radio.startListening();

    runLoop();
    
    return 0;
}

