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
#include "illumiconeTypes.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


Widget::Widget(WidgetId id, unsigned int numChannels)
    : id(id)
    , numChannels(numChannels)
{
}


Widget::~Widget()
{
    pthread_join(udpRxThread, NULL); 	// close the thread
    close(sockfd); 					// close UDP socket
}


bool Widget::init(ConfigReader& config)
{
    generateSimulatedMeasurements = config.getWidgetGenerateSimulatedMeasurements(id);
    autoInactiveMs = config.getWidgetAutoInactiveMs(id);

    if (autoInactiveMs != 0) {
        cout << "autoInactiveMs=" << autoInactiveMs << " for " << widgetIdToString(id) << endl;
    }

    for (int i = 0; i < numChannels; ++i) {
        channels.push_back(make_shared<WidgetChannel>(i, this, autoInactiveMs));
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


//bool Widget::getIsActive()
//{
//    for (auto&& channel : channels) {
//        if (channel->getIsActive()) {
//            return true;
//        }
//    }
//    return false;
//}


//bool Widget::getHasNewPositionMeasurement()
//{
//    for (auto&& channel : channels) {
//        if (channel->getHasNewPositionMeasurement()) {
//            return true;
//        }
//    }
//    return false;
//}


//bool Widget::getHasNewVelocityMeasurement()
//{
//    for (auto&& channel : channels) {
//        if (channel->getHasNewVelocityMeasurement()) {
//            return true;
//        }
//    }
//    return false;
//}


void Widget::startUdpRxThread()
{
    // TODO 6/12/2017 ross:  Get this value from config when calls to widget init are moved to PatternController.
    constexpr static unsigned int widgetPortNumberBase = 4200;

	// TODO 7/10/2016 ross:  determine if we really need to do this
    //pthread_t thisThread = pthread_self();
	//pthread_setschedprio(thisThread, SCHED_FIFO);

	// udp initialization
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
    memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(widgetPortNumberBase + widgetIdToInt(id));
	bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    if (pthread_create(&udpRxThread, NULL, udpRxThreadEntry, this)) {
        std::cerr << "pthread_create failed in Widget::startUdpRxThread" << std::endl;
    }
}


void Widget::pollForUdpRx()
{
    socklen_t len;
    struct sockaddr_in cliaddr;
    UdpPayload payload;

	while (true)
	{
		len = sizeof(cliaddr);
		ssize_t rxByteCount = recvfrom(sockfd,
                                       &payload,
                                       sizeof(payload),
                                       0,
                                       (struct sockaddr *) &cliaddr,
                                       &len);

        std::cout << "got UDP payload; "
            << " length = " << rxByteCount
            << ", id = " << (int) payload.id
            << ", channel = " << (int) payload.channel
            << ", isActive = " << (int) payload.isActive
            << ", position = " << payload.position
            << ", velocity = " << payload.velocity
            << std::endl;

        if (payload.channel < channels.size()) {
            channels[payload.channel]->setPositionAndVelocity(payload.position, payload.velocity);
            channels[payload.channel]->setIsActive(payload.isActive);
        }
        else {
            std::cerr << "pollForUdpRx:  invalid channel " << (int) payload.channel
                << " received for widget id " << (int) payload.id
                << std::endl;
        }
	}

};


void* Widget::udpRxThreadEntry(void* widgetObj)
{
	((Widget *) widgetObj)->pollForUdpRx();
    return nullptr;
}

