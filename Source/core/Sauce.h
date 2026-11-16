// Groove Grabber - SAUCE: how the drums go in each genre, style and decade, for building your own
// patterns in your DAW. The write-ups live in Resources/Sauce.md, one section per key:
//
//   @@ hiphop.east          a style (its id from Styles.cpp; a genre's General style is its plain id, "hiphop")
//   @@ era.2                a decade (0 = 60s ... 6 = today): how drums were made and sounded then
//   @@ era.2.rock           that decade in one genre (the genre's General style id)
//
// Each section is the simple markdown the manual uses (### headings, **bold**, - lists, | tables |).
#pragma once

#include "Styles.h"
#include "Grooves.h"

namespace gg
{
    // One section's text, empty if there is none
    juce::String sauceSection (const juce::String& key);
    juce::StringArray sauceKeys();                                   // every key in the file (for the tests)

    // The whole write-up for a style in a decade (era -1 = any): the style, then the decade in that genre
    juce::String sauceMarkdown (Genre, int subStyle, int era);

    // The preview grooves a style uses (its four directions' grooves, each once, in order)
    std::vector<const Groove*> sauceGrooves (const Style&);

    // The style's table: "85 to 98 BPM | 54 to 62% | Swung 16ths, laid back | Kick and a sampled bass line"
    // (0s when a range isn't there)
    struct SauceTable
    {
        int tempoLo = 0, tempoHi = 0, swingLo = 0, swingHi = 0;
        juce::String timeFeel, lowEnd;
        bool hasTempo() const noexcept   { return tempoLo > 0 && tempoHi >= tempoLo; }
        bool hasSwing() const noexcept   { return swingLo > 0 && swingHi >= swingLo; }
    };
    SauceTable sauceTable (const Style&);

    // "Kick + 808", "Tambourine, shaker", "90s" ... the facts at a glance
    juce::String sauceLowEnd (const Style&);
    juce::String saucePercussion (const Style&);
    juce::String sauceEras (const Style&);
}
