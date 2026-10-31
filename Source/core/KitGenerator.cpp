#include "KitGenerator.h"
#include <cmath>
#include <unordered_map>

namespace gg
{
    namespace
    {
        inline float clamp01 (float x) noexcept               { return juce::jlimit (0.0f, 1.0f, x); }
        inline float rise (float x, float a, float b) noexcept { return clamp01 ((x - a) / (b - a)); }

        std::array<float, Traits::num> traitWeights (Cat c)
        {
            //                    punch warm  bright room  vint  aggr  organic
            switch (c)
            {
                case Cat::kick:      return { 1.2f, 0.8f, 0.8f, 0.8f, 1.0f, 0.9f, 1.0f };
                case Cat::bass808:   return { 0.8f, 0.8f, 0.5f, 0.6f, 0.7f, 0.9f, 0.2f };
                case Cat::snare:     return { 1.1f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
                case Cat::clap:      return { 1.0f, 0.6f, 1.0f, 0.9f, 1.0f, 0.9f, 0.7f };
                case Cat::rim:       return { 0.6f, 0.6f, 0.8f, 0.6f, 0.9f, 0.6f, 0.8f };
                case Cat::closedHat:
                case Cat::openHat:   return { 0.5f, 0.4f, 1.2f, 0.6f, 1.0f, 0.8f, 1.0f };
                case Cat::tom:       return { 0.9f, 0.9f, 0.7f, 1.0f, 1.0f, 0.8f, 1.2f };
                case Cat::crash:
                case Cat::ride:      return { 0.4f, 0.5f, 1.0f, 0.8f, 1.0f, 0.7f, 1.2f };
                case Cat::perc:      return { 0.6f, 0.6f, 0.8f, 0.6f, 1.0f, 0.6f, 1.0f };
                case Cat::fx:        return { 0.4f, 0.4f, 0.6f, 0.6f, 0.8f, 0.6f, 0.6f };
                default:             return { 1, 1, 1, 1, 1, 1, 1 };
            }
        }

        // what kind of effect: a build-up (riser, reverse), a hit with a tail (impact, downlifter), a texture
        // (vinyl, noise, atmos) or a short accent (scratch, stab, laser, tag). Two FX in a kit should differ.
        enum class FxKind { build, impact, texture, accent };
        FxKind fxKindOf (const SoundInfo& s)
        {
            const juce::File f (s.path);
            const auto p = " " + (f.getParentDirectory().getFileName() + " " + f.getFileNameWithoutExtension()).toLowerCase().replaceCharacters ("_-./\\()[]0123456789", "                   ") + " ";
            auto any = [&p] (std::initializer_list<const char*> ws) { for (auto* w : ws) if (p.contains (" " + juce::String (w) + " ")) return true; return false; };
            if (any ({ "riser", "risers", "rise", "uplifter", "uplift", "build", "buildup", "reverse", "reversed", "rev", "swell", "upsweep" })) return FxKind::build;
            if (any ({ "impact", "impacts", "boom", "downlifter", "downsweep", "drop", "sub drop", "slam", "thud" }))                        return FxKind::impact;
            if (any ({ "vinyl", "noise", "texture", "textures", "atmos", "atmosphere", "ambience", "ambient", "foley", "drone", "tape", "crackle" })) return FxKind::texture;
            if (any ({ "scratch", "scratches", "stab", "stabs", "laser", "zap", "glitch", "tag", "airhorn", "horn", "siren", "chop", "vox" }))  return FxKind::accent;
            if (s.attackMs > 120.0f) return FxKind::build;
            if (s.duration > 1.5f && s.decayMs > 700.0f) return s.flatness > 0.3f ? FxKind::texture : FxKind::impact;
            return FxKind::accent;
        }

        bool isMainSlot (Slot s)
        {
            return s == Slot::kick || s == Slot::snare || s == Slot::closedHat || s == Slot::bass808 || s == Slot::clap;
        }

        int scaleNearest (float midi, int key, float maxShift, const std::vector<int>& degreesWanted)
        {
            // nearest note whose pitch class is in the wanted set; -1000 if none within maxShift
            const int tonic = key % 12;
            float best = 1000;
            int bestNote = -1000;
            const int centre = (int) std::round (midi);
            for (int n = centre - 8; n <= centre + 8; ++n)
            {
                const int iv = ((n - tonic) % 12 + 12) % 12;
                bool ok = false;
                for (int d : degreesWanted) if (d == iv) ok = true;
                if (! ok) continue;
                const float dist = std::abs ((float) n - midi);
                if (dist < best && dist <= maxShift) { best = dist; bestNote = n; }
            }
            return bestNote;
        }
    }

