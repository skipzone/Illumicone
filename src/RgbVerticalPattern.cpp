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
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "WidgetFactory.h"

using namespace std;

bool RgbVerticalPattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
//    cout << "Init RGB Vertical Pattern!" << endl;
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "RgbVerticalPattern priority: " << priority << endl;
    this->priority = priority;
    this->isActive = 0;
    this->name = "Rgb Vertical Stripe Pattern";
    this->opacity = 90;

    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    return true;
}

bool RgbVerticalPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::triObelisk);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}

bool RgbVerticalPattern::update()
{
    //cout << "Update pattern!" << endl;
    int hadActivity = 0;

    // clear pixel data
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

    for (auto&& widget:widgets) {
        // update active, position, velocity for each channel in widget
        widget->moveData();
        //cout << "back from moveData" << endl;
        for (auto&& channel:widget->getChannels()) {
            // check if the channel updated
//            if (channel->getHasNewMeasurement() && channel->getIsActive()) {
            if (channel->getIsActive()) {
                hadActivity = 1;
//                    int prevPos = channel->getPreviousPosition();
                int curPos = ((unsigned int) channel->getPosition()) % numStrings;
                //cout << "ch " << channel->getChannelNumber() << ": prev=" << prevPos << " cur=" << curPos << endl;
                //cout << "clearing pixels  in " << prevPos << " for ch " << channel->getChannelNumber() << endl;
                //cout << "setting pixels in " << curPos << " for ch " << channel->getChannelNumber() << endl;
                for (auto&& pixel:pixelArray[curPos]) {
                    switch (channel->getChannelNumber()) {
                        case 0:
                            pixel.r = 255;
                            break;
                        case 1:
                            pixel.g = 255;
                            break;
                        case 2:
                            pixel.b = 255;
                            break;
                    }
                }
            }
        }
    }

    isActive = hadActivity;

    return true;
}
