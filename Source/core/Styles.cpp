#include "Styles.h"
#include <cmath>

namespace gg
{
    namespace
    {
        using P = PercKind;
        enum { E60 = 0, E70, E80, E90, E2000, E2010, ENOW };

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

        Style style (Genre g, const char* id, const char* name, std::initializer_list<const char*> keywords, std::vector<int> eras,
                     double bpm, bool use808, int perc, float swing, std::vector<PercKind> prefs, std::vector<Direction> dirs)
        {
            Style s;
            s.genre = g;
            s.id = id;
            s.name = name;
            for (auto* k : keywords) s.keywords.add (k);
            s.eras = std::move (eras);
            s.bpm = bpm;
            s.use808 = use808;
            s.percCount = perc;
            s.swing = swing;
            s.percPrefs = std::move (prefs);
            s.directions = std::move (dirs);
            return s;
        }

        std::vector<std::vector<Style>> makeStyles()
        {
            std::vector<std::vector<Style>> all ((size_t) numGenres);
            auto add = [&all] (Style s) { all[(size_t) s.genre].push_back (std::move (s)); };

            //==========================================================================
            // HIP-HOP
            add (style (Genre::boomBap, "hiphop", "General", {}, {}, 90, false, 2, 0.55f, { P::tambourine, P::shaker, P::conga, P::snap }, {
                d ("bb.street",  "Boom Bap Classic", "Warm - Punchy - Vintage",       t (0.72f, 0.65f, 0.48f, 0.30f, 0.70f, 0.52f, 0.55f), "boombap1"),
                d ("bb.dusty",   "Dusty Hip-Hop",    "Gritty - Dark - Lo-Fi",         t (0.50f, 0.75f, 0.30f, 0.35f, 0.92f, 0.38f, 0.60f), "boombap2"),
                d ("bb.modern",  "Modern Hip-Hop Knock",     "Clean - Hard Hitting",          t (0.88f, 0.50f, 0.62f, 0.20f, 0.28f, 0.68f, 0.40f), "boombap1", 1),
                d ("bb.jazzy",   "Jazzy Hip-Hop",    "Live Feel - Organic",           t (0.45f, 0.70f, 0.50f, 0.50f, 0.60f, 0.25f, 0.85f), "boombap2") }));
            add (style (Genre::boomBap, "hiphop.east", "East Coast / Boom Bap", { "east coast", "boom bap", "boombap", "golden era", "ny hip hop", "new york hip hop", "jazz rap", "queens" },
                        { E90 }, 92, false, 2, 0.55f, { P::tambourine, P::shaker, P::conga }, {
                d ("east.golden",   "Golden Era Boom Bap",     "Classic - Punchy - Sampled",  t (0.75f, 0.65f, 0.50f, 0.30f, 0.72f, 0.55f, 0.55f), "boombap1"),
                d ("east.breaks",   "Dusty Boom Bap Breaks",   "Crunchy - Dark - Vinyl",      t (0.55f, 0.72f, 0.32f, 0.35f, 0.90f, 0.45f, 0.65f), "boombap2"),
                d ("east.concrete", "Concrete Boom Bap", "Hard - Gritty - Loud",        t (0.88f, 0.55f, 0.50f, 0.25f, 0.60f, 0.75f, 0.50f), "boombap1"),
                d ("east.jazz",     "Jazz Rap Pocket",       "Brushy - Warm - Live",        t (0.45f, 0.72f, 0.48f, 0.45f, 0.62f, 0.25f, 0.85f), "boombap2") }));
            add (style (Genre::boomBap, "hiphop.west", "West Coast / G-Funk", { "g funk", "gfunk", "west coast", "westcoast", "la hip hop", "lowrider", "talkbox" },
                        { E90 }, 94, true, 2, 0.2f, { P::tambourine, P::shaker, P::cowbell }, {
                d ("west.lowrider", "Westside Classic",       "Smooth - Clap - Bounce",      t (0.72f, 0.68f, 0.60f, 0.30f, 0.45f, 0.45f, 0.25f), "gfunk1"),
                d ("west.sunset",   "Sunset G-Funk",    "Bright - Snappy - Laid Back", t (0.78f, 0.55f, 0.72f, 0.35f, 0.35f, 0.50f, 0.20f), "gfunk1"),
                d ("west.knock",    "Westside Knock",  "Hard - Deep - Punchy",        t (0.90f, 0.60f, 0.55f, 0.22f, 0.40f, 0.70f, 0.25f), "gfunk1"),
                d ("west.funk",     "G-Funk Machine",   "Analog - Warm - Groovy",      t (0.65f, 0.75f, 0.50f, 0.30f, 0.60f, 0.35f, 0.30f), "funk1") }));
            add (style (Genre::boomBap, "hiphop.memphis", "Memphis / Phonk", { "memphis", "phonk", "drift phonk", "cowbell phonk", "horrorcore" },
                        { E90 }, 136, true, 2, 0.0f, { P::cowbell, P::other, P::shaker }, {
                d ("mem.tape",      "Tape Phonk",     "Lo-Fi - Dark - Hissy",        t (0.65f, 0.60f, 0.30f, 0.30f, 0.90f, 0.60f, 0.15f), "memphis1"),
                d ("mem.drift",     "Cowbell Phonk",  "Distorted - Loud - Driving",  t (0.85f, 0.45f, 0.60f, 0.25f, 0.45f, 0.88f, 0.10f), "memphis1"),
                d ("mem.crunchy",   "Crunchy Phonk 808",    "Saturated - Heavy - Raw",     t (0.80f, 0.55f, 0.45f, 0.25f, 0.70f, 0.80f, 0.12f), "memphis1"),
                d ("mem.dark",      "Dark Memphis",   "Murky - Cold - Eerie",        t (0.60f, 0.50f, 0.25f, 0.45f, 0.75f, 0.55f, 0.15f), "memphis1") }));
            add (style (Genre::boomBap, "hiphop.south", "Southern / Crunk", { "crunk", "dirty south", "southern hip hop", "southern rap", "bounce music", "chopped and screwed", "screwed", "snap music", "houston" },
                        { E2000 }, 78, true, 2, 0.1f, { P::snap, P::cowbell, P::other }, {
                d ("south.crunk",   "Crunk Club",     "Loud - Clappy - Hype",        t (0.88f, 0.50f, 0.65f, 0.30f, 0.25f, 0.85f, 0.15f), "crunk1"),
                d ("south.dirty",   "Dirty South Heavy",    "Heavy - Deep - Gritty",       t (0.82f, 0.60f, 0.50f, 0.30f, 0.40f, 0.70f, 0.20f), "crunk1"),
                d ("south.snap",    "Southern Snap",    "Snaps - Minimal - Bouncy",    t (0.75f, 0.50f, 0.70f, 0.20f, 0.20f, 0.45f, 0.15f), "crunk1"),
                d ("south.screwed", "Chopped & Screwed",   "Slow - Syrupy - Dark",        t (0.60f, 0.65f, 0.30f, 0.50f, 0.55f, 0.40f, 0.20f), "crunk1") }));
            add (style (Genre::boomBap, "hiphop.oldschool", "Old School / Electro", { "old school", "oldschool", "electro rap", "electro funk", "breakdance", "b boy", "bboy", "early hip hop" },
                        { E80 }, 108, true, 2, 0.1f, { P::cowbell, P::block, P::other }, {
                d ("old.block",     "Old School Block Party",    "Raw - Punchy - Breaks",       t (0.78f, 0.60f, 0.55f, 0.35f, 0.75f, 0.55f, 0.50f), "oldschool1"),
                d ("old.electro",   "Electro Breaks", "Machine - Snappy - Robotic",  t (0.75f, 0.45f, 0.70f, 0.35f, 0.55f, 0.50f, 0.08f), "oldschool1"),
                d ("old.boombox",   "Old School Boombox",        "Booming - Bright - Classic",  t (0.80f, 0.55f, 0.65f, 0.40f, 0.60f, 0.60f, 0.20f), "oldschool1"),
                d ("old.scratch",   "Old School Scratch",    "Dusty - Live Breaks - Raw",   t (0.70f, 0.60f, 0.50f, 0.40f, 0.80f, 0.50f, 0.65f), "boombap1") }));

            //==========================================================================
            // TRAP / DRILL
            add (style (Genre::trap, "trap", "General", {}, {}, 142, true, 2, 0.0f, { P::snap, P::other, P::shaker, P::cowbell }, {
                d ("trap.hard",   "Trap Hard Hitter",   "Punchy - Bright - Aggressive",  t (0.90f, 0.40f, 0.72f, 0.15f, 0.10f, 0.85f, 0.10f), "trap1"),
                d ("trap.drill",  "Dark Drill",    "Dark - Sliding - Cold",         t (0.80f, 0.50f, 0.42f, 0.22f, 0.20f, 0.75f, 0.08f), "drill1"),
                d ("trap.bounce", "Melodic Trap Bounce","Clean - Bouncy - Polished",     t (0.70f, 0.60f, 0.62f, 0.28f, 0.12f, 0.45f, 0.15f), "trap2"),
                d ("trap.under",  "Underground Trap",   "Gritty - Raw - Lo-Fi",          t (0.66f, 0.55f, 0.45f, 0.30f, 0.60f, 0.70f, 0.22f), "trap1") }));
            add (style (Genre::trap, "trap.atlanta", "Atlanta Trap", { "atlanta", "atl", "street trap", "hard trap", "southside" },
                        { E2010, ENOW }, 140, true, 2, 0.0f, { P::snap, P::other, P::cowbell }, {
                d ("atl.house",     "ATL Trap House",     "Hard - Clean - Knocking",     t (0.88f, 0.45f, 0.68f, 0.18f, 0.10f, 0.80f, 0.08f), "trap1"),
                d ("atl.rolling",   "ATL Rolling Hats",   "Crisp - Fast - Bright",       t (0.82f, 0.40f, 0.80f, 0.15f, 0.08f, 0.70f, 0.08f), "trap1"),
                d ("atl.big808",    "ATL Big 808",        "Deep - Booming - Wide",       t (0.80f, 0.60f, 0.50f, 0.30f, 0.12f, 0.75f, 0.08f), "trap1"),
                d ("atl.anthem",    "ATL Street Anthem",  "Loud - Aggressive - Punchy",  t (0.92f, 0.45f, 0.65f, 0.25f, 0.10f, 0.90f, 0.10f), "trap1") }));
            add (style (Genre::trap, "trap.ukdrill", "UK Drill", { "uk drill", "london drill", "drill uk", "road rap", "uk rap" },
                        { ENOW }, 142, true, 2, 0.0f, { P::other, P::snap, P::shaker }, {
                d ("uk.road",       "Road Drill",     "Cold - Sliding - Sharp",      t (0.82f, 0.40f, 0.60f, 0.15f, 0.12f, 0.78f, 0.08f), "ukdrill1"),
                d ("uk.grime",      "Grimey UK Drill",     "Gritty - Raw - Hard",         t (0.85f, 0.40f, 0.55f, 0.20f, 0.40f, 0.85f, 0.12f), "ukdrill1"),
                d ("uk.night",      "UK Night Slide",    "Dark - Deep - Minimal",       t (0.75f, 0.50f, 0.38f, 0.25f, 0.18f, 0.65f, 0.08f), "ukdrill1"),
                d ("uk.sharp",      "Sharp UK Drill",   "Snappy - Bright - Tight",     t (0.88f, 0.38f, 0.75f, 0.15f, 0.08f, 0.72f, 0.10f), "ukdrill1") }));
            add (style (Genre::trap, "trap.nydrill", "NY Drill", { "ny drill", "brooklyn drill", "bronx drill", "new york drill", "sample drill", "jersey drill" },
                        { ENOW }, 142, true, 2, 0.0f, { P::other, P::snap }, {
                d ("ny.brooklyn",   "Brooklyn Drill Bounce","Bouncy - Hard - Clean",       t (0.88f, 0.45f, 0.65f, 0.20f, 0.12f, 0.80f, 0.10f), "nydrill1"),
                d ("ny.sample",     "Sample Drill",   "Gritty - Sampled - Raw",      t (0.82f, 0.50f, 0.50f, 0.25f, 0.45f, 0.78f, 0.20f), "nydrill1"),
                d ("ny.heavy",      "NY Heavy Kicks",    "Punchy - Loud - Deep",        t (0.95f, 0.50f, 0.55f, 0.18f, 0.10f, 0.88f, 0.08f), "nydrill1"),
                d ("ny.late",       "Late Night NY Drill",     "Dark - Cold - Moody",         t (0.75f, 0.50f, 0.35f, 0.30f, 0.20f, 0.65f, 0.10f), "nydrill1") }));
            add (style (Genre::trap, "trap.rage", "Rage / Hyper", { "rage", "hyper trap", "rage beat", "distorted trap", "opium" },
                        { ENOW }, 155, true, 1, 0.0f, { P::other, P::snap }, {
                d ("rage.distort",  "Distorted Rage",      "Blown Out - Loud - Wild",     t (0.90f, 0.40f, 0.70f, 0.20f, 0.20f, 0.98f, 0.05f), "rage1"),
                d ("rage.arena",    "Arena Rage",     "Huge - Bright - Roomy",       t (0.88f, 0.40f, 0.75f, 0.55f, 0.08f, 0.88f, 0.05f), "rage1"),
                d ("rage.overdrive","Overdrive Rage",      "Saturated - Punchy - Hot",    t (0.92f, 0.50f, 0.62f, 0.20f, 0.30f, 0.92f, 0.05f), "rage1"),
                d ("rage.clean",    "Clean Hyper Rage",    "Crisp - Fast - Polished",     t (0.85f, 0.38f, 0.82f, 0.20f, 0.05f, 0.75f, 0.05f), "rage1") }));
            add (style (Genre::trap, "trap.plugg", "Plugg / Melodic", { "plugg", "pluggnb", "plug n b", "melodic trap", "emo rap", "emo trap" },
                        { E2010, ENOW }, 150, true, 2, 0.0f, { P::snap, P::shaker }, {
                d ("plugg.rnb",     "Pluggnb",        "Soft - Smooth - Airy",        t (0.62f, 0.65f, 0.62f, 0.35f, 0.15f, 0.35f, 0.10f), "trap2"),
                d ("plugg.soft",    "Soft Plugg",     "Gentle - Bouncy - Light",     t (0.58f, 0.60f, 0.68f, 0.30f, 0.12f, 0.30f, 0.08f), "trap2"),
                d ("plugg.dreamy",  "Dreamy Plugg",         "Spacious - Warm - Hazy",      t (0.55f, 0.70f, 0.50f, 0.55f, 0.25f, 0.30f, 0.10f), "trap2"),
                d ("plugg.bounce",  "Sweet Plugg Bounce",   "Clean - Snappy - Sweet",      t (0.72f, 0.55f, 0.68f, 0.25f, 0.10f, 0.45f, 0.10f), "trap2") }));

            //==========================================================================
            // R&B / SOUL
            add (style (Genre::rnb, "rnb", "General", {}, {}, 76, true, 2, 0.25f, { P::snap, P::shaker, P::conga, P::triangle }, {
                d ("rnb.silk",   "Smooth Silk R&B",    "Soft - Warm - Polished",        t (0.55f, 0.78f, 0.50f, 0.38f, 0.30f, 0.25f, 0.40f), "rnb1"),
                d ("rnb.neo",    "Neo Soul",       "Live - Dusty - Laid Back",      t (0.48f, 0.75f, 0.45f, 0.45f, 0.68f, 0.25f, 0.80f), "rnb1", 0),
                d ("rnb.modern", "Modern R&B",     "Crisp - Snappy - Deep",         t (0.75f, 0.55f, 0.62f, 0.25f, 0.10f, 0.45f, 0.20f), "trap2"),
                d ("rnb.90s",    "90s R&B Groove",     "Swing - Punch - Vintage",       t (0.70f, 0.65f, 0.50f, 0.30f, 0.72f, 0.45f, 0.45f), "boombap1") }));
            add (style (Genre::rnb, "rnb.newjack", "90s R&B / New Jack", { "new jack", "new jack swing", "njs", "slow jam", "quiet storm" },
                        { E90 }, 100, false, 2, 0.5f, { P::snap, P::shaker, P::tambourine }, {
                d ("nj.newjack",    "New Jack Swing",       "Swing - Snappy - Machine",    t (0.82f, 0.55f, 0.68f, 0.30f, 0.55f, 0.60f, 0.15f), "newjack1"),
                d ("nj.slowjam",    "90s Slow Jam",       "Smooth - Warm - Lush",        t (0.55f, 0.78f, 0.45f, 0.45f, 0.50f, 0.25f, 0.30f), "rnb1"),
                d ("nj.storm",      "Quiet Storm",    "Soft - Roomy - Silky",        t (0.45f, 0.75f, 0.48f, 0.55f, 0.45f, 0.20f, 0.40f), "rnb1"),
                d ("nj.street",     "Street Soul",    "Punchy - Gritty - Swung",     t (0.78f, 0.62f, 0.52f, 0.30f, 0.62f, 0.55f, 0.35f), "newjack1") }));
            add (style (Genre::rnb, "rnb.neosoul", "Neo Soul", { "neo soul", "neosoul", "nu soul" },
                        { E2000 }, 82, false, 2, 0.45f, { P::conga, P::shaker, P::triangle }, {
                d ("neo.pocket",    "Neo Soul Pocket",    "Live - Loose - Warm",         t (0.50f, 0.75f, 0.48f, 0.42f, 0.55f, 0.25f, 0.88f), "rnb1"),
                d ("neo.dusty",     "Dusty Neo Soul",     "Vinyl - Warm - Laid Back",    t (0.48f, 0.78f, 0.35f, 0.40f, 0.80f, 0.25f, 0.70f), "boombap2"),
                d ("neo.rhodes",    "Rhodes Room Soul",    "Roomy - Soft - Natural",      t (0.42f, 0.72f, 0.50f, 0.60f, 0.50f, 0.20f, 0.85f), "rnb1"),
                d ("neo.late",      "Late Night Neo Soul", "Deep - Smooth - Swung",    t (0.55f, 0.75f, 0.42f, 0.35f, 0.60f, 0.30f, 0.60f), "boombap2") }));
            add (style (Genre::rnb, "rnb.alt", "Modern / Alt R&B", { "alt r&b", "alternative r&b", "dark r&b", "pbr&b", "alt r b", "alternative r b", "alt rnb", "trap soul", "trapsoul", "dark r b", "dark rnb", "pbr b", "pbrnb" },
                        { E2010, ENOW }, 70, true, 2, 0.1f, { P::snap, P::shaker }, {
                d ("alt.trapsoul",  "Trap Soul",      "808s - Snaps - Moody",        t (0.72f, 0.55f, 0.55f, 0.30f, 0.12f, 0.45f, 0.12f), "trap2"),
                d ("alt.dark",      "Dark R&B",       "Dark - Spacious - Deep",      t (0.65f, 0.55f, 0.35f, 0.50f, 0.15f, 0.40f, 0.12f), "rnb1"),
                d ("alt.airy",      "Airy R&B",           "Bright - Soft - Floating",     t (0.55f, 0.55f, 0.72f, 0.50f, 0.10f, 0.25f, 0.15f), "rnb1"),
                d ("alt.minimal",   "Minimal R&B",        "Dry - Clean - Tight",         t (0.70f, 0.50f, 0.60f, 0.15f, 0.08f, 0.35f, 0.15f), "trap2") }));
            add (style (Genre::rnb, "rnb.funk", "Funk / Motown", { "motown", "funk", "p funk", "pfunk", "classic soul", "disco funk" },
                        { E60, E70 }, 108, false, 2, 0.15f, { P::tambourine, P::conga, P::cowbell }, {
                d ("funk.motown",   "Motown Classic",         "Tambourine - Warm - Classic", t (0.55f, 0.72f, 0.52f, 0.45f, 0.80f, 0.30f, 0.92f), "motown1"),
                d ("funk.break",    "Funk Break",     "Tight - Snappy - Ghost Notes",t (0.75f, 0.65f, 0.58f, 0.30f, 0.70f, 0.50f, 0.90f), "funk1"),
                d ("funk.disco",    "Disco Funk",     "Four On Floor - Bright - Lush", t (0.70f, 0.60f, 0.65f, 0.40f, 0.60f, 0.45f, 0.75f), "disco1"),
                d ("funk.sweet",    "Sweet Soul",     "Soft - Roomy - Vintage",      t (0.50f, 0.75f, 0.45f, 0.50f, 0.85f, 0.25f, 0.92f), "motown1") }));

            //==========================================================================
            // POP
            add (style (Genre::pop, "pop", "General", {}, {}, 108, false, 2, 0.0f, { P::shaker, P::tambourine, P::snap, P::other }, {
                d ("pop.radio",  "Radio Pop Polish",   "Bright - Tight - Modern",       t (0.80f, 0.50f, 0.75f, 0.25f, 0.10f, 0.55f, 0.35f), "pop1"),
                d ("pop.big",    "Big Room Pop",   "Huge - Roomy - Anthemic",       t (0.75f, 0.50f, 0.65f, 0.62f, 0.10f, 0.60f, 0.40f), "pop2"),
                d ("pop.indie",  "Indie Organic Pop",  "Live - Warm - Natural",         t (0.50f, 0.65f, 0.50f, 0.42f, 0.45f, 0.30f, 0.85f), "pop1", 0),
                d ("pop.80s",    "80s Synth Pop",  "Gated - Shiny - Retro",         t (0.72f, 0.45f, 0.70f, 0.58f, 0.62f, 0.50f, 0.15f), "synthpop1", 0) }));
            add (style (Genre::pop, "pop.synth", "Synth Pop / 80s", { "synth pop", "synthpop", "synthwave", "new wave", "retrowave", "outrun" },
                        { E80 }, 118, false, 2, 0.0f, { P::shaker, P::tambourine, P::other }, {
                d ("synth.gated",   "Gated Synth Pop",    "Huge - Gated - Shiny",        t (0.80f, 0.45f, 0.72f, 0.75f, 0.55f, 0.60f, 0.15f), "synthpop1"),
                d ("synth.neon",    "Neon Synth Pop",   "Drum Machine - Bright - Clean", t (0.78f, 0.40f, 0.78f, 0.45f, 0.45f, 0.55f, 0.05f), "synthpop1"),
                d ("synth.wave",    "New Wave",       "Dry - Snappy - Cool",         t (0.72f, 0.48f, 0.68f, 0.30f, 0.55f, 0.50f, 0.25f), "synthpop1"),
                d ("synth.retro",   "Retrowave",      "Wide - Reverb - Punchy",      t (0.82f, 0.45f, 0.65f, 0.65f, 0.40f, 0.60f, 0.08f), "synthpop1") }));
            add (style (Genre::pop, "pop.dance", "Dance Pop", { "dance pop", "club pop", "disco pop", "eurodance", "electropop", "electro pop" },
                        { E2000, E2010 }, 124, false, 2, 0.0f, { P::shaker, P::snap, P::other }, {
                d ("dance.club",    "Club Pop",       "Punchy - Bright - Club",      t (0.88f, 0.45f, 0.75f, 0.30f, 0.08f, 0.65f, 0.08f), "dancepop1"),
                d ("dance.floor",   "Four On The Floor Pop", "Driving - Tight - Glossy", t (0.85f, 0.45f, 0.72f, 0.25f, 0.10f, 0.60f, 0.05f), "dancepop1"),
                d ("dance.disco",   "Disco Pop",      "Groovy - Lush - Retro",       t (0.75f, 0.55f, 0.68f, 0.40f, 0.45f, 0.45f, 0.45f), "disco1"),
                d ("dance.banger",  "Dance Pop Banger",   "Loud - Polished - Huge",      t (0.90f, 0.45f, 0.78f, 0.40f, 0.05f, 0.72f, 0.15f), "dancepop1") }));
            add (style (Genre::pop, "pop.rock", "Pop Rock", { "pop rock", "power pop", "arena pop", "pop rock ballad" },
                        { E80, E2000 }, 128, false, 1, 0.0f, { P::tambourine, P::shaker }, {
                d ("poprock.arena", "Arena Pop Rock", "Huge - Live - Anthemic",      t (0.85f, 0.50f, 0.65f, 0.75f, 0.20f, 0.70f, 0.82f), "rock2"),
                d ("poprock.power", "Power Pop",      "Bright - Tight - Punchy",     t (0.85f, 0.50f, 0.72f, 0.35f, 0.20f, 0.65f, 0.85f), "rock1"),
                d ("poprock.radio", "Radio Pop Rock",     "Polished - Big - Clean",      t (0.82f, 0.50f, 0.68f, 0.50f, 0.10f, 0.60f, 0.75f), "rock1"),
                d ("poprock.garage","Garage Pop",     "Raw - Fun - Roomy",           t (0.70f, 0.55f, 0.60f, 0.50f, 0.55f, 0.65f, 0.88f), "rock1") }));
            add (style (Genre::pop, "pop.indie", "Indie / Bedroom Pop", { "indie pop", "bedroom pop", "dream pop", "chill pop", "lo fi pop" },
                        { E2010, ENOW }, 100, false, 2, 0.1f, { P::shaker, P::tambourine, P::snap }, {
                d ("indie.bedroom", "Bedroom Pop",    "Soft - Lo-Fi - Cozy",         t (0.50f, 0.70f, 0.45f, 0.35f, 0.60f, 0.25f, 0.45f), "pop1"),
                d ("indie.dream",   "Dream Pop",      "Washy - Roomy - Soft",        t (0.45f, 0.65f, 0.55f, 0.75f, 0.40f, 0.20f, 0.55f), "pop1"),
                d ("indie.live",    "Live Indie Pop",     "Natural - Room - Warm",       t (0.60f, 0.62f, 0.55f, 0.50f, 0.40f, 0.40f, 0.85f), "pop1"),
                d ("indie.chill",   "Chill Pop",      "Clean - Smooth - Snappy",     t (0.65f, 0.60f, 0.62f, 0.30f, 0.15f, 0.35f, 0.35f), "pop1") }));

            //==========================================================================
            // ROCK
            add (style (Genre::rock, "rock", "General", {}, {}, 120, false, 1, 0.0f, { P::tambourine, P::cowbell, P::shaker }, {
                d ("rock.tight", "Rock Studio Tight",   "Controlled - Punchy - Polished", t (0.80f, 0.55f, 0.60f, 0.30f, 0.25f, 0.60f, 0.90f), "rock1"),
                d ("rock.arena", "Rock Arena Thunder",  "Big Live Drums - Roomy",          t (0.85f, 0.50f, 0.60f, 0.85f, 0.20f, 0.80f, 0.85f), "rock2"),
                d ("rock.garage","Garage Rock Grit",    "Raw - Dirty - Loud",              t (0.70f, 0.50f, 0.55f, 0.45f, 0.70f, 0.85f, 0.85f), "rock1"),
                d ("rock.hybrid","Modern Hybrid Rock",  "Acoustic + Electronic Layers",    t (0.90f, 0.45f, 0.70f, 0.35f, 0.10f, 0.85f, 0.55f), "rock2") }));
            add (style (Genre::rock, "rock.classic", "Classic Rock", { "classic rock", "blues rock", "southern rock", "psychedelic", "psych rock" },
                        { E60, E70 }, 116, false, 1, 0.1f, { P::tambourine, P::cowbell }, {
                d ("classic.dry",   "Classic Rock Dry",    "Dry - Warm - Vintage",        t (0.65f, 0.75f, 0.45f, 0.25f, 0.75f, 0.50f, 0.95f), "rock1"),
                d ("classic.room",  "Classic Rock Room", "Roomy - Natural - Live",    t (0.72f, 0.65f, 0.55f, 0.70f, 0.60f, 0.60f, 0.95f), "rock2"),
                d ("classic.blues", "Blues Rock",     "Loose - Warm - Gritty",       t (0.62f, 0.72f, 0.50f, 0.45f, 0.70f, 0.55f, 0.95f), "rock1"),
                d ("classic.stadium","70s Stadium Rock",   "Huge - Bright - Powerful",    t (0.80f, 0.60f, 0.60f, 0.80f, 0.55f, 0.70f, 0.90f), "rock2") }));
            add (style (Genre::rock, "rock.hard", "Hard Rock", { "hard rock", "arena rock", "stadium rock", "hair metal", "glam metal", "glam rock", "80s rock" },
                        { E70, E80 }, 124, false, 1, 0.0f, { P::cowbell, P::tambourine }, {
                d ("hard.hitter",   "Rock Hard Hitter",     "Big - Bright - Punchy",       t (0.88f, 0.50f, 0.68f, 0.55f, 0.25f, 0.80f, 0.85f), "rock2"),
                d ("hard.arena",    "Arena Hard Rock",      "Huge - Roomy - Anthemic",     t (0.85f, 0.55f, 0.62f, 0.85f, 0.35f, 0.75f, 0.88f), "rock2"),
                d ("hard.crunch",   "Hard Rock Crunch",     "Dry - Driving - Loud",        t (0.82f, 0.58f, 0.55f, 0.40f, 0.55f, 0.85f, 0.92f), "rock1"),
                d ("hard.glam",     "Hair Metal Gloss",     "Gated - Shiny - Huge",        t (0.85f, 0.45f, 0.75f, 0.78f, 0.40f, 0.72f, 0.70f), "rock2") }));
            add (style (Genre::rock, "rock.metal", "Metal", { "metal", "heavy metal", "metalcore", "thrash", "djent", "nu metal", "numetal", "death metal" },
                        {}, 150, false, 0, 0.0f, { P::cowbell }, {
                d ("metal.precise", "Metal Precision",      "Clicky - Tight - Triggered",  t (0.98f, 0.35f, 0.80f, 0.20f, 0.05f, 0.90f, 0.55f), "metal1"),
                d ("metal.room",    "Heavy Room Metal",     "Huge - Dark - Heavy",         t (0.88f, 0.55f, 0.50f, 0.70f, 0.20f, 0.88f, 0.80f), "metal1"),
                d ("metal.thrash",  "Thrash Metal",         "Fast - Raw - Aggressive",     t (0.92f, 0.40f, 0.68f, 0.35f, 0.40f, 0.95f, 0.75f), "metal1"),
                d ("metal.nu",      "Nu Metal Bounce",      "Tight - Heavy - Swung",       t (0.92f, 0.50f, 0.62f, 0.35f, 0.25f, 0.88f, 0.60f), "metal1") }));
            add (style (Genre::rock, "rock.alt", "Alt / Grunge", { "grunge", "alternative", "alt rock", "indie rock", "post punk", "shoegaze" },
                        { E90 }, 112, false, 1, 0.0f, { P::tambourine }, {
                d ("alt.grunge",    "Grunge Slosh",         "Sloshy - Heavy - Raw",        t (0.80f, 0.55f, 0.55f, 0.60f, 0.55f, 0.85f, 0.95f), "grunge1"),
                d ("alt.room",      "Alt Rock Room",       "Natural - Roomy - Live",      t (0.72f, 0.60f, 0.55f, 0.65f, 0.40f, 0.65f, 0.92f), "rock1"),
                d ("alt.garage",    "Indie Garage Rock",   "Thin - Trashy - Fun",         t (0.65f, 0.50f, 0.62f, 0.45f, 0.65f, 0.70f, 0.90f), "grunge1"),
                d ("alt.postpunk",  "Post-Punk",      "Dry - Driving - Cold",        t (0.75f, 0.45f, 0.65f, 0.30f, 0.45f, 0.65f, 0.70f), "rock1") }));
            add (style (Genre::rock, "rock.punk", "Punk / Pop Punk", { "punk", "pop punk", "skate punk", "emo", "hardcore punk" },
                        { E2000 }, 172, false, 0, 0.0f, { P::tambourine }, {
                d ("punk.pop",      "Pop Punk",       "Fast - Bright - Tight",       t (0.88f, 0.45f, 0.75f, 0.35f, 0.15f, 0.80f, 0.85f), "punk1"),
                d ("punk.skate",    "Skate Punk",     "Fast - Raw - Light",          t (0.80f, 0.45f, 0.70f, 0.30f, 0.40f, 0.80f, 0.88f), "punk1"),
                d ("punk.raw",      "Raw Punk",       "Trashy - Loud - Live",        t (0.78f, 0.50f, 0.62f, 0.45f, 0.70f, 0.92f, 0.92f), "punk1"),
                d ("punk.emo",      "Emo Rock",            "Roomy - Emotional - Big",     t (0.80f, 0.50f, 0.65f, 0.60f, 0.25f, 0.70f, 0.85f), "punk1") }));

            //==========================================================================
            // COUNTRY
            add (style (Genre::country, "country", "General", {}, {}, 112, false, 2, 0.1f, { P::tambourine, P::shaker, P::cowbell, P::block }, {
                d ("cty.nash",   "Nashville Clean","Clean - Tight - Natural",        t (0.60f, 0.60f, 0.60f, 0.35f, 0.20f, 0.35f, 0.95f), "country1"),
                d ("cty.tonk",   "Honky Tonk",     "Warm - Vintage - Brushy",        t (0.50f, 0.70f, 0.50f, 0.40f, 0.70f, 0.30f, 0.95f), "country2"),
                d ("cty.arena",  "Arena Country",  "Big - Bright - Live",            t (0.80f, 0.55f, 0.65f, 0.70f, 0.10f, 0.60f, 0.80f), "rock2"),
                d ("cty.trap",   "Country Trap",   "808s - Snaps - Modern",          t (0.80f, 0.50f, 0.60f, 0.25f, 0.10f, 0.60f, 0.35f), "trap2", 1) }));
            add (style (Genre::country, "country.classic", "Classic / Honky Tonk", { "honky tonk", "honkytonk", "outlaw", "classic country", "traditional country", "western swing", "bakersfield", "train beat" },
                        { E60, E70, E80, E90 }, 108, false, 2, 0.3f, { P::tambourine, P::shaker, P::block }, {
                d ("cc.tonk",       "Honky Tonk Classic",     "Dry - Woody - Shuffle",       t (0.55f, 0.72f, 0.48f, 0.30f, 0.75f, 0.30f, 0.95f), "country2"),
                d ("cc.outlaw",     "Outlaw Country",         "Raw - Warm - Live",           t (0.62f, 0.70f, 0.50f, 0.40f, 0.70f, 0.45f, 0.95f), "country2"),
                d ("cc.train",      "Country Train Beat",     "Brushy - Driving - Light",    t (0.50f, 0.65f, 0.55f, 0.35f, 0.65f, 0.30f, 0.95f), "train1"),
                d ("cc.swing",      "Western Swing",  "Brushes - Swing - Soft",      t (0.42f, 0.72f, 0.45f, 0.40f, 0.75f, 0.20f, 0.95f), "country2") }));
            add (style (Genre::country, "country.modern", "Modern Nashville", { "modern country", "country pop", "nashville", "bro country", "stadium country", "radio country" },
                        { E2010, ENOW }, 116, false, 2, 0.0f, { P::tambourine, P::shaker, P::snap }, {
                d ("mc.radio",      "Radio Country",  "Clean - Polished - Punchy",   t (0.78f, 0.55f, 0.68f, 0.35f, 0.10f, 0.50f, 0.80f), "country1"),
                d ("mc.stadium",    "Stadium Country","Huge - Bright - Live",        t (0.85f, 0.55f, 0.68f, 0.72f, 0.08f, 0.65f, 0.82f), "rock2"),
                d ("mc.snap",       "Country Snap Track",     "Snaps - Loops - Modern",      t (0.75f, 0.50f, 0.68f, 0.25f, 0.08f, 0.50f, 0.45f), "country1"),
                d ("mc.pop",        "Country Pop",    "Glossy - Tight - Bright",     t (0.78f, 0.50f, 0.75f, 0.30f, 0.08f, 0.50f, 0.55f), "pop1") }));
            add (style (Genre::country, "country.rap", "Country Rap / Trap", { "country rap", "country trap", "countrytrap", "hick hop", "country hip hop" },
                        { E2010, ENOW }, 140, true, 2, 0.0f, { P::snap, P::tambourine }, {
                d ("cr.trap",       "Country Trap",   "808s - Snaps - Twang",        t (0.80f, 0.50f, 0.60f, 0.25f, 0.10f, 0.60f, 0.35f), "trap2"),
                d ("cr.truck",      "Country Truck Bounce",   "Heavy - Deep - Bouncy",       t (0.85f, 0.55f, 0.55f, 0.25f, 0.12f, 0.70f, 0.30f), "trap1"),
                d ("cr.backroad",   "Backroad Boom Bap", "Dusty - Knock - Live",     t (0.75f, 0.62f, 0.50f, 0.30f, 0.55f, 0.55f, 0.55f), "boombap1", 0),
                d ("cr.south808",   "Southern Country 808",   "Booming - Clean - Snappy",    t (0.82f, 0.55f, 0.62f, 0.25f, 0.08f, 0.65f, 0.25f), "trap2") }));
            add (style (Genre::country, "country.americana", "Americana / Folk", { "americana", "folk", "bluegrass", "roots", "alt country", "acoustic country", "stomp and clap" },
                        { E70 }, 96, false, 2, 0.1f, { P::tambourine, P::shaker, P::block }, {
                d ("am.porch",      "Front Porch Folk",    "Light - Natural - Brushy",    t (0.45f, 0.68f, 0.50f, 0.40f, 0.55f, 0.20f, 0.98f), "country2"),
                d ("am.folkroom",   "Folk Room",      "Roomy - Soft - Organic",      t (0.45f, 0.70f, 0.48f, 0.65f, 0.50f, 0.20f, 0.98f), "worship1"),
                d ("am.roots",      "Roots Rock",     "Warm - Live - Driving",       t (0.65f, 0.68f, 0.52f, 0.45f, 0.50f, 0.50f, 0.95f), "rock1"),
                d ("am.stomp",      "Stomp & Clap Folk",   "Stomps - Claps - Big",        t (0.80f, 0.60f, 0.50f, 0.60f, 0.45f, 0.55f, 0.92f), "stomp1") }));

            //==========================================================================
            // WORSHIP / GOSPEL
            add (style (Genre::worship, "worship", "General", {}, {}, 72, false, 2, 0.0f, { P::tambourine, P::shaker, P::triangle }, {
                d ("wor.live",   "Sunday Live Worship",    "Live - Roomy - Warm",            t (0.60f, 0.65f, 0.55f, 0.62f, 0.25f, 0.35f, 0.95f), "worship1"),
                d ("wor.gospel", "Gospel Chops",   "Snappy - Hard - Tight",          t (0.85f, 0.55f, 0.65f, 0.30f, 0.20f, 0.65f, 0.50f), "gospel1", 1),
                d ("wor.swell",  "Ambient Worship Swell",  "Soft - Huge - Atmospheric",      t (0.40f, 0.70f, 0.45f, 0.85f, 0.20f, 0.20f, 0.75f), "worship1"),
                d ("wor.modern", "Modern Worship", "Big - Clean - Driving",          t (0.75f, 0.55f, 0.60f, 0.55f, 0.10f, 0.50f, 0.60f), "pop2") }));
            add (style (Genre::worship, "worship.modern", "Modern Worship", { "modern worship", "ccm", "contemporary christian", "worship pop", "praise and worship" },
                        { E2010, ENOW }, 72, false, 2, 0.0f, { P::tambourine, P::shaker }, {
                d ("mw.builder",    "Worship Builder",        "Swelling - Big - Clean",      t (0.75f, 0.55f, 0.60f, 0.65f, 0.10f, 0.50f, 0.65f), "worship1"),
                d ("mw.anthem",     "Worship Anthem",         "Huge - Driving - Bright",     t (0.82f, 0.55f, 0.65f, 0.70f, 0.08f, 0.60f, 0.65f), "pop2"),
                d ("mw.electronic", "Electronic Worship", "Pads - Claps - Modern",   t (0.72f, 0.50f, 0.65f, 0.55f, 0.08f, 0.45f, 0.30f), "pop2"),
                d ("mw.acoustic",   "Acoustic Worship",   "Cajon - Shaker - Warm",       t (0.50f, 0.68f, 0.50f, 0.45f, 0.30f, 0.30f, 0.95f), "worship1") }));
            add (style (Genre::worship, "worship.gospel", "Gospel / Church", { "gospel", "church", "gospel chops", "black gospel", "shout music", "praise break" },
                        {}, 100, true, 2, 0.15f, { P::tambourine, P::snap }, {
                d ("gos.chops",     "Gospel Chops",   "Snappy - Fast - Tight",       t (0.88f, 0.55f, 0.68f, 0.25f, 0.15f, 0.68f, 0.55f), "gospel1"),
                d ("gos.shout",     "Gospel Shout",   "Driving - Loud - Live",       t (0.82f, 0.55f, 0.62f, 0.45f, 0.30f, 0.72f, 0.80f), "gospel1"),
                d ("gos.choir",     "Gospel Choir Hall",     "Roomy - Warm - Soulful",      t (0.62f, 0.72f, 0.52f, 0.70f, 0.40f, 0.45f, 0.85f), "worship1", 0),
                d ("gos.trap",      "Trap Gospel",    "808s - Claps - Modern",       t (0.80f, 0.55f, 0.60f, 0.30f, 0.10f, 0.60f, 0.20f), "trap2") }));
            add (style (Genre::worship, "worship.ambient", "Ambient / Hymns", { "ambient worship", "prayer", "soaking", "hymn", "hymns", "devotional" },
                        {}, 68, false, 1, 0.0f, { P::triangle, P::shaker }, {
                d ("amb.prayer",    "Prayer Room",    "Soft - Huge - Gentle",        t (0.35f, 0.72f, 0.45f, 0.85f, 0.20f, 0.15f, 0.75f), "worship1"),
                d ("amb.swells",    "Worship Swells",         "Mallets - Swells - Wide",     t (0.38f, 0.70f, 0.50f, 0.90f, 0.15f, 0.15f, 0.80f), "worship1"),
                d ("amb.hymn",      "Hymn Room",      "Natural - Warm - Classic",    t (0.45f, 0.72f, 0.48f, 0.65f, 0.45f, 0.20f, 0.95f), "worship1"),
                d ("amb.pulse",     "Soft Pulse Worship",     "Minimal - Soft - Steady",     t (0.45f, 0.62f, 0.50f, 0.55f, 0.10f, 0.20f, 0.40f), "worship1") }));

            //==========================================================================
            // EDM / DANCE
            add (style (Genre::edm, "edm", "General", {}, {}, 126, false, 2, 0.0f, { P::shaker, P::other, P::snap, P::cowbell }, {
                d ("edm.fest",   "Festival EDM",       "Huge - Bright - Slamming",       t (0.95f, 0.40f, 0.80f, 0.50f, 0.05f, 0.85f, 0.05f), "edm1"),
                d ("edm.house",  "EDM House Groove",          "Groovy - Warm - Classic",        t (0.75f, 0.55f, 0.65f, 0.35f, 0.35f, 0.50f, 0.15f), "house1"),
                d ("edm.future", "Future Bass",    "Snappy - Wide - Glossy",         t (0.80f, 0.50f, 0.75f, 0.45f, 0.10f, 0.60f, 0.10f), "trap2", 1),
                d ("edm.techno", "Warehouse Techno",      "Dark - Driving - Industrial",    t (0.80f, 0.40f, 0.50f, 0.60f, 0.35f, 0.78f, 0.10f), "edm1") }));
            add (style (Genre::edm, "edm.house", "House", { "house", "deep house", "tech house", "disco house", "afro house", "uk garage", "chicago house" },
                        { E90 }, 124, false, 2, 0.1f, { P::shaker, P::other, P::conga }, {
                d ("house.classic", "Classic House",  "Groovy - Warm - Analog",      t (0.75f, 0.60f, 0.62f, 0.35f, 0.45f, 0.45f, 0.15f), "house1"),
                d ("house.deep",    "Deep House",     "Soft - Deep - Smooth",        t (0.68f, 0.68f, 0.50f, 0.40f, 0.30f, 0.35f, 0.15f), "house1"),
                d ("house.tech",    "Tech House",     "Tight - Punchy - Minimal",    t (0.85f, 0.45f, 0.68f, 0.25f, 0.15f, 0.60f, 0.08f), "house1"),
                d ("house.disco",   "Disco House",    "Bright - Groovy - Lush",      t (0.75f, 0.58f, 0.70f, 0.40f, 0.50f, 0.45f, 0.45f), "disco1") }));
            add (style (Genre::edm, "edm.techno", "Techno", { "techno", "industrial", "warehouse", "minimal techno", "detroit techno", "berlin" },
                        { E90, ENOW }, 132, false, 1, 0.0f, { P::other, P::shaker }, {
                d ("techno.ware",   "Warehouse Techno",      "Dark - Driving - Industrial", t (0.85f, 0.40f, 0.50f, 0.60f, 0.35f, 0.82f, 0.08f), "edm1"),
                d ("techno.min",    "Minimal Techno",        "Dry - Clicky - Tight",        t (0.80f, 0.40f, 0.60f, 0.20f, 0.20f, 0.55f, 0.05f), "edm1"),
                d ("techno.ind",    "Industrial Techno",     "Distorted - Harsh - Heavy",   t (0.90f, 0.35f, 0.55f, 0.45f, 0.40f, 0.95f, 0.05f), "edm1"),
                d ("techno.detroit","Detroit Techno",        "Analog - Warm - Machine",     t (0.78f, 0.55f, 0.58f, 0.35f, 0.55f, 0.55f, 0.05f), "edm1") }));
            add (style (Genre::edm, "edm.dnb", "Drum & Bass", { "drum and bass", "drum n bass", "dnb", "jungle", "liquid dnb", "neurofunk" },
                        { E90, ENOW }, 174, false, 1, 0.0f, { P::shaker, P::other }, {
                d ("dnb.liquid",    "Liquid DnB",         "Smooth - Crisp - Rolling",    t (0.75f, 0.55f, 0.70f, 0.35f, 0.20f, 0.45f, 0.40f), "dnb1"),
                d ("dnb.jungle",    "Jungle Breaks",         "Breaks - Raw - Vinyl",        t (0.75f, 0.55f, 0.60f, 0.35f, 0.75f, 0.60f, 0.70f), "dnb1"),
                d ("dnb.neuro",     "Neuro DnB",          "Hard - Tight - Aggressive",   t (0.92f, 0.40f, 0.75f, 0.25f, 0.05f, 0.88f, 0.10f), "dnb1"),
                d ("dnb.rollers",   "DnB Rollers",        "Punchy - Steady - Clean",     t (0.85f, 0.50f, 0.70f, 0.25f, 0.10f, 0.65f, 0.30f), "dnb1") }));
            add (style (Genre::edm, "edm.dubstep", "Dubstep / Bass", { "dubstep", "riddim", "bass music", "brostep", "melodic dubstep" },
                        { E2010 }, 140, false, 1, 0.0f, { P::other }, {
                d ("dub.heavy",     "Heavy Dubstep", "Huge - Loud - Wide",          t (0.95f, 0.45f, 0.70f, 0.50f, 0.05f, 0.92f, 0.05f), "dubstep1"),
                d ("dub.riddim",    "Riddim",         "Tight - Snappy - Minimal",    t (0.90f, 0.40f, 0.72f, 0.25f, 0.05f, 0.85f, 0.05f), "dubstep1"),
                d ("dub.melodic",   "Melodic Dubstep",   "Big - Bright - Emotional",    t (0.85f, 0.50f, 0.75f, 0.55f, 0.05f, 0.65f, 0.08f), "dubstep1"),
                d ("dub.deep",      "Deep Bass Music",     "Deep - Dark - Heavy",         t (0.88f, 0.50f, 0.50f, 0.45f, 0.15f, 0.80f, 0.05f), "dubstep1") }));
            add (style (Genre::edm, "edm.disco", "Disco / Nu-Disco", { "disco", "nu disco", "nudisco", "italo", "boogie", "funky house" },
                        { E70, E80 }, 120, false, 2, 0.1f, { P::tambourine, P::conga, P::shaker }, {
                d ("disco.70s",     "Studio 70s Disco",     "Live - Warm - Groovy",        t (0.65f, 0.70f, 0.60f, 0.40f, 0.75f, 0.40f, 0.85f), "disco1"),
                d ("disco.nu",      "Nu-Disco",       "Polished - Bright - Funky",   t (0.78f, 0.55f, 0.72f, 0.35f, 0.30f, 0.50f, 0.40f), "disco1"),
                d ("disco.italo",   "Italo Disco",          "Machine - Shiny - Retro",     t (0.75f, 0.45f, 0.75f, 0.45f, 0.55f, 0.50f, 0.10f), "disco1"),
                d ("disco.boogie",  "Boogie Disco",         "Snappy - Funky - Clap",       t (0.72f, 0.60f, 0.68f, 0.35f, 0.50f, 0.45f, 0.45f), "disco1") }));

            //==========================================================================
            // LO-FI
            add (style (Genre::lofi, "lofi", "General", {}, {}, 82, false, 2, 0.6f, { P::shaker, P::conga, P::snap, P::block }, {
                d ("lofi.tape",  "Tape Hiss Lo-Fi",      "Muffled - Warm - Worn",          t (0.40f, 0.82f, 0.25f, 0.35f, 0.95f, 0.20f, 0.60f), "lofi1"),
                d ("lofi.night", "Late Night Lo-Fi",     "Dark - Soft - Dreamy",           t (0.45f, 0.75f, 0.30f, 0.45f, 0.80f, 0.20f, 0.50f), "lofi1"),
                d ("lofi.jazz",  "Jazz Hop",       "Brushy - Live - Swing",          t (0.50f, 0.70f, 0.42f, 0.42f, 0.75f, 0.30f, 0.80f), "boombap2"),
                d ("lofi.bed",   "Bedroom Lo-Fi",  "Clean-ish - Cozy - Modern",      t (0.58f, 0.65f, 0.42f, 0.30f, 0.55f, 0.30f, 0.35f), "lofi1", 1) }));
            add (style (Genre::lofi, "lofi.jazzhop", "Jazz Hop / Chillhop", { "jazz hop", "jazzhop", "chillhop", "lofi hip hop", "lo fi hip hop", "study beats" },
                        {}, 85, false, 2, 0.55f, { P::shaker, P::conga, P::snap }, {
                d ("jh.jazz",       "Jazz Hop Classic",       "Brushy - Live - Swing",       t (0.50f, 0.70f, 0.42f, 0.42f, 0.75f, 0.30f, 0.82f), "boombap2"),
                d ("jh.coffee",     "Coffee Shop Lo-Fi",    "Soft - Warm - Cozy",          t (0.42f, 0.78f, 0.38f, 0.38f, 0.70f, 0.20f, 0.70f), "lofi1"),
                d ("jh.vinyl",      "Vinyl Jazz Hop",    "Crackly - Dusty - Dark",      t (0.48f, 0.75f, 0.28f, 0.35f, 0.92f, 0.25f, 0.60f), "lofi1"),
                d ("jh.session",    "Jazz Hop Session",   "Roomy - Brushes - Loose",     t (0.45f, 0.72f, 0.45f, 0.55f, 0.70f, 0.20f, 0.88f), "boombap2") }));
            add (style (Genre::lofi, "lofi.trap", "Lo-Fi Trap / Sad", { "lofi trap", "lo fi trap", "sad trap", "sad beat", "sad lofi" },
                        {}, 75, true, 1, 0.1f, { P::snap, P::shaker }, {
                d ("lt.sad808",     "Sad 808",        "Soft 808 - Snaps - Hazy",     t (0.55f, 0.68f, 0.40f, 0.40f, 0.65f, 0.30f, 0.15f), "trap2"),
                d ("lt.rainy",      "Rainy Night Lo-Fi",    "Muffled - Dark - Slow",       t (0.45f, 0.72f, 0.28f, 0.45f, 0.80f, 0.20f, 0.15f), "trap2"),
                d ("lt.tape",       "Tape Trap",      "Warbly - Warm - Lo-Fi",       t (0.55f, 0.70f, 0.35f, 0.35f, 0.88f, 0.35f, 0.20f), "trap2"),
                d ("lt.bedroom",    "Bedroom 808",    "Clean-ish - Soft - Cozy",     t (0.60f, 0.62f, 0.45f, 0.30f, 0.50f, 0.30f, 0.12f), "trap2") }));

            //==========================================================================
            // CINEMATIC
            add (style (Genre::cinematic, "cinematic", "General", {}, {}, 100, false, 3, 0.0f, { P::other, P::conga, P::triangle, P::block }, {
                d ("cin.epic",   "Epic Trailer",   "Massive - Roomy - Hard",         t (0.90f, 0.50f, 0.50f, 0.95f, 0.10f, 0.80f, 0.60f), "cinematic1"),
                d ("cin.dark",   "Dark Tension",   "Low - Ominous - Wide",           t (0.60f, 0.50f, 0.30f, 0.80f, 0.20f, 0.60f, 0.40f), "cinematic1"),
                d ("cin.hybrid", "Hybrid Score",   "Processed - Punchy - Big",       t (0.80f, 0.45f, 0.60f, 0.70f, 0.10f, 0.70f, 0.35f), "cinematic1", 1),
                d ("cin.organic","Organic Score",  "Hand Drums - Natural - Warm",    t (0.50f, 0.70f, 0.45f, 0.75f, 0.30f, 0.40f, 0.90f), "cinematic1") }));
            add (style (Genre::cinematic, "cinematic.trailer", "Epic Trailer", { "trailer", "epic trailer", "action movie", "blockbuster", "hybrid trailer" },
                        {}, 100, false, 3, 0.0f, { P::other, P::conga }, {
                d ("tr.braams",     "Trailer Braams",  "Massive - Wide - Hard",       t (0.92f, 0.50f, 0.50f, 0.95f, 0.05f, 0.85f, 0.50f), "cinematic1"),
                d ("tr.taiko",      "Taiko War",      "Huge - Tribal - Roomy",       t (0.85f, 0.60f, 0.45f, 0.90f, 0.15f, 0.75f, 0.85f), "cinematic1"),
                d ("tr.rise",       "Trailer Rise & Hit",     "Impacts - Risers - Tension",  t (0.85f, 0.45f, 0.60f, 0.85f, 0.05f, 0.80f, 0.35f), "cinematic1"),
                d ("tr.hero",       "Hero Trailer",     "Big - Bright - Driving",      t (0.85f, 0.55f, 0.65f, 0.85f, 0.10f, 0.70f, 0.60f), "cinematic1") }));
            add (style (Genre::cinematic, "cinematic.horror", "Dark / Horror", { "horror", "thriller", "suspense", "horror movie" },
                        {}, 90, false, 3, 0.0f, { P::other, P::block, P::triangle }, {
                d ("hor.dread",     "Horror Dread",          "Low - Eerie - Distant",       t (0.50f, 0.50f, 0.25f, 0.85f, 0.30f, 0.50f, 0.40f), "cinematic1"),
                d ("hor.metal",     "Industrial Horror", "Metallic - Harsh - Cold",  t (0.75f, 0.35f, 0.45f, 0.70f, 0.30f, 0.85f, 0.20f), "cinematic1"),
                d ("hor.heart",     "Horror Heartbeat",      "Deep - Slow - Tense",         t (0.65f, 0.60f, 0.30f, 0.60f, 0.20f, 0.50f, 0.50f), "cinematic1"),
                d ("hor.whisper",   "Whisper Horror",   "Soft - Reversed - Creepy",    t (0.40f, 0.50f, 0.40f, 0.75f, 0.35f, 0.30f, 0.35f), "cinematic1") }));
            add (style (Genre::cinematic, "cinematic.world", "World Percussion", { "world percussion", "tribal", "taiko", "hand drums", "ethnic", "frame drum", "frame drums" },
                        {}, 100, false, 3, 0.0f, { P::conga, P::block, P::shaker, P::other }, {
                d ("world.taiko",   "Taiko World",          "Huge - Wooden - Deep",        t (0.85f, 0.60f, 0.45f, 0.80f, 0.20f, 0.70f, 0.95f), "cinematic1"),
                d ("world.frame",   "Frame Drums",    "Hand Drums - Warm",           t (0.50f, 0.70f, 0.45f, 0.60f, 0.40f, 0.30f, 0.95f), "cinematic1"),
                d ("world.desert",  "Desert Groove",  "Hand Perc - Shakers",         t (0.60f, 0.65f, 0.55f, 0.50f, 0.40f, 0.40f, 0.92f), "cinematic1"),
                d ("world.pulse",   "Tribal Pulse",   "Driving - Toms - Organic",    t (0.75f, 0.60f, 0.50f, 0.60f, 0.30f, 0.60f, 0.90f), "cinematic1") }));
            return all;
        }

