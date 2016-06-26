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
    cout << "Init Solid Black Pattern!" << endl;
    numStrings = numStrings;
    pixelsPerString = pixelsPerString;

    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    priority = 2;
    return true;
}

bool SolidBlackPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;
    cout << "Init RGB Vertical Pattern Widgets!" << endl;

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
    cout << "Updating Solid Black Pattern!" << endl;

    return true;
}
