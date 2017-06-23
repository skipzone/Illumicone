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
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


RgbVerticalPattern::RgbVerticalPattern()
    : Pattern("rgbVertical")
{
}


bool RgbVerticalPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets, int priority)
{
    numStrings = config.getNumberOfStrings();
    pixelsPerString = config.getNumberOfPixelsPerString();
    this->priority = priority;
    opacity = 90;

    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));

    rPos = 0;
    gPos = 0;
    bPos = 0;

    rIsActive = false;
    gIsActive = false;
    bIsActive = false;

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        cerr << "No valid widget channels are configured for " << name << "." << endl;
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
            cerr << "Warning:  inputName '" << channelConfig.inputName
                << "' in input configuration for " << name << " is not recognized." << endl;
            continue;
        }
        cout << name << " using " << channelConfig.widgetChannel->getName() << " for " << channelConfig.inputName << endl;

        if (channelConfig.measurement != "position") {
            cerr << "Warning:  " << name << " supports only position measurements, but the input configuration for "
                << channelConfig.inputName << " doesn't specify position." << endl;
        }
    }

    return true;
}


bool RgbVerticalPattern::update()
{
    //cout << "Update pattern!" << endl;
    int hadActivity = false;
    rIsActive = false;
    gIsActive = false;
    bIsActive = false;

    // clear pixel data
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

    // TODO 6/13/2017 ross:  If we kept the previous position and didn't clear it above, maybe we would have to do
    //                       this only if redPositionChannel->getHasNewPositionMeasurement() && redPositionChannel->getIsActive().
    //                       We'd have to remember to clear the previous position if the channel went inactive, though.
    //                       Same for green and blue.
    if (redPositionChannel != nullptr && redPositionChannel->getIsActive()) {
        hadActivity = true;
        int curPos = ((unsigned int) redPositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[curPos]) {
            rPos = curPos;
            rIsActive = true;
            pixel.r = 255;
        }
    }

    if (greenPositionChannel != nullptr && greenPositionChannel->getIsActive()) {
        hadActivity = true;
        int curPos = ((unsigned int) greenPositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[curPos]) {
            gPos = curPos;
            gIsActive = true;
            pixel.g = 255;
        }
    }

    if (bluePositionChannel != nullptr && bluePositionChannel->getIsActive()) {
        hadActivity = true;
        int curPos = ((unsigned int) bluePositionChannel->getPosition()) % numStrings;
        for (auto&& pixel:pixelArray[curPos]) {
            bPos = curPos;
            bIsActive = true;
            pixel.b = 255;
        }
    }

    if (widthChannel != nullptr && widthChannel->getIsActive()) {
        // TODO 6/13/2017 ross:  Need to make this work for more than just the Bells widget.

        int rWidthLowIndex;
        int rWidthHighIndex;
        int gWidthLowIndex;
        int gWidthHighIndex;
        int bWidthLowIndex;
        int bWidthHighIndex;
        int bellsPos;
        int stringIndex;

        float scaleValue = (1024.0 / ((float) numStrings / 3.0));

        bellsPos = widthChannel->getPosition();
        bellsPos = (int) ((float) bellsPos / scaleValue);
        if (bellsPos >= 2) {
            hadActivity = true;
        }

        rWidthLowIndex = rPos - (bellsPos / 2);
        rWidthHighIndex = rPos + (bellsPos / 2);

        gWidthLowIndex = gPos - (bellsPos / 2);
        gWidthHighIndex = gPos + (bellsPos / 2);

        bWidthLowIndex = bPos - (bellsPos / 2);
        bWidthHighIndex = bPos + (bellsPos / 2);

        for (int i = rWidthLowIndex; i < rWidthHighIndex; ++i) {
            stringIndex = (i % numStrings + numStrings) % numStrings;
            for (auto&& pixels:pixelArray[stringIndex]) {
                pixels.r = 255;
            }
        }

        for (int i = gWidthLowIndex; i < gWidthHighIndex; ++i) {
            stringIndex = (i % numStrings + numStrings) % numStrings;
            for (auto&& pixels:pixelArray[stringIndex]) {
                pixels.g = 255;
            }
        }

        for (int i = bWidthLowIndex; i < bWidthHighIndex; ++i) {
            stringIndex = (i % numStrings + numStrings) % numStrings;
            for (auto&& pixels:pixelArray[stringIndex]) {
                pixels.b = 255;
            }
        }
    }

    isActive = hadActivity;

    return true;
}
