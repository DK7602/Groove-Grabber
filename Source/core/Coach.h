// Groove Grabber - COACH MY BEAT (SAUCE): looks at a pattern you changed or built from BLANK, next to the
// style it's meant for, and says what to change. Every tip is a short, direct line, then why the style does it,
// the grid steps it's about and, where it can, a TRY IT: the pattern with the change made (SAUCE applies it as
// an edit, so UNDO takes it back).
//
// It works from what Groove Grabber already knows of the style: its own grooves (where their kick, backbeat,
// hats, ghost notes and 808 sit, how many hits a bar, whether bar 2 changes) and its SAUCE table (the tempo and
// swing ranges). No outside service: it runs on the message thread in well under a millisecond.
#pragma once

#include "Grooves.h"
#include "Styles.h"

namespace gg::coach
{
    struct StepRef { Slot slot; int step; };             // a grid cell (step 0 = bar 1's first 16th)

    enum class Area { fit, feel, space, variation };     // which rating a tip counts against

    struct Tip
    {
        juce::String id;                 // stable ("backbeat", "kick.one"...): ON PURPOSE remembers it
        bool good = false;               // something that's working (no fix)
        Area area = Area::fit;
        int weight = 0;                  // how much it matters: the biggest come first, and it comes off the rating
        juce::String title;              // the direct line ("Put the snare on 2 and 4.")
        juce::String why;                // why the style does it
        std::vector<StepRef> steps;      // the cells it's about (lit up on the grid)
        bool hasFix = false;             // TRY IT
        std::vector<GrooveHit> fixHits;  // the pattern with the change made
        float fixSwing = -1.0f;          // its swing (-1 = as it is)
    };

    struct Rating
    {
        juce::String name;               // "Groove fit", "Feel", "Space", "Variation"
        int score = 0;                   // 0..100; -1 = doesn't apply (a one-bar pattern's variation)
        juce::String word;               // "Strong", "Good", "Getting there", "Needs work"
    };

    struct Report
    {
        juce::String coachedAs;          // "East Coast / Boom Bap  -  90s  -  92 BPM"
        std::vector<Rating> ratings;
        std::vector<Tip> fixes;          // most important first (at most 6)
        std::vector<Tip> working;        // what's working (at most 2)
        int onPurpose = 0;               // tips left out because you marked them ON PURPOSE
    };

    struct Context
    {
        Genre genre = Genre::boomBap;
        int subStyle = 0;
        int era = -1;
        double tempo = 0;                // the song's (0 = not known)
    };

    // The coaching for a pattern; tips whose id is in onPurpose are left out (and don't count against it)
    Report coachBeat (const Groove&, const Context&, const juce::StringArray& onPurpose = {});
}
