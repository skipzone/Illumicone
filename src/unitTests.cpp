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


#include <assert.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <portaudio.h>

#include "ConfigReader.h"
#include "Log.h"
#include "MeasurementMapper.h"
#include "WindchimeAudioEngine.h"

using namespace std;


Log logger;                     // this is the global Log object used everywhere


void configReaderIncludeUnitTests()
{
    cout << "----- ConfigReader include -----" << endl;

    ConfigReader config;
    assert(config.loadConfiguration("../config/unitTests/unitTests_configReaderInclude_0.json"));

    json11::Json resolved(config.getConfigObject());

    ConfigReader expect;
    assert(expect.loadConfiguration("../config/unitTests/unitTests_configReaderInclude_resolved.json"));

    cout << "    resolved result:  " << resolved.dump() << endl;
    cout << "    resolved expect:  " << expect.dumpToString() << endl;

    assert(expect.getConfigObject() == config.getConfigObject());

    cout << "    All ConfigReader include tests passed." << endl;
}


void configReaderMergeUnitTests()
{
    cout << "----- ConfigReader merge -----" << endl;

    ConfigReader config;
    assert(config.loadConfiguration("../config/unitTests/unitTests_configReaderMerge.json"));

    json11::Json primary, secondary;
    assert(ConfigReader::getJsonObject(config.getConfigObject(), "primary", primary));
    assert(ConfigReader::getJsonObject(config.getConfigObject(), "secondary", secondary));
    int i = 0;
    assert(ConfigReader::getIntValue(primary, "primary-1", i));
    assert(i == 1);
    assert(ConfigReader::getIntValue(primary, "primary-2", i));
    assert(i == 2);
    assert(ConfigReader::getIntValue(secondary, "primary-1", i));
    assert(i == 10);
    assert(ConfigReader::getIntValue(secondary, "secondary-1", i));
    assert(i == 11);
    assert(ConfigReader::getIntValue(secondary, "secondary-2", i));
    assert(i == 12);
    json11::Json merged = ConfigReader::mergeConfigObjects(primary, secondary);
    i = 0;
    assert(ConfigReader::getIntValue(merged, "primary-1", i));
    assert(i == 1);
    assert(ConfigReader::getIntValue(merged, "primary-2", i));
    assert(i == 2);
    assert(ConfigReader::getIntValue(merged, "secondary-1", i));
    assert(i == 11);
    assert(ConfigReader::getIntValue(merged, "secondary-2", i));
    assert(i == 12);
    assert(merged.object_items().size() == 4);
    //cout << merged.dump();
    cout << "    merge passed." << endl;

    cout << "    All ConfigReader merge tests passed." << endl;
}


