#pragma once

#include <memory>
#include <vector>

class WidgetChannel;

class Widget
{
    public:
        Widget(unsigned int id, const std::string& name);

        Widget() {}
        virtual ~Widget() {}

        Widget(const Widget&) = delete;
        Widget& operator =(const Widget&) = delete;

        virtual void init() = 0;

        //
        // use this to read data from widget and store it
        //
        virtual bool moveData() = 0;

        virtual unsigned int getId() = 0;
        virtual std::string getName() = 0;

        virtual unsigned int getChannelCount();
        virtual std::shared_ptr<WidgetChannel> getChannel(unsigned int channelIdx);
        virtual std::vector<std::shared_ptr<WidgetChannel>> getChannels();

        virtual bool getIsActive();
        virtual bool getHasNewMeasurement();

    protected:

        std::vector<std::shared_ptr<WidgetChannel>> channels;
};

