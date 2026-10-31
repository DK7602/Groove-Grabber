// Groove Grabber test runner: engine tests on a synthesised drum library, then UI screenshots.
//   GGTests              run everything
//   GGTests --engine     engine tests only
//   GGTests --shots DIR  also write screenshots to DIR (default: build folder)
#include <set>
#include "TestSounds.h"
#include "core/Analyzer.h"
#include "core/Library.h"
#include "core/KitGenerator.h"
#include "core/SongContext.h"
#include "core/Exporter.h"
#include "core/KitPlayer.h"
#include "core/Harvester.h"
#include "core/SongAnalysis.h"
#include "core/MpcTemplate.h"
#include <juce_events/juce_events.h>
#include <cmath>
#include <iostream>
#include <limits>

int runScreenshots (const juce::File& outDir);   // TestScreens.cpp

namespace
{
    int failures = 0, checks = 0;

    void check (bool ok, const juce::String& what)
    {
        ++checks;
        if (! ok) { ++failures; std::cout << "  FAIL: " << what << std::endl; }
    }

    void section (const char* name)   { std::cout << "\n== " << name << std::endl; }

    juce::File testRoot()
    {
        static auto root = [] {
            auto r = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("GrooveGrabberTests");
            r.deleteRecursively();
            r.createDirectory();
            return r;
        }();
        return root;
    }

    //==============================================================================
    // A small library: named packs plus anonymous files the audio alone has to classify
    struct Expected { juce::File file; gg::Cat cat; };
    std::vector<Expected> makeLibrary (const juce::File& root)
    {
        ggtest::Synth s;
        std::vector<Expected> out;
        auto add = [&] (const juce::String& rel, juce::AudioBuffer<float> b, gg::Cat cat, bool vintage = false)
        {
            if (vintage) s.vintage (b);
            auto f = root.getChildFile (rel);
            ggtest::Synth::write (f, b, s.sr);
            out.push_back ({ f, cat });
        };

        // ---- "Trap Essentials" pack (clean, electronic) ----
        add ("Trap Essentials/Kicks/Kick Hard 01.wav", s.kick (52, 0.22f, 0.8f), gg::Cat::kick);
        add ("Trap Essentials/Kicks/Kick Punch 02.wav", s.kick (60, 0.18f, 1.0f), gg::Cat::kick);
        add ("Trap Essentials/808s/808 Long G.wav", s.bass808 (49, 1.4f), gg::Cat::bass808);
        add ("Trap Essentials/808s/808 Short C.wav", s.bass808 (65.4f, 0.9f), gg::Cat::bass808);
        add ("Trap Essentials/Snares/Snare Crack.wav", s.snare (230, 0.14f, 1.0f), gg::Cat::snare);
        add ("Trap Essentials/Claps/Clap Wide.wav", s.clap (0.14f), gg::Cat::clap);
        add ("Trap Essentials/Hats/HH Closed 1.wav", s.hat (0.03f), gg::Cat::closedHat);
        add ("Trap Essentials/Hats/OH Open 1.wav", s.hat (0.32f), gg::Cat::openHat);
        add ("Trap Essentials/Perc/Snap 1.wav", s.rim(), gg::Cat::perc);
        add ("Trap Essentials/FX/Riser Up.wav", s.riser(), gg::Cat::fx);

        // ---- "Dusty Boom Bap" pack (vintage: dull + hiss) ----
        add ("Dusty Boom Bap/BD Dusty 1.wav", s.kick (58, 0.3f, 0.3f), gg::Cat::kick, true);
        add ("Dusty Boom Bap/BD Dusty 2.wav", s.kick (50, 0.36f, 0.25f), gg::Cat::kick, true);
        add ("Dusty Boom Bap/SD Dusty 1.wav", s.snare (180, 0.22f, 0.7f), gg::Cat::snare, true);
        add ("Dusty Boom Bap/SD Dusty 2.wav", s.snare (200, 0.18f, 0.9f), gg::Cat::snare, true);
        add ("Dusty Boom Bap/Hat Dusty.wav", s.hat (0.04f), gg::Cat::closedHat, true);
        add ("Dusty Boom Bap/Open Hat Dusty.wav", s.hat (0.4f), gg::Cat::openHat, true);
        add ("Dusty Boom Bap/Rim Dusty.wav", s.rim(), gg::Cat::rim, true);
        add ("Dusty Boom Bap/Tamb Dusty.wav", s.shaker(), gg::Cat::perc, true);
        add ("Dusty Boom Bap/Crash Dusty.wav", s.crash (2.0f), gg::Cat::crash, true);

        // ---- "Live Rock Kit" pack (acoustic-ish) ----
        add ("Live Rock Kit/Kick Room.wav", s.kick (62, 0.38f, 0.7f), gg::Cat::kick);
        add ("Live Rock Kit/Snare Room.wav", s.snare (210, 0.3f, 0.85f), gg::Cat::snare);
        add ("Live Rock Kit/Tom Floor.wav", s.tom (90), gg::Cat::tom);
        add ("Live Rock Kit/Tom Mid.wav", s.tom (130), gg::Cat::tom);
        add ("Live Rock Kit/Tom High.wav", s.tom (185), gg::Cat::tom);
        add ("Live Rock Kit/Ride Bell.wav", s.ride(), gg::Cat::ride);
        add ("Live Rock Kit/Crash 18.wav", s.crash(), gg::Cat::crash);
        add ("Live Rock Kit/Hihat Closed.wav", s.hat (0.045f), gg::Cat::closedHat);
        add ("Live Rock Kit/Hihat Open.wav", s.hat (0.45f), gg::Cat::openHat);
        add ("Live Rock Kit/Cowbell.wav", s.cowbell(), gg::Cat::perc);
        add ("Live Rock Kit/Shaker.wav", s.shaker(), gg::Cat::perc);

        // ---- anonymous sounds: the audio has to decide ----
        add ("Mystery/Sound 01.wav", s.kick (55, 0.25f, 0.6f), gg::Cat::kick);
        add ("Mystery/Sound 02.wav", s.bass808 (44, 1.5f), gg::Cat::bass808);
        add ("Mystery/Sound 03.wav", s.snare (200, 0.18f, 0.9f), gg::Cat::snare);
        add ("Mystery/Sound 04.wav", s.clap (0.15f), gg::Cat::clap);
        add ("Mystery/Sound 05.wav", s.hat (0.03f), gg::Cat::closedHat);
        add ("Mystery/Sound 06.wav", s.hat (0.38f), gg::Cat::openHat);
        add ("Mystery/Sound 07.wav", s.tom (120), gg::Cat::tom);
        add ("Mystery/Sound 08.wav", s.crash (2.6f), gg::Cat::crash);
        add ("Mystery/Sound 09.wav", s.rim(), gg::Cat::rim);
        add ("Mystery/Sound 10.wav", s.riser (2.5f), gg::Cat::fx);

        // a loop whose hits run together (no gaps to count) is still a loop when its name says so
        add ("Loops/BerserkTrap_Full_Drum_Loop_140_06_Am.wav", s.riser (5.0f), gg::Cat::loop);

        // vocals and instruments never go in a kit
        add ("Vocal - Loops/ChantsShouts_Adlibs_Dry_C#m_140_Hey.wav", s.riser (1.4f), gg::Cat::melodic);
        add ("Vocal - One Shots/ChantsShouts_Shouts_Saturated_Yo_3.wav", s.snare (300, 0.06f, 0.5f), gg::Cat::melodic);
        add ("Synth - Loops/BerserkTrap_Synth_Loop_15_150_Gm_Synth.wav", s.riser (2.7f), gg::Cat::melodic);
        {
            juce::AudioBuffer<float> note (2, (int) (s.sr * 2.5));
            for (int i = 0; i < note.getNumSamples(); ++i)
            {
                const float t = (float) (i / s.sr);
                const float v = (std::sin (juce::MathConstants<float>::twoPi * 330.0f * t) + 0.3f * std::sin (juce::MathConstants<float>::twoPi * 660.0f * t))
                              * 0.5f * std::exp (-t / 1.5f) * juce::jmin (1.0f, t / 0.01f);
                note.setSample (0, i, v);
                note.setSample (1, i, v);
            }
            add ("Mystery/Sound 11.wav", note, gg::Cat::melodic);     // a held instrument note, no name
        }
        return out;
    }

