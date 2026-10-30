// Synthesised drum sounds for the tests: kicks, 808s, snares, claps, rims, hats, toms, cymbals,
// percussion and FX, in clean and "vintage" (dull + hissy) flavours.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace ggtest
{
    struct Synth
    {
        double sr = 44100;
        juce::Random rng { 1234 };

        juce::AudioBuffer<float> kick (float f0 = 55, float decay = 0.32f, float click = 0.5f);
        juce::AudioBuffer<float> bass808 (float f0 = 49, float length = 1.3f);
        juce::AudioBuffer<float> snare (float tone = 190, float decay = 0.2f, float noiseAmt = 0.8f);
        juce::AudioBuffer<float> clap (float decay = 0.16f);
        juce::AudioBuffer<float> rim();
        juce::AudioBuffer<float> hat (float decay);
        juce::AudioBuffer<float> tom (float f0, float decay = 0.45f);
        juce::AudioBuffer<float> crash (float decay = 2.4f);
        juce::AudioBuffer<float> ride (float decay = 2.0f);
        juce::AudioBuffer<float> shaker();
        juce::AudioBuffer<float> cowbell();
        juce::AudioBuffer<float> riser (float length = 2.2f);

        // dull the top end and add hiss
        void vintage (juce::AudioBuffer<float>&, float cutoffHz = 5000, float hissDb = -52);

        static bool write (const juce::File&, const juce::AudioBuffer<float>&, double sr);
    };
}
