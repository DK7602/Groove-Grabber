// Groove Grabber - artists and songs you can name in a description: "like Dr. Dre", "Still D.R.E. vibe",
// "Metro Boomin type beat", "Bon Jovi drums", "Morgan Wallen".
//
// Each one is a built-in map from the name to how its drums are known to sound, per genre: the style
// (Dr. Dre -> West Coast / G-Funk), the era (90s), the direction that becomes kit 1 (Westside Knock) and a
// few describing words (punchy, clean, dry) that move the sliders. Nothing is downloaded or listened to:
// a name that isn't in the list is simply ignored. An artist with more than one sound (Post Malone,
// Taylor Swift) has a take per genre, and the genre you picked decides which one is used.
#pragma once

#include "Styles.h"

namespace gg
{
    struct ReferenceTake                 // what an artist or song means inside one genre
    {
        juce::String styleId;            // "hiphop.west"
        Genre genre = Genre::boomBap;    // (from the style)
        int subStyle = 0;                // (from the style)
        int era = -1;                    // -1 = no particular era
        int lead = -1;                   // a direction (index in the style) for kit 1, -1 = none
        juce::String sound;              // describing words: "punchy clean dry 808"
    };

    struct Reference
    {
        juce::String name;               // "Dr. Dre", "Still D.R.E."
        juce::String by;                 // songs: the artist ("Dr. Dre"); empty for artists
        std::vector<ReferenceTake> takes;   // [0] = their main sound
        bool isSong() const              { return by.isNotEmpty(); }
        const ReferenceTake* takeFor (Genre) const;   // nullptr if none in that genre
        juce::String label() const;      // "Dr. Dre", "Still D.R.E. (Dr. Dre)"
    };

    const std::vector<Reference>& allReferences();

    struct ReferenceHit { int index = -1; int start = 0, end = 0; };   // into allReferences(); a span of the text

    // Names in normalised text (normaliseText). A name that's also a common word ("Future", "Prince",
    // "Queen") counts only with a cue next to it: "like Future", "Future type beat", "in the style of Prince".
    // `blocked`: phrases that are never names, even when one contains a name ("future bass").
    std::vector<ReferenceHit> findReferences (const juce::String& text, const juce::StringArray& blocked);

    // " lower case words " - letters, digits, & and # kept, apostrophes dropped ("90's" -> "90s"), the rest spaces
    juce::String normaliseText (const juce::String&);

    // Any table line that didn't parse, or names a style / direction that doesn't exist (for the tests)
    juce::StringArray referenceTableProblems();
}
