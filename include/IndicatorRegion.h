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

#pragma once

#include "json11.hpp"
#include "illumiconePixelTypes.h"


class IndicatorRegion {

    public:

        IndicatorRegion() {};
        virtual ~IndicatorRegion() {};

        IndicatorRegion(const IndicatorRegion&) = delete;
        IndicatorRegion& operator =(const IndicatorRegion&) = delete;

        // A derived class's init must call this init before doing anything else.
        virtual bool init(unsigned int numStrings, unsigned int pixelsPerString, const json11::Json& indicatorConfig);

        // A derived class must implement runAnimation if it has animation or
        // transition code to execute.  runAnimation will be called frequently
        // via the update function of an IndicatorRegionsPattern object.  It
        // should simply return false if isAnimating is false (unless it wants
        // to be a jackass and burn cycles).  It should return true if it has an
        // animation or transition in progress and wants display.
        virtual bool runAnimation() { return false; };

        // A derived classe can implement whichever of these is appropriate for it.
        // At a minimum, it should implement at least one pair of off/on functions.
        virtual void makeAnimating(bool enable) { isAnimating = enable; }
        virtual void makeHighlighted(bool enable) { isHighlighted = enable; }
        virtual void makeSubtle(bool enable) { isSubtle = enable; }
        virtual void transitionOff() {}
        virtual void transitionOn() {}
        virtual void turnOffImmediately() {}
        virtual void turnOnImmediately() {}

        // A derived class should implement any of these for which it does something unholy.
        virtual HsvPixel        getBackgroundColor()      { return backgroundColor; }
        virtual HsvConeStrings* getConeStrings()          { return coneStrings; }
        virtual HsvPixel        getForegroundColor()      { return foregroundColor; }
        virtual unsigned int    getHeightInPixels()       { return heightInPixels; }
        virtual unsigned int    getIndex()                { return index; }
        virtual bool            getIsAnimating()          { return isAnimating; }
        virtual bool            getIsHighlighted()        { return isHighlighted; }
        virtual bool            getIsSubtle()             { return isSubtle; }
        virtual unsigned int    getUpperLeftStringIndex() { return upperLeftStringIdx; }
        virtual unsigned int    getUpperLeftPixelIndex()  { return upperLeftPixelIdx; }
        virtual unsigned int    getWidthInStrings()       { return widthInStrings; }

        // A derived class should implement these if it wants notifications of changes.
        virtual void setBackgroundColor(const HsvPixel& color) { backgroundColor = color; }
        virtual void setConeStrings(HsvConeStrings* p)         { coneStrings = p; }
        virtual void setForegroundColor(const HsvPixel& color) { foregroundColor = color; }

    protected:

        void fillRegion(const HsvPixel& color);

        HsvPixel backgroundColor;
        // TODO 8/8/2017 ross:  use std::shared_ptr instead
        HsvConeStrings* coneStrings;
        unsigned int endPixelIdx;
        unsigned int endStringIdx;
        HsvPixel foregroundColor;
        unsigned int heightInPixels;
        unsigned int index;
        bool isAnimating;
        bool isHighlighted;
        bool isSubtle;
        unsigned int numStrings;
        unsigned int pixelsPerString;
        unsigned int startPixelIdx;
        unsigned int startStringIdx;
        unsigned int upperLeftStringIdx;
        unsigned int upperLeftPixelIdx;
        unsigned int widthInStrings;

    private:

};

