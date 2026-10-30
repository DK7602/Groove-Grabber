// Groove Grabber - a kit: which sound sits in each slot and how it's shaped, plus saving kits to disk.
#pragma once

#include "Styles.h"

namespace gg
{
    struct SlotState
    {
        juce::String path;           // empty = no sound
        float autoTune = 0;          // semitones, set by "tune to key"
        float userTune = 0;          // semitones, your TUNE knob
        float startMs = 0;           // skip into the sound
        float lengthPct = 100;       // how much of it plays (fades out at the end)
        float gainDb = 0;
        bool reverse = false;
        bool locked = false;         // kept by REGENERATE

        float tune (bool keyTuning) const noexcept   { return (keyTuning ? autoTune : 0.0f) + userTune; }
        bool isEmpty() const noexcept                { return path.isEmpty(); }
        bool sameSound (const SlotState& o) const noexcept;
        juce::var toVar() const;
        static SlotState fromVar (const juce::var&);
    };

    struct Kit
    {
        juce::String id, name, blurb, directionId;
        Genre genre = Genre::boomBap;
        Traits target;
        int key = -1;
        double bpm = 0;
        bool tuneToKey = true;
        bool favorite = false;
        juce::String description;
        juce::int64 created = 0;
        std::array<SlotState, numSlots> slots;

        SlotState& operator[] (Slot s) noexcept               { return slots[(size_t) s]; }
        const SlotState& operator[] (Slot s) const noexcept   { return slots[(size_t) s]; }
        int numFilled() const noexcept;
        bool isValid() const noexcept                         { return numFilled() > 0; }

        juce::var toVar() const;
        static Kit fromVar (const juce::var&);
        static juce::String newId();
    };

    // Your saved kits (Documents/Groove Grabber/Kits/*.ggkit) and the recent ones (app data)
    namespace kitstore
    {
        juce::File save (const Kit&);                         // returns the file
        std::vector<std::pair<juce::File, Kit>> listSaved();
        bool remove (const juce::File&);
        void addToHistory (const std::vector<Kit>&);
        std::vector<Kit> history();
    }
}
