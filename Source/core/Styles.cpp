#include "Styles.h"

namespace gg
{
    namespace
    {
        Traits t (float punch, float warmth, float bright, float room, float vintage, float aggr, float organic)
        {
            Traits x;
            x.punch = punch; x.warmth = warmth; x.bright = bright; x.room = room;
            x.vintage = vintage; x.aggr = aggr; x.organic = organic;
            return x;
        }

        Direction d (const char* id, const char* name, const char* blurb, Traits target, const char* groove, int use808 = -1)
        {
            return { id, name, blurb, target, use808, groove };
        }

        std::vector<GenreStyle> makeStyles()
        {
            using P = PercKind;
            std::vector<GenreStyle> s;

            s.push_back ({ Genre::trap, 142, true, 2, 0.0f, { P::snap, P::other, P::shaker, P::cowbell },
            {
                d ("trap.hard",   "Hard Hitter",   "Punchy - Bright - Aggressive",  t (0.90f, 0.40f, 0.72f, 0.15f, 0.10f, 0.85f, 0.10f), "trap1"),
                d ("trap.drill",  "Dark Drill",    "Dark - Sliding - Cold",         t (0.80f, 0.50f, 0.42f, 0.22f, 0.20f, 0.75f, 0.08f), "drill1"),
                d ("trap.bounce", "Melodic Bounce","Clean - Bouncy - Polished",     t (0.70f, 0.60f, 0.62f, 0.28f, 0.12f, 0.45f, 0.15f), "trap2"),
                d ("trap.under",  "Underground",   "Gritty - Raw - Lo-Fi",          t (0.66f, 0.55f, 0.45f, 0.30f, 0.60f, 0.70f, 0.22f), "trap1"),
            } });

            s.push_back ({ Genre::boomBap, 90, false, 2, 0.55f, { P::tambourine, P::shaker, P::conga, P::snap },
            {
                d ("bb.street",  "Street Classic", "Warm - Punchy - Vintage",       t (0.72f, 0.65f, 0.48f, 0.30f, 0.70f, 0.52f, 0.55f), "boombap1"),
                d ("bb.dusty",   "Dusty Vinyl",    "Gritty - Dark - Lo-Fi",         t (0.50f, 0.75f, 0.30f, 0.35f, 0.92f, 0.38f, 0.60f), "boombap2"),
                d ("bb.modern",  "Modern Bap",     "Clean - Hard Hitting",          t (0.88f, 0.50f, 0.62f, 0.20f, 0.28f, 0.68f, 0.40f), "boombap1", 1),
                d ("bb.jazzy",   "Jazzy Touch",    "Live Feel - Organic",           t (0.45f, 0.70f, 0.50f, 0.50f, 0.60f, 0.25f, 0.85f), "boombap2"),
            } });

            s.push_back ({ Genre::rnb, 76, true, 2, 0.25f, { P::snap, P::shaker, P::conga, P::triangle },
            {
                d ("rnb.silk",   "Smooth Silk",    "Soft - Warm - Polished",        t (0.55f, 0.78f, 0.50f, 0.38f, 0.30f, 0.25f, 0.40f), "rnb1"),
                d ("rnb.neo",    "Neo Soul",       "Live - Dusty - Laid Back",      t (0.48f, 0.75f, 0.45f, 0.45f, 0.68f, 0.25f, 0.80f), "rnb1", 0),
                d ("rnb.modern", "Modern R&B",     "Crisp - Snappy - Deep",         t (0.75f, 0.55f, 0.62f, 0.25f, 0.10f, 0.45f, 0.20f), "trap2"),
                d ("rnb.90s",    "90s Groove",     "Swing - Punch - Vintage",       t (0.70f, 0.65f, 0.50f, 0.30f, 0.72f, 0.45f, 0.45f), "boombap1"),
            } });

            s.push_back ({ Genre::pop, 108, false, 2, 0.0f, { P::shaker, P::tambourine, P::snap, P::other },
            {
                d ("pop.radio",  "Radio Polish",   "Bright - Tight - Modern",       t (0.80f, 0.50f, 0.75f, 0.25f, 0.10f, 0.55f, 0.35f), "pop1"),
                d ("pop.big",    "Big Room Pop",   "Huge - Roomy - Anthemic",       t (0.75f, 0.50f, 0.65f, 0.62f, 0.10f, 0.60f, 0.40f), "pop2"),
                d ("pop.indie",  "Indie Organic",  "Live - Warm - Natural",         t (0.50f, 0.65f, 0.50f, 0.42f, 0.45f, 0.30f, 0.85f), "pop1", 0),
                d ("pop.80s",    "Synth Pop 80s",  "Gated - Shiny - Retro",         t (0.72f, 0.45f, 0.70f, 0.58f, 0.62f, 0.50f, 0.15f), "pop2", 0),
            } });

            s.push_back ({ Genre::rock, 120, false, 1, 0.0f, { P::tambourine, P::cowbell, P::shaker },
            {
                d ("rock.tight", "Studio Tight",   "Controlled - Punchy - Polished", t (0.80f, 0.55f, 0.60f, 0.30f, 0.25f, 0.60f, 0.90f), "rock1"),
                d ("rock.arena", "Arena Thunder",  "Big Live Drums - Roomy",          t (0.85f, 0.50f, 0.60f, 0.85f, 0.20f, 0.80f, 0.85f), "rock2"),
                d ("rock.garage","Garage Grit",    "Raw - Dirty - Loud",              t (0.70f, 0.50f, 0.55f, 0.45f, 0.70f, 0.85f, 0.85f), "rock1"),
                d ("rock.hybrid","Modern Hybrid",  "Acoustic + Electronic Layers",    t (0.90f, 0.45f, 0.70f, 0.35f, 0.10f, 0.85f, 0.55f), "rock2"),
            } });

            s.push_back ({ Genre::country, 112, false, 2, 0.1f, { P::tambourine, P::shaker, P::cowbell, P::block },
            {
                d ("cty.nash",   "Nashville Clean","Clean - Tight - Natural",        t (0.60f, 0.60f, 0.60f, 0.35f, 0.20f, 0.35f, 0.95f), "country1"),
                d ("cty.tonk",   "Honky Tonk",     "Warm - Vintage - Brushy",        t (0.50f, 0.70f, 0.50f, 0.40f, 0.70f, 0.30f, 0.95f), "country2"),
                d ("cty.arena",  "Arena Country",  "Big - Bright - Live",            t (0.80f, 0.55f, 0.65f, 0.70f, 0.10f, 0.60f, 0.80f), "rock2"),
                d ("cty.trap",   "Country Trap",   "808s - Snaps - Modern",          t (0.80f, 0.50f, 0.60f, 0.25f, 0.10f, 0.60f, 0.35f), "trap2", 1),
            } });

            s.push_back ({ Genre::worship, 72, false, 2, 0.0f, { P::tambourine, P::shaker, P::triangle },
            {
                d ("wor.live",   "Sunday Live",    "Live - Roomy - Warm",            t (0.60f, 0.65f, 0.55f, 0.62f, 0.25f, 0.35f, 0.95f), "worship1"),
                d ("wor.gospel", "Gospel Chops",   "Snappy - Hard - Tight",          t (0.85f, 0.55f, 0.65f, 0.30f, 0.20f, 0.65f, 0.50f), "gospel1", 1),
                d ("wor.swell",  "Ambient Swell",  "Soft - Huge - Atmospheric",      t (0.40f, 0.70f, 0.45f, 0.85f, 0.20f, 0.20f, 0.75f), "worship1"),
                d ("wor.modern", "Modern Worship", "Big - Clean - Driving",          t (0.75f, 0.55f, 0.60f, 0.55f, 0.10f, 0.50f, 0.60f), "pop2"),
            } });

            s.push_back ({ Genre::edm, 126, false, 2, 0.0f, { P::shaker, P::other, P::snap, P::cowbell },
            {
                d ("edm.fest",   "Festival",       "Huge - Bright - Slamming",       t (0.95f, 0.40f, 0.80f, 0.50f, 0.05f, 0.85f, 0.05f), "edm1"),
                d ("edm.house",  "House",          "Groovy - Warm - Classic",        t (0.75f, 0.55f, 0.65f, 0.35f, 0.35f, 0.50f, 0.15f), "house1"),
                d ("edm.future", "Future Bass",    "Snappy - Wide - Glossy",         t (0.80f, 0.50f, 0.75f, 0.45f, 0.10f, 0.60f, 0.10f), "trap2", 1),
                d ("edm.techno", "Warehouse",      "Dark - Driving - Industrial",    t (0.80f, 0.40f, 0.50f, 0.60f, 0.35f, 0.78f, 0.10f), "edm1"),
            } });

            s.push_back ({ Genre::lofi, 82, false, 2, 0.6f, { P::shaker, P::conga, P::snap, P::block },
            {
                d ("lofi.tape",  "Tape Hiss",      "Muffled - Warm - Worn",          t (0.40f, 0.82f, 0.25f, 0.35f, 0.95f, 0.20f, 0.60f), "lofi1"),
                d ("lofi.night", "Late Night",     "Dark - Soft - Dreamy",           t (0.45f, 0.75f, 0.30f, 0.45f, 0.80f, 0.20f, 0.50f), "lofi1"),
                d ("lofi.jazz",  "Jazz Hop",       "Brushy - Live - Swing",          t (0.50f, 0.70f, 0.42f, 0.42f, 0.75f, 0.30f, 0.80f), "boombap2"),
                d ("lofi.bed",   "Bedroom Beats",  "Clean-ish - Cozy - Modern",      t (0.58f, 0.65f, 0.42f, 0.30f, 0.55f, 0.30f, 0.35f), "lofi1", 1),
            } });

            s.push_back ({ Genre::cinematic, 100, false, 3, 0.0f, { P::other, P::conga, P::triangle, P::block },
            {
                d ("cin.epic",   "Epic Trailer",   "Massive - Roomy - Hard",         t (0.90f, 0.50f, 0.50f, 0.95f, 0.10f, 0.80f, 0.60f), "cinematic1"),
                d ("cin.dark",   "Dark Tension",   "Low - Ominous - Wide",           t (0.60f, 0.50f, 0.30f, 0.80f, 0.20f, 0.60f, 0.40f), "cinematic1"),
                d ("cin.hybrid", "Hybrid Score",   "Processed - Punchy - Big",       t (0.80f, 0.45f, 0.60f, 0.70f, 0.10f, 0.70f, 0.35f), "cinematic1", 1),
                d ("cin.organic","Organic Score",  "Hand Drums - Natural - Warm",    t (0.50f, 0.70f, 0.45f, 0.75f, 0.30f, 0.40f, 0.90f), "cinematic1"),
            } });
            return s;
        }
    }

