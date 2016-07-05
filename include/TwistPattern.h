#ifndef TWIST_PATTERN_H
#define TWIST_PATTERN_H
#include "Pattern.h"

class TwistPattern : public Pattern {
    public:
        TwistPattern() {};
        ~TwistPattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* TWIST_PATTERN_H */