void measurementMapperUnitTests()
{
    cout << "----- MeasurementMapper -----" << endl;

    MeasurementMapper<int, float> mapper0;
    assert(!mapper0.addRange(100, 0, 0, 10));
    cout << "    mapper0 passed." << endl;

    MeasurementMapper<int, float> mapper1;
    assert(mapper1.addRange(0, 100, 0, 10));

    float result1;

    assert(mapper1.mapMeasurement(0, result1));
    assert(result1 == 0);
    assert(mapper1.getLastRawMeasurement() == 0);
    assert(mapper1.getLastMappedMeasurement() == 0);

    assert(mapper1.mapMeasurement(50, result1));
    assert(result1 == 5);
    assert(mapper1.getLastRawMeasurement() == 50);
    assert(mapper1.getLastMappedMeasurement() == 5);

    assert(mapper1.mapMeasurement(99, result1));
    cout << "    1)  99 -> " << result1 << endl;
    assert(9.9 - result1 < 0.0001);
    assert(mapper1.getLastRawMeasurement() == 99);
    assert(9.9 - mapper1.getLastMappedMeasurement()  < 0.0001);

    assert(!mapper1.mapMeasurement(100, result1));

    assert(!mapper1.mapMeasurement(-1, result1));

    cout << "    mapper1 passed." << endl;

    MeasurementMapper<int, float> mapper2;
    assert(mapper2.addRange(0, 100, 10, 0));

    assert(mapper2.mapMeasurement(0));
    assert(mapper2.getLastRawMeasurement() == 0);
    assert(10.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(49, result1));
    assert(mapper2.getLastRawMeasurement() == 49);
    assert(5.1 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(50));
    assert(mapper2.getLastRawMeasurement() == 50);
    assert(5.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(60));
    assert(mapper2.getLastRawMeasurement() == 60);
    cout << "    2)  60 -> " << mapper2.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(99, result1));
    assert(mapper2.getLastRawMeasurement() == 99);
    assert(0.1 - mapper2.getLastMappedMeasurement()  < 0.0001);

    assert(!mapper2.mapMeasurement(100));
    assert(!mapper2.mapMeasurement(-1));

    cout << "    mapper2 passed." << endl;

    MeasurementMapper<int, float> mapper3;
    assert(mapper3.addRange(-9001, -3000, 0.1, 0.1));
    assert(mapper3.addRange(-3000,    -2, 0.1, 1.0));
    assert(mapper3.addRange(    1,  3000, 1.0, 4.0));
    assert(mapper3.addRange( 3000,  9001, 4.0, 4.0));

    assert(mapper3.mapMeasurement(-9001));
    cout << "    3)  -9001 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3001));
    cout << "    3)  -3001 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3000));
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3));
    cout << "    3)  -3 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(1.0 - mapper3.getLastMappedMeasurement() < 0.001);

    assert(mapper3.mapMeasurement(1));
    assert(1.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(2999));
    cout << "    3)  2999 -> " << mapper3.getLastMappedMeasurement() << ", diff = " << 4.0 - mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0011);

    assert(mapper3.mapMeasurement(3000));
    cout << "    3)  3000 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(9000));
    cout << "    3)  9000 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(!mapper3.mapMeasurement(-32768));
    assert(!mapper3.mapMeasurement(-9002));
    assert(!mapper3.mapMeasurement(-2));
    assert(!mapper3.mapMeasurement(-1));
    assert(!mapper3.mapMeasurement(0));
    assert(!mapper3.mapMeasurement(9001));
    assert(!mapper3.mapMeasurement(32767));

    cout << "    mapper3 passed." << endl;


    ConfigReader config;
    assert(config.loadConfiguration("../config/unitTests/unitTests_configReaderMerge.json"));

    MeasurementMapper<int, float> mapper3config;
    assert(mapper3config.readConfig(config.getConfigObject(), "mapper3", "unitTests mapper3config"));

    assert(mapper3config.mapMeasurement(-9001));
    cout << "    3)  -9001 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3001));
    cout << "    3)  -3001 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3000));
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3));
    cout << "    3)  -3 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(1.0 - mapper3config.getLastMappedMeasurement() < 0.001);

    assert(mapper3config.mapMeasurement(1));
    assert(1.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(2999));
    cout << "    3)  2999 -> " << mapper3config.getLastMappedMeasurement() << ", diff = " << 4.0 - mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0011);

    assert(mapper3config.mapMeasurement(3000));
    cout << "    3)  3000 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(9000));
    cout << "    3)  9000 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(!mapper3config.mapMeasurement(-32768));
    assert(!mapper3config.mapMeasurement(-9002));
    assert(!mapper3config.mapMeasurement(-2));
    assert(!mapper3config.mapMeasurement(-1));
    assert(!mapper3config.mapMeasurement(0));
    assert(!mapper3config.mapMeasurement(9001));
    assert(!mapper3config.mapMeasurement(32767));

    cout << "    mapper3config passed." << endl;


    MeasurementMapper<int, float> mapper4(mapper3);

    assert(mapper4.mapMeasurement(-9001));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3001));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3000));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3));
    assert(1.0 - mapper4.getLastMappedMeasurement() < 0.001);

    assert(mapper4.mapMeasurement(1));
    assert(1.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(2999));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0011);

    assert(mapper4.mapMeasurement(3000));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(9000));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(!mapper4.mapMeasurement(-32768));
    assert(!mapper4.mapMeasurement(-9002));
    assert(!mapper4.mapMeasurement(-2));
    assert(!mapper4.mapMeasurement(-1));
    assert(!mapper4.mapMeasurement(0));
    assert(!mapper4.mapMeasurement(9001));
    assert(!mapper4.mapMeasurement(32767));

    cout << "    mapper4 passed." << endl;

    MeasurementMapper<int, float> mapper5;
    assert(mapper5.addRange(-2, 1, 999, 999));
    assert(mapper5.mapMeasurement(0));
    assert(999 - mapper5.getLastMappedMeasurement() < 0.0001);
    assert(!mapper5.mapMeasurement(-3));
    assert(!mapper5.mapMeasurement(1));
    mapper5 = mapper3;

    assert(mapper5.mapMeasurement(-9001));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3001));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3000));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3));
    assert(1.0 - mapper5.getLastMappedMeasurement() < 0.001);

    assert(mapper5.mapMeasurement(1));
    assert(1.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(2999));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0011);

    assert(mapper5.mapMeasurement(3000));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(9000));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(!mapper5.mapMeasurement(-32768));
    assert(!mapper5.mapMeasurement(-9002));
    assert(!mapper5.mapMeasurement(-2));
    assert(!mapper5.mapMeasurement(-1));
    assert(!mapper5.mapMeasurement(0));
    assert(!mapper5.mapMeasurement(9001));
    assert(!mapper5.mapMeasurement(32767));

    cout << "    mapper5 passed." << endl;

    cout << "    All MeasurementMapper tests passed." << endl;
}


