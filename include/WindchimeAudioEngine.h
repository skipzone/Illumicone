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

#include <mutex>
#include <vector>

// WindchimeAudioEngine converts widget measurements into a simple real-time
// synthesis voice bank. The engine is intentionally small and deterministic so
// it can be used by the standalone audio program without pulling in the full
// pattern renderer or any display-specific code.
class WindchimeAudioEngine
{
    public:

        // Construct a new engine with a fixed 48 kHz sample rate. The sample rate
        // matches the default PortAudio stream used by the standalone audio app,
        // which keeps the phase math simple and avoids resampling.
        WindchimeAudioEngine();

        // The engine owns a few per-voice fields and does not need custom cleanup.
        virtual ~WindchimeAudioEngine();

        // The engine is intentionally non-copyable so there is only a single
        // authoritative voice state shared between the UDP receiver and the audio
        // callback.
        WindchimeAudioEngine(const WindchimeAudioEngine&) = delete;
        WindchimeAudioEngine& operator =(const WindchimeAudioEngine&) = delete;

        // Start or retrigger one synthesized note. The widgetId selects the voice
        // slot, and the position and velocity are treated as the two user-facing
        // measurements that drive pitch and loudness.
        void noteOn(unsigned int widgetId, unsigned int channel, int position, int velocity, bool isActive);

        // Render one block of stereo audio into output. The caller is expected to
        // provide a buffer large enough for framesPerBuffer * 2 floats.
        void render(float* output, unsigned long framesPerBuffer);

    private:

        // VoiceState represents the mutable state for one windchime voice.
        // The envelope decays over time, the phase advances according to the
        // mapped frequency, and pan gives each voice a fixed stereo position.
        struct VoiceState
        {
            bool active;      // Whether the voice is still producing sound.
            float pan;        // Fixed stereo position between left and right.
            float gain;       // Mapped velocity, used to compute the initial level.
            float envelope;   // Current amplitude multiplier for the oscillator.
            float phase;      // Current oscillator phase in radians.
            float phaseStep;  // Phase increment per sample.
            float decayRate;  // Per-sample envelope decay coefficient.
        };

        // Map a widget position sample into a musically useful frequency range.
        // The current mapping keeps the pitch in a bright bell-like range while
        // preserving the full widget input range as a continuous control signal.
        float mapPositionToFrequency(int position) const;

        // Convert velocity into a gain value. The mapping is intentionally
        // bounded so that a strong measurement does not clip the output stream.
        float mapVelocityToGain(int velocity) const;

        // Clamp an intermediate sample to the nominal float audio range.
        float clampSample(float sample) const;

        std::vector<VoiceState> voices; // One fixed slot per widget ID.
        std::mutex mutex;               // Protects concurrent UDP and audio access.
        const unsigned int sampleRate;  // Output sample rate for the current engine.
};