    const GenreStyle& styleFor (Genre g)
    {
        static const auto styles = makeStyles();
        for (auto& s : styles)
            if (s.genre == g) return s;
        return styles[1];
    }

    Traits genreCenter (Genre g)
    {
        const auto& s = styleFor (g);
        Traits c;
        for (int i = 0; i < Traits::num; ++i)
        {
            float sum = 0;
            for (auto& d : s.directions) sum += d.target[i];
            c[i] = sum / (float) s.directions.size();
        }
        return c;
    }

    //==============================================================================
    bool KitOptions::wants808() const
    {
        return lowEnd < 0 ? styleFor (genre).use808 : lowEnd == 1;
    }

    int KitOptions::wantedPerc() const
    {
        return percCount < 0 ? styleFor (genre).percCount : juce::jlimit (0, 3, percCount);
    }

    Traits KitOptions::targetFor (const Direction& dir) const
    {
        const auto center = genreCenter (genre);
        Traits out;
        for (int i = 0; i < Traits::num; ++i)
            out[i] = juce::jlimit (0.0f, 1.0f, dir.target[i] + (character[i] - center[i]));

        // a little of another genre's character ("rock with a hip-hop influence")
        if (secondaryGenre >= 0 && secondaryGenre < numGenres && secondaryGenre != (int) genre)
        {
            const auto other = genreCenter ((Genre) secondaryGenre);
            for (int i = 0; i < Traits::num; ++i)
                out[i] = juce::jlimit (0.0f, 1.0f, out[i] * 0.72f + other[i] * 0.28f);
        }

        switch (feel)
        {
            case 1:  out.organic = juce::jlimit (0.0f, 1.0f, juce::jmax (out.organic, 0.75f)); break;
            case 2:  out.organic = 0.5f; break;
            case 3:  out.organic = juce::jlimit (0.0f, 1.0f, juce::jmin (out.organic, 0.2f)); break;
            default: break;
        }
        return out;
    }