void windchimeAudioEngineUnitTests()
{
    cout << "----- WindchimeAudioEngine -----" << endl;

    // This test exercises the audio engine directly so we can confirm the
    // synthesis path works without requiring the live UDP receiver or a real
    // PortAudio device. It verifies that a note produces samples and that it
    // remains audible for approximately three seconds after a single trigger.
    WindchimeAudioEngine engine;
    vector<float> output(2048 * 2, 0.0f);

    engine.noteOn(1, 0, 0, 1000, true);
    engine.render(output.data(), 2048);

    bool anyNonZero = false;
    for (float sample : output) {
        if (fabs(sample) > 1e-6f) {
            anyNonZero = true;
            break;
        }
    }

    assert(anyNonZero);

    const unsigned long framesPerBlock = 512;
    const unsigned long threeSecondFrames = 48000 * 3;
    const unsigned long fullDecayFrames = 15 * 48000;
    vector<float> sustainedOutput(framesPerBlock * 2, 0.0f);

    float maxAbsSampleDuringThreeSeconds = 0.0f;
    for (unsigned long renderedFrames = 0; renderedFrames < threeSecondFrames; renderedFrames += framesPerBlock) {
        engine.render(sustainedOutput.data(), framesPerBlock);
        for (float sample : sustainedOutput) {
            maxAbsSampleDuringThreeSeconds = std::max(maxAbsSampleDuringThreeSeconds, fabs(sample));
        }
    }

    if (maxAbsSampleDuringThreeSeconds <= 1e-6f) {
        cout << "    Sample " << maxAbsSampleDuringThreeSeconds << " faded out before three seconds." << endl;
    }

    assert(maxAbsSampleDuringThreeSeconds > 1e-6f);

    vector<float> decayedOutput(framesPerBlock * 2, 0.0f);
    for (unsigned long renderedFrames = 0; renderedFrames < fullDecayFrames; renderedFrames += framesPerBlock) {
        engine.render(decayedOutput.data(), framesPerBlock);
    }

    bool decayedToNearZero = true;
    for (float sample : decayedOutput) {
        if (fabs(sample) > 1e-4f) {
            decayedToNearZero = false;
            cout << "    Sample " << sample << " not decayed to near zero." << endl;
            break;
        }
    }

    assert(decayedToNearZero);

    cout << "    windchime audio engine passed." << endl;
}


void windchimeAudioConfigUnitTests()
{
    cout << "----- WindchimeAudioConfig -----" << endl;

    WindchimeAudioEngineConfig defaultConfig;
    WindchimeAudioEngineConfig customConfig = defaultConfig;
    customConfig.fastDecayDbPerSecond = 80.0f;
    customConfig.slowDecayDbPerSecond = 6.0f;
    customConfig.tailStartDb = -30.0f;
    customConfig.floorDb = -120.0f;

    WindchimeAudioEngineConfig resonanceDisabledConfig = defaultConfig;
    resonanceDisabledConfig.secondaryResonanceGain = 0.0f;
    resonanceDisabledConfig.secondaryResonancePitchRatio = 1.0f;
    resonanceDisabledConfig.secondaryResonanceDecayDbPerSecond = defaultConfig.secondaryResonanceDecayDbPerSecond;

    WindchimeAudioEngine defaultEngine(defaultConfig);
    WindchimeAudioEngine configuredEngine(customConfig);
    WindchimeAudioEngine resonanceDisabledEngine(resonanceDisabledConfig);

    vector<float> defaultOutput(512 * 2, 0.0f);
    vector<float> configuredOutput(512 * 2, 0.0f);
    vector<float> resonanceDisabledOutput(512 * 2, 0.0f);

    defaultEngine.noteOn(1, 0, 0, 1000, true);
    configuredEngine.noteOn(1, 0, 0, 1000, true);
    resonanceDisabledEngine.noteOn(1, 0, 0, 1000, true);

    defaultEngine.render(defaultOutput.data(), 512);
    configuredEngine.render(configuredOutput.data(), 512);
    resonanceDisabledEngine.render(resonanceDisabledOutput.data(), 512);

    float defaultPeak = 0.0f;
    float configuredPeak = 0.0f;
    float resonanceDisabledPeak = 0.0f;
    for (float sample : defaultOutput) {
        defaultPeak = std::max(defaultPeak, fabs(sample));
    }
    for (float sample : configuredOutput) {
        configuredPeak = std::max(configuredPeak, fabs(sample));
    }
    for (float sample : resonanceDisabledOutput) {
        resonanceDisabledPeak = std::max(resonanceDisabledPeak, fabs(sample));
    }

    assert(configuredPeak < defaultPeak);
    assert(resonanceDisabledPeak < defaultPeak);

    for (int i = 0; i < 100; ++i) {
        defaultEngine.render(defaultOutput.data(), 512);
        configuredEngine.render(configuredOutput.data(), 512);
        resonanceDisabledEngine.render(resonanceDisabledOutput.data(), 512);
    }

    cout << "    windchime audio config passed." << endl;
}


