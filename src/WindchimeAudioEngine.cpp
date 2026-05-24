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

#include <algorithm>
#include <cmath>
#include <cstring>

#include "WindchimeAudioEngine.h"

// Construct the engine and initialize all 32 voice slots. The fixed voice count
// mirrors the number of widget IDs used by the UDP receiver and gives us a
// stable mapping between incoming widget packets and synthesized output.
WindchimeAudioEngine::WindchimeAudioEngine()
    : sampleRate(48000)
{
    voices.resize(32);
    for (unsigned int i = 0; i < voices.size(); ++i) {
        voices[i].active = false;

        // The pan value spreads the voices across the stereo field. The values
        // are not meant to be perceptually exact; they simply keep multiple
        // simultaneous events from collapsing into a single center image.
        voices[i].pan = -0.65f + (static_cast<float>(i % 8) * 0.18f);

        voices[i].gain = 0.0f;
        voices[i].envelope = 0.0f;
        voices[i].phase = 0.0f;
        voices[i].phaseStep = 0.0f;

        // A longer decay keeps the note audible for a few seconds so it feels
        // like a lingering windchime strike.
        voices[i].decayRate = 0.9999f;
    }
}


WindchimeAudioEngine::~WindchimeAudioEngine()
{
}


// The UDP payload carries the raw widget measurements. This method translates
// those measurements into the internal oscillator state so the audio callback can
// render the current sound in real time.
void WindchimeAudioEngine::noteOn(unsigned int widgetId, unsigned int channel, int position, int velocity, bool isActive)
{
    // Ignore inactive packets and any widget IDs outside the voice table.
    if (!isActive || widgetId >= voices.size()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    VoiceState& voice = voices[widgetId];

    // Velocity controls how loud the attack is, while position selects the
    // pitched component. We keep the gain bounded so the output stays within the
    // float audio range even when the widget reports large values.
    voice.gain = mapVelocityToGain(velocity);
    voice.envelope = std::max(0.05f, std::min(0.25f, voice.gain * 0.65f));
    voice.phase = 0.0f;
    voice.phaseStep = 2.0f * static_cast<float>(M_PI) * mapPositionToFrequency(position) / sampleRate;
    voice.decayRate = 0.9999f;
    voice.active = true;

    // The channel field is currently not used by the synthesis path, but it is
    // preserved in the API so the engine can evolve without changing the UDP
    // protocol boundary.
    (void) channel;
}


// Render one block of stereo audio. We clear the buffer first, then accumulate
// all active voices so overlapping notes can be heard together. Each voice uses a
// simple sine oscillator with an exponential envelope.
void WindchimeAudioEngine::render(float* output, unsigned long framesPerBuffer)
{
    std::memset(output, 0, framesPerBuffer * 2 * sizeof(float));

    std::lock_guard<std::mutex> lock(mutex);

    for (unsigned int voiceIdx = 0; voiceIdx < voices.size(); ++voiceIdx) {
        VoiceState& voice = voices[voiceIdx];
        if (!voice.active) {
            continue;
        }

        for (unsigned long sampleIdx = 0; sampleIdx < framesPerBuffer; ++sampleIdx) {
            float sample = std::sin(voice.phase) * voice.envelope;
            float left = sample * (1.0f - voice.pan) * 0.5f;
            float right = sample * (1.0f + voice.pan) * 0.5f;

            // Accumulate each voice contribution into the shared stereo buffer.
            output[sampleIdx * 2] += clampSample(left);
            output[sampleIdx * 2 + 1] += clampSample(right);

            // Advance the oscillator phase and decay the envelope for the next
            // sample. The wraparound keeps phase math bounded and avoids drift.
            voice.phase += voice.phaseStep;
            if (voice.phase > 2.0f * static_cast<float>(M_PI)) {
                voice.phase -= 2.0f * static_cast<float>(M_PI);
            }

            voice.envelope *= voice.decayRate;
            if (voice.envelope < 1e-5f) {
                voice.active = false;
                voice.envelope = 0.0f;
                break;
            }
        }
    }
}


// Map the raw widget position term into a frequency range that sounds like a
// suspended chime. The math preserves the input domain while translating it into
// a comfortable, audible pitch range.
float WindchimeAudioEngine::mapPositionToFrequency(int position) const
{
    float normalized = (static_cast<float>(position) + 32768.0f) / 65535.0f;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    return 220.0f + normalized * (1760.0f - 220.0f);
}


// Velocity is treated as a simple amplitude control. The input can be negative
// or positive, so we normalize its magnitude and then clamp the result.
float WindchimeAudioEngine::mapVelocityToGain(int velocity) const
{
    float normalized = std::abs(static_cast<float>(velocity)) / 32768.0f;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    return 0.04f + normalized * 0.80f;
}


// Clamp a single sample to the nominal full-scale range used by the audio
// callback. This protects the output from accidental overshoot when multiple
// voices overlap.
float WindchimeAudioEngine::clampSample(float sample) const
{
    return std::max(-1.0f, std::min(1.0f, sample));
}
