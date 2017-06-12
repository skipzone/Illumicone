/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SPARKLE_PATTERN_H
#define SPARKLE_PATTERN_H
#include "Pattern.h"

class SparklePattern : public Pattern {
    public:
        SparklePattern() {};
        ~SparklePattern() {};

        bool initPattern(unsigned int numStrings, unsigned int pixelsPerString, int priority);
        bool initWidgets(int numWidgets, int channelsPerWidget);
        bool update();
};

#endif /* SPARKLE_PATTERN_H */
