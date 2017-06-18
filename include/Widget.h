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

        Widget(WidgetId id, unsigned int numChannels);
        virtual ~Widget();

        Widget() = delete;
        Widget(const Widget&) = delete;
        Widget& operator =(const Widget&) = delete;

        WidgetId getId();
        std::string getName();

        virtual bool init(ConfigReader& config);

        virtual bool moveData() = 0;

        virtual std::shared_ptr<WidgetChannel> getChannel(unsigned int channelIdx);
        virtual std::vector<std::shared_ptr<WidgetChannel>> getChannels();

        virtual bool getIsActive();
        virtual bool getHasNewMeasurement();

        static void* udpRxThreadEntry(void* widgetObj);

    protected:

        void startUdpRxThread();

        const WidgetId id;
        std::vector<std::shared_ptr<WidgetChannel>> channels;
        bool generateSimulatedMeasurements;
        unsigned int autoInactiveMs;
        unsigned int numChannels;

    private:

        void pollForUdpRx();

        pthread_t udpRxThread;
        int sockfd;
        struct sockaddr_in servaddr;
};

