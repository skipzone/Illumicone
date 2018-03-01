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

        ParticlesPattern(const std::string& name);
        ~ParticlesPattern() {};

        ParticlesPattern() = delete;
        ParticlesPattern(const ParticlesPattern&) = delete;
        ParticlesPattern& operator =(const ParticlesPattern&) = delete;

        bool update();

    protected:

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        std::shared_ptr<WidgetChannel> emitRateChannel;
        std::shared_ptr<WidgetChannel> emitColorChannel;

        // pattern configuration
        int emitBatchSize;
        CHSV emitColorDefault;
        CHSV emitColorHigh;
        bool emitColorIntegrateMeasmt;
        CHSV emitColorLow;
        double emitColorMeasmtHigh;
        double emitColorMeasmtLow;
        double emitColorMeasmtMultiplier;
        bool emitDirectionIsUp;
        int emitIntervalMeasmtHigh;
        int emitIntervalMeasmtLow;
        int emitIntervalHighMs;
        int emitIntervalLowMs;
        unsigned int particleMoveIntervalMs;

        int numRotationsNeededToClearParticles;
        unsigned int nextMoveParticlesMs;
        unsigned int particleEmitIntervalMs;
        unsigned int nextEmitParticlesMs;
        bool usePositionMeasurement;

        bool moveParticles();
};

