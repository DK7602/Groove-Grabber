#include "Coach.h"
#include "Sauce.h"
#include <cmath>
#include <functional>

namespace gg::coach
{
    namespace
    {
        constexpr int barSteps = 16;
        constexpr float ghostMax = 0.55f;        // softer than this off the backbeat: a ghost note
        constexpr float backbeatMin = 0.6f;      // at least this hard on the backbeat: the backbeat

        using Lanes = std::vector<Slot>;
        const Lanes kickLanes  { Slot::kick, Slot::kick2 };
        const Lanes backLanes  { Slot::snare, Slot::snare2, Slot::clap, Slot::clap2, Slot::rim, Slot::rim2 };
        const Lanes hatLanes   { Slot::closedHat, Slot::closedHat2 };
        const Lanes openLanes  { Slot::openHat, Slot::openHat2 };
        const Lanes lowLanes   { Slot::bass808, Slot::bass808_2 };
        const Lanes rideLanes  { Slot::ride, Slot::ride2 };
        const Lanes crashLanes { Slot::crash, Slot::crash2 };
        const Lanes tomLanes   { Slot::tomLow, Slot::tomMid, Slot::tomHigh, Slot::tomLow2, Slot::tomMid2, Slot::tomHigh2 };
        const Lanes percLanes  { Slot::perc1, Slot::perc2, Slot::perc3, Slot::perc4 };
        const Lanes fxLanes    { Slot::fx1, Slot::fx2, Slot::fx3, Slot::fx4 };

        //==============================================================================
        // A pattern on the grid: for each sound and 16th step, the hardest hit there and how many (a roll)
        struct Grid
        {
            int bars = 1, steps = barSteps;
            std::array<std::vector<float>, numSlots> vel;
            std::array<std::vector<int>, numSlots> count;

            explicit Grid (const Groove& g)
            {
                bars = juce::jlimit (1, 8, g.bars);
                steps = bars * barSteps;
                for (auto& v : vel) v.assign ((size_t) steps, 0.0f);
                for (auto& c : count) c.assign ((size_t) steps, 0);
                for (const auto& h : g.hits)
                {
                    const int st = (int) std::floor (h.beat * 4.0 + 1.0e-6);
                    const int s = (int) h.slot;
                    if (st < 0 || st >= steps || s < 0 || s >= numSlots) continue;
                    vel[(size_t) s][(size_t) st] = juce::jmax (vel[(size_t) s][(size_t) st], h.velocity);
                    ++count[(size_t) s][(size_t) st];
                }
            }

            float at (const Lanes& lanes, int step) const
            {
                if (step < 0 || step >= steps) return 0.0f;
                float v = 0;
                for (auto s : lanes) v = juce::jmax (v, vel[(size_t) s][(size_t) step]);
                return v;
            }
            bool on (const Lanes& lanes, int step, float minVel = 0.01f) const   { return at (lanes, step) >= minVel; }
            int stepsWith (const Lanes& lanes, int from = 0, int to = -1) const
            {
                if (to < 0) to = steps;
                int n = 0;
                for (int st = juce::jmax (0, from); st < juce::jmin (steps, to); ++st) if (on (lanes, st)) ++n;
                return n;
            }
            int inBar (const Lanes& lanes, int bar) const   { return stepsWith (lanes, bar * barSteps, (bar + 1) * barSteps); }
            bool rolls (const Lanes& lanes) const
            {
                for (auto s : lanes) for (int c : count[(size_t) s]) if (c > 1) return true;
                return false;
            }
            int hits() const
            {
                int n = 0;
                for (const auto& lane : vel) for (float v : lane) if (v > 0) ++n;
                return n;
            }
            float hitsPerBar() const   { return (float) hits() / (float) bars; }
            // the lane of a kind it uses most (or the first)
            Slot most (const Lanes& lanes) const
            {
                Slot best = lanes.front();
                int n = 0;
                for (auto s : lanes)
                    if (const int c = stepsWith ({ s }); c > n) { n = c; best = s; }
                return best;
            }
            // how many strengths its hits of these kinds come in (hard / medium / soft)
            int levels (const Lanes& lanes) const
            {
                bool hard = false, mid = false, soft = false;
                for (auto s : lanes)
                    for (float v : vel[(size_t) s])
                    {
                        if (v <= 0) continue;
                        if (v >= 0.86f) hard = true; else if (v >= ghostMax) mid = true; else soft = true;
                    }
                return (hard ? 1 : 0) + (mid ? 1 : 0) + (soft ? 1 : 0);
            }
            bool barsSame (int a, int b) const
            {
                for (int s = 0; s < numSlots; ++s)
                    for (int i = 0; i < barSteps; ++i)
                    {
                        const auto x = (size_t) (a * barSteps + i), y = (size_t) (b * barSteps + i);
                        if (std::abs (vel[(size_t) s][x] - vel[(size_t) s][y]) > 0.05f || count[(size_t) s][x] != count[(size_t) s][y]) return false;
                    }
                return true;
            }
            bool ghostNotes() const
            {
                for (int st = 0; st < steps; ++st)
                    if (const float v = at (backLanes, st); v > 0 && v < ghostMax && st % barSteps != 4 && st % barSteps != 8 && st % barSteps != 12) return true;
                return false;
            }
        };

        //==============================================================================
        // What the style's own grooves do. The coach suggests adding something only when all of them do it, and
        // taking something away only when none of them do, so a style's own grooves pass, and so does anything
        // any of them does.
        enum class Backbeat { twoFour, half, other };

