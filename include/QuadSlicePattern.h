#ifndef QUAD_SLICE_PATTERN_H
#define QUAD_SLICE_PATTERN_H
#include "Pattern.h"

class QuadSlicePattern : public Pattern {
    public:
        QuadSlicePattern() {};
        ~QuadSlicePattern() {};

        bool initPattern(int numStrings, int pixelsPerString);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* QUAD_SLICE_PATTERN_H */
