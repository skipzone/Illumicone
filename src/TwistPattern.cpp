#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "WidgetFactory.h"
#include "TwistPattern.h"

using namespace std;

bool TwistPattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
//    cout << "Init Solid Black Pattern!" << endl;
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "TwistPattern";

    this->isActive = 0;
    this->opacity = 100;
    return true;
}

bool TwistPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(4));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, 0, 0);
            ii++;
        }
    }

    return true;
}

bool TwistPattern::update()
{
    int i;
    int hadActivity = 0;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
//            cout << "Updating widget's channel!" << endl;
            if (channel.isActive) {
                hadActivity = 1;
                switch (channel.number) {
                    case 0:
                        // set entire pixelArray black (off)
                        for (i = 0; i < NUM_STRINGS; i++) {
                            for (auto&& pixel:pixelArray[i]) {
                                pixel.r = 0;
                                pixel.g = 0;
                                pixel.b = 0;
                            }
                        }

                        break;

                    default:
                        // shouldn't get here, solid black uses the eye widget which
                        // should only have one channel.
                        cout << "SOMETHING'S FUCKY : channel number for Solid Black Pattern widget" << endl;
                        break;
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