        const std::vector<std::vector<Style>>& all()
        {
            static const auto s = makeStyles();
            return s;
        }
    }

    const std::vector<Style>& stylesOf (Genre g)
    {
        return all()[(size_t) juce::jlimit (0, numGenres - 1, (int) g)];
    }

    const Style& styleFor (Genre g, int sub)
    {
        const auto& list = stylesOf (g);
        return list[(size_t) juce::jlimit (0, (int) list.size() - 1, sub)];
    }

    int styleIndex (Genre g, const juce::String& id)
    {
        const auto& list = stylesOf (g);
        for (size_t i = 0; i < list.size(); ++i)
            if (list[i].id == id) return (int) i;
        return 0;
    }

    bool findStyle (const juce::String& id, Genre& genre, int& subStyle)
    {
        for (int g = 0; g < numGenres; ++g)
        {
            const auto& list = stylesOf ((Genre) g);
            for (size_t i = 0; i < list.size(); ++i)
                if (list[i].id == id) { genre = (Genre) g; subStyle = (int) i; return true; }
        }
        return false;
    }

    int directionIndex (const Style& st, const juce::String& id)
    {
        for (size_t i = 0; i < st.directions.size(); ++i)
            if (st.directions[i].id == id) return (int) i;
        return -1;
    }

