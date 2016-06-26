#ifndef SOLID_BLACK_PATTERN_H
#define SOLID_BLACK_PATTERN_H
#include "Pattern.h"

class SolidBlackPattern : public Pattern {
    public:
        SolidBlackPattern() {};
        ~SolidBlackPattern() {};

        int priority;
        bool initPattern(int numStrings, int pixelsPerString);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* SOLID_BLACK_PATTERN_H */