struct AudioOutputTestState
{
    WindchimeAudioEngine* engine;
    atomic<unsigned long> callbackCount;
};


static int windchimeOutputDeviceCallback(const void* inputBuffer,
                                         void* outputBuffer,
                                         unsigned long framesPerBuffer,
                                         const PaStreamCallbackTimeInfo* timeInfo,
                                         PaStreamCallbackFlags statusFlags,
                                         void* userData)
{
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;

    AudioOutputTestState* state = static_cast<AudioOutputTestState*>(userData);
    state->engine->render(static_cast<float*>(outputBuffer), framesPerBuffer);
    ++state->callbackCount;
    return paContinue;
}


void windchimeAudioOutputDeviceUnitTests()
{
    const unsigned long expectedCallbacks = static_cast<unsigned long>(3.0 * 48000.0 / 512.0);
    const chrono::seconds testDuration(3);

    cout << "----- WindchimeAudioOutputDevice -----" << endl;

    PaError err = Pa_Initialize();
    assert(err == paNoError);

    if (Pa_GetDefaultOutputDevice() == paNoDevice) {
        cout << "    no default output device available." << endl;
        Pa_Terminate();
        return;
    }

    WindchimeAudioEngine engine;

    AudioOutputTestState state;
    state.engine = &engine;
    state.callbackCount = 0;

    PaStream* stream = nullptr;
    err = Pa_OpenDefaultStream(&stream,
                               0,
                               2,
                               paFloat32,
                               48000,
                               512,
                               windchimeOutputDeviceCallback,
                               &state);
    assert(err == paNoError);
    assert(stream != nullptr);

    err = Pa_StartStream(stream);
    assert(err == paNoError);

    engine.noteOn(1, 0, 0, 1000, true);

    auto deadline = chrono::steady_clock::now() + testDuration;
    while (chrono::steady_clock::now() < deadline) {
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    cout << "    callback count: " << state.callbackCount.load() << " / " << expectedCallbacks << endl;
    assert(state.callbackCount.load() >= expectedCallbacks);

    err = Pa_StopStream(stream);
    assert(err == paNoError);

    err = Pa_CloseStream(stream);
    assert(err == paNoError);

    Pa_Terminate();

    cout << "    windchime audio output device test passed." << endl;
}


static void printUnitTestUsage(const char* programName)
{
    cout << "Usage: " << programName << " [--skip-audio-output-test]" << endl;
    cout << "  --skip-audio-output-test   Skip the PortAudio output-device test." << endl;
}


int main(int argc, char **argv)
{
    bool skipAudioOutputTest = false;

    for (int i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg == "--skip-audio-output-test") {
            skipAudioOutputTest = true;
        }
        else if (arg == "--help" || arg == "-h") {
            printUnitTestUsage(argv[0]);
            return 0;
        }
        else {
            cerr << "Unknown option: " << arg << endl;
            printUnitTestUsage(argv[0]);
            return 1;
        }
    }

    cout << "Illumicone unit tests." << endl;

    logger.startLogging("unitTests", Log::LogTo::console);

    configReaderIncludeUnitTests();
    configReaderMergeUnitTests();
    measurementMapperUnitTests();
    windchimeAudioEngineUnitTests();
    windchimeAudioConfigUnitTests();
    if (!skipAudioOutputTest) {
        windchimeAudioOutputDeviceUnitTests();
    }
    else {
        cout << "----- WindchimeAudioOutputDevice -----" << endl;
        cout << "    skipped by command-line option." << endl;
    }

    logger.stopLogging();

    cout << "All unit tests passed." << endl;
}