    //==============================================================================
    void testNames()
    {
        section ("Names");
        using namespace gg::analysis;
        struct C { const char* name; const char* folders; gg::Cat cat; };
        const C cases[] = {
            { "BD_Dusty_03", "", gg::Cat::kick },          { "Kick Hard 01", "", gg::Cat::kick },
            { "KickPunch", "", gg::Cat::kick },             { "808 Long G", "", gg::Cat::bass808 },
            { "SD Crack", "", gg::Cat::snare },             { "Snare_Top_3", "", gg::Cat::snare },
            { "OH Open 2", "", gg::Cat::openHat },          { "HH_Closed", "", gg::Cat::closedHat },
            { "Hi-Hat Open", "", gg::Cat::openHat },        { "Clap 3", "", gg::Cat::clap },
            { "Rimshot", "", gg::Cat::rim },                { "Side Stick 1", "", gg::Cat::rim },
            { "Ride Bell", "", gg::Cat::ride },             { "Crash 18", "", gg::Cat::crash },
            { "Floor Tom", "", gg::Cat::tom },              { "Shaker 2", "", gg::Cat::perc },
            { "Tambourine Hit", "", gg::Cat::perc },        { "Riser Up", "", gg::Cat::fx },
            { "Sound 12", "Kicks", gg::Cat::kick },         { "001", "Drums/Snares", gg::Cat::snare },
            { "808 Snare", "", gg::Cat::snare },            { "808 Clap", "", gg::Cat::clap },
            { "ChantsShouts_Chants2_Ambient_Gm_140_Hey", "", gg::Cat::melodic },  { "Amen06_CreamFilling_Keys_94bpm_Db_25", "", gg::Cat::melodic },
            { "BerserkTrap_Bass_Loop_01_140_E", "", gg::Cat::melodic },          { "Rhodes Chord Cm", "", gg::Cat::melodic },
            { "Hey 01", "Vocal - One Shots", gg::Cat::melodic },                 { "Vocal Snare 2", "", gg::Cat::snare },
            { "Synth Kick 3", "", gg::Cat::kick },                               { "Kick 1", "Synth Drums", gg::Cat::kick },
            { "Guitar Hit", "", gg::Cat::melodic },                              { "808 Synth Bass C", "", gg::Cat::bass808 },
        };
        for (auto& c : cases)
        {
            const auto g = guessFromName (c.name, c.folders);
            check (g.cat == c.cat, juce::String ("name '") + c.name + "' -> " + gg::catName (g.cat) + ", wanted " + gg::catName (c.cat));
        }
        const auto v = guessFromName ("BD Dusty Vinyl", "SP1200 Classics");
        check (v.vintage > 0.3f, "vintage words raise vintage");
        const auto o = guessFromName ("Kick", "Acoustic Live Drums/Ludwig");
        check (o.organic > 0.3f, "acoustic words raise live feel");
        check (gg::genreTagsForPath ("trap essentials vol 2/kicks/kick 1") & (1u << (int) gg::Genre::trap), "trap tag from path");
        check ((gg::genreTagsForPath ("trapeze/kick") & (1u << (int) gg::Genre::trap)) == 0, "no trap tag inside another word");
    }

