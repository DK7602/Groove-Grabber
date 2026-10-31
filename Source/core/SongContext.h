// Groove Grabber - what the song is: read from the description you type, and from Key Compass.
#pragma once

#include "References.h"

namespace gg
{
    // "Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor", "like Dr. Dre, 92 BPM"
    struct ParsedDescription
    {
        // what the words themselves say, before any artist or song is read in
        struct Said
        {
            bool hasGenre = false;
            Genre genre = Genre::boomBap;
            int subStyle = -1;           // a style named ("g-funk"), -1 = none
            int era = -1;
            int secondaryGenre = -1;
            std::array<float, Traits::num> nudge {};
            int lowEnd = -1;
        } said;
        std::vector<ReferenceHit> refs; // artists and songs named, in order
        juce::StringArray extras;       // character words, BPM, key (shown after the genre)

        // the result (resolveDescription): the words plus the artist or song, in the genre that applies
        bool hasGenre = false;
        Genre genre = Genre::boomBap;
        int subStyle = -1;               // a named style ("g-funk" -> Hip-Hop: West Coast / G-Funk), -1 = none
        bool styleFromEra = false;       // picked because of the era ("80s pop" -> Synth Pop / 80s)
        int era = -1;                    // "80s" -> 2, -1 = not said
        int secondaryGenre = -1;         // "...with a little hip-hop influence"
        int lead = -1;                   // a direction for kit 1: the artist's own sound (-1 = in order)
        bool referenceInGenre = false;   // the artist or song has a take in this genre (else: just their feel)
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

    // Work out genre, style, era and character from what was said plus any artist or song named:
    //  - a genre typed in the text wins; with keepGenre (you just picked one) that one does;
    //  - otherwise the genre you picked by hand, or the current one when the artist has a take in it,
    //    or else the artist's own genre ("Morgan Wallen" -> Country);
    //  - in a genre the artist has a take for: their style, era, kit-1 direction and character;
    //    in another: just their era and character, plus a little of their genre.
    // parseDescription already resolves with no genre picked.
    void resolveDescription (ParsedDescription&, int currentGenre = -1, bool genreByHand = false, bool keepGenre = false);

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