    //==============================================================================
    KitGenerator::KitGenerator (SoundListPtr s, const KitOptions& o)
        : sounds (std::move (s)), opts (o), style (styleFor (o.genre, o.subStyle))
    {
        if (sounds == nullptr) sounds = std::make_shared<const SoundList>();
        for (int i = 0; i < (int) sounds->size(); ++i)
        {
            const auto& snd = (*sounds)[(size_t) i];
            byPath[snd.path] = i;
            if (! snd.isUsable()) continue;
            if (! opts.sources[(size_t) snd.source]) continue;
            pools[(size_t) snd.cat].push_back (i);
        }

        // a pack named after the style ("West Coast Drums", "Phonk Kit") or the era ("80s Drum Machines")
        pathBonus.assign (sounds->size(), 0.0f);
        const auto eraWords = eraPathWords (opts.era);
        if (opts.subStyle > 0 || ! eraWords.isEmpty())
            for (auto& pool : pools)
                for (int idx : pool)
                {
                    const auto lower = " " + (*sounds)[(size_t) idx].path.toLowerCase().replaceCharacters ("_-./\\", "      ") + " ";
                    float b = 0;
                    if (opts.subStyle > 0)
                        for (auto& kw : style.keywords)
                            if (lower.contains (" " + kw + " ") || lower.contains (" " + kw.removeCharacters (" ") + " ")) { b += 0.12f; break; }
                    for (auto& w : eraWords)
                        if (lower.contains (" " + w + " ")) { b += 0.08f; break; }
                    pathBonus[(size_t) idx] = b;
                }
    }

    int KitGenerator::totalUsable() const noexcept
    {
        int n = 0;
        for (auto& p : pools) n += (int) p.size();
        return n;
    }

    const SoundInfo* KitGenerator::find (const juce::String& path) const
    {
        auto it = byPath.find (path);
        return it != byPath.end() ? &(*sounds)[(size_t) it->second] : nullptr;
    }

    bool KitGenerator::slotWanted (Slot s, const Direction& dir) const
    {
        switch (s)
        {
            case Slot::bass808:
                if (opts.lowEnd >= 0) return opts.lowEnd == 1;
                return dir.use808 >= 0 ? dir.use808 == 1 : style.use808;
            case Slot::perc1: return opts.wantedPerc() >= 1;
            case Slot::perc2: return opts.wantedPerc() >= 2;
            case Slot::perc3: return opts.wantedPerc() >= 3;
            case Slot::fx1:
            case Slot::fx2:   return opts.fx;
            default:          return true;
        }
    }

    //==============================================================================
    float KitGenerator::baseScore (const SoundInfo& s, Slot slot, const Traits& target) const
    {
        const auto w = traitWeights (s.cat);
        float dist = 0, wsum = 0;
        for (int i = 0; i < Traits::num; ++i)
        {
            const float d = s.traits[i] - target[i];
            dist += w[(size_t) i] * d * d;
            wsum += w[(size_t) i];
        }
        float score = std::exp (-(dist / wsum) / 0.045f);
        if (auto it = byPath.find (s.path); it != byPath.end() && (size_t) it->second < pathBonus.size())
            score += pathBonus[(size_t) it->second];

        // genre tags in the path ("Trap Essentials/...")
        if (s.genreTags != 0)
        {
            if (s.genreTags & (1u << (int) opts.genre)) score += 0.12f;
            else if (opts.secondaryGenre >= 0 && (s.genreTags & (1u << opts.secondaryGenre))) score += 0.05f;
            else score -= 0.04f;
        }

        score += 0.1f * (s.catConfidence - 0.7f);
        score += 0.15f * (float) s.rating;

        // quality
        if (s.clipped) score -= target.aggr > 0.7f ? 0.03f : 0.12f;
        if (s.tailFloorDb > -45.0f) score += target.vintage > 0.6f ? 0.03f : -0.08f;
        if (s.leadSilenceMs > 40.0f) score -= 0.03f;

        const bool with808 = opts.wants808();
        switch (slot)
        {
            case Slot::kick:
                // with an 808 the kick has to get out of its way: short, not much ring, not much sub
                if (with808) score -= 0.15f * juce::jmax (rise (s.decayMs, 250, 560), rise (s.ringMs, 450, 900)) + 0.05f * s.subShare;
                else         score += 0.08f * rise (s.subShare + s.lowShare, 0.4f, 0.7f);
                break;
            case Slot::bass808:
                score += 0.12f * s.pitchConf;
                if (s.decayMs < 210) score -= 0.2f;
                break;
            case Slot::tomLow: case Slot::tomMid: case Slot::tomHigh:
                score += 0.08f * s.pitchConf;
                break;
            case Slot::perc1: case Slot::perc2: case Slot::perc3:
            {
                const auto& prefs = style.percPrefs;
                for (size_t i = 0; i < prefs.size(); ++i)
                    if (prefs[i] == s.percKind) { score += 0.12f * (1.0f - (float) i / 4.0f); break; }
                break;
            }
            default: break;
        }
        return score;
    }

