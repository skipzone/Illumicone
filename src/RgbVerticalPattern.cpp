#include "Widget.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"

using namespace std;

bool RgbVerticalPattern::initPattern(int numWidgets, int channelsPerWidget[])
{
    int i;

    nextUpdateMs = 5;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(channelsPerWidget[i]);
    }
    return true;
}

bool RgbVerticalPattern::update()
{
    return true;
}
