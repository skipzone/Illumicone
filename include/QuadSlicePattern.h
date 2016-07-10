#pragma once

#include "Pattern.h"

class QuadSlicePattern : public Pattern {
    public:
        QuadSlicePattern() {};
        ~QuadSlicePattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};
