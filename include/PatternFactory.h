#ifndef PATTERN_FACTORY_H
#define PATTERN_FACTORY_H

#include "RgbVerticalPattern.h"

static Pattern* patternFactory(uint8_t id)
{
    switch (id) {
        case 1:
            return new RgbVerticalPattern;
        default:
            return new RgbVerticalPattern;
    }
}

#endif /* PATTERN_FACTORY_H */
