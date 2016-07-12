#include <iostream>
#include <memory>

#include <string.h>
#include <sys/socket.h>	// for UDP
#include <netinet/in.h>	// for UDP
#include <unistd.h>

#include "illumiconeTypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "WidgetId.h"

Widget::Widget(WidgetId id, std::string name)
    : id(id)
    , name(name)
{
}

Widget::~Widget()
{
    pthread_join(udpRxThread, NULL); 	// close the thread
    close(sockfd); 					// close UDP socket
}


WidgetId Widget::getId()
{
    return id;
}


std::string Widget::getName()
{
    return name;
}


unsigned int Widget::getChannelCount()
{
    return channels.size();
}


std::shared_ptr<WidgetChannel> Widget::getChannel(unsigned int channelIdx)
{
    return (channelIdx <= channels.size()) ? channels[channelIdx] : nullptr;
}


std::vector<std::shared_ptr<WidgetChannel>> Widget::getChannels()
{
    return channels;
}


bool Widget::getIsActive()
{
    for (auto&& channel : channels) {
        if (channel->getIsActive()) {
            return true;
        }
    }
    return false;
}


bool Widget::getHasNewMeasurement()
{
    for (auto&& channel : channels) {
        if (channel->getHasNewMeasurement()) {
            return true;
        }
    }
    return false;
}


void Widget::startUdpRxThread()
{
	// TODO 7/10/2016 ross:  determine if we really need to do this
    pthread_t thisThread = pthread_self();
	pthread_setschedprio(thisThread, SCHED_FIFO);

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

