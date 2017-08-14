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

#include <cstdint>
#include <time.h>
#include <string>


struct SchedulePeriod {
    bool isDaily;
    std::string description;
    time_t startTime;
    time_t endTime;
};


typedef enum {
    MIDI_NOTE_OFF = 0,
    MIDI_NOTE_ON,
    MIDI_POLYPHONIC_AFTERTOUCH,
    MIDI_CONTROL_CHANGE,
    MIDI_PROGRAM_CHANGE,
    MIDI_CHANNEL_AFTERTOUCH,
    MIDI_PITCH_WHEEL,
    MIDI_IS_SYSTEM_MESSAGE
} MidiChannelMessage_t;


struct MidiPositionMeasurement {
    union {
        struct {
            union {
                uint8_t l;
                struct {
                    uint8_t channelNumber : 4;
                    uint8_t channelMessageType : 3;
                    uint8_t : 1;
                };
                struct {
                    uint8_t systemMessageType : 4;
                    uint8_t systemMessageIndicator : 4;    // all 1s if system message
                };
            };
            uint8_t h;
        };
        int16_t raw;
    };
};


struct MidiVelocityMeasurement {
    union {
        struct {
            union {
                uint8_t data2;
                uint8_t velocity;
                uint8_t pressure;
                uint8_t pitchH;
            };
            union {
                uint8_t data1;
                uint8_t noteNumber;
                uint8_t controllerNumber;
                uint8_t programNumber;
                uint8_t channelPressure;
                uint8_t pitchL;
            };
        };
        int16_t raw;
    };
};