    Traits styleCenter (Genre g, int sub)
    {
        const auto& s = styleFor (g, sub);
        Traits c;
        for (int i = 0; i < Traits::num; ++i)
        {
            float sum = 0;
            for (auto& dir : s.directions) sum += dir.target[i];
            c[i] = sum / (float) s.directions.size();
        }
        return c;
    }

    Traits genreCenter (Genre g)   { return styleCenter (g, 0); }

    //==============================================================================
    juce::String eraName (int era)
    {
        static const char* names[numEras] = { "60s", "70s", "80s", "90s", "2000s", "2010s", "Today" };
        return juce::isPositiveAndBelow (era, numEras) ? juce::String (names[era]) : juce::String ("Any era");
    }

    std::array<float, Traits::num> eraDelta (int era)
    {
        //                                      punch   warmth  bright  room    vintage aggr    live
        switch (era)
        {
            case E60:   return { -0.10f,  0.15f, -0.15f,  0.10f,  0.40f, -0.15f,  0.35f };   // warm, roomy, all live
            case E70:   return { -0.05f,  0.15f, -0.10f, -0.10f,  0.32f, -0.10f,  0.30f };   // dry, deadened, warm
            case E80:   return {  0.05f, -0.05f,  0.12f,  0.25f,  0.15f,  0.05f, -0.20f };   // gated rooms, drum machines
            case E90:   return {  0.08f,  0.10f, -0.02f, -0.05f,  0.18f,  0.05f,  0.00f };   // samplers, punchy and warm
            case E2000: return {  0.10f,  0.00f,  0.08f, -0.05f, -0.12f,  0.10f, -0.05f };   // tighter, louder, cleaner
            case E2010: return {  0.10f, -0.05f,  0.12f, -0.05f, -0.22f,  0.08f, -0.10f };   // crisp and clean
            case ENOW:  return {  0.12f, -0.05f,  0.12f, -0.05f, -0.28f,  0.10f, -0.10f };   // modern, polished, hard
            default:    return {};
        }
    }