    juce::var KitOptions::toVar() const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("genre", genreId (genre));
        o->setProperty ("secondary", secondaryGenre);
        o->setProperty ("character", character.toVar());
        o->setProperty ("feel", feel);
        o->setProperty ("lowEnd", lowEnd);
        o->setProperty ("perc", percCount);
        o->setProperty ("fx", fx);
        o->setProperty ("tuneToKey", tuneToKey);
        o->setProperty ("key", key);
        o->setProperty ("bpm", bpm);
        juce::Array<juce::var> src;
        for (bool b : sources) src.add (b);
        o->setProperty ("sources", src);
        return juce::var (o);
    }

    KitOptions KitOptions::fromVar (const juce::var& v)
    {
        KitOptions k;
        if (! v.isObject()) return k;
        k.genre = genreFromId (v["genre"].toString());
        k.secondaryGenre = (int) v.getProperty ("secondary", -1);
        k.character = v.hasProperty ("character") ? Traits::fromVar (v["character"]) : genreCenter (k.genre);
        k.feel = juce::jlimit (0, 3, (int) v["feel"]);
        k.lowEnd = juce::jlimit (-1, 1, (int) v.getProperty ("lowEnd", -1));
        k.percCount = juce::jlimit (-1, 3, (int) v.getProperty ("perc", -1));
        k.fx = (bool) v["fx"];
        k.tuneToKey = (bool) v.getProperty ("tuneToKey", true);
        k.key = juce::jlimit (-1, 23, (int) v.getProperty ("key", -1));
        k.bpm = (double) v.getProperty ("bpm", 0.0);
        if (auto* a = v["sources"].getArray())
            for (int i = 0; i < juce::jmin ((int) k.sources.size(), a->size()); ++i)
                k.sources[(size_t) i] = (bool) a->getReference (i);
        return k;
    }
}
