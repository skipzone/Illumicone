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
#include <time.h>

#include "AnnoyingFlashingPattern.h"
#include "ConfigReader.h"
#include "hsv2rgb.h"
#include "illumiconePixelUtility.h"
#include "lib8tion.h"
#include "log.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


AnnoyingFlashingPattern::AnnoyingFlashingPattern(const std::string& name)
    : Pattern(name)
{
};


AnnoyingFlashingPattern::~AnnoyingFlashingPattern()
{
    freeConePixels(hsvConePixels);
};


bool AnnoyingFlashingPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    allocateConePixels(hsvConePixels, pixelsPerString, numStrings);

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["activationThreshold"].is_number()) {
        logMsg(LOG_ERR, "activationThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    activationThreshold = patternConfig["activationThreshold"].int_value();
    logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!patternConfig["flashingTimeoutSeconds"].is_number()) {
        logMsg(LOG_ERR, "flashingTimeoutSeconds not specified in " + name + " pattern configuration.");
        return false;
    }
    flashingTimeoutSeconds = patternConfig["flashingTimeoutSeconds"].int_value();
    logMsg(LOG_INFO, name + " flashingTimeoutSeconds=" + to_string(flashingTimeoutSeconds));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "intensity") {
            intensityChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_ERR, "Warning:  inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logMsg(LOG_ERR, "Warning:  " + name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }

    return true;
}


bool AnnoyingFlashingPattern::goInactive()
{
    // If we're just now going inactive, we need to return true
    // so that this pattern can be cleared from display.
    bool retval = isActive;

    if (isActive) {
        isActive = false;
        timeExceededThreshold = 0;
        // Set all the pixels to 0 intensity to make this pattern effectively transparent.
        for (auto&& pixels:pixelArray) {
            for (auto&& pixel:pixels) {
                pixel = CRGB::Black;
            }
        }
    }

    return retval;
}


bool AnnoyingFlashingPattern::update()
{
    // TODO:  The frequency of the flashing is determined by the frequency at which the widget
    //        sends measurements. Instead, the frequency should be determined here in the pattern.
    //        Also, when this pattern disables itself because it has flashed for flashingTimeoutSeconds,
    //        it should not re-enable itself immediately after the widget goes inactive or the
    //        measurement drops below the threshold.  Instead, it should wait for some predetermined
    //        period of time before becoming enabled again.

    // Don't do anything if no input channel was assigned.
    if (intensityChannel == nullptr) {
        return false;
    }

    // If the widget channel has gone inactive, turn off this pattern.
    if (!intensityChannel->getIsActive()) {
        //logMsg(LOG_DEBUG, "channel inactive");
        disableFlashing = false;
        return goInactive();
    }

    // No change to the pattern if we haven't received a new measurement.
    if (!intensityChannel->getHasNewPositionMeasurement()) {
        //logMsg(LOG_DEBUG, "no new measurement");
        return isActive;
    }

    // If the latest measurement is below the activation threshold, turn off this pattern.
    if (intensityChannel->getPosition() <= activationThreshold) {
        //logMsg(LOG_DEBUG, "below activation threshold");
        return goInactive();
    }

    // Once flashing is disabled, it will remain disabled until the widget goes inactive.
    if (disableFlashing) {
        //logMsg(LOG_DEBUG, "flashing disabled");
        return isActive;
    }

    if (!isActive) {
        isActive = true;
        // The threshold was just crossed, so initialize auto-disable.
        time(&timeExceededThreshold);
    }
    else {
        // If we've been above the threshold for too long, automatically disable
        // this pattern so that it doesn't continually override other patterns.
        time_t now;
        time(&now);
        if (now - timeExceededThreshold >= flashingTimeoutSeconds) {
            //logMsg(LOG_DEBUG, "disabling");
            disableFlashing = true;
            return goInactive();
        }
    }

    //logMsg(LOG_DEBUG, "flashing the cone");
    HsvPixel hsvColor;
    hsvColor.h = random8();
    hsvColor.s = hsvColor.v = 255;
    fillSolid(hsvConePixels, hsvColor);
    hsv2rgb(hsvConePixels, pixelArray);

    return isActive;
}

