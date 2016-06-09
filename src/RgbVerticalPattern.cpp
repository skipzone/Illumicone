#include <iostream>
#include "Widget.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "PatternFactory.h"
#include "WidgetFactory.h"

using namespace std;

bool RgbVerticalPattern::initPattern(int numStrings, int pixelsPerString)
{
    cout << "Init RGB Vertical Pattern!" << endl;
    numStrings = numStrings;
    pixelsPerString = pixelsPerString;

    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    return true;
}

bool RgbVerticalPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;

    nextUpdateMs = 5;

    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(1));
    }
    return true;
}

bool RgbVerticalPattern::update()
{
    // channel iterator
    int channel_num = 0;
    for (auto widget:widgets) {
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto channel:widget->channels) {
            // check if the channel updated
            if (channel.isActive) {
                switch (channel_num) {
                    case 0:
                        for (auto pixel:pixelArray[channel.position]) {
                            pixel.r = 255;
                        }
                        for (auto pixel:pixelArray[channel.prevPosition]) {
                            pixel.r = 0;
                        }
                        break;

                    case 1:
                        for (auto pixel:pixelArray[channel.position]) {
                            pixel.g = 255;
                        }
                        for (auto pixel:pixelArray[channel.prevPosition]) {
                            pixel.g = 0;
                        }
                        break;

                    case 2:
                        for (auto pixel:pixelArray[channel.position]) {
                            pixel.b = 255;
                        }
                        for (auto pixel:pixelArray[channel.prevPosition]) {
                            pixel.b = 0;
                        }
                        break;

                    default:
                        break;
                }
            }

            channel_num++;
        }
    }

    return true;
}
