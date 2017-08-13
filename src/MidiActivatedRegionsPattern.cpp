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

#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "IndicatorRegion.h"
#include "log.h"
#include "MidiActivatedRegionsPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


MidiActivatedRegionsPattern::MidiActivatedRegionsPattern(const std::string& name)
    : IndicatorRegionsPattern(name, true)
{
}


bool MidiActivatedRegionsPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(config, widgets)) {
        return false;
    }


    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    string errMsgSuffix = " in " + name + " pattern configuration.";


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "midiInput") {
            midiInputChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "Warning:  inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

    }

    return true;
}


string MidiActivatedRegionsPattern::midiMessageToString(MidiPositionMeasurement pos, MidiVelocityMeasurement vel)
{
    string msg;
    uint16_t pitch;
    switch(pos.messageType) {
        case MIDI_NOTE_OFF:
            msg = "note off, note=" + to_string(vel.noteNumber) + ", velocity=" + to_string(vel.velocity);
            break;
        case MIDI_NOTE_ON:
            msg = "note on, note=" + to_string(vel.noteNumber) + ", velocity=" + to_string(vel.velocity);
            break;
        case MIDI_POLYPHONIC_AFTERTOUCH:
            msg = "polyphonic aftertouch, note=" + to_string(vel.noteNumber) + ", pressure=" + to_string(vel.pressure);
            break;
        case MIDI_CONTROL_CHANGE:
            msg = "control change, note=" + to_string(vel.controllerNumber) + ", data=" + to_string(vel.data2);
            break;
        case MIDI_PROGRAM_CHANGE:
            msg = "program change, program=" + to_string(vel.programNumber);
            break;
        case MIDI_CHANNEL_AFTERTOUCH:
            msg = "channel aftertouch, pressure=" + to_string(vel.channelPressure);
            break;
        case MIDI_PITCH_WHEEL:
            pitch = vel.pitchH << 8 | vel.pitchL;
            msg = "pitch wheel, high=" + to_string(vel.pitchH) + ", low=" + to_string(vel.pitchL) + ", pitch=" + to_string(pitch);
            break;
        default:
            msg = "???, type=" + to_string(pos.messageType) + ", data1=" + to_string(vel.data1) + ", data2=" + to_string(vel.data2);
    }
    msg += ", channel=" + to_string(pos.channelNumber);
    return msg;
}


bool MidiActivatedRegionsPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (midiInputChannel == nullptr) {
        return false;
    }

//    unsigned int nowMs = getNowMs();

    // Let the regions do their animations.
    bool animationWantsDisplay = IndicatorRegionsPattern::update();

    if (!midiInputChannel->getIsActive()) {
        //logMsg(LOG_DEBUG, "midiInputChannel is inactive");
        if (!activeIndicators.empty()) {
            // If the widget has just gone inactive, turn off all the indicators.
            for (auto&& activeIndicator : activeIndicators) {
                activeIndicator->turnOffImmediately();
            }
            activeIndicators.clear();
        }
        isActive = false;
    }

    else if (midiInputChannel->getHasNewPositionMeasurement() && midiInputChannel->getHasNewVelocityMeasurement()) {

        MidiPositionMeasurement pos;
        MidiVelocityMeasurement vel;
        pos.raw = midiInputChannel->getPosition();
        vel.raw = midiInputChannel->getVelocity();

        string msg = midiMessageToString(pos, vel);
        logMsg(LOG_DEBUG, "got MIDI message:  " + msg);

/*
        bool anyNoteIsOn = false;
        for (unsigned int iSwitch = 0; iSwitch < 16 && iSwitch <= indicatorRegions.size(); ++iSwitch) {
            bool switchIsOn = measmt & (1 << iSwitch);
            IndicatorRegion* indicatorRegion = indicatorRegions[iSwitch];
            if (switchIsOn) {
                if (activeIndicators.find(indicatorRegion) == activeIndicators.end()) {
                    //logMsg(LOG_DEBUG, "switch " + to_string(iSwitch) + " turned on");
                    activeIndicators.insert(indicatorRegion);
                    indicatorRegion->makeAnimating(true);
                }
                anyNoteIsOn = true;
            }
            else {
                if (activeIndicators.find(indicatorRegion) != activeIndicators.end()) {
                    //logMsg(LOG_DEBUG, "switch " + to_string(iSwitch) + " turned off");
                    activeIndicators.erase(indicatorRegion);
                    indicatorRegion->turnOffImmediately();
                }
            }
        }
        isActive = anyNoteIsOn;
*/
    }

    return isActive | animationWantsDisplay;
}

