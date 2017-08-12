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


#include "AnnoyingFlashingPattern.h"
#include "FillAndBurstPattern.h"
#include "HorizontalStripePattern.h"
#include "log.h"
#include "ParticlesPattern.h"
#include "patternFactory.h"
#include "RainbowExplosionPattern.h"
#include "RgbVerticalPattern.h"
#include "SparklePattern.h"
#include "SpinnerPattern.h"
#include "SwitchActivatedRegionsPattern.h"


Pattern* patternFactory(const std::string& patternClassName, const std::string& patternName)
{
    if (patternClassName == "AnnoyingFlashingPattern") {
        return new AnnoyingFlashingPattern(patternName);
    }
    else if (patternClassName == "FillAndBurstPattern") {
        return new FillAndBurstPattern(patternName);
    }
    else if (patternClassName == "HorizontalStripePattern") {
        return new HorizontalStripePattern(patternName);
    }
    else if (patternClassName == "ParticlesPattern") {
        return new ParticlesPattern(patternName);
    }
    else if (patternClassName == "RainbowExplosionPattern") {
        return new RainbowExplosionPattern(patternName);
    }
    else if (patternClassName == "RgbVerticalPattern") {
        return new RgbVerticalPattern(patternName);
    }
    else if (patternClassName == "SparklePattern") {
        return new SparklePattern(patternName);
    }
    else if (patternClassName == "SpinnerPattern") {
        return new SpinnerPattern(patternName);
    }
    else if (patternClassName == "SwitchActivatedRegionsPattern") {
        return new SwitchActivatedRegionsPattern(patternName);
    }

    logMsg(LOG_ERR, "Unsupported pattern class name \"" + patternClassName + "\" for pattern " + patternName);
    return nullptr;
}

