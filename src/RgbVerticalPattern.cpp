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

#include "ConfigReader.h"
#include "log.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


RgbVerticalPattern::RgbVerticalPattern(const std::string& name)
    : Pattern(name)
{
}


bool RgbVerticalPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    rPos = 0;
    gPos = 0;
    bPos = 0;

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["widthScaleFactor"].is_number()) {
        logMsg(LOG_ERR, "widthScaleFactor not specified in " + name + " pattern configuration.");
        return false;
    }
    widthScaleFactor = patternConfig["widthScaleFactor"].int_value();
    logMsg(LOG_INFO, name + " widthScaleFactor=" + to_string(widthScaleFactor));

    if (!patternConfig["maxCyclicalWidth"].is_number()) {
        logMsg(LOG_ERR, "maxCyclicalWidth not specified in " + name + " pattern configuration.");
        return false;
    }
    maxCyclicalWidth = patternConfig["maxCyclicalWidth"].int_value();
    logMsg(LOG_INFO, name + " maxCyclicalWidth=" + to_string(maxCyclicalWidth));

    if (!patternConfig["widthResetTimeoutSeconds"].is_number()) {
        logMsg(LOG_ERR, "widthResetTimeoutSeconds not specified in " + name + " pattern configuration.");
        return false;
    }
    widthResetTimeoutSeconds = patternConfig["widthResetTimeoutSeconds"].int_value();
    logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "redPosition") {
            redPositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "greenPosition") {
            greenPositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "bluePosition") {
            bluePositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "width") {
            widthChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logMsg(LOG_WARNING, name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }

    return true;
}


bool RgbVerticalPattern::update()
{
    isActive = false;

    // clear pixel data
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel = CRGB::Black;
        }
    }

    // TODO 7/24/2017 ross:  We need to figure out the width first, then set the pixels.
    //                       That will allow us to do blending more easily.

    // TODO 6/13/2017 ross:  If we kept the previous position and didn't clear it above, maybe we would have to do
    //                       this only if redPositionChannel->getHasNewPositionMeasurement() && redPositionChannel->getIsActive().
    //                       We'd have to remember to clear the previous position if the channel went inactive, though.
    //                       Same for green and blue.
    if (redPositionChannel != nullptr && redPositionChannel->getIsActive()) {
        isActive = true;
        rPos = ((unsigned int) redPositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[rPos]) {
            pixel.r = 255;
        }
    }

    if (greenPositionChannel != nullptr && greenPositionChannel->getIsActive()) {
        isActive = true;
        gPos = ((unsigned int) greenPositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[gPos]) {
            pixel.g = 255;
        }
    }

    if (bluePositionChannel != nullptr && bluePositionChannel->getIsActive()) {
        isActive = true;
        bPos = ((unsigned int) bluePositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[bPos]) {
            pixel.b = 255;
        }
    }

    // TODO 6/25/2017 ross:  set width back to 1 when width channel has been inactive for widthResetTimeoutSeconds 

    if (widthChannel != nullptr && widthChannel->getIsActive()) {

        isActive = true;

        int widthPos = widthChannel->getPosition() / widthScaleFactor;

        if (maxCyclicalWidth != 0) {
            // This is a triangle wave function where the period is (maxCyclicalWidth - 1) * 2 and the range is 1 to maxCyclicalWidth.
            widthPos = abs(abs(widthPos) % ((maxCyclicalWidth - 1) * 2) - (maxCyclicalWidth - 1)) + 1;
        }

        if (widthPos >= 2) {

            int leftExtraWidth = widthPos / 2;
            int rightExtraWidth = widthPos - leftExtraWidth;

            int rWidthLowIndex = rPos - leftExtraWidth;
            int rWidthHighIndex = rPos + rightExtraWidth;

            int gWidthLowIndex = gPos - leftExtraWidth;
            int gWidthHighIndex = gPos + rightExtraWidth;

            int bWidthLowIndex = bPos - leftExtraWidth;
            int bWidthHighIndex = bPos + rightExtraWidth;

            for (int i = rWidthLowIndex; i < rWidthHighIndex; ++i) {
                int stringIndex = (i % numStrings + numStrings) % numStrings;
                for (auto&& pixels:pixelArray[stringIndex]) {
                    pixels.r = 255;
                }
            }

            for (int i = gWidthLowIndex; i < gWidthHighIndex; ++i) {
                int stringIndex = (i % numStrings + numStrings) % numStrings;
                for (auto&& pixels:pixelArray[stringIndex]) {
                    pixels.g = 255;
                }
            }

            for (int i = bWidthLowIndex; i < bWidthHighIndex; ++i) {
                int stringIndex = (i % numStrings + numStrings) % numStrings;
                for (auto&& pixels:pixelArray[stringIndex]) {
                    pixels.b = 255;
                }
            }
        }
    }

    return isActive;
}
