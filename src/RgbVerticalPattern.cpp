#include "Widget.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "PatternFactory.h"
#include "WidgetFactory.h"

using namespace std;

bool RgbVerticalPattern::initPattern(int numWidgets, int channelsPerWidget)
{
    int i;

    nextUpdateMs = 5;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(1));
    }
    return true;
}

bool RgbVerticalPattern::update()
{
    return true;
}
