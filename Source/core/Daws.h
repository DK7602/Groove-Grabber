// Groove Grabber - getting an exported kit into each DAW's own drum sampler: the steps, what the DAW calls
// the notes (C1 in Cubase is C3 in FL Studio), how to make the hats cut each other off, how to keep the kit.
//
// The plug-in picks the guide for the DAW it's running in; the standalone app (or anyone who likes)
// picks one by hand in the export panel. "other" is the general guide for any sampler.
#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace gg
{
    struct DawGuide
    {
        juce::String id;              // "cubase", "ableton" ... "other"
        juce::String name;            // "Cubase / Nuendo"
        juce::String sampler;         // "Groove Agent SE"
        juce::String dragLabel;       // the export panel's drag handle: "DRAG TO GROOVE AGENT - PAD 1 (C1)"
        juce::String dragHint;        // one line under it
        int middleCOctave = 3;        // the octave number this DAW gives MIDI note 60 (C3 = 60 -> note 36 is C1)
        juce::StringArray steps;      // loading steps, plain text with **bold**
        juce::String hats;            // making closed / pedal / open hats cut each other off
        juce::String keep;            // saving it as a kit you can load anywhere
        juce::String groove;          // where the groove MIDI goes
    };

    const std::vector<DawGuide>& dawGuides();          // [0] = Cubase ... last = "other"
    const DawGuide& dawGuide (const juce::String& id); // "other" if unknown
    juce::String detectedDaw();                        // the host's guide id, empty if not known (or the app)
    juce::String chosenDaw (const juce::String& setting);   // the setting ("" = automatic) -> a guide id

    // A MIDI note as this DAW names it: 36 -> "C1" (Cubase), "C3" (FL Studio), "C2" (REAPER)
    juce::String dawNoteName (int midiNote, const DawGuide&);

    // The whole guide as plain text (Kit Info.txt) and as the manual's markdown
    juce::String dawGuideText (const DawGuide&, int numPads, const juce::String& lastFileName);
    juce::String dawGuidesMarkdown();                  // every DAW, for the manual ({DAW_GUIDES})
}
