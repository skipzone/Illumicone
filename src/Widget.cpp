#include "Widget.h"
#include "WidgetChannel.h"
#include <memory>


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



