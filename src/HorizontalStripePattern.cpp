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

#include "ConfigReader.h"
#include "HorizontalStripePattern.h"
#include "illumiconePixelUtility.h"
#include "log.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


HorizontalStripePattern::HorizontalStripePattern(const std::string& name)
    : Pattern(name)
{
}


bool HorizontalStripePattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    widthPos = 1;
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


bool HorizontalStripePattern::update()
{
    isActive = false;

    bool gotPositionOrWidthUpdate = false;

    // Get any updated positions of the stripes.
    if (redPositionChannel != nullptr) {
        if (redPositionChannel->getIsActive()) {
            isActive = true;
            if (redPositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                rPos = ((unsigned int) redPositionChannel->getPosition()) % pixelsPerString;
            }
        }
    }
    if (greenPositionChannel != nullptr) {
        if (greenPositionChannel->getIsActive()) {
            isActive = true;
            if (greenPositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                gPos = ((unsigned int) greenPositionChannel->getPosition()) % pixelsPerString;
            }
        }
    }
    if (bluePositionChannel != nullptr) {
        if (bluePositionChannel->getIsActive()) {
            isActive = true;
            if (bluePositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                bPos = ((unsigned int) bluePositionChannel->getPosition()) % pixelsPerString;
            }
        }
    }

    // Get an updated stripe width, if available.
    // TODO 6/25/2017 ross:  set width back to 1 when width channel has been inactive for widthResetTimeoutSeconds 
    if (widthChannel != nullptr) {
        if (widthChannel->getIsActive()) {
            isActive = true;
            if (widthChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                widthPos = widthChannel->getPosition() / widthScaleFactor;
                if (maxCyclicalWidth != 0) {
                    // This is a triangle wave function where the period is
                    // (maxCyclicalWidth - 1) * 2 and the range is 1 to maxCyclicalWidth.
                    widthPos = abs(abs(widthPos) % ((maxCyclicalWidth - 1) * 2) - (maxCyclicalWidth - 1)) + 1;
                }
                if (widthPos < 1) {
                    widthPos = 1;
                }
                //logMsg(LOG_DEBUG, "widthPos=" + to_string(widthPos));
            }
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(pixelArray);

        int upperExtraWidth = 0;
        int lowerExtraWidth = 0;
        if (widthPos >= 2) {
            upperExtraWidth = widthPos / 2;
            lowerExtraWidth = widthPos - upperExtraWidth;
        }
        //logMsg(LOG_DEBUG, "upperExtraWidth=" + to_string(upperExtraWidth));
        //logMsg(LOG_DEBUG, "lowerExtraWidth=" + to_string(lowerExtraWidth));

        int rWidthLowIndex = rPos - upperExtraWidth;
        int rWidthHighIndex = rPos + lowerExtraWidth;

        int gWidthLowIndex = gPos - upperExtraWidth;
        int gWidthHighIndex = gPos + lowerExtraWidth;

        int bWidthLowIndex = bPos - upperExtraWidth;
        int bWidthHighIndex = bPos + lowerExtraWidth;

        for (auto&& stringPixels : pixelArray) {
            for (int i = rWidthLowIndex; i <= rWidthHighIndex; ++i) {
                unsigned int pixelIdx = (i % pixelsPerString + pixelsPerString) % pixelsPerString;
                stringPixels[pixelIdx].r = 255;
            }
            for (int i = gWidthLowIndex; i <= gWidthHighIndex; ++i) {
                unsigned int pixelIdx = (i % pixelsPerString + pixelsPerString) % pixelsPerString;
                stringPixels[pixelIdx].g = 255;
            }
            for (int i = bWidthLowIndex; i <= bWidthHighIndex; ++i) {
                unsigned int pixelIdx = (i % pixelsPerString + pixelsPerString) % pixelsPerString;
                stringPixels[pixelIdx].b = 255;
            }
        }
    }

/*
    if (isActive) {
        rgb2hsv(pixelArray, coneStrings);
        clearAllPixels(pixelArray);
        hsv2rgb(coneStrings, pixelArray);
    }
*/

/*
    if (!isActive) {
        logMsg(LOG_DEBUG, name + " inactive");
    }
*/

    return isActive;
}
