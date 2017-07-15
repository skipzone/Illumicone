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

#include <map>
#include <memory>

#include "Pattern.h"
#include "WidgetId.h"


class ConfigReader;
class Widget;


class ParticlesPattern : public Pattern {

    public:

        ParticlesPattern();
        ~ParticlesPattern() {};

        ParticlesPattern(const ParticlesPattern&) = delete;
        ParticlesPattern& operator =(const ParticlesPattern&) = delete;

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets, int priority);
        bool update();

    private:

        std::shared_ptr<WidgetChannel> emitRateChannel;

        // pattern configuration
        opc_pixel_t emitColor;
        int emitIntervalMeasmtLow;
        int emitIntervalMeasmtHigh;
        int emitIntervalLowMs;
        int emitIntervalHighMs;
        bool emitDirectionIsUp;
        unsigned int particleMoveIntervalMs;

        int numRotationsNeededToClearParticles;
        unsigned int nextMoveParticlesMs;
        unsigned int particleEmitIntervalMs;
        unsigned int nextEmitParticlesMs;

        bool moveParticles();
};