    void testDescriptions()
    {
        section ("Descriptions and keys");
        check (gg::parseKey ("A minor") == 21, "parseKey A minor");
        check (gg::parseKey ("Am") == 21, "parseKey Am");
        check (gg::parseKey ("F#") == 6, "parseKey F#");
        check (gg::parseKey ("Bbm") == 22, "parseKey Bbm");
        check (gg::parseKey ("Eb major") == 3, "parseKey Eb major");

        auto p = gg::parseDescription ("Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor");
        check (p.hasGenre && p.genre == gg::Genre::boomBap, "boom bap genre");
        check (std::abs (p.bpm - 88) < 0.1, "88 BPM, got " + juce::String (p.bpm));
        check (p.key == 21, "A minor key, got " + juce::String (p.key));
        check (p.nudge[2] < 0, "dark lowers brightness");
        check (p.nudge[0] > 0 && p.nudge[1] > 0 && p.nudge[4] > 0, "punchy, warm, vinyl");

        p = gg::parseDescription ("Modern country rock, 108 BPM, driving electric guitars, powerful live drums with a little hip-hop influence");
        check (p.genre == gg::Genre::country, "country first, got " + gg::genreName (p.genre));
        check (p.secondaryGenre == (int) gg::Genre::boomBap, "hip-hop influence as flavour, got " + juce::String (p.secondaryGenre) + " [" + p.understood.joinIntoString (", ") + "]");
        check (p.feel == 1, "live drums -> live feel");
        check (std::abs (p.bpm - 108) < 0.1, "108 BPM");
        check (p.key < 0, "no key in that text (the word 'a' is not A major)");

        p = gg::parseDescription ("hard trap beat with 808 slides and risers, 140bpm, key of F# minor");
        check (p.genre == gg::Genre::trap && p.lowEnd == 1 && p.fx == 1, "trap + 808 + fx");
        check (std::abs (p.bpm - 140) < 0.1, "140bpm without a space");
        check (p.key == 18, "F# minor, got " + juce::String (p.key));

        // styles and eras
        {
            struct S { const char* text; gg::Genre genre; const char* style; int era; };
            const S cases[] = {
                { "90s g-funk", gg::Genre::boomBap, "hiphop.west", 3 },
                { "90's west coast with a talkbox", gg::Genre::boomBap, "hiphop.west", 3 },
                { "80s pop rock", gg::Genre::pop, "pop.rock", 2 },
                { "80s pop", gg::Genre::pop, "pop.synth", 2 },
                { "uk drill 142 bpm", gg::Genre::trap, "trap.ukdrill", -1 },
                { "modern country rap with 808s", gg::Genre::country, "country.rap", -1 },
                { "70s classic rock", gg::Genre::rock, "rock.classic", 1 },
                { "dark 90s boom bap, punchy but warm", gg::Genre::boomBap, "hiphop.east", 3 },
                { "memphis phonk with cowbells", gg::Genre::boomBap, "hiphop.memphis", -1 },
                { "honky tonk shuffle", gg::Genre::country, "country.classic", -1 },
                { "90s r&b", gg::Genre::rnb, "rnb.newjack", 3 },
                { "deep house", gg::Genre::edm, "edm.house", -1 },
                { "trap", gg::Genre::trap, "trap", -1 },
            };
            for (auto& c : cases)
            {
                auto q = gg::parseDescription (c.text);
                const auto& st = gg::styleFor (q.genre, juce::jmax (0, q.subStyle));
                check (q.genre == c.genre && st.id == c.style && q.era == c.era,
                       juce::String ("\"") + c.text + "\" -> " + gg::genreName (q.genre) + " / " + st.id + " / era " + juce::String (q.era)
                       + " (wanted " + c.style + ", era " + juce::String (c.era) + ")");
            }
            auto g = gg::parseDescription ("90s g-funk");
            check (g.secondaryGenre < 0, "g-funk isn't also R&B (funk)");
            gg::KitOptions any, eighties;
            eighties.era = 2;
            check (eighties.center().room > any.center().room + 0.1f && eighties.center().organic < any.center().organic, "80s: roomier, more drum machine");
        }

        // artists and songs
        {
            const auto problems = gg::referenceTableProblems();
            check (problems.isEmpty(), "reference table: " + problems.joinIntoString (" | "));
            check (gg::allReferences().size() > 250, "lots of artists and songs (" + juce::String ((int) gg::allReferences().size()) + ")");

            struct R { const char* text; gg::Genre genre; const char* style; int era; const char* lead; };
            const R cases[] = {
                { "like Dr. Dre",                      gg::Genre::boomBap,   "hiphop.west",       3, "west.knock" },
                { "Still D.R.E. vibe, 93 bpm",         gg::Genre::boomBap,   "hiphop.west",       3, "west.knock" },
                { "something like Dre's old stuff",    gg::Genre::boomBap,   "hiphop.west",       3, "west.knock" },
                { "80s Dr. Dre",                       gg::Genre::boomBap,   "hiphop.west",       2, "west.knock" },
                { "Dr. Dre but boom bap",              gg::Genre::boomBap,   "hiphop.east",       3, "" },
                { "Metro Boomin type beat",            gg::Genre::trap,      "trap.atlanta",      5, "atl.house" },
                { "dark Future type beat",             gg::Genre::trap,      "trap.atlanta",      5, "atl.house" },
                { "Pop Smoke",                         gg::Genre::trap,      "trap.nydrill",      6, "ny.brooklyn" },
                { "Daft Punk",                         gg::Genre::edm,       "edm.house",         3, "house.disco" },
                { "Bon Jovi drums",                    gg::Genre::rock,      "rock.hard",         2, "hard.arena" },
                { "Livin' on a Prayer",                gg::Genre::rock,      "rock.hard",         2, "hard.arena" },
                { "hard rock",                         gg::Genre::rock,      "rock.hard",        -1, "" },
                { "Morgan Wallen",                     gg::Genre::country,   "country.modern",    6, "mc.radio" },
                { "In the Air Tonight",                gg::Genre::pop,       "pop.synth",         2, "synth.gated" },
                { "Hillsong",                          gg::Genre::worship,   "worship.modern",    5, "mw.anthem" },
                { "Kirk Franklin",                     gg::Genre::worship,   "worship.gospel",    3, "gos.chops" },
                { "Mötley Crüe",                       gg::Genre::rock,      "rock.hard",         2, "hard.glam" },
                { "AC/DC",                             gg::Genre::rock,      "rock.hard",         1, "hard.crunch" },
            };
            for (auto& c : cases)
            {
                auto q = gg::parseDescription (juce::String::fromUTF8 (c.text));
                const auto& st = gg::styleFor (q.genre, juce::jmax (0, q.subStyle));
                const juce::String lead = q.lead >= 0 ? st.directions[(size_t) q.lead].id : juce::String();
                check (q.genre == c.genre && st.id == c.style && q.era == c.era && lead == c.lead,
                       juce::String ("\"") + juce::String::fromUTF8 (c.text) + "\" -> " + gg::genreName (q.genre) + " / " + st.id + " / era "
                       + juce::String (q.era) + " / kit 1 " + lead + " [" + q.understood.joinIntoString (", ") + "]");
            }

            // common words are only names with a cue
            for (auto* text : { "future bass drop", "punchy kick for a prince", "queens hip hop", "a common snare", "dmx drum machine" })
                check (gg::parseDescription (text).refs.empty(), juce::String ("no artist in \"") + text + "\"");
            check (gg::parseDescription ("future bass drop").genre == gg::Genre::edm, "future bass is still EDM");
            check (! gg::parseDescription ("like Prince").refs.empty(), "\"like Prince\" is an artist");

            // names don't count as genres or character words
            auto ps = gg::parseDescription ("Pop Smoke type beat");
            check (ps.secondaryGenre < 0 && ps.said.hasGenre == false, "Pop Smoke isn't Pop");
            auto bd = gg::parseDescription ("Big Daddy Kane");
            check (std::abs (bd.said.nudge[(size_t) 3]) < 1.0e-6f, "Big Daddy Kane isn't 'big'");

            // the genre you picked decides which take: Post Malone in Country, Trap by default
            auto pm = gg::parseDescription ("Post Malone");
            check (pm.genre == gg::Genre::trap, "Post Malone alone -> Trap");
            gg::resolveDescription (pm, (int) gg::Genre::country, true, false);
            check (pm.genre == gg::Genre::country && gg::styleFor (pm.genre, pm.subStyle).id == "country.modern" && pm.referenceInGenre,
                   "Post Malone with Country picked -> Modern Nashville");

            // an artist from another genre: their era and feel on your genre
            auto dr = gg::parseDescription ("like Dr. Dre");
            gg::resolveDescription (dr, (int) gg::Genre::rock, true, false);
            check (dr.genre == gg::Genre::rock && ! dr.referenceInGenre && dr.era == 3 && dr.secondaryGenre == (int) gg::Genre::boomBap
                   && gg::styleFor (dr.genre, dr.subStyle).id == "rock.alt" && dr.lead < 0,
                   "Dr. Dre on Rock -> 90s Alt rock with a little hip-hop [" + dr.understood.joinIntoString (", ") + "]");
            check (dr.understood.contains ("Dr. Dre feel"), "shows 'Dr. Dre feel'");

            // a genre typed in the text wins over the artist's own
            auto rk = gg::parseDescription ("Dr. Dre style rock song");
            check (rk.genre == gg::Genre::rock && ! rk.referenceInGenre, "typed Rock wins over Dr. Dre's genre");

            // picking the genre keeps a style you typed for it
            auto gf = gg::parseDescription ("90s g-funk");
            gg::resolveDescription (gf, (int) gg::Genre::boomBap, true, true);
            check (gg::styleFor (gf.genre, gf.subStyle).id == "hiphop.west" && ! gf.styleFromEra, "90s g-funk + Hip-Hop picked stays West Coast");

            // kit 1 is the artist's direction, the other three follow
            gg::KitOptions o;
            gg::applyDescription (gg::parseDescription ("like Dr. Dre"), o);
            check (o.leadDirection == gg::directionIndex (o.style(), "west.knock") && o.directionFor (0) == o.leadDirection, "kit 1 = Westside Knock");
            std::set<int> dirs;
            for (int i = 0; i < 4; ++i) dirs.insert (o.directionFor (i));
            check (dirs.size() == 4, "all four directions still used");
            check (o.character.punch > o.center().punch, "Dr. Dre: punchier than the style's center");
            check (gg::KitOptions::fromVar (o.toVar()).leadDirection == o.leadDirection, "kit-1 direction saved with the options");
        }

        check (gg::keyFromChords (juce::StringArray::fromTokens ("Am F C G", " ", "")) == 21, "Am F C G -> A minor");
        check (gg::keyFromChords (juce::StringArray::fromTokens ("C G Am F", " ", "")) == 0, "C G Am F -> C major");
        check (gg::keyFromChords (juce::StringArray::fromTokens ("Em7 Cmaj7 G D", " ", "")) == 16, "Em7 Cmaj7 G D -> E minor");

        // Key Compass song file
        auto f = testRoot().getChildFile ("kc/abc.kcsong");
        f.getParentDirectory().createDirectory();
        f.replaceWithText (R"(<?xml version="1.0"?><KeyCompassSong version="1.5.0" id="abc" name="My Song" modified="1700000000000" rev="0" dirty="1" deleted="0" device="PC"><SongMap name="My Song" keyOverride="-1" lastSetKey="-1" bpm="94"><Section name="Verse" bars="8" barsPerChord="1" chords="Dm Bb F C"/><Section name="Chorus" bars="8" barsPerChord="1" chords="Bb F C Dm"/></SongMap></KeyCompassSong>)");
        auto songs = gg::listKeyCompassSongs (f.getParentDirectory());
        check (songs.size() == 1, "one Key Compass song");
        if (! songs.empty())
        {
            check (songs[0].name == "My Song" && std::abs (songs[0].bpm - 94) < 0.1, "song name and BPM");
            check (songs[0].key == 14, "key from chords Dm Bb F C -> D minor, got " + gg::keyName (songs[0].key));
            check (songs[0].sections.size() == 2, "two sections");
        }
    }