    float KitGenerator::contextScore (const SoundInfo& s, Slot slot, const Kit& kit, const std::vector<Kit>& others) const
    {
        float score = 0;
        auto snd = [&] (Slot sl) -> const SoundInfo* { return kit[sl].isEmpty() ? nullptr : find (kit[sl].path); };

        // already in this kit? never twice
        for (int i = 0; i < numSlots; ++i)
            if ((Slot) i != slot && kit.slots[(size_t) i].path == s.path)
                return -10.0f;

        // agree on how vintage / roomy the kit is
        {
            float vSum = 0, rSum = 0;
            int n = 0;
            for (auto sl : { Slot::kick, Slot::snare, Slot::closedHat, Slot::clap })
                if (sl != slot)
                    if (auto* o = snd (sl)) { vSum += o->traits.vintage; rSum += o->traits.room; ++n; }
            if (n > 0)
            {
                score -= 0.12f * std::abs (s.traits.vintage - vSum / (float) n);
                score -= 0.05f * std::abs (s.traits.room - rSum / (float) n);
            }
        }

        switch (slot)
        {
            case Slot::snare:
            case Slot::clap:
                if (auto* k = snd (Slot::kick); k != nullptr && k->pack == s.pack) score += 0.03f;
                if (slot == Slot::clap)
                    if (auto* sn = snd (Slot::snare); sn != nullptr && sn->pack == s.pack) score += 0.03f;
                break;
            case Slot::openHat:
                if (auto* ch = snd (Slot::closedHat))
                {
                    if (ch->pack == s.pack) score += 0.12f;
                    score += 0.06f * (1.0f - clamp01 (std::abs (ch->traits.bright - s.traits.bright) * 3.0f));
                }
                break;
            case Slot::closedHat:
                if (auto* oh = snd (Slot::openHat); oh != nullptr && oh->pack == s.pack) score += 0.12f;
                break;
            case Slot::crash:
            case Slot::ride:
                if (auto* other = snd (slot == Slot::crash ? Slot::ride : Slot::crash); other != nullptr && other->pack == s.pack) score += 0.06f;
                if (auto* ch = snd (Slot::closedHat); ch != nullptr && ch->pack == s.pack) score += 0.04f;
                break;
            case Slot::tomLow: case Slot::tomMid: case Slot::tomHigh:
                for (auto sl : { Slot::tomLow, Slot::tomMid, Slot::tomHigh })
                    if (sl != slot)
                        if (auto* t = snd (sl))
                        {
                            if (t->pack == s.pack) score += 0.1f;
                            if (t->hasPitch() && s.hasPitch() && std::abs (t->midiPitch() - s.midiPitch()) < 2.0f) score -= 0.15f;
                        }
                break;
            case Slot::perc1: case Slot::perc2: case Slot::perc3:
                for (auto sl : { Slot::perc1, Slot::perc2, Slot::perc3 })
                    if (sl != slot)
                        if (auto* p = snd (sl); p != nullptr && p->percKind == s.percKind) score -= 0.2f;
                break;
            case Slot::kick:
                if (auto* b = snd (Slot::bass808)) score -= 0.08f * rise (s.decayMs, 210, 490) * b->pitchConf;
                break;
            case Slot::bass808:
                if (auto* k = snd (Slot::kick)) score -= 0.06f * rise (k->decayMs, 210, 490);
                break;
            case Slot::fx2:
                if (auto* f = snd (Slot::fx1); f != nullptr && fxKindOf (*f) == fxKindOf (s)) score -= 0.15f;
                break;
            default: break;
        }

        // keep the four kits apart: their main sounds shouldn't repeat
        if (isMainSlot (slot))
            for (auto& o : others)
                if (o[slot].path == s.path) score -= 0.4f;
        return score;
    }

