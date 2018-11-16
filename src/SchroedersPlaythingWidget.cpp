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

#include <string>

#include "illumiconeTypes.h"
#include "illumiconeWidgetTypes.h"
#include "log.h"
#include "SchroedersPlaythingWidget.h"
#include "WidgetId.h"

using namespace std;


SchroedersPlaythingWidget::SchroedersPlaythingWidget()
    : Widget(WidgetId::schroedersPlaything, 1, true)
    , currentNote(35)
{
    simulationUpdateIntervalMs[0] = 180;
}


void SchroedersPlaythingWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    ++currentNote;
    if (currentNote > 96) {
        currentNote = 36;
    }

    MidiPositionMeasurement pos;
    MidiVelocityMeasurement vel;

    pos.channelNumber = 0;
    pos.channelMessageType = MIDI_NOTE_ON;
    vel.noteNumber = currentNote;
    vel.velocity = 64;

    channels[chIdx]->setPositionAndVelocity(pos.raw, vel.raw);
    channels[chIdx]->setIsActive(true);

    //logMsg(LOG_DEBUG, "added MIDI_NOTE_ON message for note " + to_string(currentNote));
}