    juce::StringArray eraPathWords (int era)
    {
        switch (era)
        {
            case E60:   return { "60s", "1960s", "sixties", "motown", "ludwig", "vintage kit" };
            case E70:   return { "70s", "1970s", "seventies", "disco", "funk", "dry kit" };
            case E80:   return { "80s", "1980s", "eighties", "linn", "linndrum", "dmx", "simmons", "drumtraks", "707", "gated" };
            case E90:   return { "90s", "1990s", "nineties", "sp1200", "sp 1200", "sp12", "mpc60", "mpc3000", "s950", "golden era" };
            case E2000: return { "2000s", "00s", "y2k", "mpc2000" };
            case E2010: return { "2010s", "2010" };
            case ENOW:  return { "2020s", "2024", "2025", "2026", "modern" };
            default:    return {};
        }
    }

    int styleForEra (Genre g, int era)
    {
        if (era < 0) return 0;
        const auto& list = stylesOf (g);
        for (size_t i = 1; i < list.size(); ++i)
            for (int e : list[i].eras)
                if (e == era) return (int) i;
        return 0;
    }

    //==============================================================================
    Traits KitOptions::center() const
    {
        auto c = styleCenter (genre, subStyle);
        const auto d = eraDelta (era);
        for (int i = 0; i < Traits::num; ++i) c[i] = juce::jlimit (0.0f, 1.0f, c[i] + d[(size_t) i]);
        return c;
    }

