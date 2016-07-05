#ifndef RGB_VERTICAL_PATTERN_H
#define RGB_VERTICAL_PATTERN_H
#include "Pattern.h"

class RgbVerticalPattern : public Pattern {
    public:

        RgbVerticalPattern() {};
        ~RgbVerticalPattern() {};

        bool initPattern(int numStrings, int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();       
};
#endif /* RGB_VERTICAL_PATTERN_H */
