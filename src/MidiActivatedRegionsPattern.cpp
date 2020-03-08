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
#include <vector>

#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "IndicatorRegion.h"
#include "Log.h"
#include "MidiActivatedRegionsPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


MidiActivatedRegionsPattern::MidiActivatedRegionsPattern(const std::string& name)
    : IndicatorRegionsPattern(name, true)   // usesHsvModel = true
{
}


bool MidiActivatedRegionsPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(widgets)) {
        return false;
    }


    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "midiInput") {
            midiInputChannel = channelConfig.widgetChannel;
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

    }

    return true;
}


string MidiActivatedRegionsPattern::midiMessageToString(MidiPositionMeasurement pos, MidiVelocityMeasurement vel)
{
    string msg;
    uint16_t pitch;
    switch(pos.channelMessageType) {
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
        case MIDI_IS_SYSTEM_MESSAGE:
            msg = "system message, type=" + to_string(pos.systemMessageType)
                  + ", data1=" + to_string(vel.data1) + ", data2=" + to_string(vel.data2);
            break;
        default:
            msg = "???, type=" + to_string(pos.channelMessageType)
                  + ", data1=" + to_string(vel.data1) + ", data2=" + to_string(vel.data2);
    }
    if (pos.channelMessageType != MIDI_IS_SYSTEM_MESSAGE) {
        msg += ", channel=" + to_string(pos.channelNumber);
    }
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

    // Tell any indicators that we just turned on to start transitioning off,
    // thus giving them a natural decay appearance as a note decays.  Remove any
    // indicators that have fully transitioned off from the active indicators list.
    vector<IndicatorRegion*> inactiveIndicators;
    for (auto&& activeIndicator : activeIndicators) {
        if (!activeIndicator->getIsTransitioning()) {
            if (activeIndicator->getIsOn()) {
                activeIndicator->transitionOff();
            }
            else {
                // KLUDGE 2019-08-28:  After Schroeder's is active for a while, blocks appear to be turning black without being transparent.
                // We'll try to work around that problem by calling turnOffImmediately, which sets the block to the background
                // color (transparent by default).
                activeIndicator->turnOffImmediately();
                // END KLUDGE
                inactiveIndicators.push_back(activeIndicator);
            }
        }
    }
    for (auto&& inactiveIndicator : inactiveIndicators) {
        activeIndicators.erase(inactiveIndicator);
    }

    if (!midiInputChannel->getIsActive()) {
        //logger.logMsg(LOG_DEBUG, "midiInputChannel is inactive");
        if (!activeIndicators.empty()) {
            // If the widget has just gone inactive, turn off all the indicators.
            for (auto&& activeIndicator : activeIndicators) {
                activeIndicator->turnOffImmediately();
            }
            activeIndicators.clear();
        }
        // KLUDGE 2019-08-28:  After Schroeder's is active for a while, blocks appear to be turning black without being transparent.
        // We'll try to work around the problem by making sure all the regions are transparent (the default background color).
        HsvPixel transparentPixel(0, 0, 0);
        for (auto&& indicatorRegion : indicatorRegions) {
            indicatorRegion->setBackgroundColor(transparentPixel);
            indicatorRegion->turnOffImmediately();
        }
        // END KLUDGE
        isActive = false;
    }

    else {
        // We'll process a limited number of MIDI messages so that we don't
        // completely block pattern updates and displays if the widget sends
        // a message storm or starts chattering incessantly like some people.
        unsigned int midiMessageCount = 0;
        while (midiMessageCount < maxMidiMessagesPerPatternUpdate
               && midiInputChannel->getHasNewPositionMeasurement()
               && midiInputChannel->getHasNewVelocityMeasurement())
        {
            ++midiMessageCount;
            //logger.logMsg(LOG_DEBUG, "processing MIDI message #" + to_string(midiMessageCount));

            MidiPositionMeasurement pos;
            MidiVelocityMeasurement vel;
            pos.raw = midiInputChannel->getPosition();
            vel.raw = midiInputChannel->getVelocity();

            // Handle note on and note off (which is also note on with velocity 0).
            if (pos.channelMessageType == MIDI_NOTE_OFF || pos.channelMessageType == MIDI_NOTE_ON) {
                // TODO 8/13/2017 ross:  replace magic number 36 with noteNumberOffset
                unsigned int normalizedNoteNumber = vel.noteNumber - 36;
                if (normalizedNoteNumber < indicatorRegions.size()) {
                    //logger.logMsg(LOG_DEBUG, "note " + to_string(normalizedNoteNumber) + " is in range");
                    IndicatorRegion* indicatorRegion = indicatorRegions[normalizedNoteNumber];
                    if (pos.channelMessageType == MIDI_NOTE_ON && vel.velocity != 0) {
                        if (activeIndicators.find(indicatorRegion) == activeIndicators.end()) {
                            //logger.logMsg(LOG_DEBUG, "note " + to_string(normalizedNoteNumber) + " turned on");
                            activeIndicators.insert(indicatorRegion);
                        }
                        //indicatorRegion->transitionOn();
                        indicatorRegion->turnOnImmediately();
                        isActive = true;
                    }
                    else {
                        if (activeIndicators.find(indicatorRegion) != activeIndicators.end()) {
                            //logger.logMsg(LOG_DEBUG, "note " + to_string(normalizedNoteNumber) + " turned off");
                            activeIndicators.erase(indicatorRegion);
                        }
                        indicatorRegion->turnOffImmediately();
                    }
                }
                else {
                    logger.logMsg(LOG_WARNING, name + ":  Note " + to_string(vel.noteNumber)
                                        + " (normalized to " + to_string(normalizedNoteNumber)
                                        + ") is out of range.");
                }
            }

            // TODO 8/13/2017 ross:  Add support for pitch bend here.

            else {
                string msg = midiMessageToString(pos, vel);
                logger.logMsg(LOG_WARNING, name + ":  Unsupported MIDI message received:  " + msg);
            }
        }
    }

    return isActive | animationWantsDisplay;
}