        // a bar's backbeat: 2 and 4, or 3 (half-time); a fill at the end of the bar stands in for the last one
        bool fillAtEnd (const Grid& g, int bar)
        {
            int n = 0;
            for (int i = 8; i < barSteps; ++i) if (g.on (tomLanes, bar * barSteps + i)) ++n;
            return n >= 2;
        }
        bool twoFourBar (const Grid& g, int bar)
        {
            const int base = bar * barSteps;
            return g.on (backLanes, base + 4, backbeatMin) && (g.on (backLanes, base + 12, backbeatMin) || fillAtEnd (g, bar));
        }
        bool halfBar (const Grid& g, int bar)
        {
            const int base = bar * barSteps;
            return (g.on (backLanes, base + 8, backbeatMin) || fillAtEnd (g, bar)) && ! g.on (backLanes, base + 4, backbeatMin);
        }
        // a style's groove: what most of its bars do; your pattern (strict): what every bar does
        Backbeat backbeatOf (const Grid& g, bool strict = false)
        {
            int two = 0, half = 0;
            for (int b = 0; b < g.bars; ++b) { if (twoFourBar (g, b)) ++two; else if (halfBar (g, b)) ++half; }
            if (strict) return two == g.bars ? Backbeat::twoFour : half == g.bars ? Backbeat::half : Backbeat::other;
            if (two * 2 >= g.bars && two >= half) return Backbeat::twoFour;
            if (half * 2 >= g.bars) return Backbeat::half;
            return Backbeat::other;
        }
        int hatClassOf (float hatsPerBar)   { return hatsPerBar >= 11.0f ? 3 : hatsPerBar >= 6.0f ? 2 : hatsPerBar >= 2.0f ? 1 : 0; }
        int kindsAt (const Grid& g, int st);

        struct Profile
        {
            std::vector<const Groove*> refs;
            const Groove* main = nullptr;          // its first groove
            int twoFourRefs = 0, halfRefs = 0, otherRefs = 0;
            bool fourFloor = true, kickOnOne = true, kickUnder = false;
            int kickMax = 0;
            float kicksPerBar = 0, kickAvgMin = 99, hitsMax = 0;
            int hatMin = 3, hatMax = 0;             // the hats' subdivisions its grooves use (0 none ... 3 16ths)
            bool rollsAll = true, rollsAny = false, accentsAll = true, ghostsAll = true, openAny = false, crashAll = true,
                 tambAll = true, lowAll = true, lowAny = false, lowOnKick = true, variesAll = true, rideWithHats = false;
            int maxKinds = 0;
            Slot backbeat = Slot::snare;
            std::array<bool, barSteps> kickAt {}, ghostAt {};

            Backbeat usual() const   { return twoFourRefs >= halfRefs ? Backbeat::twoFour : Backbeat::half; }
            bool allows (Backbeat b) const
            {
                return otherRefs > 0 || (b == Backbeat::twoFour && twoFourRefs > 0) || (b == Backbeat::half && halfRefs > 0);
            }
        };

        Profile profileOf (const Style& style)
        {
            Profile p;
            p.refs = sauceGrooves (style);
            if (p.refs.empty()) { p.fourFloor = p.kickOnOne = p.rollsAll = p.accentsAll = p.ghostsAll = p.crashAll = p.tambAll = p.lowAll = false; return p; }
            p.main = p.refs.front();
            int snares = 0, claps = 0, hatRefs = 0;
            float kicks = 0;
            int bars = 0;
            for (const auto* r : p.refs)
            {
                const Grid g (*r);
                switch (backbeatOf (g))
                {
                    case Backbeat::twoFour: ++p.twoFourRefs; break;
                    case Backbeat::half:    ++p.halfRefs; break;
                    case Backbeat::other:   ++p.otherRefs; break;
                }
                int floorBars = 0, tambBars = 0, refKicks = 0;
                for (int b = 0; b < g.bars; ++b)
                {
                    ++bars;
                    const int base = b * barSteps;
                    if (g.on (kickLanes, base) && g.on (kickLanes, base + 4) && g.on (kickLanes, base + 8) && g.on (kickLanes, base + 12)) ++floorBars;
                    for (int pos : { 4, 8, 12 })
                        if (g.on (kickLanes, base + pos) && g.on (backLanes, base + pos, backbeatMin)) p.kickUnder = true;
                    const int k = g.inBar (kickLanes, b);
                    refKicks += k;
                    p.kickMax = juce::jmax (p.kickMax, k);
                    for (int i = 0; i < barSteps; ++i)
                    {
                        if (g.on (kickLanes, base + i)) p.kickAt[(size_t) i] = true;
                        const float v = g.at (backLanes, base + i);
                        if (v > 0 && v < ghostMax && i != 4 && i != 8 && i != 12) p.ghostAt[(size_t) i] = true;
                    }
                    if (g.on ({ Slot::perc1 }, base + 4) && g.on ({ Slot::perc1 }, base + 12)) ++tambBars;
                }
                kicks += (float) refKicks;
                p.kickAvgMin = juce::jmin (p.kickAvgMin, (float) refKicks / (float) g.bars);
                p.fourFloor = p.fourFloor && floorBars * 4 >= g.bars * 3;
                p.kickOnOne = p.kickOnOne && g.on (kickLanes, 0);
                p.tambAll = p.tambAll && tambBars * 2 >= g.bars;
                p.hitsMax = juce::jmax (p.hitsMax, g.hitsPerBar());
                const int cls = hatClassOf ((float) g.stepsWith (hatLanes) / (float) g.bars);
                p.hatMin = juce::jmin (p.hatMin, cls);
                p.hatMax = juce::jmax (p.hatMax, cls);
                const bool rolls = g.rolls (hatLanes);
                p.rollsAll = p.rollsAll && rolls;
                p.rollsAny = p.rollsAny || rolls;
                if (g.stepsWith (hatLanes) >= 4) { ++hatRefs; p.accentsAll = p.accentsAll && g.levels (hatLanes) >= 2; }
                p.ghostsAll = p.ghostsAll && g.ghostNotes();
                p.openAny = p.openAny || g.stepsWith (openLanes) > 0;
                p.crashAll = p.crashAll && g.on (crashLanes, 0);
                const int lows = g.stepsWith (lowLanes);
                p.lowAll = p.lowAll && lows > 0;
                p.lowAny = p.lowAny || lows > 0;
                if (lows > 0)
                {
                    int onKick = 0;
                    for (int st = 0; st < g.steps; ++st) if (g.on (lowLanes, st) && g.on (kickLanes, st)) ++onKick;
                    p.lowOnKick = p.lowOnKick && onKick * 4 >= lows * 3;
                }
                if (g.bars >= 2) p.variesAll = p.variesAll && ! g.barsSame (0, 1);
                if (g.stepsWith (rideLanes) >= 4 * g.bars && g.stepsWith (hatLanes) >= 4 * g.bars) p.rideWithHats = true;
                for (int st = 0; st < g.steps; ++st)
                {
                    p.maxKinds = juce::jmax (p.maxKinds, kindsAt (g, st));
                    if (g.on ({ Slot::snare, Slot::snare2 }, st, backbeatMin)) ++snares;
                    if (g.on ({ Slot::clap, Slot::clap2 }, st, backbeatMin)) ++claps;
                }
            }
            if (hatRefs == 0) p.accentsAll = false;
            if (! p.lowAny) p.lowOnKick = false;
            p.kicksPerBar = kicks / (float) juce::jmax (1, bars);
            p.backbeat = claps > snares ? Slot::clap : Slot::snare;
            return p;
        }