    //==============================================================================
    gg::SoundListPtr scanLibrary (const std::vector<Expected>& expected)
    {
        section ("Analysis and classification");
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        int right = 0, anonRight = 0, anonTotal = 0;
        for (auto& e : expected)
        {
            gg::SoundInfo s;
            const auto folders = e.file.getParentDirectory().getRelativePathFrom (testRoot().getChildFile ("Library"));
            const bool ok = gg::analysis::analyzeFile (e.file, folders, fm, s);
            check (ok, "analyse " + e.file.getFileName());
            const bool anon = e.file.getParentDirectory().getFileName() == "Mystery";
            if (s.cat == e.cat) { ++right; if (anon) ++anonRight; }
            else std::cout << "  (" << e.file.getParentDirectory().getFileName() << "/" << e.file.getFileName() << " -> "
                           << gg::catName (s.cat) << ", wanted " << gg::catName (e.cat)
                           << "; cen " << (int) s.centroidHz << " decay " << (int) s.decayMs << " flat " << s.flatness
                           << " low " << s.subShare + s.lowShare << " high " << s.highShare << " pitch " << (int) s.pitchHz
                           << " conf " << s.pitchConf << " bursts " << s.burstCount << ")" << std::endl;
            if (anon) ++anonTotal;
        }
        std::cout << "  " << right << " / " << expected.size() << " right, anonymous " << anonRight << " / " << anonTotal << std::endl;
        check (right >= (int) expected.size() - 2, "classification: at most 2 wrong overall");
        check (anonRight >= anonTotal - 2, "audio-only classification: at most 2 wrong");

        // pitch
        ggtest::Synth syn;
        auto b = syn.bass808 (49.0f, 1.2f);
        gg::SoundInfo s808;
        gg::analysis::measure (b.getReadPointer (0), b.getNumSamples(), syn.sr, s808);
        check (std::abs (s808.midiPitch() - 31.0f) < 0.6f, "808 pitch G1 (31), got " + juce::String (s808.midiPitch(), 2));

        // traits: the vintage pack reads as more vintage than the trap pack
        gg::SoundInfo dusty, clean;
        gg::analysis::analyzeFile (testRoot().getChildFile ("Library/Dusty Boom Bap/SD Dusty 1.wav"), "Dusty Boom Bap", fm, dusty);
        gg::analysis::analyzeFile (testRoot().getChildFile ("Library/Trap Essentials/Snares/Snare Crack.wav"), "Trap Essentials/Snares", fm, clean);
        check (dusty.traits.vintage > clean.traits.vintage + 0.15f, "dusty snare more vintage (" + juce::String (dusty.traits.vintage, 2) + " vs " + juce::String (clean.traits.vintage, 2) + ")");
        check (dusty.traits.bright < clean.traits.bright, "dusty snare darker");

        // ---- the Library class (background scan) ----
        section ("Library scan");
        auto lib = std::make_unique<gg::Library>();
        lib->addSource (testRoot().getChildFile ("Library"), gg::Source::packs);
        for (int i = 0; i < 400 && (lib->isScanning() || lib->getSounds()->empty()); ++i)
            juce::Thread::sleep (50);
        const auto sounds = lib->getSounds();
        check ((int) sounds->size() == (int) expected.size(), "scanned " + juce::String ((int) sounds->size()) + " of " + juce::String ((int) expected.size()));
        check (lib->getCacheFile().existsAsFile(), "cache written");

        // a second library (another plug-in window / the app) reads the cache without re-analysing
        auto lib2 = std::make_unique<gg::Library>();
        check (lib2->getSounds()->size() == sounds->size(), "cache read back");
        lib->setCategory (expected.front().file.getFullPathName(), gg::Cat::perc);
        check (lib->find (expected.front().file.getFullPathName())->cat == gg::Cat::perc, "user category sticks");
        lib->setCategory (expected.front().file.getFullPathName(), gg::Cat::kick);
        return lib->getSounds();
    }

