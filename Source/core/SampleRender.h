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
        // FIT: make it last fitBeats at fitBpm, with its end (0), loudest point (1) or start (2) on the bar line
        // (see SlotState::fitBars); 0 = as it is
        double fitBeats = 0, fitBpm = 0;
        int fitAnchor = 0;
        bool fitStretch = true;
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

        // FIT, on audio already rendered: afterwards the anchor (0 the end, 1 the loudest point, 2 the start) is exactly
        // `seconds` from the start (0, 1), or the sound lasts `seconds` (2). Too long: trimmed (from the front for 0 and
        // 1, so a build-up keeps its end; at the back for 2, faded). Too short: stretched without changing its pitch
        // (Signalsmith Stretch), at most 4 times, the rest made up with silence in front (0, 1); with stretch off, only
        // the silence. Returns how much it was stretched (1 = not).
        double fitTo (juce::AudioBuffer<float>&, double sampleRate, double seconds, int anchor, bool stretch);
        int anchorSample (const juce::AudioBuffer<float>&, int anchor);   // where the end (-40 dB) / loudest point is

        // Min / max pairs for drawing a waveform (numPoints columns)
        std::vector<std::pair<float, float>> peaks (const juce::AudioBuffer<float>&, int numPoints);

        // How much silence the renderer trims from the front (ms)
        // (threshold: below this much of the peak counts as silence; effects use a lower one, so a riser's
        // quiet start isn't cut)
        float leadingSilenceMs (const juce::AudioBuffer<float>&, double sampleRate, float threshold = 0.01f);
    }
}