        // how many kinds of drum hit on a step (the 808 goes with the kick; a crash on a bar's 1 is expected)
        int kindsAt (const Grid& g, int st)
        {
            int n = 0;
            for (const Lanes* l : { &kickLanes, &backLanes, &rideLanes, &tomLanes, &percLanes, &fxLanes })
                if (g.on (*l, st)) ++n;
            if (g.on (hatLanes, st) || g.on (openLanes, st)) ++n;
            if (st % barSteps != 0 && g.on (crashLanes, st)) ++n;
            return n;
        }

        //==============================================================================
        juce::String stepName (int step, int bars)
        {
            return (bars > 1 ? "bar " + juce::String (step / barSteps + 1) + ", step " : juce::String ("step ")) + juce::String (step % barSteps + 1);
        }

        juce::String hatWord (int hatClass)   { return hatClass >= 3 ? "16ths" : hatClass == 2 ? "8ths" : "quarter notes"; }

        // how strong a hat is where it falls: hard on the beat, medium on the &, soft between
        float hatAccent (int step, int hatClass)
        {
            const int i = step % barSteps;
            if (i % 4 == 0) return hatClass <= 1 && i % 8 != 0 ? mediumHit : hardHit;
            return i % 2 == 0 ? mediumHit : softHit;
        }

        juce::String backbeatWhy (Genre g, const juce::String& name, const juce::String& w)
        {
            switch (g)
            {
                case Genre::boomBap:
                case Genre::lofi:    return "In " + name + " the " + w + " cracks on 2 and 4 (steps 5 and 13). It's the hit heads nod to, and the kick, hats and ghost notes all play around it.";
                case Genre::rnb:     return name + " keeps a firm " + w + " on 2 and 4 (steps 5 and 13), often with a clap or snap on top. It anchors the groove so the vocal can float.";
                case Genre::pop:     return "Pop's backbeat is a " + w + " on 2 and 4 (steps 5 and 13). It's what people clap along to, and it keeps the chorus driving.";
                case Genre::rock:    return "The rock backbeat is a hard " + w + " on 2 and 4 (steps 5 and 13). From the 60s to today, every rock groove is built on it.";
                case Genre::country: return "Country leans on a steady " + w + " on 2 and 4 (steps 5 and 13), the boom-chick of the two-step. It keeps the song moving under the vocal.";
                case Genre::worship: return "Worship grooves keep the " + w + " on 2 and 4 (steps 5 and 13), so a whole room can feel the pulse and sing with it.";
                case Genre::edm:     return "Dance music lays a clap or " + w + " on 2 and 4 (steps 5 and 13) over the kick. Together they're the pulse of the track.";
                case Genre::trap:
                case Genre::cinematic:
                case Genre::count:   break;
            }
            return name + "'s grooves land the " + w + " on 2 and 4 (steps 5 and 13). It's the beat everything else is built around.";
        }

        juce::String kickBusyWhy (Genre g, const juce::String& name, int n)
        {
            const auto nn = juce::String (n);
            switch (g)
            {
                case Genre::trap:    return "Trap keeps the kick sparse (about " + nn + " a bar in " + name + "'s grooves) because the 808 carries the low end. More kicks crowd the 808 and the vocal.";
                case Genre::boomBap:
                case Genre::lofi:
                case Genre::rnb:     return name + " kicks answer the snare: a few well-placed kicks (about " + nn + " a bar in its grooves) swing harder than a busy line.";
                case Genre::rock:
                case Genre::country:
                case Genre::worship:
                case Genre::pop:     return "The kick locks with the bass. " + name + "'s grooves use about " + nn + " a bar; a busier kick fights the bass and loses the push.";
                case Genre::edm:
                case Genre::cinematic:
                case Genre::count:   break;
            }
            return name + "'s grooves use about " + nn + " kicks a bar. Fewer, stronger kicks leave room for the bass.";
        }

        void put (Groove& w, Slot s, int step, int count, float vel)   { w.hits = withStep (w, s, step, count, vel); }
    }

