// Groove Grabber - what each genre wants from a kit, and four different takes on it ("directions").
//
// Each genre has four directions that pull its kits apart on purpose (e.g. Rock: Studio Tight, Arena
// Thunder, Garage Grit, Modern Hybrid), so GENERATE gives four real choices instead of four near-copies.
// The kit character sliders move all four together; the directions keep their distance.
#pragma once

#include "SoundInfo.h"
#include <vector>

namespace gg
{
    struct Direction
    {
        juce::String id, name, blurb;
        Traits target;
        int use808 = -1;             // -1 = the genre decides
        juce::String groove;         // preview groove id
    };

    struct GenreStyle
    {
        Genre genre;
        double bpm = 100;
        bool use808 = false;
        int percCount = 2;
        float swing = 0.0f;          // 0..0.6 for the preview grooves
        std::vector<PercKind> percPrefs;
        std::vector<Direction> directions;   // always 4
    };

    const GenreStyle& styleFor (Genre);
    Traits genreCenter (Genre);             // the average of its directions = the sliders' starting point

    //==============================================================================
    // What you asked for: genre, character, options. The kit builder turns this into four kits.
    struct KitOptions
    {
        Genre genre = Genre::boomBap;
        int secondaryGenre = -1;             // a little influence from another genre (-1 = none)
        Traits character;                    // the slider values (0..1)
        int feel = 0;                        // 0 auto, 1 live, 2 hybrid, 3 programmed
        int lowEnd = -1;                     // -1 genre default, 0 kick only, 1 kick + 808
        int percCount = -1;                  // -1 genre default, else 0..3
        bool fx = false;
        bool tuneToKey = true;
        int key = -1;
        double bpm = 0;
        std::array<bool, (size_t) Source::count> sources { true, true, true, true, true };

        bool wants808() const;
        int wantedPerc() const;
        Traits targetFor (const Direction&) const;   // the direction moved by the sliders and feel
        juce::var toVar() const;
        static KitOptions fromVar (const juce::var&);
    };
}
