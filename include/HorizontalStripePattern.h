#pragma once

#include "Pattern.h"

class HorizontalStripePattern : public Pattern
{
    public:
        HorizontalStripePattern() {};
        ~HorizontalStripePattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};
