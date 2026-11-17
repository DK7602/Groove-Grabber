// Groove Grabber - drop in your song (or a rough mix) and get its tempo and key, for when the song
// isn't in Key Compass.
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <array>

namespace gg
{
    struct SongFacts
    {
        bool ok = false;
        double bpm = 0;
        int key = -1;            // 0..23
        float keyConfidence = 0;
        juce::String error;
    };

    // shouldStop: checked now and then; return true to give up early
    SongFacts analyseSong (const juce::File&, const std::function<bool()>& shouldStop = {});
    double detectTempo (const float* mono, int numSamples, double sampleRate);
    int detectKey (const float* mono, int numSamples, double sampleRate, float& confidence);
    // The key (0..23) a pitch-class profile (C..B: how much of each, from audio or notes) fits best; -1 when it's
    // empty. confidence 0..1: how far ahead of the next best key it is.
    int keyFromChroma (const std::array<double, 12>& chroma, float& confidence);
}
