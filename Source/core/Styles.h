// Groove Grabber - what each genre and sub-style wants from a kit, four takes on each ("directions"),
// and the eras (60s ... today) that shift a kit toward the sound of a decade.
//
// Every genre has a General style plus sub-styles (Hip-Hop: East Coast, West Coast / G-Funk, Memphis /
// Phonk, Southern / Crunk, Old School / Electro...). Each style has four directions that pull its kits
// apart on purpose, its own tempo, preview grooves, 808 and percussion choices, and the words that
// name it in a description or a pack's folder name. An era ("80s") moves the character of all four
// (80s: gated rooms, drum machines; 70s: dry, warm, live), and with a genre alone picks the style that
// era is known for ("80s pop" -> Synth Pop / 80s).
#pragma once

#include "SoundInfo.h"
#include <vector>

namespace gg
{
    struct Direction
    {
        juce::String id, name, blurb;
        Traits target;
        int use808 = -1;             // -1 = the style decides
        juce::String groove;         // preview groove id
    };

    struct Style
    {
        Genre genre = Genre::boomBap;
        juce::String id, name;       // "hiphop.west", "West Coast / G-Funk"
        juce::StringArray keywords;  // words that name it, in a description or a folder name
        std::vector<int> eras;       // the eras it's known for
        double bpm = 100;
        bool use808 = false;
        int percCount = 2;
        float swing = 0.0f;          // 0..0.6 for the preview grooves
        std::vector<PercKind> percPrefs;
        std::vector<Direction> directions;   // always 4
    };

    using GenreStyle = Style;        // (older name)

    const std::vector<Style>& stylesOf (Genre);              // [0] = General
    const Style& styleFor (Genre, int subStyle = 0);
    int styleIndex (Genre, const juce::String& id);          // 0 if unknown
    bool findStyle (const juce::String& id, Genre&, int& subStyle);   // any genre's style by id
    int directionIndex (const Style&, const juce::String& id);        // -1 if unknown
    Traits styleCenter (Genre, int subStyle = 0);            // the average of its directions
    Traits genreCenter (Genre);                              // = styleCenter (genre, 0)

    //==============================================================================
    // Eras: 0 = 60s, 1 = 70s, 2 = 80s, 3 = 90s, 4 = 2000s, 5 = 2010s, 6 = today; -1 = any
    constexpr int numEras = 7;
    juce::String eraName (int era);                          // "80s", "2000s", "Today"
    std::array<float, Traits::num> eraDelta (int era);       // how an era moves the character
    juce::StringArray eraPathWords (int era);                // folder words that suggest it ("linn", "sp1200"...)
    int styleForEra (Genre, int era);                        // the style an era is known for in a genre (0 = none)

    //==============================================================================
    // What you asked for: genre, style, era, character, options. The kit builder turns this into four kits.
    struct KitOptions
    {
        Genre genre = Genre::boomBap;
        int subStyle = 0;                    // index into stylesOf (genre)
        int era = -1;
        int leadDirection = -1;              // a direction to make kit 1 (an artist's own sound), -1 = in order
        int secondaryGenre = -1;             // a little influence from another genre (-1 = none)
        Traits character;                    // the slider values (0..1)
        int feel = 0;                        // 0 auto, 1 live, 2 hybrid, 3 programmed
        int lowEnd = -1;                     // -1 style default, 0 kick only, 1 kick + 808
        int percCount = -1;                  // -1 style default (a 23-sound kit: 3, a 32-sound kit: 4), else 0..3
        bool fx = false;
        int kitSize = defaultKitSize;        // KIT SIZE: 17, 23 or 32 sounds
        bool tuneToKey = true;
        int key = -1;
        double bpm = 0;
        std::array<bool, (size_t) Source::count> sources { true, true, true, true, true };

        const Style& style() const           { return styleFor (genre, subStyle); }
        Traits center() const;               // the style's center moved by the era (where the sliders start)
        int directionFor (int kitIndex) const;   // which of the style's directions kit 1..4 uses
        bool wants808() const;
        int wantedPerc() const;
        Traits targetFor (const Direction&) const;   // the direction moved by the sliders and feel
        juce::var toVar() const;
        static KitOptions fromVar (const juce::var&);
    };
}
