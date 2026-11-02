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
        bool swingEighths = false;       // swing moves the off-beat 8ths (a shuffle), not the 16ths
        std::vector<GrooveHit> hits;     // sorted by time, swing NOT applied
        double lengthBeats() const noexcept   { return bars * 4.0; }
    };

    const std::vector<Groove>& allGrooves();
    const Groove& grooveFor (const juce::String& id);          // falls back to the first
    double swungBeat (double beat, float swing, bool eighths = false) noexcept;   // delays the off-beat 16ths (or 8ths)

    // A Standard MIDI File of the groove on GM drum notes, repeated 'times'. Channel 1, which every sampler
    // listens to (the old GM drum channel 10 left Groove Agent silent on a plain MIDI track).
    juce::MemoryBlock grooveToMidi (const Groove&, double bpm, float swing, int times = 2);

    //==============================================================================
    // Editing a groove a step at a time (the SAUCE grid). A step of one lane holds 0 hits, 1, 2 (a quick
    // double) or 3 (a triplet burst); the first hit's velocity is the step's level.
    struct StepHits { int count = 0; float velocity = 0; };
    constexpr float hardHit = 1.0f, mediumHit = 0.72f, softHit = 0.42f;
    StepHits stepHits (const Groove&, Slot, int step);
    std::vector<GrooveHit> withStep (const Groove&, Slot, int step, int count, float velocity);   // count 0 empties it
    float nextLevel (float velocity) noexcept;                       // hard -> medium -> soft -> hard
    void sortHits (std::vector<GrooveHit>&);
    bool sameHits (std::vector<GrooveHit>, std::vector<GrooveHit>);
    juce::var hitsToVar (const std::vector<GrooveHit>&);             // [[beat, "kick", velocity], ...]
    std::vector<GrooveHit> hitsFromVar (const juce::var&, const Groove& original);   // checked: on the grid, known drums
}
