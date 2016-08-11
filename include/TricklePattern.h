#ifndef TRICKLE_PATTERN_H
#define TRICKLE_PATTERN_H
#include "Pattern.h"

class TricklePattern : public Pattern
{
    public:
        TricklePattern() {};
        ~TricklePattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* TRICKLE_PATTERN_H */
