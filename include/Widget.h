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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <sys/socket.h>	// for UDP
#include <netinet/in.h>	// for UDP

#include "WidgetId.h"


class ConfigReader;
class WidgetChannel;


class Widget
{
    public:

        Widget(WidgetId id, unsigned int numChannels, bool useQueuedChannels = false);
        virtual ~Widget();

        Widget() = delete;
        Widget(const Widget&) = delete;
        Widget& operator =(const Widget&) = delete;

        WidgetId getId();

        virtual bool init(ConfigReader& config);

        virtual std::shared_ptr<WidgetChannel> getChannel(unsigned int channelIdx);
        virtual std::vector<std::shared_ptr<WidgetChannel>> getChannels();

        static void* udpRxThreadEntry(void* widgetObj);

        void updateSimulatedMeasurements();
        virtual void updateChannelSimulatedMeasurements(unsigned int chIdx) {};


    protected:

        void startUdpRxThread();

        const WidgetId id;
        std::vector<std::shared_ptr<WidgetChannel>> channels;
        unsigned int autoInactiveMs;
        unsigned int numChannels;
        bool generateSimulatedMeasurements;
        // TODO 8/7/2017 ross:  These need to be sized dynamically to agree with the number of channels.
        unsigned int simulationNextUpdateMs[8];
        unsigned int simulationUpdateIntervalMs[8];

    private:

        void pollForUdpRx();

        pthread_t udpRxThread;
        int sockfd;
        struct sockaddr_in servaddr;
        bool stopUdpRxPolling;

        bool useQueuedChannels;
};

