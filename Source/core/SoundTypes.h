// Groove Grabber - the kinds of sound, the kit's slots, and the pad layout every export uses.
#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace gg
{
    //==============================================================================
    // What a one-shot is
    enum class Cat : int
    {
        kick, bass808, snare, clap, rim, closedHat, openHat, tom, crash, ride, perc, fx,
        loop,       // a loop or a whole song: never put in a kit
        unknown,
        melodic,    // vocals, chants, synths, keys, guitars, bass loops: never put in a kit
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
    juce::String genreFolderName (Genre);     // short, safe for a folder: "Hip-Hop", "R&B", "EDM"
    juce::String genreId (Genre);
    Genre genreFromId (const juce::String&, Genre fallback = Genre::boomBap);

    //==============================================================================
    // A kit's slots. The first 17 are every kit's (one of each kind); the rest are the second sounds and the
    // extra percussion and FX of the bigger kits (KIT SIZE 23 and 32). Never reorder: kits, grooves and the
    // pad layout name them by id, and the numbers stay put.
    enum class Slot : int
    {
        kick, bass808, snare, clap, rim, closedHat, openHat, crash, ride,
        tomLow, tomMid, tomHigh, perc1, perc2, perc3, fx1, fx2,
        kick2, bass808_2, snare2, clap2, rim2, closedHat2, openHat2, crash2, ride2,
        tomLow2, tomMid2, tomHigh2, perc4, fx3, fx4,
        count
    };
    constexpr int numSlots = (int) Slot::count;

    juce::String slotName (Slot);            // "Kick", "Tom Low", "Perc 2", "Kick 2"...
    juce::String slotId (Slot);
    Slot slotFromId (const juce::String&);
    Cat slotCat (Slot) noexcept;
    bool slotIsOptional (Slot) noexcept;     // 808, perc, fx: depend on the kit's options
    Slot slotKind (Slot) noexcept;           // the first slot of its kind: Kick 2 -> Kick, Perc 4 -> Perc 1, FX 3 -> FX 1
    bool isSecondSound (Slot) noexcept;      // Kick 2, 808 2 ... Tom High 2 (a second sound of a kind the kit has)

    // KIT SIZE: how many sounds a kit holds. 17 = one of each kind (the default), 23 = two kicks, 808s, snares,
    // closed and open hats and three FX, 32 = two of everything, four percussion and four FX.
    constexpr int defaultKitSize = 17;
    int validKitSize (int) noexcept;                     // 17, 23 or 32 (anything else: 17)
    const std::vector<Slot>& kitSlots (int kitSize);     // the kit's slots, in the order they're shown
    bool slotInKit (Slot, int kitSize) noexcept;
    int slotRank (Slot) noexcept;                        // where it's shown among all 32 (Kick, Kick 2, 808, 808 2...)
    int padCountFor (int kitSize) noexcept;              // the pads (and export files) it uses: 22, 27 or 33

    //==============================================================================
    // The pad layout: General MIDI drums on 36..51 (so GM grooves and Groove Agent's own grooves play
    // the right drums), then 808, percussion and FX above. Exports number their files in this order,
    // so dragging them onto Groove Agent's first pad (C1) lands each one on the right note.
    //
    // The bigger kits' second sounds: Snare 2 and the second toms take General MIDI's own pads for them (40, 41,
    // 45, 48), where a 17-sound kit plays its snare and toms tuned down or a little quieter ("stand-ins"), and
    // the rest go on above, the 23-sound kit's first: 58 Kick 2 ... 62 FX 3, then 63 Clap 2 ... 68 FX 4. So a
    // 17-sound kit exports exactly the 22 files it always has, a 23-sound kit 27 and a 32-sound kit 33.
    struct PadDef
    {
        int note;                 // MIDI note
        Slot slot;                // which kit sound plays here
        float tuneOffset;         // semitones on top of the slot's tuning (the extra GM toms)
        float gainDb;             // e.g. the pedal hat is a quieter closed hat
        float lengthScale;        // e.g. the pedal hat is a shorter closed hat
        const char* label;        // file name / pad label
        int chokeGroup;           // 0 = none; hats share one so a closed hat cuts an open one
        Slot standIn = Slot::count;   // when the kit has no sound in `slot`: this one plays, shaped by the
                                      // offsets above (no stand-in: the offsets always apply)
    };

    constexpr int firstPadNote = 36;
    constexpr int numPads = 33;                        // every pad any kit uses (36..68)
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
