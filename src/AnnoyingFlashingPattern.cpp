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
#include "illumiconeUtility.h"
#include "lib8tion.h"
#include "Log.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


AnnoyingFlashingPattern::AnnoyingFlashingPattern(const std::string& name)
    : Pattern(name, true)
{
};


AnnoyingFlashingPattern::~AnnoyingFlashingPattern()
{
};


bool AnnoyingFlashingPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    nextFlashChangeMs = 0;

    // ----- get pattern configuration -----

    if (!patternConfigObject["activationThreshold"].is_number()) {
        logger.logMsg(LOG_ERR, "activationThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    activationThreshold = patternConfigObject["activationThreshold"].int_value();
    logger.logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!patternConfigObject["reactivationThreshold"].is_number()) {
        logger.logMsg(LOG_ERR, "reactivationThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    reactivationThreshold = patternConfigObject["reactivationThreshold"].int_value();
    logger.logMsg(LOG_INFO, name + " reactivationThreshold=" + to_string(reactivationThreshold));

    if (!patternConfigObject["autoDisableTimeoutMs"].is_number()) {
        logger.logMsg(LOG_ERR, "autoDisableTimeoutMs not specified in " + name + " pattern configuration.");
        return false;
    }
    autoDisableTimeoutMs = patternConfigObject["autoDisableTimeoutMs"].int_value();
    logger.logMsg(LOG_INFO, name + " autoDisableTimeoutMs=" + to_string(autoDisableTimeoutMs));

    if (!patternConfigObject["allStringsSameColor"].is_bool()) {
        logger.logMsg(LOG_ERR, "allStringsSameColor not specified in " + name + " pattern configuration.");
        return false;
    }
    allStringsSameColor = patternConfigObject["allStringsSameColor"].bool_value();
    logger.logMsg(LOG_INFO, name + " allStringsSameColor=" + to_string(allStringsSameColor));

    if (!patternConfigObject["flashChangeIntervalMs"].is_number()) {
        logger.logMsg(LOG_ERR, "flashChangeIntervalMs not specified in " + name + " pattern configuration.");
        return false;
    }
    flashChangeIntervalMs = patternConfigObject["flashChangeIntervalMs"].int_value();
    logger.logMsg(LOG_INFO, name + " flashChangeIntervalMs=" + to_string(flashChangeIntervalMs));

    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "intensity") {
            intensityChannel = channelConfig.widgetChannel;
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logger.logMsg(LOG_WARNING, name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }

    return true;
}


bool AnnoyingFlashingPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (intensityChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    // If the widget channel has gone inactive, turn off this pattern but allow
    // it to become active again immediatelly if the next measurement crosses
    // the activation threshold.
    if (!intensityChannel->getIsActive()) {
        //logger.logMsg(LOG_DEBUG, "channel inactive");
        if (disableFlashing) {
            logger.logMsg(LOG_INFO, "Re-enabling " + name
                             + " because " + intensityChannel->getName() + " has gone inactive.");
            disableFlashing = false;
        }
        isActive = false;
        return false;
    }

    if (intensityChannel->getHasNewPositionMeasurement()) {

        int intensityMeasmt = intensityChannel->getPosition();

        // Once flashing is disabled, it will remain disabled until the widget goes
        // inactive or the current measurement is below the reactivation theshold.
        if (disableFlashing) {
            if (intensityMeasmt < reactivationThreshold) {
                logger.logMsg(LOG_INFO, "Re-enabling " + name
                                 + " because the last intensity measurement (" + to_string(intensityMeasmt)
                                 + ") crossed the reactivation threshold (" + to_string(reactivationThreshold) + ").");
                disableFlashing = false;
            }
            return false;
        }

        // If the latest measurement is below the activation threshold, turn off this pattern.
        if (intensityMeasmt <= activationThreshold) {
            //logger.logMsg(LOG_DEBUG, "below activation threshold");
            isActive = false;
            return false;
        }

        // If the threshold was just crossed, initialize auto-disable and start flashing.
        if (!isActive) {
            //logger.logMsg(LOG_DEBUG, "activation threshold crossed");
            isActive = true;
            disableMs = nowMs + autoDisableTimeoutMs;
            nextFlashChangeMs = nowMs;
        }
    }

    if (isActive) {

        // If we've been above the threshold for too long, automatically disable
        // this pattern so that it doesn't continually override other patterns.
        if ((int) (nowMs - disableMs) >= 0) {
            logger.logMsg(LOG_INFO, "Disabling " + name
                             + " because it has been active for " + to_string(autoDisableTimeoutMs) + " ms.");
            disableFlashing = true;
            isActive = false;
            return false;
        }

        // Change the flash color if it is time to do so.
        if ((int) (nowMs - nextFlashChangeMs) >= 0) {
            nextFlashChangeMs = nowMs + flashChangeIntervalMs;
            //logger.logMsg(LOG_DEBUG, "flashing the cone");
            HsvPixel hsvColor;
            hsvColor.s = hsvColor.v = 255;
            if (allStringsSameColor) {
                hsvColor.h = random8();
                fillSolid(coneStrings, hsvColor);
            }
            else {
                for (unsigned int i = 0; i < numStrings; ++i) {
                    hsvColor.h = random8();
                    fillSolid(coneStrings, i, hsvColor);
                }
            }
        }
    }

    return isActive;
}

