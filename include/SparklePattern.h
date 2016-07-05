#ifndef SPARKLE_PATTERN_H
#define SPARKLE_PATTERN_H
#include "Pattern.h"

class SparklePattern : public Pattern {
    public:
        SparklePattern() {};
        ~SparklePattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* SPARKLE_PATTERN_H */
