// Groove Grabber - turns a slot into the audio you hear and export: trimmed, tuned, shortened,
// reversed and level-matched. Nothing here ever changes your original files.
//
// Level matching ("Sample Doctor" balance): every sound is brought to a standard loudness for its kind
// (kicks loud, hats quieter...), so kits from different packs sit together, and the four kits you're
// comparing play at the same loudness - the louder kit never wins just for being louder.
#pragma once

#include "SoundInfo.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include <mutex>

namespace gg
{
    struct RenderParams
    {
        float tune = 0;              // semitones
        float startMs = 0;
        float lengthPct = 100;
        float gainDb = 0;
        bool reverse = false;
        float lengthScale = 1.0f;    // extra (the pedal hat is a shorter closed hat)
        float padGainDb = 0;         // extra (the pedal hat is quieter)
        bool normalise = true;
    };

    // Decoded audio files, shared by everything in the process (most recently used kept, ~150 MB max)
    class AudioCache
    {
    public:
        struct Entry { juce::AudioBuffer<float> audio; double sampleRate = 44100; };
        std::shared_ptr<const Entry> get (const juce::String& path);
        void clear();

    private:
        std::mutex lock;
        juce::AudioFormatManager formats;
        bool formatsReady = false;
        struct Item { juce::String path; std::shared_ptr<const Entry> entry; juce::int64 lastUse; };
        std::vector<Item> items;
        size_t totalSamples = 0;
        juce::int64 counter = 0;
    };

    namespace render
    {
        float targetLoudnessDb (Cat) noexcept;

        // Render one sound. 'cat' decides the loudness it is matched to. Empty buffer if it can't be read.
        juce::AudioBuffer<float> sound (const juce::String& path, Cat cat, const RenderParams&, double outSampleRate);

        // Min / max pairs for drawing a waveform (numPoints columns)
        std::vector<std::pair<float, float>> peaks (const juce::AudioBuffer<float>&, int numPoints);

        // How much silence the renderer trims from the front (ms)
        // (threshold: below this much of the peak counts as silence; effects use a lower one, so a riser's
        // quiet start isn't cut)
        float leadingSilenceMs (const juce::AudioBuffer<float>&, double sampleRate, float threshold = 0.01f);
    }
}
