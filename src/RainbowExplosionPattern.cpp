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
#include "WidgetFactory.h"
#include "RainbowExplosionPattern.h"

using namespace std;

bool RainbowExplosionPattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "RainbowExplosionPattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "RainbowExplosionPattern";

    this->isActive = 0;
    this->opacity = 100;

    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

    return true;
}

bool RainbowExplosionPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::plunger);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}


bool RainbowExplosionPattern::update()
{
    bool hadActivity = false;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        if (widget->getIsActive()) {
            for (auto&& channel:widget->getChannels()) {
//                cout << "Updating widget's channel!" << endl;
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    hadActivity = true;
                    // TODO: Do stuff
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
