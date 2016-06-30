#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "SolidBlackPattern.h"
#include "WidgetFactory.h"

using namespace std;

bool SolidBlackPattern::initPattern(int numStrings, int pixelsPerString)
{
//    cout << "Init Solid Black Pattern!" << endl;
    numStrings = numStrings;
    pixelsPerString = pixelsPerString;
    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    name = "SolidBlackPattern";

    isActive = false;

    priority = 0;
    return true;
}

bool SolidBlackPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(2));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, 0, 0);
            ii++;
        }
    }

    return true;
}

bool SolidBlackPattern::update()
{
    int i;
    bool hadActivity = false;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
//            cout << "Updating widget's channel!" << endl;
            if (channel.isActive) {
                hadActivity = true;
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
