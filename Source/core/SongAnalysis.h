// Groove Grabber - drop in your song (or a rough mix) and get its tempo and key, for when the song
// isn't in Key Compass.
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

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

    SongFacts analyseSong (const juce::File&);
    double detectTempo (const float* mono, int numSamples, double sampleRate);
    int detectKey (const float* mono, int numSamples, double sampleRate, float& confidence);
}
