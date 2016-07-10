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
    int i;  //, ii, iii;
    //iii = 2;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::triObelisk);
        widgets.emplace_back(newWidget);
        newWidget->init();
/*
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, iii, 0);
            ii++;
            iii--;
        }
*/
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
        // clear the previous array vals
        if (widget->getIsActive()) {
            for (auto&& channel:widget->getChannels()) {
                // check if the channel updated
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    hadActivity = 1;
//                    int prevPos = channel->getPreviousPosition();
                    int curPos = channel->getPosition();
                    //cout << "ch " << channel->getChannelNumber() << ": prev=" << prevPos << " cur=" << curPos << endl;
                    //cout << "clearing pixels  in " << prevPos << " for ch " << channel->getChannelNumber() << endl;
//                    for (auto&& pixel:pixelArray[prevPos]) {
//                        switch (channel->getChannelNumber()) {
//                            case 0:
//                                pixel.r = 0;
//                                break;
//                            case 1:
//                                pixel.g = 0;
//                                break;
//                            case 2:
//                                pixel.b = 0;
//                                break;
//                        }
//                    }
                    //cout << "setting pixels in " << curPos << " for ch " << channel->getChannelNumber() << endl;
                    for (auto&& pixel:pixelArray[curPos]) {
                        switch (channel->getChannelNumber()) {
                            case 0:
                                pixel.r = 48;
                                break;
                            case 1:
                                pixel.g = 48;
                                break;
                            case 2:
                                pixel.b = 48;
                                break;
                        }
                    }
    /*****
                    switch (channel->getChannelNumber()) {
                        case 0:
                            for (auto&& pixel:pixelArray[prevPos]) {
                                pixel.r = 0;
                            }
                            for (auto&& pixel:pixelArray[curPos]) {
                                pixel.r = 255;
                            }
                            break;

                        case 1:
                            for (auto&& pixel:pixelArray[channel->getPreviousPosition()]) {
                                pixel.g = 0;
                            }
                            for (auto&& pixel:pixelArray[channel->getPosition()]) {
                                pixel.g = 255;
                            }
                            break;

                        case 2:
                            for (auto&& pixel:pixelArray[channel->getPreviousPosition()]) {
                                pixel.b = 0;
                            }
                            for (auto&& pixel:pixelArray[channel->getPosition()]) {
                                pixel.b = 255;
                            }
                            break;

                        default:
                            cout << "SOMETHING'S FUCKY : channel number of " << channel->getChannelNumber() << endl;
                            break;
                    }
    ******/
                }
            }
        }
    }

    isActive = hadActivity;

    return true;
}
