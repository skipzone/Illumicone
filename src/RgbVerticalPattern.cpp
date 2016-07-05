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
    int i, ii, iii;
    iii = 2;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(1));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, iii, 0);
            ii++;
            iii--;
        }
    }

    return true;
}

bool RgbVerticalPattern::update()
{
//    cout << "Update pattern!" << endl;
    int hadActivity = 0;

    for (auto&& widget:widgets) {
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
            // check if the channel updated
            if (channel.isActive) {
                hadActivity = 1;
                switch (channel.number) {
                    case 0:
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.r = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.r = 0;
                        }
                        break;

                    case 1:
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.g = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.g = 0;
                        }
                        break;

                    case 2:
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.b = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.b = 0;
                        }
                        break;

                    default:
                        cout << "SOMETHING'S FUCKY : channel number of " << channel.number << endl;
                        break;
                }
            }
        }
    }

    isActive = hadActivity;

    return true;
}
