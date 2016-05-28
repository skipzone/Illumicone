#ifndef PATTERN_H
#define PATTERN_H

#include "ledscape.h"
#include <vector>
#include "Widget.h"

class Pattern
{
    public:
        Pattern() {}
        virtual ~Pattern() {}
        int pixelsPerString;
        int numStrings;
        std::vector<std::vector<ledscape_pixel_t>> opcVector;
        std::vector<Widget*> widgets;

        bool init (int numStrings, int pixelsPerString) {
            numStrings = numStrings;
            pixelsPerString = pixelsPerString;

            opcVector.resize(numStrings, std::vector<ledscape_pixel_t>(pixelsPerString));
            return true;
        }

        virtual bool initPattern (int numWidgets, int channelsPerWidget) = 0;
        virtual bool update() = 0;

        uint32_t nextUpdateMs;
};

#endif /* PATTERN_H */
