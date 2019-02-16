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

#include <iostream>
#include <memory>

#include <string.h>
#include <sys/socket.h>	// for UDP
#include <netinet/in.h>	// for UDP
#include <unistd.h>

#include "ConfigReader.h"
#include "illumiconeWidgetTypes.h"
#include "illumiconeUtility.h"
#include "Log.h"
#include "QueuedWidgetChannel.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


Widget::Widget(WidgetId id, unsigned int numChannels, bool useQueuedChannels)
    : id(id)
    , numChannels(numChannels)
    , stopUdpRxPolling(false)
    , useQueuedChannels(useQueuedChannels)
{
    unsigned int nowMs = getNowMs();

    for (unsigned int i = 0; i < 8; ++i) {
        simulationUpdateIntervalMs[i] = 0;      // disabled for channel
        simulationNextUpdateMs[i] = nowMs;      // start doing updates immediately if enabled
    }
}


Widget::~Widget()
{
    if (!generateSimulatedMeasurements) {

        stopUdpRxPolling = true;

        // We need to close the UDP socket before waiting for the rx thread to
        // terminate because the recvfrom call blocks until a message is
        // received or an error occurs.  Closing the socket will cause a file
        // descriptor error, thus allowing the thread to figure out it needs to
        // terminate.
        logger.logMsg(LOG_DEBUG, "closing UDP socket for " + widgetIdToString(id));
        close(sockfd); 					            // close the UDP socket

        logger.logMsg(LOG_DEBUG, "Waiting for UDP rx thread termination for " + widgetIdToString(id));
        pthread_cancel(udpRxThread); 	            // kill the rx thread
        pthread_join(udpRxThread, NULL); 	        // wait for the rx thread to terminate
    }
}


bool Widget::init(const json11::Json& widgetConfigObject, const json11::Json& topLevelConfigObject)
{
    string logMsgSuffix = " for widget " + widgetIdToString(id);

    if (!ConfigReader::getUnsignedIntValue(topLevelConfigObject, "widgetPortNumberBase", widgetPortNumberBase,
                                           logMsgSuffix, 1024, 65535))
    {
        return false;
    }

    generateSimulatedMeasurements = false;
    ConfigReader::getBoolValue(widgetConfigObject, "generateSimulatedMeasurements", generateSimulatedMeasurements);
    if (generateSimulatedMeasurements) {
        logger.logMsg(LOG_INFO, "simulated measurements will be generated" + logMsgSuffix);
    }
    
    // If autoInactiveMs isn't present, the value returned will
    // be zero, which disables the auto-inactive feature.
    autoInactiveMs = 0;
    ConfigReader::getUnsignedIntValue(widgetConfigObject, "autoInactiveMs", autoInactiveMs);
    if (autoInactiveMs != 0) {
        logger.logMsg(LOG_INFO, "autoInactiveMs=" + to_string(autoInactiveMs) + logMsgSuffix);
    }

    for (unsigned int i = 0; i < numChannels; ++i) {
        if (useQueuedChannels) {
            channels.push_back(make_shared<QueuedWidgetChannel>(i, this, autoInactiveMs));
        }
        else {
            channels.push_back(make_shared<WidgetChannel>(i, this, autoInactiveMs));
        }
    }

    if (!generateSimulatedMeasurements) {
        startUdpRxThread();
    }

    return true;
}


WidgetId Widget::getId()
{
    return id;
}


std::shared_ptr<WidgetChannel> Widget::getChannel(unsigned int channelIdx)
{
    return channelIdx < channels.size() ? channels[channelIdx] : nullptr;
}


std::vector<std::shared_ptr<WidgetChannel>> Widget::getChannels()
{
    return channels;
}


void Widget::startUdpRxThread()
{
    widgetPortNumber = widgetPortNumberBase + widgetIdToInt(id);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        logger.logMsg(LOG_ERR, errno, "Failed to create socket in Widget::startUdpRxThread for " + widgetIdToString(id));
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(widgetPortNumber);

    if (::bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) {
        logger.logMsg(LOG_ERR, errno, "Failed to bind socket in Widget::startUdpRxThread for " + widgetIdToString(id));
        return;
    }

    if (pthread_create(&udpRxThread, NULL, udpRxThreadEntry, this)) {
        logger.logMsg(LOG_ERR, "pthread_create failed in Widget::startUdpRxThread for " + widgetIdToString(id));
    }
    else {
        logger.logMsg(LOG_INFO, "Listening on port " + to_string(widgetPortNumber)
        + " (" + to_string(servaddr.sin_port) + ") for " + widgetIdToString(id));
    }
}


void Widget::pollForUdpRx()
{
    socklen_t len;
    struct sockaddr_in cliaddr;
    UdpPayload payload;

	while (!stopUdpRxPolling)
	{
		len = sizeof(cliaddr);
		ssize_t rxByteCount = recvfrom(sockfd,
                                       &payload,
                                       sizeof(payload),
                                       0,
                                       (struct sockaddr *) &cliaddr,
                                       &len);

        if (rxByteCount < 0) {
            logger.logMsg(LOG_ERR, errno, "Error receiving UDP message for " + widgetIdToString(id) + ".");
            continue;
        }

        logger.logMsg(LOG_INFO, 
            "payload: len=" + to_string(rxByteCount)
            + " id=" + to_string((int) payload.id)
            + " ch=" + to_string((int) payload.channel)
            + " a=" + to_string((int) payload.isActive)
            + " p=" + to_string(payload.position)
            + " v=" + to_string(payload.velocity));

        if (payload.channel < channels.size()) {
            channels[payload.channel]->setPositionAndVelocity(payload.position, payload.velocity);
            channels[payload.channel]->setIsActive(payload.isActive);
        }
        else {
            logger.logMsg(LOG_ERR, "pollForUdpRx:  invalid channel " + to_string((int) payload.channel)
                + " received for widget id " + to_string((int) payload.id));
        }
	}

    logger.logMsg(LOG_INFO, "UDP message polling for " + widgetIdToString(id) + " stopped.");
};


void* Widget::udpRxThreadEntry(void* widgetObj)
{
	((Widget *) widgetObj)->pollForUdpRx();
    return nullptr;
}


void Widget::updateSimulatedMeasurements()
{
    if (!generateSimulatedMeasurements) {
        return;
    }

    unsigned int nowMs = getNowMs();

    for (unsigned int i = 0; i < numChannels; ++i) {
        if (simulationUpdateIntervalMs[i] > 0 && (int) (nowMs - simulationNextUpdateMs[i]) >= 0) {
            ///simulationNextUpdateMs[i] = nowMs + simulationUpdateIntervalMs[i];
            simulationNextUpdateMs[i] += simulationUpdateIntervalMs[i];
            updateChannelSimulatedMeasurements(i);
        }
    }
}

