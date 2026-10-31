// Groove Grabber - builds kits from your library.
//
// For every slot it scores each sound of the right kind against the direction's target character
// (punch, warmth, brightness, room, vintage, aggression, live feel), its genre tags ("Trap Kit/..."),
// its quality (clipping, hiss) and your ratings. Then it scores how the sound fits the kit so far
// ("kit chemistry"): open hat from the same pack as the closed hat, toms that step apart in pitch,
// percussion of different kinds, a kick that leaves room for the 808, a kit that agrees on how vintage
// it is. The four kits are steered apart so they don't share their main sounds.
#pragma once

#include "Kit.h"
#include "Library.h"
#include <random>

namespace gg
{
    class KitGenerator
    {
    public:
        KitGenerator (SoundListPtr sounds, const KitOptions& options);

        // Four kits, one per direction of the genre. Locked slots of 'previous' (same order) are kept.
        std::vector<Kit> generateAll (juce::uint32 seed, float temperature = 0.035f, const std::vector<Kit>* previous = nullptr);

        // One kit for one direction (REGENERATE): unlocked slots get new sounds, staying apart from 'others'
        Kit generate (int directionIndex, juce::uint32 seed, float temperature, const Kit* previous, const std::vector<Kit>& others);

        // The next-best sounds for a slot, best first (not counting the one it has now)
        juce::StringArray alternatives (const Kit&, Slot, int count = 5) const;

        // Set every slot's autoTune for the kit's key (kick to root/fifth, 808 to root, toms to the scale)
        void tuneToKey (Kit&, int key) const;
        static float autoTuneFor (const SoundInfo&, Slot, int key);
        // With no key: semitones that put the kick on the 808's note or its fifth (0 if more than 2.5 away)
        static float kickTo808 (const SoundInfo& kick, const SoundInfo& bass);

        int usableCount (Cat c) const noexcept   { return (int) pools[(size_t) c].size(); }
        int totalUsable() const noexcept;
        const SoundInfo* find (const juce::String& path) const;

    private:
        float baseScore (const SoundInfo&, Slot, const Traits& target) const;
        float contextScore (const SoundInfo&, Slot, const Kit& partial, const std::vector<Kit>& others) const;
        int pick (Slot, const Kit& partial, const std::vector<Kit>& others, std::mt19937& rng, float temperature) const;
        bool slotWanted (Slot, const Direction&) const;

        SoundListPtr sounds;
        KitOptions opts;
        const GenreStyle& style;
        std::array<std::vector<int>, numCats> pools;
        std::unordered_map<juce::String, int> byPath;
        std::vector<float> pathBonus;      // per sound: its folders name the style or the era
    };
}
