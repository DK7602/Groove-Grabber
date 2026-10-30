// Groove Grabber - what the song is: read from the description you type, and from Key Compass.
#pragma once

#include "Styles.h"

namespace gg
{
    // "Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor"
    struct ParsedDescription
    {
        bool hasGenre = false;
        Genre genre = Genre::boomBap;
        int secondaryGenre = -1;         // "...with a little hip-hop influence"
        std::array<float, Traits::num> nudge {};    // added to the genre's center, -1..1
        int feel = 0;                    // 0 = not said, 1 live, 2 hybrid, 3 programmed
        int lowEnd = -1;                 // -1 not said, 0 no 808, 1 with 808
        int fx = -1;                     // -1 not said
        int percCount = -1;
        double bpm = 0;
        int key = -1;
        juce::StringArray understood;    // the words it acted on, to show you
    };

    ParsedDescription parseDescription (const juce::String&);

    // Apply a description to options: genre, sliders (genre center + nudges), toggles, key, BPM
    void applyDescription (const ParsedDescription&, KitOptions&);

    //==============================================================================
    // A song from Key Compass's "My Songs" folder
    struct KeyCompassSong
    {
        juce::String name;
        int key = -1;                    // 0..23, or worked out from the chords
        double bpm = 0;
        juce::StringArray sections;
        juce::Time modified;
        juce::File file;
    };

    std::vector<KeyCompassSong> listKeyCompassSongs (const juce::File& folder, int maxSongs = 12);
    std::optional<KeyCompassSong> readKeyCompassSong (const juce::File&);

    // The key a list of chord names most likely belongs to ("Am F C G" -> A minor)
    int keyFromChords (const juce::StringArray& chords);
}
