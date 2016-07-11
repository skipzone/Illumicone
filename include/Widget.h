#pragma once

#include <memory>
#include <string>
#include <vector>

#include <sys/socket.h>	// for UDP
#include <netinet/in.h>	// for UDP

#include "WidgetId.h"

class WidgetChannel;


class Widget
{
    public:
        ///Widget(unsigned int id, const std::string& name);

        Widget(WidgetId id, std::string name);
        virtual ~Widget();

        Widget() = delete;
        Widget(const Widget&) = delete;
        Widget& operator =(const Widget&) = delete;

        WidgetId getId();
        std::string getName();

        virtual void init(bool generateSimulatedMeasurements = true) = 0;

        virtual bool moveData() = 0;

        virtual unsigned int getChannelCount();
        virtual std::shared_ptr<WidgetChannel> getChannel(unsigned int channelIdx);
        virtual std::vector<std::shared_ptr<WidgetChannel>> getChannels();

        virtual bool getIsActive();
        virtual bool getHasNewMeasurement();

        static void* udpRxThreadEntry(void* widgetObj);

    protected:

        void startUdpRxThread();

        const WidgetId id;
        const std::string name;
        std::vector<std::shared_ptr<WidgetChannel>> channels;
        bool generateSimulatedMeasurements;

    private:

        void pollForUdpRx();

        pthread_t udpRxThread;
        int sockfd;
        struct sockaddr_in servaddr;
};

