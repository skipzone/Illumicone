#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

#include <RF24/RF24.h>
#include <stdio.h>
#include <time.h>

using namespace std;


/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Ray's Eye
 *  2:  Reiley's Hypnotyzer (the bike wheel)
 *  3:  Ray's Bells
 *  4:  Kelli's Steps
 *  5:  Naked's Rain Stick
 *  6:  Phyxx's Obelisk (the triple rotary thing)
 *  7:  Cowboy's Box Theramin
 *  8:  Kayla's Plunger
 *  9:  unassigned
 * 10:  unassigned
 * 11:  unassigned
 * 12:  unassigned
 * 13:  unassigned
 * 14:  unassigned
 * 15:  unassigned
 *
 * For stress tests, widget ids are reused as needed because stress-test
 * payloads are handled separately from all other types of payloads.
 */


/************
 * Payloads *
 ************/

#pragma pack(push)
#pragma pack(1)

union WidgetHeader {
  struct {
    uint8_t id       : 4;
    bool    isActive : 1;
    uint8_t channel  : 3;
  };
  uint8_t raw;
};

// pipe 0
struct StressTestPayload {
    WidgetHeader widgetHeader;
    uint32_t     payloadNum;
    uint32_t     numTxFailures;
};

// pipe 1
struct PositionVelocityPayload {
    WidgetHeader widgetHeader;
    int16_t      position;
    int16_t      velocity;
};

// pipe 2
struct MeasurementVectorPayload {
    WidgetHeader widgetHeader;
    int16_t      measurements[15];
};

// pipe 5
struct CustomPayload {
    WidgetHeader widgetHeader;
    uint8_t      buf[31];
};

#pragma pack(pop)


/***********************
 * Radio Configuration *
 ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939
// and the related enumerations for pin information.  (That's some pretty useful shit right there, Maynard.)
// Raspberry Pi B+:  CE connected to GPIO25 on J8-22, CSN connected to CE0
RF24 radio(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// We're using dynamic payload size, but we still need to know what the largest can be.
constexpr const uint8_t maxPayloadSize = 32;

constexpr const uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr const int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

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
        cerr << hex << (int) payload->buf[i] << " ";
    }
    cerr << endl;
}


/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

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
    configureRadio();

    radio.startListening();

    runLoop();
    
    return 0;
}

