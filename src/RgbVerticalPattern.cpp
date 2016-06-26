#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "PatternFactory.h"
#include "WidgetFactory.h"

using namespace std;

bool RgbVerticalPattern::initPattern(int numStrings, int pixelsPerString)
{
    cout << "Init RGB Vertical Pattern!" << endl;
    cout << "initPattern numStrings: " << numStrings << endl;
    cout << "initPattern pixelsPerString: " << pixelsPerString << endl;
    numStrings = numStrings;
    pixelsPerString = pixelsPerString;

    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
//    pixelArray.resize(std::vector<opc_pixel_t>(pixelsPerString), numStrings);
    return true;
}

bool RgbVerticalPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;

    nextUpdateMs = 5;

    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(1));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, 0, 0);
            ii++;
        }
    }

    return true;
}

bool RgbVerticalPattern::update()
{
    // channel iterator

    cout << "Update pattern!" << endl;

    for (auto&& widget:widgets) {
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
            // check if the channel updated
            if (channel.isActive) {
                switch (channel.number) {
                    case 0:
                        cout << "Update channel 0" << endl;
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.r = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.r = 0;
                        }
                        break;

                    case 1:
                        cout << "Update channel 1" << endl;
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.g = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.g = 0;
                        }
                        break;

                    case 2:
                        cout << "Update channel 2" << endl;
                        for (auto&& pixel:pixelArray[channel.position]) {
                            pixel.b = 255;
                        }
                        for (auto&& pixel:pixelArray[channel.prevPosition]) {
                            pixel.b = 0;
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }

    return true;
}