    int KitGenerator::pick (Slot slot, const Kit& kit, const std::vector<Kit>& others, std::mt19937& rng, float temperature) const
    {
        const auto& pool = pools[(size_t) slotCat (slot)];
        if (pool.empty()) return -1;

        std::vector<std::pair<float, int>> scored;
        scored.reserve (pool.size());
        for (int idx : pool)
        {
            const auto& s = (*sounds)[(size_t) idx];
            const float sc = baseScore (s, slot, kit.target) + contextScore (s, slot, kit, others);
            if (sc > -5.0f) scored.emplace_back (sc, idx);
        }
        if (scored.empty()) return -1;
        const size_t keep = juce::jmin ((size_t) 24, scored.size());
        std::partial_sort (scored.begin(), scored.begin() + (long) keep, scored.end(), [] (auto& a, auto& b) { return a.first > b.first; });
        scored.resize (keep);

        if (temperature <= 0.0f) return scored.front().second;

        // softmax sampling: mostly the best, sometimes the next best
        const float top = scored.front().first;
        std::vector<double> weights;
        for (auto& [sc, idx] : scored) weights.push_back (std::exp ((sc - top) / temperature));
        std::discrete_distribution<size_t> dist (weights.begin(), weights.end());
        return scored[dist (rng)].second;
    }

    //==============================================================================
    Kit KitGenerator::generate (int directionIndex, juce::uint32 seed, float temperature, const Kit* previous, const std::vector<Kit>& others)
    {
        const auto& dir = style.directions[(size_t) juce::jlimit (0, (int) style.directions.size() - 1, directionIndex)];
        std::mt19937 rng (seed * 2654435761u + (juce::uint32) directionIndex * 40503u + 17u);

        Kit kit;
        kit.id = Kit::newId();
        // "90s" + "Westside Classic" -> "90s Westside Classic" (not when the name already says it: "70s Stadium Rock")
        kit.name = dir.name;
        if (opts.era >= 0 && ! dir.name.containsIgnoreCase (eraName (opts.era)))
            kit.name = eraName (opts.era) + " " + dir.name;
        kit.blurb = dir.blurb;
        kit.directionId = dir.id;
        kit.genre = opts.genre;
        kit.styleId = style.id;
        kit.era = opts.era;
        kit.target = opts.targetFor (dir);
        kit.key = opts.key;
        kit.bpm = opts.bpm > 0 ? opts.bpm : style.bpm;
        kit.tuneToKey = opts.tuneToKey;
        kit.created = juce::Time::currentTimeMillis();

        if (previous != nullptr)
        {
            kit.name = previous->name.isNotEmpty() ? previous->name : kit.name;
            for (int i = 0; i < numSlots; ++i)
                if (previous->slots[(size_t) i].locked)
                    kit.slots[(size_t) i] = previous->slots[(size_t) i];
        }

        static const Slot order[] = { Slot::kick, Slot::snare, Slot::closedHat, Slot::bass808, Slot::clap, Slot::openHat,
                                      Slot::rim, Slot::crash, Slot::ride, Slot::tomLow, Slot::tomMid, Slot::tomHigh,
                                      Slot::perc1, Slot::perc2, Slot::perc3, Slot::fx1, Slot::fx2 };
        bool tomsLocked = false;
        for (auto sl : { Slot::tomLow, Slot::tomMid, Slot::tomHigh })
            tomsLocked = tomsLocked || kit[sl].locked;

        for (auto slot : order)
        {
            if (kit[slot].locked) continue;
            kit[slot] = {};
            if (! slotWanted (slot, dir)) continue;
            const int idx = pick (slot, kit, others, rng, temperature);
            if (idx >= 0) kit[slot].path = (*sounds)[(size_t) idx].path;
        }

        // toms low -> high by pitch (only when none was locked in place)
        if (! tomsLocked)
        {
            std::vector<std::pair<float, juce::String>> toms;
            for (auto sl : { Slot::tomLow, Slot::tomMid, Slot::tomHigh })
                if (! kit[sl].isEmpty())
                    if (auto* s = find (kit[sl].path))
                        toms.emplace_back (s->hasPitch() ? s->midiPitch() : 12.0f * std::log2 (juce::jmax (40.0f, s->centroidHz) / 440.0f) + 69.0f, s->path);
            std::sort (toms.begin(), toms.end());
            Slot slots3[3] = { Slot::tomLow, Slot::tomMid, Slot::tomHigh };
            // two toms: low + high; one: mid
            std::vector<Slot> targets;
            if (toms.size() == 3) targets = { Slot::tomLow, Slot::tomMid, Slot::tomHigh };
            else if (toms.size() == 2) targets = { Slot::tomLow, Slot::tomHigh };
            else if (toms.size() == 1) targets = { Slot::tomMid };
            for (auto sl : slots3) kit[sl] = {};
            for (size_t i = 0; i < toms.size(); ++i) kit[targets[i]].path = toms[i].second;
        }

        tuneToKey (kit, opts.key);    // (no key: the kick to the 808)
        return kit;
    }

