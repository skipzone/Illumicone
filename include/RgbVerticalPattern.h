#pragma once

#include "Pattern.h"

class RgbVerticalPattern : public Pattern {
    public:

        RgbVerticalPattern() {};
        ~RgbVerticalPattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();       
};
