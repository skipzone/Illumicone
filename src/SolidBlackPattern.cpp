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
#include "SolidBlackPattern.h"
#include "WidgetFactory.h"

using namespace std;


SolidBlackPattern::SolidBlackPattern()
    : loggedShutoffMessage(false)
    , loggedQuiescentModeMessage(false)
    , timeExceededThreshold(0)
{
};


bool SolidBlackPattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
//    cout << "Init Solid Black Pattern!" << endl;
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "SolidBlackPattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "SolidBlackPattern";

    this->isActive = 0;
    this->opacity = 100;
    return true;
}

bool SolidBlackPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::eye);
        widgets.emplace_back(newWidget);
        newWidget->init(false);

//        widgets.emplace_back(widgetFactory(eye));
//        widgets[i]->init(channelsPerWidget);
//        ii = 0;
//        for (auto&& channel:widgets[i]->channels) {
//            channel.initChannel(ii, 0, 0);
//            ii++;
//        }
    }

    return true;
}


bool SolidBlackPattern::update()
{
    int hadActivity = 0;
    time_t now;

//    cout << "Updating Solid Black Pattern!" << endl;

    time(&now);

    struct tm tmStruct = *localtime(&now);

#ifndef DISABLE_AUTO_SHUTOFF
    // Shut off the cone during the daytime even if the Eye widget isn't present.
    if ( ( (tmStruct.tm_hour == turnoffStartHour && tmStruct.tm_min >= turnoffStartMinute)
           || tmStruct.tm_hour > turnoffStartHour )
         && ( (tmStruct.tm_hour == turnoffEndHour && tmStruct.tm_min < turnoffEndMinute)
              || tmStruct.tm_hour < turnoffEndHour ) )
    {
        if (!loggedShutoffMessage) {
            loggedShutoffMessage = true;
            cout << "Shutting off cone because hour=" << tmStruct.tm_hour
                << ", minute=" << tmStruct.tm_min
                << endl;
        }
        for (auto&& pixels:pixelArray) {
            for (auto&& pixel:pixels) {
                pixel.r = 1;
                pixel.g = 1;
                pixel.b = 1;
            }
        }
        isActive = true;
        return true;
    }
    else
    {
        if (loggedShutoffMessage) {
            loggedShutoffMessage = false;
            cout << "Turning on cone because hour=" << tmStruct.tm_hour
                << ", minute=" << tmStruct.tm_min
                << endl;
        }
    }
#endif	// #ifndef DISABLE_AUTO_SHUTOFF

#ifndef DISABLE_QUIESCENT_MODE
    // Go into quiescent mode while something special is happening.
    if ( ( (tmStruct.tm_hour == quiescentModeStartHour && tmStruct.tm_min >= quiescentModeStartMinute)
           || tmStruct.tm_hour > quiescentModeStartHour )
         && ( (tmStruct.tm_hour == quiescentModeEndHour && tmStruct.tm_min < quiescentModeEndMinute)
              || tmStruct.tm_hour < quiescentModeEndHour ) )
    {
        if (!loggedQuiescentModeMessage) {
            loggedQuiescentModeMessage = true;
            cout << "Going into quiescent mode because hour=" << tmStruct.tm_hour
                << ", minute=" << tmStruct.tm_min
                << endl;
        }
        for (auto&& pixels:pixelArray) {
            for (auto&& pixel:pixels) {
                pixel.r = 1;
                pixel.g = 1;
                pixel.b = 64;
            }
        }
        isActive = true;
        return true;
    }
    else
    {
        if (loggedQuiescentModeMessage) {
            loggedQuiescentModeMessage = false;
            cout << "Coming out of quiescent mode because hour=" << tmStruct.tm_hour
                << ", minute=" << tmStruct.tm_min
                << endl;
        }
    }
#endif	// #ifndef DISABLE_QUIESCENT_MODE
    
    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();

        if (widget->getIsActive()) {
            for (auto&& channel:widget->getChannels()) {
    //            cout << "Updating widget's channel!" << endl;
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    if (channel->getPosition() <= 200) {     // 300 is a basic LED flashlight from a few feet away
                        cout << "making black pattern transparent" << endl;
                        timeExceededThreshold = 0;
                        for (auto&& pixels:pixelArray) {
                            for (auto&& pixel:pixels) {
                                pixel.r = 0;
                                pixel.g = 0;
                                pixel.b = 0;
                            }
                        }
                    } else {
                        // TODO:  move this back to below the has new measmt check after the widget is fixed to go inactive instead of always active
                        cout << "making black pattern dark or something" << endl;
                        hadActivity = 1;
                        // I'm sitting out here at Illumiconw at 06:03 Saturday morning.  Just met Kameron, Mark,
                        // and Savannah.  Gave techical tour of Illumicone.  Need to make it go black now that sun
                        // is coming up but then go back to multicolor pattern for Eye widget when darkness returns.
                        // Kameron (Utah regional rep.), Mark Hammond, and Savannah--see Mark play 7:00 PM center camp
                        bool turnOff = false;
                        if (timeExceededThreshold == 0) {
                            time(&timeExceededThreshold);
                        }
                        else {
                            if (now - timeExceededThreshold > flashingTimeoutSeconds) {
                                turnOff = true;
                            }
                        }
                        uint8_t redVal;
                        uint8_t greenVal;
                        uint8_t blueVal;
                        if (turnOff) {
                            redVal = 1;
                            greenVal = 1;
                            blueVal = 1;
                        }
                        else {
                            redVal = rand() % 255;
                            greenVal = rand() % 255;
                            blueVal = rand() % 255;
                        }
                        for (auto&& pixels:pixelArray) {
                            for (auto&& pixel:pixels) {
                                pixel.r = redVal;  // TODO ross:  this is really blue!
                                pixel.g = greenVal;
                                pixel.b = blueVal;  // TODO ross:  this is really red!
                            }
                        }
                    }
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
