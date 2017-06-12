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
        int opacity;
        std::string name;
        int isActive;

        virtual bool initPattern(unsigned int numStrings, unsigned int pixelsPerString, int priority) = 0;
        virtual bool initWidgets(int numWidgets, int channelsPerWidget) = 0;

        virtual bool update() = 0;
};

#endif /* PATTERN_H */
