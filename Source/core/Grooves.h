// Groove Grabber - two-bar preview grooves for hearing kits in context, one or more per genre.
#pragma once

#include "SoundTypes.h"
#include <vector>

namespace gg
{
    struct GrooveHit
    {
        double beat;       // from the start of the groove (quarter notes)
        Slot slot;
        float velocity;    // 0..1
    };

    struct Groove
    {
        juce::String id, name;
        double bpm = 100;
        int bars = 2;
        float swing = 0;
        std::vector<GrooveHit> hits;     // sorted by time, swing NOT applied
        double lengthBeats() const noexcept   { return bars * 4.0; }
    };

    const std::vector<Groove>& allGrooves();
    const Groove& grooveFor (const juce::String& id);          // falls back to the first
    double swungBeat (double beat, float swing) noexcept;      // delays the off-beat 16ths

    // A Standard MIDI File of the groove on GM drum notes (channel 10), repeated 'times'
    juce::MemoryBlock grooveToMidi (const Groove&, double bpm, float swing, int times = 2);
}