    //==============================================================================
    Report coachBeat (const Groove& groove, const Context& ctx, const juce::StringArray& onPurpose)
    {
        const auto& style = styleFor (ctx.genre, ctx.subStyle);
        const auto name = ctx.subStyle <= 0 ? genreName (ctx.genre) : style.name;
        const auto table = sauceTable (style);
        const auto prof = profileOf (style);
        const Grid g (groove);
        const int bars = g.bars;

        Report rep;
        rep.coachedAs = name + (ctx.era >= 0 ? "  -  " + eraName (ctx.era) : juce::String())
                        + (ctx.tempo > 0 ? "  -  " + juce::String (juce::roundToInt (ctx.tempo)) + " BPM" : juce::String());

        std::vector<Tip> fixes, good;
        const auto keep = [&] (Tip t)
        {
            if (onPurpose.contains (t.id)) { ++rep.onPurpose; return; }
            (t.good ? good : fixes).push_back (std::move (t));
        };
        const auto changed = [&groove] (const std::function<void (Groove&)>& change)
        {
            Groove w = groove;
            change (w);
            sortHits (w.hits);
            return w.hits;
        };
        const auto fix = [] (juce::String id, Area area, int weight, juce::String title, juce::String why)
        {
            Tip t;
            t.id = std::move (id); t.area = area; t.weight = weight; t.title = std::move (title); t.why = std::move (why);
            return t;
        };
        const auto praise = [] (juce::String id, juce::String title, juce::String why)
        {
            Tip t;
            t.id = std::move (id); t.good = true; t.title = std::move (title); t.why = std::move (why);
            return t;
        };

        const int kicks = g.stepsWith (kickLanes), backs = g.stepsWith (backLanes), hats = g.stepsWith (hatLanes);
        const Slot bbLane = backs > 0 ? g.most ({ Slot::snare, Slot::clap, Slot::snare2, Slot::clap2, Slot::rim, Slot::rim2 }) : prof.backbeat;
        const auto bb = slotName (slotKind (bbLane)).toLowerCase();       // "snare", "clap", "rim"
        const Slot hatLane = hats > 0 ? g.most (hatLanes) : Slot::closedHat;

        // ---- nothing yet: the skeleton ----
        if (kicks == 0 && backs == 0)
        {
            auto t = fix ("skeleton", Area::fit, 60, "Start with the kick and the " + bb + ".",
                          "Every " + name + " groove is built on a kick and a " + bb
                          + (prof.otherRefs > 0 ? juce::String() : prof.usual() == Backbeat::half ? " on beat 3" : " on 2 and 4")
                          + ", with the hats keeping time between them. TRY IT lays down " + name + "'s basic kick, " + bb + " and hats to build from.");
            if (prof.main != nullptr)
            {
                const auto* ref = prof.main;
                const bool addHats = hats == 0, addOpen = g.stepsWith (openLanes) == 0, addLow = g.stepsWith (lowLanes) == 0;
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w)
                {
                    const int refBars = juce::jmax (1, ref->bars);
                    for (const auto& h : ref->hits)
                    {
                        const auto in = [&h] (const Lanes& l) { return std::find (l.begin(), l.end(), h.slot) != l.end(); };
                        if (! (in (kickLanes) || in (backLanes) || (addHats && in (hatLanes)) || (addOpen && in (openLanes)) || (addLow && in (lowLanes)))) continue;
                        const int refBar = (int) std::floor (h.beat / 4.0);
                        for (int b = refBar; b < bars; b += refBars)
                            w.hits.push_back ({ h.beat + 4.0 * (b - refBar), h.slot, h.velocity });
                    }
                });
                for (const auto& h : t.fixHits) t.steps.push_back ({ h.slot, (int) std::floor (h.beat * 4.0 + 1.0e-6) });
            }
            keep (t);
            for (auto& r : { "Groove fit", "Feel", "Space", "Variation" }) rep.ratings.push_back ({ r, -1, "-" });
            rep.ratings.front() = { "Groove fit", 0, "Empty" };
            rep.fixes = fixes;
            return rep;
        }

        // ---- the backbeat ----
        const auto mine = backbeatOf (g, true);
        if (prof.otherRefs == 0 && ! prof.refs.empty())
        {
            if (prof.allows (mine))
                keep (praise ("backbeat.good", "Your backbeat sits right.",
                              mine == Backbeat::half ? "The " + bb + " on beat 3 gives it " + name + "'s half-time weight."
                                                     : "The " + bb + " on 2 and 4 is where " + name + " wants it."));
            else if (! prof.allows (Backbeat::twoFour) || (mine == Backbeat::other && prof.usual() == Backbeat::half))
            {
                // half-time: beat 3
                std::vector<StepRef> missing;
                bool on24 = false;
                for (int b = 0; b < bars; ++b)
                {
                    if (! halfBar (g, b) && ! g.on (backLanes, b * barSteps + 8, backbeatMin)) missing.push_back ({ bbLane, b * barSteps + 8 });
                    if (g.on (backLanes, b * barSteps + 4, backbeatMin) || g.on (backLanes, b * barSteps + 12, backbeatMin)) on24 = true;
                }
                auto t = fix ("halftime", Area::fit, 40, (on24 ? "Move the " : "Put the ") + bb + " to beat 3.",
                              name + " is half-time: the " + bb + " lands on beat 3 of each bar (step 9), so a fast tempo feels half as fast and leaves room for quick hats and flows.");
                t.steps = missing;
                for (int b = 0; b < bars; ++b)
                    for (int pos : { 4, 12 })
                        if (g.on (backLanes, b * barSteps + pos, backbeatMin)) t.steps.push_back ({ bbLane, b * barSteps + pos });
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w)
                {
                    for (int b = 0; b < bars; ++b)
                    {
                        for (int pos : { 4, 12 })
                            for (auto l : backLanes)
                                if (g.vel[(size_t) l][(size_t) (b * barSteps + pos)] >= backbeatMin) put (w, l, b * barSteps + pos, 0, 0);
                        if (! g.on (backLanes, b * barSteps + 8, backbeatMin)) put (w, bbLane, b * barSteps + 8, 1, hardHit);
                    }
                });
                keep (t);
            }
            else
            {
                // 2 and 4 (a fill at the end of a bar can stand in for its 4)
                std::vector<StepRef> missing;
                for (int b = 0; b < bars; ++b)
                {
                    if (! g.on (backLanes, b * barSteps + 4, backbeatMin)) missing.push_back ({ bbLane, b * barSteps + 4 });
                    if (! g.on (backLanes, b * barSteps + 12, backbeatMin) && ! fillAtEnd (g, b)) missing.push_back ({ bbLane, b * barSteps + 12 });
                }
                auto t = fix ("backbeat", Area::fit, (int) missing.size() >= bars ? 40 : 28, "Put the " + bb + " on 2 and 4.", backbeatWhy (ctx.genre, name, bb));
                t.steps = missing;
                t.hasFix = ! missing.empty();
                t.fixHits = changed ([&] (Groove& w) { for (auto& m : missing) put (w, bbLane, m.step, 1, hardHit); });
                keep (t);
            }
        }

        // ---- the kick ----
        const auto kickStepsOf = [&g] (int bar) { std::vector<int> v; for (int i = 0; i < barSteps; ++i) if (g.on (kickLanes, bar * barSteps + i)) v.push_back (bar * barSteps + i); return v; };
        if (prof.fourFloor)
        {
            std::vector<StepRef> missing;
            for (int b = 0; b < bars; ++b)
                for (int pos : { 0, 4, 8, 12 })
                    if (! g.on (kickLanes, b * barSteps + pos)) missing.push_back ({ Slot::kick, b * barSteps + pos });
            if (! missing.empty())
            {
                auto t = fix ("kick.floor", Area::fit, 35, "Keep the kick on every beat.",
                              name + " runs four on the floor: a kick on every beat (steps 1, 5, 9 and 13). It's the steady pulse the dance floor locks to.");
                t.steps = missing;
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { for (auto& m : missing) put (w, Slot::kick, m.step, 1, hardHit); });
                keep (t);
            }
        }
        else if (kicks > 0)
        {
            if (prof.kickOnOne && ! g.on (kickLanes, 0))
            {
                auto t = fix ("kick.one", Area::fit, 20, "Start the bar with the kick.",
                              name + " drops the kick on the 1 (step 1). It tells the listener where the bar starts; leave it out only on purpose, for a drop.");
                t.steps = { { Slot::kick, 0 } };
                t.hasFix = true;
                t.fixHits = changed ([] (Groove& w) { put (w, Slot::kick, 0, 1, hardHit); });
                keep (t);
            }

            // too busy / too sparse, against its grooves
            int busiest = 0;
            for (int b = 0; b < bars; ++b) busiest = juce::jmax (busiest, g.inBar (kickLanes, b));
            const int typical = juce::jmax (1, juce::roundToInt (prof.kicksPerBar));
            if (prof.refs.size() > 0 && busiest > prof.kickMax + 1 && busiest > 4)
            {
                auto t = fix ("kick.busy", Area::space, 20, "Thin out the kick.", kickBusyWhy (ctx.genre, name, typical));
                std::vector<int> drop;
                for (int b = 0; b < bars; ++b)
                {
                    // the kicks its grooves don't use first, then the softest, until the bar is down to their busiest
                    // (never the kick on the 1)
                    auto ks = kickStepsOf (b);
                    std::vector<std::tuple<bool, float, int>> spare;
                    for (int st : ks)
                        if (st % barSteps != 0) spare.emplace_back (prof.kickAt[(size_t) (st % barSteps)], g.at (kickLanes, st), st);
                    std::sort (spare.begin(), spare.end());
                    int n = (int) ks.size();
                    for (auto& [theirs, v, st] : spare)
                    {
                        juce::ignoreUnused (theirs, v);
                        if (n <= prof.kickMax) break;
                        drop.push_back (st);
                        --n;
                    }
                }
                for (int st : drop) t.steps.push_back ({ g.most (kickLanes), st });
                if (! drop.empty())
                {
                    t.hasFix = true;
                    t.fixHits = changed ([&] (Groove& w) { for (int st : drop) for (auto s : kickLanes) put (w, s, st, 0, 0); });
                }
                keep (t);
            }
            else if (prof.refs.size() > 0 && prof.kickAvgMin >= 2.0f && (float) kicks / (float) bars < prof.kickAvgMin - 1.0f)
            {
                auto t = fix ("kick.sparse", Area::fit, 15, "Give the kick more to say.",
                              name + "'s grooves use about " + juce::String (typical) + " kicks a bar. With fewer, the groove loses its bottom; the kick and the " + bb + " should trade places through the bar.");
                std::vector<int> add;
                if (prof.main != nullptr)
                {
                    const Grid ref (*prof.main);
                    for (int b = 0; b < bars; ++b)
                    {
                        int n = g.inBar (kickLanes, b);
                        const int rb = b % ref.bars;
                        for (int i = 0; i < barSteps && n < typical; ++i)
                            if (ref.on (kickLanes, rb * barSteps + i) && ! g.on (kickLanes, b * barSteps + i)) { add.push_back (b * barSteps + i); ++n; }
                    }
                }
                for (int st : add) t.steps.push_back ({ Slot::kick, st });
                if (! add.empty())
                {
                    t.hasFix = true;
                    t.fixHits = changed ([&] (Groove& w) { for (int st : add) put (w, Slot::kick, st, 1, mediumHit); });
                }
                keep (t);
            }
        }
        if (kicks > 0 && ! prof.fourFloor && ! prof.kickUnder)
        {
            // a kick stacked under the backbeat
            std::vector<int> under;
            const std::vector<int> backbeatSteps = mine == Backbeat::half ? std::vector<int> { 8 } : std::vector<int> { 4, 12 };
            for (int b = 0; b < bars; ++b)
                for (int pos : backbeatSteps)
                    if (g.on (kickLanes, b * barSteps + pos) && g.on (backLanes, b * barSteps + pos, backbeatMin)) under.push_back (b * barSteps + pos);
            if (! under.empty())
            {
                auto t = fix ("kick.snare", Area::space, 15, "Take the kick off the " + bb + ".",
                              "A kick under the " + bb + (mine == Backbeat::half ? " on beat 3" : " on 2 and 4") + " blurs the backbeat. In " + name + " they trade places: kick, then " + bb + ".");
                for (int st : under) t.steps.push_back ({ g.most (kickLanes), st });
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { for (int st : under) for (auto s : kickLanes) put (w, s, st, 0, 0); });
                keep (t);
            }
        }

        // ---- the hats ----
        const int hatClass = [&] { const float h = (float) hats / (float) bars; return h >= 11.0f ? 3 : h >= 6.0f ? 2 : h >= 2.0f ? 1 : 0; }();
        if (prof.hatMax > 0)
        {
            const bool rideKeepsTime = g.stepsWith (rideLanes) >= 4 * bars;
            const auto every = [] (int cls) { return cls >= 3 ? 1 : cls == 2 ? 2 : 4; };
            const auto fill = [&] (Groove& w, int cls)
            {
                for (int st = 0; st < g.steps; st += every (cls))
                    if (! g.on (hatLanes, st) && ! g.on (openLanes, st)) put (w, hatLane, st, 1, hatAccent (st, cls));
            };
            const auto filled = [&] (int cls)
            {
                std::vector<StepRef> v;
                for (int st = 0; st < g.steps; st += every (cls)) if (! g.on (hatLanes, st) && ! g.on (openLanes, st)) v.push_back ({ hatLane, st });
                return v;
            };
            if (hats == 0 && ! rideKeepsTime && prof.hatMin > 0)
            {
                auto t = fix ("hats.none", Area::fit, 20, "Add the hats.",
                              "The hats keep time between the kick and the " + bb + ". " + name + " plays them in " + hatWord (prof.hatMin) + ".");
                t.steps = filled (prof.hatMin);
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { fill (w, prof.hatMin); });
                keep (t);
            }
            else if (hats > 0 && hatClass < prof.hatMin)
            {
                auto t = fix ("hats.more", Area::fit, 18, "Run the hats in " + hatWord (prof.hatMin) + ".",
                              name + " keeps the hats moving in " + hatWord (prof.hatMin) + ". That steady ticking between the kick and the " + bb + " is where the energy comes from.");
                t.steps = filled (prof.hatMin);
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { fill (w, prof.hatMin); });
                keep (t);
            }
            else if (hatClass > prof.hatMax)
            {
                auto t = fix ("hats.less", Area::space, 18, "Ease the hats back to " + hatWord (prof.hatMax) + ".",
                              name + "'s hats ride " + hatWord (prof.hatMax) + ". They drive the song without crowding the vocal; busier hats here sound cluttered.");
                for (int st = 0; st < g.steps; ++st)
                    if (st % every (prof.hatMax) != 0 && g.on (hatLanes, st)) t.steps.push_back ({ hatLane, st });
                t.hasFix = ! t.steps.empty();
                t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) for (auto l : hatLanes) put (w, l, r.step, 0, 0); });
                keep (t);
            }

            if (prof.rollsAll && hats >= 6 && ! g.rolls (hatLanes))
            {
                const int st = (bars - 1) * barSteps + 14;
                auto t = fix ("hats.roll", Area::feel, 15, "Add a hat roll.",
                              name + "'s hats stutter: quick rolls (Ctrl-click a step for 2 or 3 hits) before a " + bb + " or at the end of a bar are the sound of the style.");
                t.steps = { { hatLane, st } };
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { put (w, hatLane, st, 3, mediumHit); });
                keep (t);
            }
            else if (! prof.rollsAny && g.rolls (hatLanes))
            {
                const auto when = ctx.era >= 0 && ctx.era <= 4 ? "In the " + eraName (ctx.era) : "In " + name;
                auto t = fix ("hats.norolls", Area::feel, 18, "Drop the hat rolls.",
                              "Fast hat rolls came in with 2010s trap. " + when + ", the hats play straight " + hatWord (juce::jmax (1, prof.hatMax)) + ", so rolls sound out of place.");
                for (auto s : hatLanes)
                    for (int st = 0; st < g.steps; ++st)
                        if (g.count[(size_t) s][(size_t) st] > 1) t.steps.push_back ({ s, st });
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) put (w, r.slot, r.step, 1, g.vel[(size_t) r.slot][(size_t) r.step]); });
                keep (t);
            }

            if (prof.accentsAll && hats >= 6)
            {
                if (g.levels (hatLanes) < 2)
                {
                    auto t = fix ("hats.accent", Area::feel, 18, "Accent the hats.",
                                  "Drummers hit the hat harder on the beat and lighter in between, and " + name + " programmers copy that. With every hat the same strength, the groove sounds stiff.");
                    for (auto s : hatLanes)
                        for (int st = 0; st < g.steps; ++st)
                            if (g.vel[(size_t) s][(size_t) st] > 0) t.steps.push_back ({ s, st });
                    t.hasFix = true;
                    t.fixHits = changed ([&] (Groove& w)
                    {
                        for (auto& r : t.steps) put (w, r.slot, r.step, juce::jmax (1, g.count[(size_t) r.slot][(size_t) r.step]), hatAccent (r.step, juce::jmax (hatClass, 2)));
                    });
                    keep (t);
                }
                else
                    keep (praise ("accents.good", "Your hats breathe.", "Strong and soft hats make the groove feel played, not programmed."));
            }

        }

        // ---- ghost notes ----
        if (prof.ghostsAll && backs > 0)
        {
            if (! g.ghostNotes())
            {
                auto t = fix ("ghosts", Area::feel, 18, "Add a ghost note or two.",
                              name + " lives on quiet ghost notes on the snare between the backbeats (right-click a step for soft). They're where the bounce and the swing come from.");
                const Slot ghostLane = g.stepsWith ({ Slot::snare }) > 0 || bbLane != Slot::clap ? Slot::snare : bbLane;
                std::vector<int> spots;
                for (int i = 0; i < barSteps; ++i) if (prof.ghostAt[(size_t) i]) spots.push_back (i);
                if (spots.empty()) spots = { 7, 15 };
                for (int b = 0; b < bars; ++b)
                {
                    int n = 0;
                    for (int i : spots)
                    {
                        const int st = b * barSteps + i;
                        if (n >= 2 || g.on (backLanes, st) || g.on (kickLanes, st)) continue;
                        t.steps.push_back ({ ghostLane, st });
                        ++n;
                    }
                }
                if (! t.steps.empty())
                {
                    t.hasFix = true;
                    t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) put (w, r.slot, r.step, 1, softHit); });
                }
                keep (t);
            }
            else
                keep (praise ("ghosts.good", "Nice ghost notes.", "Those quiet hits between the backbeats give it " + name + "'s bounce."));
        }

        // ---- swing and tempo ----
        if (table.hasSwing())
        {
            const int pct = swingToPercent (groove.swing);
            const int own = prof.main != nullptr ? swingPercent (*prof.main) : (table.swingLo + table.swingHi) / 2;
            const int target = juce::jlimit (table.swingLo, table.swingHi, own > 50 || table.swingLo > 50 ? own : (table.swingLo + table.swingHi) / 2);
            const auto feel = table.timeFeel.isNotEmpty() ? " (" + table.timeFeel.toLowerCase() + ")" : juce::String();
            const auto range = table.swingLo == table.swingHi ? juce::String (table.swingLo) + "%" : juce::String (table.swingLo) + " to " + juce::String (table.swingHi) + "%";
            const juce::String what = groove.swingEighths ? "8th" : "16th";
            if (pct < table.swingLo - 1)
            {
                auto t = fix ("swing.more", Area::feel, 25, "Swing it to about " + juce::String (target) + "%.",
                              name + " swings " + range + feel + ". Straight " + what + "s sound stiff here; swing pushes every other " + what + " a little late so the groove leans back.");
                t.hasFix = true;
                t.fixHits = groove.hits;
                t.fixSwing = swingFromPercent (target);
                keep (t);
            }
            else if (pct > table.swingHi + 1)
            {
                auto t = fix ("swing.less", Area::feel, 25, target <= 50 ? juce::String ("Play it straight (50% swing).") : "Ease the swing to about " + juce::String (target) + "%.",
                              name + " plays " + (table.swingHi <= 50 ? juce::String ("straight") : "a " + range + " swing") + feel + ". Heavy swing turns its " + what + "s into a shuffle.");
                t.hasFix = true;
                t.fixHits = groove.hits;
                t.fixSwing = swingFromPercent (target);
                keep (t);
            }
            else if (table.swingHi > 52)
                keep (praise ("swing.good", "The swing is in the pocket.", juce::String (pct) + "% sits inside " + name + "'s " + range + "."));
        }
        if (ctx.tempo > 0 && table.hasTempo())
        {
            const auto inRange = [&table] (double t) { return t >= table.tempoLo - 3 && t <= table.tempoHi + 3; };
            if (! inRange (ctx.tempo) && ! inRange (ctx.tempo * 2.0) && ! inRange (ctx.tempo / 2.0))
            {
                const auto range = juce::String (table.tempoLo) + " to " + juce::String (table.tempoHi) + " BPM";
                keep (fix ("tempo", Area::feel, 15, "Your song is at " + juce::String (juce::roundToInt (ctx.tempo)) + " BPM; " + name + " usually sits at " + range + ".",
                           "Tempo sets the feel before any pattern does: " + name + "'s grooves are made for " + range + ". Change the BPM in the header, or pick a style that fits your tempo."));
            }
        }

        // ---- bar 2 ----
        if (bars >= 2 && prof.variesAll)
        {
            bool same = true;
            for (int b = 1; b < bars; ++b) same = same && g.barsSame (0, b);
            if (same)
            {
                const int last = (bars - 1) * barSteps;
                auto t = fix ("bar2", Area::variation, 30, "Change something in bar " + juce::String (bars) + ".",
                              name + "'s grooves move one thing in the last bar, like a pickup kick before the 1, an open hat or a ghost note, so the loop breathes instead of repeating.");
                // the first of these the last bar doesn't have yet: an open hat on the & of 4 (when the style uses
                // them), a pickup kick, a ghost note
                struct Change { Slot slot; int step; float vel; };
                std::vector<Change> options;
                if (prof.openAny) options.push_back ({ Slot::openHat, last + 14, mediumHit });
                options.push_back ({ Slot::kick, last + 14, mediumHit });
                options.push_back ({ Slot::kick, last + 15, mediumHit });
                options.push_back ({ Slot::snare, last + 15, softHit });
                options.push_back ({ Slot::snare, last + 7, softHit });
                for (const auto& c : options)
                    if (! g.on ({ c.slot }, c.step))
                    {
                        t.steps = { { c.slot, c.step } };
                        t.fixHits = changed ([&] (Groove& w)
                        {
                            if (c.slot == Slot::openHat) for (auto l : hatLanes) put (w, l, c.step, 0, 0);
                            put (w, c.slot, c.step, 1, c.vel);
                        });
                        t.hasFix = true;
                        break;
                    }
                keep (t);
            }
            else
                keep (praise ("bar2.good", "Bar " + juce::String (bars) + " answers bar 1.", "The change near the end keeps the loop moving."));
        }

        // ---- space ----
        {
            // the busiest step (more kinds of drum at once than any of the style's grooves has)
            const std::vector<std::pair<const Lanes*, const char*>> kinds { { &kickLanes, "the kick" }, { &backLanes, "the " }, { &hatLanes, "the hat" },
                                                                            { &rideLanes, "the ride" }, { &tomLanes, "a tom" }, { &percLanes, "percussion" },
                                                                            { &fxLanes, "an FX" }, { &crashLanes, "a crash" } };
            int worst = -1, most = 0;
            const int limit = juce::jmax (4, prof.maxKinds + 1);      // this many kinds at once: crowded
            std::vector<int> crowded;
            for (int st = 0; st < g.steps; ++st)
            {
                const int n = kindsAt (g, st);
                if (n > most) { most = n; worst = st; }
                if (n >= limit) crowded.push_back (st);
            }
            if (! crowded.empty())
            {
                juce::StringArray what;
                for (auto& [lanes, word] : kinds)
                {
                    if (lanes == &crashLanes && worst % barSteps == 0) continue;
                    if (lanes == &hatLanes ? (g.on (hatLanes, worst) || g.on (openLanes, worst)) : g.on (*lanes, worst))
                        what.add (lanes == &backLanes ? "the " + bb : juce::String (word));
                }
                const auto list = what.size() > 1 ? what.joinIntoString (", ", 0, what.size() - 1) + " and " + what[what.size() - 1] : what.joinIntoString ("");
                auto t = fix ("crowd", Area::space, 15, crowded.size() == 1 ? "Clear up " + stepName (worst, bars) + "." : juce::String ("Clear up the busiest steps."),
                              juce::String (list.substring (0, 1).toUpperCase() + list.substring (1)) + " all hit on " + stepName (worst, bars)
                              + ". Too much at once masks the sounds and muddies the hit; " + name + " spreads its parts out.");
                // on each crowded step the FX, percussion, toms, a ride under the hats and a crash make way, in that
                // order, until it's no busier than the style's grooves get (the kick, backbeat and hats stay)
                Groove w = groove;
                bool cleared = true;
                for (int st : crowded)
                {
                    for (const Lanes* l : { &fxLanes, &percLanes, &tomLanes, &rideLanes, &crashLanes })
                    {
                        if (kindsAt (Grid (w), st) < limit) break;
                        if (l == &rideLanes && ! g.on (hatLanes, st)) continue;
                        for (auto sl : *l)
                            if (g.on ({ sl }, st)) { t.steps.push_back ({ sl, st }); put (w, sl, st, 0, 0); }
                    }
                    cleared = cleared && kindsAt (Grid (w), st) < limit;
                }
                if (cleared && ! t.steps.empty())
                {
                    sortHits (w.hits);
                    t.hasFix = true;
                    t.fixHits = w.hits;
                }
                else
                    t.steps = { { g.most (kickLanes), worst } };
                keep (t);
            }
            if (prof.hitsMax > 0 && g.hitsPerBar() > juce::jmax (prof.hitsMax * 1.6f, prof.hitsMax + 10.0f))
                keep (fix ("density", Area::space, 20, "Leave more space.",
                           name + "'s grooves use up to " + juce::String (juce::roundToInt (prof.hitsMax)) + " hits a bar; yours has " + juce::String (juce::roundToInt (g.hitsPerBar()))
                           + ". Space is part of the groove: it gives the bass and the vocal room."));
        }

        // ---- the low end ----
        {
            const int lows = g.stepsWith (lowLanes);
            const bool kickOnly = sauceLowEnd (style) == "Kick";
            const Slot lowLane = lows > 0 ? g.most (lowLanes) : Slot::bass808;
            const auto onKicks = [&] (Groove& w)
            {
                for (auto l : lowLanes) for (int st = 0; st < g.steps; ++st) if (g.on ({ l }, st)) put (w, l, st, 0, 0);
                for (int st = 0; st < g.steps; ++st) if (g.on (kickLanes, st)) put (w, lowLane, st, 1, juce::jmax (mediumHit, g.at (kickLanes, st)));
            };
            if (lows > 0 && kicks > 0 && prof.lowOnKick)
            {
                int off = 0;
                std::vector<StepRef> offSteps;
                for (int st = 0; st < g.steps; ++st) if (g.on (lowLanes, st) && ! g.on (kickLanes, st)) { ++off; offSteps.push_back ({ lowLane, st }); }
                if (off * 2 > lows)
                {
                    auto t = fix ("808.follow", Area::fit, 20, "Start the 808 with the kick.",
                                  "In " + name + " the 808 is the kick's long tail: it starts on the kick's step and holds its note, so the two sound like one big hit. An 808 off the kick fights it.");
                    t.steps = offSteps;
                    t.hasFix = true;
                    t.fixHits = changed (onKicks);
                    keep (t);
                }
                else
                    keep (praise ("808.good", "The 808 rides the kick.", "Starting together, they hit as one."));
            }
            else if (lows == 0 && kicks > 0 && prof.lowAll)
            {
                auto t = fix ("808.add", Area::fit, 15, "Put an 808 under the kick.",
                              name + " gets its low end from an 808 that starts with the kick and rings out. Without it the beat sounds thin under the vocal.");
                for (int st = 0; st < g.steps; ++st) if (g.on (kickLanes, st)) t.steps.push_back ({ Slot::bass808, st });
                t.hasFix = true;
                t.fixHits = changed (onKicks);
                keep (t);
            }
            else if (lows > 0 && kickOnly && ! prof.lowAny)
            {
                auto t = fix ("808.none", Area::space, 12, "Let the kick carry the low end.",
                              name + " has no 808: a bass guitar or synth bass plays the low notes, and an 808 under them muddies both. Keep it if you're mixing styles on purpose.");
                for (int st = 0; st < g.steps; ++st) if (g.on (lowLanes, st)) t.steps.push_back ({ lowLane, st });
                t.hasFix = true;
                t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) for (auto l : lowLanes) put (w, l, r.step, 0, 0); });
                keep (t);
            }
        }

        // ---- the little things its grooves do ----
        if (prof.tambAll && g.stepsWith (percLanes) == 0 && backs > 0)
        {
            auto t = fix ("tamb", Area::feel, 8, "Add a tambourine on 2 and 4.",
                          genreName (ctx.genre) + " records often double the backbeat with a tambourine on 2 and 4. It brightens the " + bb + " and lifts the chorus.");
            for (int b = 0; b < bars; ++b) for (int pos : { 4, 12 }) t.steps.push_back ({ Slot::perc1, b * barSteps + pos });
            t.hasFix = true;
            t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) put (w, Slot::perc1, r.step, 1, mediumHit); });
            keep (t);
        }
        if (prof.crashAll && bars >= 2 && g.stepsWith (crashLanes) == 0)
        {
            auto t = fix ("crash", Area::variation, 6, "Crash the top of the loop.",
                          "A crash on the 1 marks the start of a section in " + name + "; drummers hit one coming out of a fill.");
            t.steps = { { Slot::crash, 0 } };
            t.hasFix = true;
            t.fixHits = changed ([] (Groove& w) { put (w, Slot::crash, 0, 1, hardHit); });
            keep (t);
        }
        if (! prof.rideWithHats && g.stepsWith (rideLanes) >= 4 * bars && hats >= 4 * bars)
        {
            auto t = fix ("ridehat", Area::space, 12, "Ride or hat, not both.",
                          "Drummers keep time on the hi-hat or the ride, one at a time (the ride for a bigger chorus). Both at once clutter the top end.");
            for (auto l : rideLanes) for (int st = 0; st < g.steps; ++st) if (st % barSteps != 0 && g.on ({ l }, st)) t.steps.push_back ({ l, st });
            t.hasFix = true;
            t.fixHits = changed ([&] (Groove& w) { for (auto& r : t.steps) put (w, r.slot, r.step, 0, 0); });
            keep (t);
        }

        // ---- the ratings and the order ----
        const auto rating = [&fixes] (const juce::String& n, Area a)
        {
            int s = 100;
            for (auto& t : fixes) if (t.area == a) s -= t.weight;
            s = juce::jlimit (5, 100, s);
            return Rating { n, s, s >= 85 ? "Strong" : s >= 65 ? "Good" : s >= 40 ? "Getting there" : "Needs work" };
        };
        rep.ratings = { rating ("Groove fit", Area::fit), rating ("Feel", Area::feel), rating ("Space", Area::space),
                        bars >= 2 ? rating ("Variation", Area::variation) : Rating { "Variation", -1, "One bar" } };
        std::stable_sort (fixes.begin(), fixes.end(), [] (const Tip& a, const Tip& b) { return a.weight > b.weight; });
        if (fixes.size() > 6) fixes.resize (6);
        if (good.size() > 2) good.resize (2);
        rep.fixes = std::move (fixes);
        rep.working = std::move (good);
        return rep;
    }
}
