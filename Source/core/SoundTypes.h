// Groove Grabber - the kinds of sound, the kit's slots, and the pad layout every export uses.
#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace gg
{
    //==============================================================================
    // What a one-shot is
    enum class Cat : int
    {
        kick, bass808, snare, clap, rim, closedHat, openHat, tom, crash, ride, perc, fx,
        loop,       // a loop or a whole song: never put in a kit
        unknown,
        count
    };

    constexpr int numCats = (int) Cat::count;

    juce::String catName (Cat);              // "Kick", "808", "Closed Hat"...
    juce::String catId (Cat);                // "kick", "808" ... (stable, for files)
    Cat catFromId (const juce::String&);
    bool catIsKitSound (Cat) noexcept;       // everything except loop / unknown

    // Finer kinds of percussion, so a kit's perc pads are different from each other
    enum class PercKind : int { none, shaker, tambourine, conga, cowbell, snap, block, triangle, other };
    juce::String percKindName (PercKind);

    //==============================================================================
    // Genres a sound's path can be tagged with ("Trap Kit Vol 2/...") and that a song can be
    enum class Genre : int
    {
        trap, boomBap, rnb, pop, rock, country, worship, edm, lofi, cinematic,
        count
    };
    constexpr int numGenres = (int) Genre::count;
    juce::String genreName (Genre);
    juce::String genreId (Genre);
    Genre genreFromId (const juce::String&, Genre fallback = Genre::boomBap);

    //==============================================================================
    // A kit's slots, in the order they're shown
    enum class Slot : int
    {
        kick, bass808, snare, clap, rim, closedHat, openHat, crash, ride,
        tomLow, tomMid, tomHigh, perc1, perc2, perc3, fx1, fx2,
        count
    };
    constexpr int numSlots = (int) Slot::count;

    juce::String slotName (Slot);            // "Kick", "Tom Low", "Perc 2"...
    juce::String slotId (Slot);
    Slot slotFromId (const juce::String&);
    Cat slotCat (Slot) noexcept;
    bool slotIsOptional (Slot) noexcept;     // 808, perc, fx: depend on the kit's options

    //==============================================================================
    // The pad layout: General MIDI drums on 36..51 (so GM grooves and Groove Agent's own grooves play
    // the right drums), then 808, percussion and FX above. Exports number their files in this order,
    // so dragging them onto Groove Agent's first pad (C1) lands each one on the right note.
    struct PadDef
    {
        int note;                 // MIDI note
        Slot slot;                // which kit sound plays here
        float tuneOffset;         // semitones on top of the slot's tuning (the extra GM toms)
        float gainDb;             // e.g. the pedal hat is a quieter closed hat
        float lengthScale;        // e.g. the pedal hat is a shorter closed hat
        const char* label;        // file name / pad label
        int chokeGroup;           // 0 = none; hats share one so a closed hat cuts an open one
    };

    constexpr int firstPadNote = 36;
    constexpr int numPads = 22;
    const std::array<PadDef, numPads>& padLayout();
    const PadDef* padForNote (int note) noexcept;      // nullptr if not one of ours
    int mainNoteForSlot (Slot) noexcept;               // the note a slot is "on" (first pad using it)

    //==============================================================================
    // Musical keys: 0..11 = C..B major, 12..23 = C..B minor, -1 = none
    juce::String keyName (int key);                    // "A Minor", "F# Major"
    juce::String keyShortName (int key);               // "Am", "F#"
    juce::String noteName (int midiNote);              // "C1", "A#2"
    juce::String pitchClassName (int pc);
    int parseKey (const juce::String&);                // "A minor", "Am", "F# maj", "Bb" -> key, -1 if none
}
