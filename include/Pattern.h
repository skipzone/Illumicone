#ifndef PATTERN_H
#define PATTERN_H

#include "illumiconeTypes.h"
#include <vector>
#include "Widget.h"

class Pattern
{
    public:
        Pattern() {}
        virtual ~Pattern() {}
        int pixelsPerString;
        int numStrings;
        std::vector<std::vector<opc_pixel_t>> pixelArray;
        std::vector<Widget*> widgets;

        int priority;
        std::string name;
        bool isActive;

        virtual bool initPattern(int numStrings, int pixelsPerString) = 0;
        virtual bool initWidgets(int numWidgets, int channelsPerWidget) = 0;

        virtual bool update() = 0;
};

#endif /* PATTERN_H */