    std::vector<Kit> KitGenerator::generateAll (juce::uint32 seed, float temperature, const std::vector<Kit>* previous)
    {
        std::vector<Kit> kits;
        const int n = (int) style.directions.size();
        for (int d = 0; d < n; ++d)
        {
            const Kit* prev = previous != nullptr && d < (int) previous->size() ? &(*previous)[(size_t) d] : nullptr;
            kits.push_back (generate (opts.directionFor (d), seed, temperature, prev, kits));
        }
        return kits;
    }

    juce::StringArray KitGenerator::alternatives (const Kit& kit, Slot slot, int count) const
    {
        const auto& pool = pools[(size_t) slotCat (slot)];
        Kit without = kit;
        without[slot] = {};
        std::vector<std::pair<float, int>> scored;
        for (int idx : pool)
        {
            const auto& s = (*sounds)[(size_t) idx];
            if (s.path == kit[slot].path) continue;
            const float sc = baseScore (s, slot, kit.target) + contextScore (s, slot, without, {});
            if (sc > -5.0f) scored.emplace_back (sc, idx);
        }
        const size_t keep = juce::jmin ((size_t) count, scored.size());
        std::partial_sort (scored.begin(), scored.begin() + (long) keep, scored.end(), [] (auto& a, auto& b) { return a.first > b.first; });
        juce::StringArray out;
        for (size_t i = 0; i < keep; ++i) out.add ((*sounds)[(size_t) scored[i].second].path);
        return out;
    }

    //==============================================================================
    float KitGenerator::autoTuneFor (const SoundInfo& s, Slot slot, int key)
    {
        if (key < 0 || ! s.hasPitch()) return 0.0f;
        const float midi = s.midiPitch();
        static const std::vector<int> minorScale { 0, 2, 3, 5, 7, 8, 10 }, majorScale { 0, 2, 4, 5, 7, 9, 11 };
        const auto& scale = key >= 12 ? minorScale : majorScale;
        int target = -1000;
        switch (slot)
        {
            case Slot::bass808:
                target = scaleNearest (midi, key, 6.5f, { 0 });
                break;
            case Slot::kick:
                target = scaleNearest (midi, key, 2.5f, { 0, 7 });
                if (target < -999) target = scaleNearest (midi, key, 1.0f, scale);
                break;
            case Slot::tomLow: case Slot::tomMid: case Slot::tomHigh:
                target = scaleNearest (midi, key, 1.0f, scale);
                break;
            default:
                return 0.0f;
        }
        if (target < -999) return 0.0f;
        return std::round (((float) target - midi) * 100.0f) / 100.0f;
    }

    float KitGenerator::kickTo808 (const SoundInfo& kick, const SoundInfo& bass)
    {
        // the 808's own note (not the nearest semitone: an 808 can sit between notes) or its fifth
        if (! kick.hasPitch() || ! bass.hasPitch()) return 0.0f;
        const float k = kick.midiPitch(), b = bass.midiPitch();
        float best = 1000;
        for (int oct = -3; oct <= 3; ++oct)
            for (float iv : { 0.0f, 7.0f })
            {
                const float t = b + iv + 12.0f * (float) oct;
                if (std::abs (t - k) < std::abs (best)) best = t - k;
            }
        return std::abs (best) <= 2.5f ? std::round (best * 100.0f) / 100.0f : 0.0f;
    }

    void KitGenerator::tuneToKey (Kit& kit, int key) const
    {
        kit.key = key;
        for (int i = 0; i < numSlots; ++i)
        {
            auto& st = kit.slots[(size_t) i];
            st.autoTune = 0;
            if (st.isEmpty() || key < 0) continue;
            if (auto* s = find (st.path))
                st.autoTune = autoTuneFor (*s, (Slot) i, key);
        }

        // no key: the kick is still tuned to the 808, so the low end agrees with itself
        if (key < 0 && ! kit[Slot::kick].isEmpty() && ! kit[Slot::bass808].isEmpty())
            if (auto* k = find (kit[Slot::kick].path))
                if (auto* b = find (kit[Slot::bass808].path))
                    kit[Slot::kick].autoTune = kickTo808 (*k, *b);
    }
}
