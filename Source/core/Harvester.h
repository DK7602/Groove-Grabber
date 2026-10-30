// Groove Grabber - harvests one-shots from a drums-only recording: a bounce of a beat you made, a
// drum stem, a break, or a "sampling pass" (each drum hit on its own at soft, medium and hard).
//
// Finds every hit, works out what it is, groups hits of the same drum together and keeps the cleanest
// ones - up to three loudness versions (soft / medium / hard) of each different drum. Overlapping hits
// (a kick under a hat) are ranked lower. Full mixes with vocals and bass should go through Sample
// Snagger's stem split first; its drum stem can then be harvested here.
#pragma once

#include "SoundInfo.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>

namespace gg
{
    struct HarvestResult
    {
        bool ok = false;
        juce::String error;
        juce::File folder;
        juce::Array<juce::File> files;
        int hitsFound = 0;
    };

    // progress: 0..1; return false from it to cancel
    HarvestResult harvestHits (const juce::File& audioFile, const juce::File& destinationRoot,
                               const std::function<bool (float)>& progress = {});

    // The onsets (sample positions) in mono audio. Exposed for tests.
    std::vector<int> findOnsets (const float* mono, int numSamples, double sampleRate);
}