    int KitOptions::directionFor (int kitIndex) const
    {
        // the lead direction (an artist's own sound) becomes kit 1, the rest follow in order
        const int n = (int) style().directions.size();
        if (leadDirection <= 0 || leadDirection >= n || kitIndex < 0 || kitIndex >= n) return kitIndex;
        if (kitIndex == 0) return leadDirection;
        return kitIndex <= leadDirection ? kitIndex - 1 : kitIndex;
    }

    bool KitOptions::wants808() const
    {
        return lowEnd < 0 ? style().use808 : lowEnd == 1;
    }

    int KitOptions::wantedPerc() const
    {
        // AUTO: the style's own count, or every perc pad a bigger kit has
        if (percCount < 0)
            return kitSize >= 32 ? 4 : kitSize >= 23 ? 3 : style().percCount;
        return juce::jlimit (0, 3, percCount);
    }

    Traits KitOptions::targetFor (const Direction& dir) const
    {
        // the sliders start at center() (style + era); how far they sit from the style's own center moves
        // every direction the same way, so the era and your tweaks apply to all four kits
        const auto base = styleCenter (genre, subStyle);
        Traits out;
        for (int i = 0; i < Traits::num; ++i)
            out[i] = juce::jlimit (0.0f, 1.0f, dir.target[i] + (character[i] - base[i]));

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
        o->setProperty ("style", style().id);
        o->setProperty ("era", era);
        o->setProperty ("lead", leadDirection);
        o->setProperty ("secondary", secondaryGenre);
        o->setProperty ("character", character.toVar());
        o->setProperty ("feel", feel);
        o->setProperty ("lowEnd", lowEnd);
        o->setProperty ("perc", percCount);
        o->setProperty ("fx", fx);
        o->setProperty ("kitSize", kitSize);
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
        k.subStyle = styleIndex (k.genre, v["style"].toString());
        k.era = juce::jlimit (-1, numEras - 1, (int) v.getProperty ("era", -1));
        k.leadDirection = juce::jlimit (-1, (int) k.style().directions.size() - 1, (int) v.getProperty ("lead", -1));
        k.secondaryGenre = juce::jlimit (-1, numGenres - 1, (int) v.getProperty ("secondary", -1));
        k.character = v.hasProperty ("character") ? Traits::fromVar (v["character"]) : k.center();
        k.feel = juce::jlimit (0, 3, (int) v["feel"]);
        k.lowEnd = juce::jlimit (-1, 1, (int) v.getProperty ("lowEnd", -1));
        k.percCount = juce::jlimit (-1, 3, (int) v.getProperty ("perc", -1));
        k.fx = (bool) v["fx"];
        k.kitSize = validKitSize ((int) v.getProperty ("kitSize", defaultKitSize));
        k.tuneToKey = (bool) v.getProperty ("tuneToKey", true);
        k.key = juce::jlimit (-1, 23, (int) v.getProperty ("key", -1));
        k.bpm = (double) v.getProperty ("bpm", 0.0);
        if (! std::isfinite (k.bpm) || k.bpm < 0 || k.bpm > 300) k.bpm = 0;
        if (auto* a = v["sources"].getArray())
            for (int i = 0; i < juce::jmin ((int) k.sources.size(), a->size()); ++i)
                k.sources[(size_t) i] = (bool) a->getReference (i);
        return k;
    }
}