    void testGenerator (gg::SoundListPtr sounds)
    {
        section ("Kit generator");
        gg::KitOptions o;
        o.genre = gg::Genre::boomBap;
        o.character = gg::genreCenter (o.genre);
        o.key = 21;            // A minor
        o.lowEnd = 1;          // with 808
        o.percCount = 2;
        o.fx = true;
        gg::KitGenerator gen (sounds, o);
        auto kits = gen.generateAll (7);
        check (kits.size() == 4, "four kits");
        for (auto& k : kits)
        {
            std::cout << "  " << k.name << ":";
            for (auto sl : { gg::Slot::kick, gg::Slot::snare, gg::Slot::closedHat, gg::Slot::bass808, gg::Slot::clap })
                std::cout << " " << gg::slotName (sl) << "=" << juce::File (k[sl].path).getFileNameWithoutExtension();
            std::cout << std::endl;
        }
        for (auto& k : kits)
        {
            check (! k[gg::Slot::kick].isEmpty() && ! k[gg::Slot::snare].isEmpty() && ! k[gg::Slot::closedHat].isEmpty(), k.name + ": kick, snare, hat");
            check (! k[gg::Slot::bass808].isEmpty(), k.name + ": 808");
            check (! k[gg::Slot::fx1].isEmpty(), k.name + ": fx");
            // every sound once
            juce::StringArray used;
            for (auto& s : k.slots) if (! s.isEmpty()) { check (! used.contains (s.path), k.name + ": no sound twice"); used.add (s.path); }
            // categories right
            for (int i = 0; i < gg::numSlots; ++i)
                if (! k.slots[(size_t) i].isEmpty())
                    check (gen.find (k.slots[(size_t) i].path)->cat == gg::slotCat ((gg::Slot) i), k.name + ": " + gg::slotName ((gg::Slot) i) + " has the right kind of sound");
        }
        // the four kits shouldn't all share the same kick and snare
        juce::StringArray kicks, snares;
        for (auto& k : kits) { kicks.addIfNotAlreadyThere (k[gg::Slot::kick].path); snares.addIfNotAlreadyThere (k[gg::Slot::snare].path); }
        check (kicks.size() >= 3, "kits use different kicks (" + juce::String (kicks.size()) + ")");
        check (snares.size() >= 3, "kits use different snares (" + juce::String (snares.size()) + ")");

        // Dusty Vinyl should lean on the dusty pack
        const auto& dusty = kits[1];      // "Dusty Hip-Hop"
        int dustyCount = 0;
        for (auto sl : { gg::Slot::kick, gg::Slot::snare, gg::Slot::closedHat })
            if (gen.find (dusty[sl].path)->pack == "Dusty Boom Bap") ++dustyCount;
        check (dustyCount >= 2, dusty.name + " picks the dusty pack (" + juce::String (dustyCount) + "/3)");

        // lock + regenerate
        auto locked = kits;
        locked[0][gg::Slot::kick].locked = true;
        const auto keptKick = locked[0][gg::Slot::kick].path;
        auto again = gen.generateAll (99, 0.2f, &locked);
        check (again[0][gg::Slot::kick].path == keptKick, "locked kick kept");

        // tune to key: the 808 lands on A
        const auto& k0 = kits[0];
        const auto* s808 = gen.find (k0[gg::Slot::bass808].path);
        if (s808 != nullptr)
        {
            const float tuned = s808->midiPitch() + k0[gg::Slot::bass808].autoTune;
            check (std::abs (tuned - std::round (tuned)) < 0.05f && ((int) std::round (tuned)) % 12 == 9,
                   "808 tuned to A (from " + juce::String (s808->midiPitch(), 2) + " by " + juce::String (k0[gg::Slot::bass808].autoTune, 2) + ")");
            // and it really sounds there: render it and measure
            auto audio = gg::renderSlotPreview (k0, gg::Slot::bass808, 44100.0);
            check (audio != nullptr && audio->getNumSamples() > 1000, "808 renders");
            if (audio != nullptr)
            {
                gg::SoundInfo m;
                gg::analysis::measure (audio->getReadPointer (0), audio->getNumSamples(), 44100.0, m);
                check (((int) std::round (m.midiPitch()) % 12) == 9, "rendered 808 measures as an A (" + juce::String (m.midiPitch(), 2) + ")");
            }
        }

        auto alts = gen.alternatives (k0, gg::Slot::snare, 5);
        check (alts.size() >= 3 && ! alts.contains (k0[gg::Slot::snare].path), "alternatives exclude the current snare");

        // a style + era: names and the style's directions
        {
            gg::KitOptions w;
            w.genre = gg::Genre::boomBap;
            w.subStyle = gg::styleIndex (w.genre, "hiphop.west");
            w.era = 3;
            w.character = w.center();
            gg::KitGenerator wg (sounds, w);
            auto wk = wg.generateAll (5);
            check (wk.size() == 4 && wk[0].name == "90s Westside Classic" && wk[0].styleId == "hiphop.west" && wk[0].era == 3,
                   "90s G-Funk kits are named and tagged (" + (wk.empty() ? juce::String() : wk[0].name) + ")");
            check (! wk[0][gg::Slot::bass808].isEmpty(), "G-Funk kits have an 808");
            for (auto& st : gg::stylesOf (gg::Genre::boomBap)) check (st.directions.size() == 4, st.name + " has 4 directions");

            gg::KitOptions d;
            gg::applyDescription (gg::parseDescription ("like Dr. Dre"), d);
            gg::KitGenerator dg (sounds, d);
            auto dk = dg.generateAll (5);
            check (dk.size() == 4 && dk[0].name == "90s Westside Knock" && dk[1].name == "90s Westside Classic",
                   "Dr. Dre: kit 1 is 90s Westside Knock (" + (dk.empty() ? juce::String() : dk[0].name + ", " + dk[1].name) + ")");
            std::set<juce::String> names;
            for (auto& k : dk) names.insert (k.name);
            check (names.size() == 4, "four different kit names");
        }
        for (int gi = 0; gi < gg::numGenres; ++gi)
            for (auto& st : gg::stylesOf ((gg::Genre) gi))
            {
                check (st.directions.size() == 4, st.id + ": 4 directions");
                for (auto& dir : st.directions) check (gg::grooveFor (dir.groove).id == dir.groove, st.id + ": groove " + dir.groove + " exists");
            }

        // rock: toms low -> high, no 808 by default
        gg::KitOptions r;
        r.genre = gg::Genre::rock;
        r.character = gg::genreCenter (r.genre);
        gg::KitGenerator rg (sounds, r);
        auto rk = rg.generate (0, 3, 0.0f, nullptr, {});
        check (rk[gg::Slot::bass808].isEmpty(), "rock: no 808 by default");
        const auto* lo = rg.find (rk[gg::Slot::tomLow].path);
        const auto* hi = rg.find (rk[gg::Slot::tomHigh].path);
        check (lo != nullptr && hi != nullptr && lo->pitchHz < hi->pitchHz, "rock: toms low to high");

        // kit file round trip
        auto v = gg::Kit::fromVar (juce::JSON::parse (juce::JSON::toString (kits[2].toVar())));
        check (v.name == kits[2].name && v[gg::Slot::snare].path == kits[2][gg::Slot::snare].path
                && v[gg::Slot::bass808].autoTune == kits[2][gg::Slot::bass808].autoTune, "kit saves and loads");

        // ---- export ----
        section ("Export");
        gg::ExportOptions eo;
        eo.destination = testRoot().getChildFile ("Exports");
        eo.grooveId = "boombap1";
        auto res = gg::exportKit (kits[0], eo);
        check (res.ok, "export ok " + res.error);
        check (res.padFiles.size() == gg::numPads, "22 pad files");
        for (int i = 0; i < res.padFiles.size(); ++i)
            check (res.padFiles[i].getFileName().startsWith (juce::String (i + 1).paddedLeft ('0', 2) + " "), "file " + juce::String (i + 1) + " numbered");
        check (res.padFiles[0].getFileName() == "01 Kick.wav" && res.padFiles[2].getFileName() == "03 Snare.wav", "kick first, snare third");
        check (res.programFile.getSize() == gg::mpc::templateSize, "MPC program is 10756 bytes");
        {
            juce::MemoryBlock mb;
            res.programFile.loadFileAsData (mb);
            auto* d = static_cast<const juce::uint8*> (mb.getData());
            check (juce::String::fromUTF8 ((const char*) d + 4, 16) == "MPC1000 PGM 1.00", "MPC header");
            check (juce::String::fromUTF8 ((const char*) d + 24, 7) == "01 Kick", "pad 1 = 01 Kick");
            check (d[24 + 64 * 164] == 36 && d[24 + 64 * 164 + 2] == 38, "pad notes 36, 37, 38...");
            check (d[24 + 64 * 164 + 64 + 38] == 2, "note 38 -> pad 3");
        }
        juce::FileInputStream mis (res.midiFile);
        juce::MidiFile mf;
        check (mf.readFrom (mis) && mf.getNumTracks() == 1 && mf.getTrack (0)->getNumEvents() > 20, "groove MIDI readable");
        check (res.infoFile.loadFileAsString().contains ("LOAD INTO GROOVE AGENT"), "kit info written");
        {
            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (res.padFiles[0]));
            check (rd != nullptr && rd->bitsPerSample == 24 && (int) rd->sampleRate == 44100 && rd->lengthInSamples > 1000, "kick WAV 24-bit 44.1k");
        }
    }

    void testPlayer (gg::SoundListPtr sounds)
    {
        section ("Player");
        gg::KitOptions o;
        o.genre = gg::Genre::trap;
        o.character = gg::genreCenter (o.genre);
        gg::KitGenerator gen (sounds, o);
        auto kit = gen.generate (0, 1, 0.0f, nullptr, {});
        auto pk = gg::buildPlayable (kit, 48000.0);
        check (pk->pads[0] != nullptr && pk->pads[0]->getNumSamples() > 0, "kick pad rendered");

        gg::KitPlayer player;
        player.prepare (48000.0, 512);
        player.setKit (pk);
        juce::AudioBuffer<float> out (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (10, 36, (juce::uint8) 110), 10);
        float peak = 0;
        for (int b = 0; b < 8; ++b)
        {
            player.process (out, midi, {});
            midi.clear();
            peak = juce::jmax (peak, out.getMagnitude (0, 0, 512));
        }
        check (peak > 0.05f, "MIDI note 36 plays the kick (" + juce::String (peak, 3) + ")");

        player.setGroove (&gg::grooveFor ("trap1"));
        player.setTempo (140);
        player.setGroovePlaying (true);
        double energy = 0;
        for (int b = 0; b < 200; ++b)
        {
            player.process (out, midi, {});
            energy += out.getRMSLevel (0, 0, 512);
        }
        check (energy > 1.0, "groove plays");

        // locked to the host transport
        gg::KitPlayer::HostInfo host;
        host.valid = true; host.playing = true; host.bpm = 120; host.ppq = 3.999;
        player.allNotesOff();
        player.process (out, midi, host);
        check (player.isHostDriving(), "host drives the groove while playing");
        check (out.getMagnitude (0, 0, 512) > 0.01f, "the downbeat (bar 2) plays on time");

        // level matching: two different kicks render at about the same loudness
        auto a = gg::render::sound (testRoot().getChildFile ("Library/Trap Essentials/Kicks/Kick Hard 01.wav").getFullPathName(), gg::Cat::kick, {}, 44100);
        auto b = gg::render::sound (testRoot().getChildFile ("Library/Dusty Boom Bap/BD Dusty 2.wav").getFullPathName(), gg::Cat::kick, {}, 44100);
        const float* ca[2] = { a.getReadPointer (0), a.getReadPointer (1) };
        const float* cb[2] = { b.getReadPointer (0), b.getReadPointer (1) };
        const float la = gg::analysis::shortTermLoudnessDb (ca, 2, a.getNumSamples(), 44100);
        const float lb = gg::analysis::shortTermLoudnessDb (cb, 2, b.getNumSamples(), 44100);
        check (std::abs (la - lb) < 1.5f, "kicks level-matched (" + juce::String (la, 1) + " vs " + juce::String (lb, 1) + " dB)");

        // damaged values never crash the renderer or poison the mix
        {
            gg::RenderParams wild;
            wild.tune = 400; wild.gainDb = 1.0e6f; wild.startMs = -50;
            auto w1 = gg::render::sound (testRoot().getChildFile ("Library/Live Rock Kit/Crash 18.wav").getFullPathName(), gg::Cat::crash, wild, 44100);
            wild.tune = -400; wild.gainDb = std::numeric_limits<float>::infinity();
            auto w2 = gg::render::sound (testRoot().getChildFile ("Library/Live Rock Kit/Crash 18.wav").getFullPathName(), gg::Cat::crash, wild, 44100);
            bool finite = true;
            for (auto* b : { &w1, &w2 })
                for (int c = 0; c < b->getNumChannels(); ++c)
                    for (int i = 0; i < b->getNumSamples(); ++i)
                        if (! std::isfinite (b->getSample (c, i))) finite = false;
            check (finite, "extreme tune / gain render safely");
            auto st = gg::SlotState::fromVar (juce::JSON::parse (R"({"path":"x.wav","tune":400,"autoTune":-1e300,"gain":1e999,"start":-5,"length":0})"));
            check (st.userTune == 12.0f && st.autoTune == -12.0f && st.gainDb == 0.0f && st.startMs == 0.0f && st.lengthPct == 5.0f, "kit file values clamped");
        }

        // render: length and reverse
        gg::RenderParams half;
        half.lengthPct = 40;
        auto c = gg::render::sound (testRoot().getChildFile ("Library/Live Rock Kit/Crash 18.wav").getFullPathName(), gg::Cat::crash, half, 44100);
        auto full = gg::render::sound (testRoot().getChildFile ("Library/Live Rock Kit/Crash 18.wav").getFullPathName(), gg::Cat::crash, {}, 44100);
        check (c.getNumSamples() < full.getNumSamples() * 0.5, "LENGTH shortens");
        gg::RenderParams up;
        up.tune = 12;
        auto octave = gg::render::sound (testRoot().getChildFile ("Library/Live Rock Kit/Crash 18.wav").getFullPathName(), gg::Cat::crash, up, 44100);
        check (std::abs (octave.getNumSamples() - full.getNumSamples() / 2) < full.getNumSamples() * 0.06, "an octave up is half as long");
    }

    void testHarvest()
    {
        section ("Harvest hits from a drum bounce");
        ggtest::Synth s;
        const double sr = 44100;
        const double bpm = 100, spb = sr * 60.0 / bpm;
        juce::AudioBuffer<float> bounce (2, (int) (spb * 16 + sr));
        bounce.clear();
        auto kick = s.kick (55, 0.25f, 0.6f), snare = s.snare (200, 0.16f, 0.9f), hat = s.hat (0.03f);
        auto place = [&] (const juce::AudioBuffer<float>& b, double beat, float gain)
        {
            const int at = (int) (beat * spb);
            const int n = juce::jmin (b.getNumSamples(), bounce.getNumSamples() - at);
            for (int c = 0; c < 2; ++c) bounce.addFrom (c, at, b, c, 0, n, gain);
        };
        // a "sampling pass": kick, snare, hat on their own, each at three loudnesses, with gaps
        const float gains[3] = { 1.0f, 0.5f, 0.22f };
        double beat = 0;
        for (auto* b : { &kick, &snare, &hat })
            for (float g : gains) { place (*b, beat, g); beat += 1.5; }
        auto file = testRoot().getChildFile ("Bounce/My Beat Drums.wav");
        ggtest::Synth::write (file, bounce, sr);

        auto r = gg::harvestHits (file, testRoot().getChildFile ("Harvested"));
        for (auto& f : r.files)
        {
            gg::SoundInfo hi;
            juce::AudioFormatManager fm; fm.registerBasicFormats();
            gg::analysis::analyzeFile (f, {}, fm, hi);
            std::cout << "  " << f.getFileName() << " cen " << (int) hi.centroidHz << " decay " << (int) hi.decayMs << " pitch " << hi.pitchHz << " loud " << hi.loudDb << std::endl;
        }
        check (r.ok, "harvest ok " + r.error);
        check (r.hitsFound == 9, "found 9 hits, got " + juce::String (r.hitsFound));
        juce::StringArray names;
        for (auto& f : r.files) names.add (f.getFileNameWithoutExtension());
        std::cout << "  harvested: " << names.joinIntoString (", ") << std::endl;
        check (names.contains ("Kick 1 (hard)") && names.contains ("Kick 1 (soft)"), "kick in soft and hard");
        check (names.contains ("Snare 1 (medium)"), "snare medium");
        check (names.contains ("Closed Hat 1 (hard)"), "closed hat");
    }

    void testSongAnalysis()
    {
        section ("Song tempo and key");
        ggtest::Synth s;
        const double sr = 22050;
        s.sr = sr;
        const double bpm = 120, spb = sr * 60.0 / bpm;
        juce::AudioBuffer<float> song (1, (int) (sr * 20));
        song.clear();
        auto kick = s.kick (50, 0.2f, 0.5f), hat = s.hat (0.03f);
        for (int b = 0; b < 38; ++b)
        {
            const int at = (int) (b * spb);
            song.addFrom (0, at, kick, 0, 0, juce::jmin (kick.getNumSamples(), song.getNumSamples() - at), 0.8f);
            const int at2 = (int) ((b + 0.5) * spb);
            if (at2 < song.getNumSamples())
                song.addFrom (0, at2, hat, 0, 0, juce::jmin (hat.getNumSamples(), song.getNumSamples() - at2), 0.3f);
        }
        // an A minor pad: A C E + a bass A
        for (int i = 0; i < song.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            float v = 0;
            for (float f : { 220.0f, 261.63f, 329.63f, 110.0f, 440.0f })
                v += std::sin (juce::MathConstants<float>::twoPi * f * t) * 0.06f;
            song.addSample (0, i, v);
        }
        const double tempo = gg::detectTempo (song.getReadPointer (0), song.getNumSamples(), sr);
        check (std::abs (tempo - 120) < 1.5, "tempo 120, got " + juce::String (tempo));
        float conf = 0;
        const int key = gg::detectKey (song.getReadPointer (0), song.getNumSamples(), sr, conf);
        check (key == 21, "key A minor, got " + gg::keyName (key));
    }
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (argv[i]);

    // keep the tests out of the real settings / library
    const auto home = testRoot().getChildFile ("Home");
    home.createDirectory();
   #if JUCE_WINDOWS
    _putenv_s ("GG_TEST_HOME", home.getFullPathName().toRawUTF8());
   #else
    setenv ("GG_TEST_HOME", home.getFullPathName().toRawUTF8(), 1);
   #endif

    std::cout << "Groove Grabber tests" << std::endl;
    const auto expected = makeLibrary (testRoot().getChildFile ("Library"));
    testNames();
    testDescriptions();
    auto sounds = scanLibrary (expected);
    testGenerator (sounds);
    testPlayer (sounds);
    testHarvest();
    testSongAnalysis();

    if (! args.contains ("--engine"))
    {
        auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile ("screenshots");
        const int i = args.indexOf ("--shots");
        if (i >= 0 && i + 1 < args.size()) outDir = juce::File (args[i + 1]);
        failures += runScreenshots (outDir);
    }

    std::cout << "\n" << (checks - failures) << " / " << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
