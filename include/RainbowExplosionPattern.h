#pragma once
#include "Pattern.h"

class RainbowExplosionPattern : public Pattern
{
    public:
        RainbowExplosionPattern() {};
        ~RainbowExplosionPattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};
