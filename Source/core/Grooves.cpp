#include "Grooves.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <map>
#include <optional>

namespace gg
{
    namespace
    {
        Slot laneSlot (char c)
        {
            switch (c)
            {
                case 'K': return Slot::kick;     case 'B': return Slot::bass808;  case 'S': return Slot::snare;
                case 'C': return Slot::clap;     case 'R': return Slot::rim;      case 'H': return Slot::closedHat;
                case 'O': return Slot::openHat;  case 'T': return Slot::tomLow;   case 'M': return Slot::tomMid;
                case 'N': return Slot::tomHigh;  case 'Y': return Slot::crash;    case 'D': return Slot::ride;
                case '1': return Slot::perc1;    case '2': return Slot::perc2;    case '3': return Slot::perc3;
                case 'F': return Slot::fx1;      case 'G': return Slot::fx2;
                default:  return Slot::count;
            }
        }

        // lanes: "K:x---x---...|...;S:----x---..." 16 steps per bar
        Groove make (const char* id, const char* name, double bpm, float swing, std::initializer_list<const char*> lanes)
        {
            Groove g;
            g.id = id;
            g.name = name;
            g.bpm = bpm;
            g.swing = swing;
            int steps = 0;
            for (auto* lane : lanes)
            {
                const juce::String l (lane);
                const auto slot = laneSlot ((char) l[0]);
                const auto pattern = l.substring (2).removeCharacters ("| ");
                steps = juce::jmax (steps, pattern.length());
                for (int i = 0; i < pattern.length(); ++i)
                {
                    const double beat = i * 0.25;
                    switch (pattern[i])
                    {
                        case 'x': g.hits.push_back ({ beat, slot, 1.0f }); break;
                        case 'X': g.hits.push_back ({ beat, slot, 1.0f }); break;
                        case 'o': g.hits.push_back ({ beat, slot, 0.72f }); break;
                        case '.': g.hits.push_back ({ beat, slot, 0.42f }); break;
                        case 'r': g.hits.push_back ({ beat, slot, 0.85f }); g.hits.push_back ({ beat + 0.125, slot, 0.6f }); break;
                        case 't': for (int k = 0; k < 3; ++k) g.hits.push_back ({ beat + k * 0.25 / 3.0, slot, k == 0 ? 0.85f : 0.62f }); break;
                        default: break;
                    }
                }
            }
            g.bars = juce::jmax (1, (steps + 15) / 16);
            std::stable_sort (g.hits.begin(), g.hits.end(), [] (auto& a, auto& b) { return a.beat < b.beat; });
            return g;
        }

        std::vector<Groove> makeAll()
        {
            std::vector<Groove> v;
            v.push_back (make ("boombap1", "Boom Bap", 90, 0.30f, {      // 60% (MPC-style swing)
                "K:x-------x-x-----|x------x--x---x-",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "O:----------------|--------------o-",
                "1:--o---o---o---o-|--o---o---o---o-",
                "Y:x---------------|----------------" }));
            v.push_back (make ("boombap2", "Dusty Swing", 88, 0.36f, {   // 62%
                "K:x-----x---x-----|x-----x-x-----x-",
                "S:----x-------x---|----x-------x--.",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "R:--------------.-|------.---------",
                "1:----------x-----|----------x-----" }));
            v.push_back (make ("trap1", "Trap", 142, 0.0f, {
                "K:x------x--x-----|x-----x---x-x---",
                "B:x------x--x-----|x-----x---x-x---",
                "C:--------x-------|--------x-------",
                "S:--------x-------|--------x-------",
                "H:x-x-xrx-x-x-xtx-|x-x-x-x-xrxrx-x-",
                "O:----------------|------o---------",
                "1:---x-------x----|---x--------x---",
                "Y:x---------------|----------------",
                "F:x---------------|----------------",
                "G:----------------|------------x---" }));
            v.push_back (make ("drill1", "Drill", 142, 0.0f, {
                "K:x--------x------|x-----x---------",
                "B:x--------x---x--|x-----x-----x---",
                "S:--------x-----x-|--------x-------",
                "C:--------x-----x-|--------x-------",
                "H:x--x--x-x--x--x-|x--x--x-xtx--x-x",
                "1:--x-------x-----|--x-------x-----",
                "Y:x---------------|----------------" }));
            v.push_back (make ("trap2", "Bounce", 150, 0.0f, {
                "K:x---------x-----|x-----x-------x-",
                "B:x---------x-----|x-----x-------x-",
                "C:--------x-------|--------x-------",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-xrx-x-xt",
                "O:------o---------|----------------",
                "1:----x-----------|----x-----------",
                "2:-------x-------x|-------x-------x",
                "Y:x---------------|----------------" }));
            v.push_back (make ("rnb1", "R&B", 76, 0.25f, {
                "K:x--x------x-----|x--x----x-x-----",
                "B:x---------x-----|x-------x-------",
                "S:----x-------x---|----x-------x---",
                "1:----x-------x---|----x-------x---",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.xox.",
                "R:------.-------.-|------.-------.-",
                "2:o.o.o.o.o.o.o.o.|o.o.o.o.o.o.o.o." }));
            v.push_back (make ("pop1", "Pop", 108, 0.0f, {
                "K:x-----x-x-------|x-----x-x---x---",
                "C:----x-------x---|----x-------x---",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:xoxoxoxoxoxoxoxo|xoxoxoxoxoxoxoxo",
                "Y:x---------------|----------------" }));
            v.push_back (make ("pop2", "Big Room", 104, 0.0f, {
                "K:x-------x-------|x-------x-x-----",
                "S:----x-------x---|----x-------x---",
                "C:----x-------x---|----x-----------",
                "H:x---x---x---x---|x---x---x---x---",
                "O:--o---o---o---o-|--o---o---------",
                "N:----------------|------------x---",
                "M:----------------|-------------x--",
                "T:----------------|--------------xx",
                "1:----x-------x---|----x-------x---",
                "Y:x---------------|----------------" }));
            v.push_back (make ("rock1", "Rock", 120, 0.0f, {
                "K:x-----x-x-------|x-----x-x-x-----",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:----x-------x---|----x-------x---",
                "Y:x---------------|----------------" }));
            v.push_back (make ("rock2", "Arena", 128, 0.0f, {
                "K:x-------x-x-----|x-------x-------",
                "S:----x-------x---|----x-----------",
                "D:x-x-x-x-x-x-x-x-|x-x-x-x-x-------",
                "N:----------------|----------xx----",
                "M:----------------|------------xx--",
                "T:----------------|--------------xx",
                "Y:x---------------|----------------" }));
            v.push_back (make ("country1", "Country", 112, 0.0f, {
                "K:x-------x-------|x-------x-------",
                "R:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:----x-------x---|----x-------x---",      // perc 1 (country kits: the tambourine) on 2 and 4
                "2:xoxoxoxoxoxoxoxo|xoxoxoxoxoxoxoxo",      // perc 2 (the shaker) 16ths
                "S:----------------|------------x-x-" }));
            v.push_back (make ("country2", "Shuffle", 104, 0.35f, {
                "K:x-------x-------|x-------x-------",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:x-o-x-o-x-o-x-o-|x-o-x-o-x-o-x-o-",
                "2:x-------x-------|x-------x-------" }));
            v.back().swingEighths = true;     // a shuffle: the &s move toward the triplet
            v.push_back (make ("worship1", "Worship", 72, 0.0f, {
                "K:x-------x-x-----|x-------x-------",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:----x-------x---|----x-------x---",
                "2:o-o-o-o-o-o-o-o-|o-o-o-o-o-o-o-o-",
                "Y:x---------------|----------------" }));
            v.push_back (make ("gospel1", "Gospel Chops", 100, 0.15f, {
                "K:x--x--x---x-----|x--x----x-x--x--",
                "B:x--x--x---x-----|x--x----x-x-----",
                "S:----x-------x---|----x------xx-x-",
                "C:----x-------x---|----x-------x---",
                "H:x-xxx-xxx-xxx-xx|x-xxx-xxx-xxx-xx",
                "Y:x---------------|----------------" }));
            v.push_back (make ("edm1", "Festival", 126, 0.0f, {
                "K:x---x---x---x---|x---x---x---x---",
                "C:----x-------x---|----x-------x---",
                "O:--x---x---x---x-|--x---x---x---x-",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:-o-o-o-o-o-o-o-o|-o-o-o-o-o-o-o-o",
                "Y:x---------------|----------------",
                "F:x---------------|----------------",
                "G:----------------|------------x---" }));
            v.push_back (make ("house1", "House", 124, 0.1f, {
                "K:x---x---x---x---|x---x---x---x---",
                "C:----x-------x---|----x-------x---",
                "O:--x---x---x---x-|--x---x---x---x-",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "1:----------x-----|----------x-----",
                "2:---x-------x----|---x-------x----" }));
            v.push_back (make ("lofi1", "Lo-Fi", 82, 0.36f, {               // 62%
                "K:x------x--x-----|x-------x-x-----",
                "S:----x-------x---|----x-------x--.",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "R:.---------------|--------.-------",
                "1:o.o.o.o.o.o.o.o.|o.o.o.o.o.o.o.o." }));
            v.push_back (make ("cinematic1", "Cinematic", 100, 0.0f, {
                "K:x-------x-------|x-----x-x-------",
                "T:x---x---x-x-x---|x---x---x-x-xxxx",
                "M:----------x-----|------------x-x-",
                "S:--------x-------|--------x-------",
                "1:x---x---x---x---|x---x---x---x---",
                "Y:x---------------|----------------",
                "F:x---------------|----------------",
                "G:----------------|------------x---" }));
            // ---- grooves for the sub-styles ----
            v.push_back (make ("gfunk1", "G-Funk", 94, 0.2f, {
                "K:x-----x---x-----|x-----x-x-------",
                "B:x-----x---x-----|x-----x-x-------",
                "C:----x-------x---|----x-------x---",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-xox-",
                "O:----------------|--------------o-",
                "1:o.o.o.o.o.o.o.o.|o.o.o.o.o.o.o.o.",
                "2:----x-------x---|----x-------x---" }));
            v.push_back (make ("funk1", "Funk", 100, 0.15f, {
                "K:x-----x---x-x---|x--x--x---x-----",
                "S:----x--.-.--x---|----x--.-.--x-.-",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "O:--------------o-|--------------o-",
                "1:--x--x----x--x--|--x--x----x--x--" }));
            v.push_back (make ("memphis1", "Memphis", 136, 0.0f, {
                "K:x-----x---x-----|x--x--x---x-----",
                "B:x-----x---x-----|x--x--x---x-----",
                "C:--------x-------|--------x-------",
                "S:--------x-------|--------x-------",
                "H:x-x-xtx-x-x-xtx-|xtx-x-x-xtx-xrxt",
                "1:x--x--x-x--x--x-|x--x--x-x--x--x-" }));
            v.push_back (make ("crunk1", "Crunk", 78, 0.1f, {
                "K:x--x--x-x--x----|x--x--x-x-x-x---",
                "B:x--x--x-x--x----|x--x--x-x-x-x---",
                "C:----x-------x---|----x-------x---",
                "S:----x-------x---|----x-------xxxx",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "1:--x---x---x---x-|--x---x---x---x-" }));
            v.push_back (make ("oldschool1", "Old School", 108, 0.1f, {
                "K:x-------x-x-----|x-------x-------",
                "B:x-------x-x-----|x-------x-------",
                "C:----x-------x---|----x-------x---",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:--x-------x-----|--x-------x---x-",
                "Y:x---------------|----------------" }));
            v.push_back (make ("ukdrill1", "UK Drill", 142, 0.0f, {
                "K:x---------------|x-----x---------",
                "B:x--x------x-----|x------x--x--x--",
                "S:--------x-----x-|-----x--x-------",
                "C:--------x-------|--------x-------",
                "H:x--x--x-x--x--x-|x--xx-x-x--x-xx-",
                "1:--x-------x-----|--x-------x-----" }));
            v.push_back (make ("nydrill1", "NY Drill", 142, 0.0f, {
                "K:x-----x---x-----|x--x------x-----",
                "B:x-----x---x-----|x--x------x-----",
                "S:--------x-------|--------x-----x-",
                "C:--------x-------|--------x-------",
                "H:x-xx-xx-x-xx-x-x|x-xx-xx-xtx-x-xx",
                "1:-----x--------x-|-----x--------x-" }));
            v.push_back (make ("rage1", "Rage", 155, 0.0f, {
                "K:x---x---x---x---|x---x---x-x-x---",
                "B:x---x---x---x---|x---x---x-x-x---",
                "S:--------x-------|--------x-------",
                "C:--------x-------|--------x-------",
                "H:x-x-x-x-x-x-x-x-|x-x-x-xrx-x-xrxr",
                "Y:x---------------|----------------" }));
            v.push_back (make ("newjack1", "New Jack Swing", 104, 0.45f, {   // 65%
                "K:x---x-x---x-----|x---x-x---x--x--",
                "S:----x--.----x---|----x--.----x-..",
                "C:----x-------x---|----x-------x---",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "1:--x---x---x---x-|--x---x---x---x-" }));
            v.push_back (make ("motown1", "Motown", 120, 0.1f, {
                "K:x-------x-------|x-------x-------",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:x---x---x---x---|x---x---x---x---" }));
            v.push_back (make ("disco1", "Disco", 120, 0.05f, {
                "K:x---x---x---x---|x---x---x---x---",
                "S:----x-------x---|----x-------x---",
                "C:----x-------x---|----x-------x---",
                "O:--x---x---x---x-|--x---x---x---x-",
                "H:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "1:xoxoxoxoxoxoxoxo|xoxoxoxoxoxoxoxo",
                "2:---x------x-----|---x------x--x--" }));
            v.push_back (make ("synthpop1", "Synth Pop", 118, 0.0f, {
                "K:x-------x-------|x-------x-x-----",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-----",
                "N:----------------|------------x---",
                "M:----------------|-------------x--",
                "T:----------------|--------------xx",
                "1:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "Y:x---------------|----------------" }));
            v.push_back (make ("dancepop1", "Dance Pop", 124, 0.0f, {
                "K:x---x---x---x---|x---x---x---x---",
                "C:----x-------x---|----x-------x---",
                "S:----x-------x---|----x-------x---",
                "O:--x---x---x---x-|--x---x---x---x-",
                "H:.x.x.x.x.x.x.x.x|.x.x.x.x.x.x.x.x",
                "1:x.x.x.x.x.x.x.x.|x.x.x.x.x.x.x.x.",
                "Y:x---------------|----------------" }));
            v.push_back (make ("metal1", "Metal", 150, 0.0f, {
                "K:xxxxxxxxxxxxxxxx|xxxxxxxxxxxxxxxx",
                "S:----x-------x---|----x-------x---",
                "D:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-----",
                "N:----------------|------------xx--",
                "T:----------------|--------------xx",
                "Y:x---------------|----------------" }));
            v.push_back (make ("grunge1", "Grunge", 110, 0.0f, {
                "K:x-----x-x-------|x-----x-x--x----",
                "S:----x-------x---|----x-------x---",
                "O:o-o-o-o-o-o-o-o-|o-o-o-o-o-o-o-o-",
                "Y:x---------------|x---------------" }));
            v.push_back (make ("punk1", "Punk", 172, 0.0f, {
                "K:x---x---x---x---|x---x---x-x-x---",
                "S:--x---x---x---x-|--x---x---x---x-",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "Y:x---------------|x---------------" }));
            v.push_back (make ("train1", "Train Beat", 116, 0.0f, {
                "K:x-------x-------|x-------x-------",
                "S:..o.x.o...o.x.o.|..o.x.o...o.x.o.",
                "1:xoxoxoxoxoxoxoxo|xoxoxoxoxoxoxoxo" }));
            v.push_back (make ("stomp1", "Stomp & Clap", 96, 0.0f, {
                "K:x---x---x---x---|x---x---x---x---",
                "C:----x-------x---|----x-------x---",
                "1:----x-------x---|----x-------x---",      // perc 1 (the tambourine) with the claps
                "2:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",      // perc 2 (the shaker) 8ths
                "Y:x---------------|----------------" }));
            v.push_back (make ("dnb1", "Drum & Bass", 174, 0.0f, {
                "K:x---------x-----|x---------x--x--",
                "S:----x-------x---|----x--x----x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "1:-x-x-x-x-x-x-x-x|-x-x-x-x-x-x-x-x",
                "Y:x---------------|----------------" }));
            v.push_back (make ("dubstep1", "Dubstep", 140, 0.0f, {
                "K:x---------------|x-------------x-",
                "S:--------x-------|--------x-------",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-xrxr",
                "Y:x---------------|----------------",
                "F:x---------------|----------------",
                "G:----------------|------------x---" }));
            // a blank pad: two empty bars to build a pattern from scratch (SAUCE's BLANK)
            v.push_back (make ("blank", "Blank", 100, 0.0f, {}));
            v.back().bars = 2;
            return v;
        }
    }

    const std::vector<Groove>& allGrooves()
    {
        static const auto g = makeAll();
        return g;
    }

    const Groove& grooveFor (const juce::String& id)
    {
        for (auto& g : allGrooves())
            if (g.id == id) return g;
        return allGrooves().front();
    }

    double swungBeat (double beat, float swing, bool eighths) noexcept
    {
        // off-beat 16ths (or 8ths, for a shuffle) move later; 0.5 is a triplet feel
        const double unitsPerBeat = eighths ? 2.0 : 4.0;
        const double pos = beat * unitsPerBeat;
        const double whole = std::floor (pos + 1.0e-6);
        if (((long long) whole) % 2 == 1 && std::abs (pos - whole) < 1.0e-6)
            return beat + swing * (eighths ? 1.0 / 3.0 : 1.0 / 6.0);
        return beat;
    }

    int swingPercent (const Groove& g) noexcept
    {
        // the late note's place between its two neighbours: (0.5 + swing / 3) of the way, for 16ths and 8ths alike
        return juce::roundToInt (50.0 + g.swing * 100.0 / 3.0);
    }

    juce::MemoryBlock grooveToMidi (const Groove& g, double bpm, float swing, int times)
    {
        constexpr int ppq = 480;
        juce::MidiMessageSequence seq;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (20.0, bpm))), 0);
        seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
        seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Groove Grabber - " + g.name), 0);
        seq.addEvent (juce::MidiMessage::textMetaEvent (1, midiDrumsTag), 0);
        for (int rep = 0; rep < times; ++rep)
            for (auto& h : g.hits)
            {
                const int note = mainNoteForSlot (h.slot);
                if (note < 0) continue;
                const double beat = rep * g.lengthBeats() + swungBeat (h.beat, swing, g.swingEighths);
                const double t = beat * ppq;
                const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (h.velocity * 118.0f + 6.0f));
                seq.addEvent (juce::MidiMessage::noteOn (1, note, vel), t);
                seq.addEvent (juce::MidiMessage::noteOff (1, note), t + ppq / 8.0);
            }
        seq.addEvent (juce::MidiMessage::endOfTrack(), times * g.lengthBeats() * ppq);
        seq.updateMatchedPairs();

        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (ppq);
        mf.addTrack (seq);
        juce::MemoryOutputStream out;
        mf.writeTo (out, 0);
        return out.getMemoryBlock();
    }

    //==============================================================================
    namespace
    {
        // General MIDI's drums (what a DAW, a drum map or Song Savant's other kits use)
        std::optional<Slot> gmSlot (int note)
        {
            switch (note)
            {
                case 35: case 36:          return Slot::kick;
                case 37:                   return Slot::rim;
                case 38: case 40:          return Slot::snare;
                case 39:                   return Slot::clap;
                case 42: case 44:          return Slot::closedHat;
                case 46:                   return Slot::openHat;
                case 41: case 43:          return Slot::tomLow;
                case 45: case 47:          return Slot::tomMid;
                case 48: case 50:          return Slot::tomHigh;
                case 49: case 52: case 55: case 57: return Slot::crash;
                case 51: case 53: case 59: return Slot::ride;
                case 54:                   return Slot::perc1;      // tambourine
                case 69: case 70: case 82: return Slot::perc2;      // cabasa, maracas, shaker
                default: break;
            }
            if (note == 56 || (note >= 60 && note <= 81)) return Slot::perc3;   // cowbell, congas, bongos, claves...
            return std::nullopt;
        }

        struct RawHit { double beat; Slot slot; float velocity; };

        // how far the hits sit from the grid with this swing (each capped, so a quick double doesn't decide)
        double gridError (const std::vector<RawHit>& hits, float swing, bool eighths)
        {
            double err = 0.0;
            for (const auto& h : hits)
            {
                const double beatStart = std::floor (h.beat);
                double best = 1.0;
                for (int k = 0; k <= 4; ++k)
                    best = std::min (best, std::abs (h.beat - swungBeat (beatStart + k * 0.25, swing, eighths)));
                best = std::min (best, 0.06);
                err += best * best;
            }
            return err;
        }

        // how many hits sit on a late (swung) place, at least `minShift` beats late, within `tol` of it
        int swungHits (const std::vector<RawHit>& hits, float swing, bool eighths, double minShift, double tol)
        {
            int n = 0;
            for (const auto& h : hits)
            {
                const double beatStart = std::floor (h.beat + 0.03);
                for (int k = 0; k < 4; ++k)
                {
                    const double straight = beatStart + k * 0.25, late = swungBeat (straight, swing, eighths);
                    if (late - straight > minShift && std::abs (h.beat - late) < tol) { ++n; break; }
                }
            }
            return n;
        }

        // how far the hits that sit near a grid place are from it (made by a computer: almost nothing)
        double gridNoise (const std::vector<RawHit>& hits, float swing, bool eighths)
        {
            double sum = 0.0;
            int n = 0;
            for (const auto& h : hits)
            {
                const double beatStart = std::floor (h.beat);
                double best = 1.0;
                for (int k = 0; k <= 4; ++k)
                    best = std::min (best, std::abs (h.beat - swungBeat (beatStart + k * 0.25, swing, eighths)));
                if (best < 0.03) { sum += best * best; ++n; }
            }
            return n > 0 ? std::sqrt (sum / n) : 0.0;
        }

        // the step a hit belongs to: the last grid place at or just before it (a hit a little early still counts)
        int stepOf (double beat, float swing, bool eighths)
        {
            constexpr double early = 0.06;
            int step = 0;
            for (int k = (int) std::floor (beat * 4.0) - 1; k <= (int) std::floor (beat * 4.0) + 1; ++k)
                if (k >= 0 && swungBeat (k * 0.25, swing, eighths) <= beat + early) step = std::max (step, k);
            return step;
        }
    }

    MidiGroove grooveFromMidi (const juce::MemoryBlock& data, const juce::String& fileName)
    {
        MidiGroove r;
        juce::MemoryInputStream in (data, false);
        juce::MidiFile mf;
        if (! mf.readFrom (in) || mf.getTimeFormat() <= 0)
        {
            r.why = "It isn't a MIDI pattern Groove Grabber can read.";
            return r;
        }
        const double tpq = mf.getTimeFormat();

        // what the file says about itself, and its notes
        juce::String tag, ourName;
        double bpm = 0.0, end = 0.0;
        struct Note { double beat; int note; int velocity; };
        std::vector<Note> notes;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            const auto* seq = mf.getTrack (t);
            for (int i = 0; i < seq->getNumEvents(); ++i)
            {
                const auto& m = seq->getEventPointer (i)->message;
                const double beat = m.getTimeStamp() / tpq;
                if (m.isTextMetaEvent())
                {
                    const auto text = m.getTextFromTextMetaEvent().trim();
                    if (text.startsWithIgnoreCase ("drums:")) tag = text.fromFirstOccurrenceOf (":", false, false).trim();
                    if (text.startsWith ("Groove Grabber - ")) ourName = text.fromFirstOccurrenceOf (" - ", false, false).trim();
                }
                else if (m.isTempoMetaEvent() && bpm <= 0.0 && m.getTempoSecondsPerQuarterNote() > 0.0)
                    bpm = 60.0 / m.getTempoSecondsPerQuarterNote();
                else if (m.isEndOfTrackMetaEvent())
                    end = std::max (end, beat);
                else if (m.isNoteOn())
                    notes.push_back ({ beat, m.getNoteNumber(), (int) m.getVelocity() });
            }
        }
        if (notes.empty())
        {
            r.why = "It has no notes.";
            return r;
        }

        // each note's drum
        const bool ourPads = tag.containsIgnoreCase ("Groove Grabber") || (tag.isEmpty() && ourName.isNotEmpty());
        std::vector<RawHit> hits;
        for (const auto& n : notes)
        {
            std::optional<Slot> slot;
            if (ourPads)
                if (const auto* pad = padForNote (n.note)) slot = pad->slot;
            if (! slot) slot = gmSlot (n.note);
            if (! slot) { ++r.notesLeftOut; continue; }
            // the velocity SAUCE's drag writes (118 * v + 6), back again
            const float v = juce::jlimit (0.05f, 1.0f, (float) (n.velocity - 6) / 118.0f);
            hits.push_back ({ n.beat, *slot, v });
        }
        if (hits.empty())
        {
            r.why = "None of its notes are drums Groove Grabber knows (it reads Groove Grabber's pads and General MIDI drums).";
            return r;
        }

        // from the first bar with a hit
        double first = hits.front().beat;
        for (const auto& h : hits) first = std::min (first, h.beat);
        const double shift = std::floor ((first + 0.06) / 4.0) * 4.0;
        for (auto& h : hits) h.beat = std::max (0.0, h.beat - shift);
        end -= shift;

        // the swing that fits best (to 1%), if a few hits show it: by a hair in a file made by a computer (Groove
        // Grabber's own, a DAW's swing setting), clearly in one played in (where a loose hit isn't swing)
        float swing = 0.0f;
        bool eighths = false;
        double bestErr = gridError (hits, 0.0f, false);
        for (int e = 0; e < 2; ++e)
            for (int k = 1; k <= 50; ++k)
            {
                const float sw = k * 0.01f;
                const double err = gridError (hits, sw, e == 1);
                if (err < bestErr - 1.0e-12) { bestErr = err; swing = sw; eighths = e == 1; }
            }
        if (swing > 0.0f)
        {
            const bool exact = gridNoise (hits, swing, eighths) < 0.003;
            const double minShift = exact ? 0.004 : 0.02, tol = exact ? 0.003 : 0.03;
            if (swungHits (hits, swing, eighths, minShift, tol) < (exact ? 2 : 3)) { swing = 0.0f; eighths = false; }
            else
            {
                // exactly: how late the late hits are, on average
                const double per = eighths ? 1.0 / 3.0 : 1.0 / 6.0;
                double sum = 0.0;
                int n = 0;
                for (const auto& h : hits)
                {
                    const double beatStart = std::floor (h.beat + 0.03);
                    for (int k = 0; k < 4; ++k)
                    {
                        const double straight = beatStart + k * 0.25, late = swungBeat (straight, swing, eighths);
                        if (late - straight > minShift && std::abs (h.beat - late) < tol) { sum += (h.beat - straight) / per; ++n; break; }
                    }
                }
                if (n > 0) swing = juce::jlimit (0.01f, 0.5f, (float) std::round (sum / n * 100.0) / 100.0f);
            }
        }

        // on the grid: each step's hits, evenly spread in the step (one, a quick double, or a triplet burst)
        double last = 0.0;
        std::map<std::pair<int, int>, std::vector<RawHit>> steps;   // (step, slot)
        for (const auto& h : hits)
            steps[{ stepOf (h.beat, swing, eighths), (int) h.slot }].push_back (h);
        std::vector<GrooveHit> out;
        for (auto& [key, inStep] : steps)
        {
            std::sort (inStep.begin(), inStep.end(), [] (const RawHit& a, const RawHit& b) { return a.beat < b.beat; });
            // two notes for one drum at once (a pedal hat and a closed hat, say) are one hit
            std::vector<RawHit> distinct;
            for (const auto& h : inStep)
                if (distinct.empty() || h.beat - distinct.back().beat > 0.03) distinct.push_back (h);
                else distinct.back().velocity = std::max (distinct.back().velocity, h.velocity);
            const int count = std::min (3, (int) distinct.size());
            for (int k = 0; k < count; ++k)
                out.push_back ({ key.first * 0.25 + k * 0.25 / count, (Slot) key.second, distinct[(size_t) k].velocity });
            last = std::max (last, key.first * 0.25);
        }
        sortHits (out);

        // how many bars: the last hit's, or the file's own length when that's a little longer (an empty last bar)
        int bars = (int) std::floor (last / 4.0) + 1;
        if (const int fileBars = (int) std::round (end / 4.0); fileBars > bars && fileBars <= 4) bars = fileBars;
        if (bars > 4)
        {
            r.barsLeftOut = bars - 4;
            bars = 4;
            out.erase (std::remove_if (out.begin(), out.end(), [] (const GrooveHit& h) { return h.beat >= 16.0 - 1.0e-6; }), out.end());
        }
        // the same two bars twice: those two bars (SAUCE's drag plays a groove twice)
        if (bars == 4)
        {
            std::vector<GrooveHit> a, b;
            for (const auto& h : out) (h.beat < 8.0 - 1.0e-6 ? a : b).push_back (h);
            for (auto& h : b) h.beat -= 8.0;
            if (sameHits (a, b)) { out = a; bars = 2; }
        }

        auto& g = r.groove;
        g.name = ourName;
        if (g.name.isEmpty())
        {
            // the file's name, without the tempo and "(my edit)" SAUCE's drag adds
            auto name = fileName.upToLastOccurrenceOf (".", false, false).trim();
            name = name.replace ("(my edit)", "").trim();
            const auto words = juce::StringArray::fromTokens (name, " ", "");
            if (words.size() > 2 && words[words.size() - 1].equalsIgnoreCase ("BPM") && words[words.size() - 2].containsOnly ("0123456789."))
                name = name.upToLastOccurrenceOf (words[words.size() - 2], false, false).trim();
            g.name = name.isNotEmpty() ? name : juce::String ("Dropped groove");
        }
        g.bpm = bpm > 0.0 ? juce::jlimit (40.0, 240.0, bpm) : 0.0;
        g.bars = bars;
        g.swing = swing;
        g.swingEighths = eighths;
        g.hits = std::move (out);
        return r;
    }

    //==============================================================================
    StepHits stepHits (const Groove& g, Slot slot, int step)
    {
        StepHits r;
        const double start = step * 0.25, end = start + 0.25;
        for (const auto& h : g.hits)
            if (h.slot == slot && h.beat >= start - 1.0e-6 && h.beat < end - 1.0e-6)
            {
                if (r.count == 0) r.velocity = h.velocity;
                ++r.count;
            }
        return r;
    }

    std::vector<GrooveHit> withStep (const Groove& g, Slot slot, int step, int count, float velocity)
    {
        const double start = step * 0.25, end = start + 0.25;
        std::vector<GrooveHit> out;
        for (const auto& h : g.hits)
            if (! (h.slot == slot && h.beat >= start - 1.0e-6 && h.beat < end - 1.0e-6)) out.push_back (h);
        count = juce::jlimit (0, 3, count);
        if (step >= 0 && step < g.bars * 16)
            for (int k = 0; k < count; ++k)
                out.push_back ({ start + k * 0.25 / count, slot, k == 0 ? velocity : velocity * 0.72f });
        sortHits (out);
        return out;
    }

    float nextLevel (float v) noexcept
    {
        if (v >= 0.86f) return mediumHit;
        if (v >= 0.57f) return softHit;
        return hardHit;
    }

    void sortHits (std::vector<GrooveHit>& hits)
    {
        std::stable_sort (hits.begin(), hits.end(), [] (const GrooveHit& a, const GrooveHit& b)
        {
            if (std::abs (a.beat - b.beat) > 1.0e-9) return a.beat < b.beat;
            return (int) a.slot < (int) b.slot;
        });
    }

    bool sameHits (std::vector<GrooveHit> a, std::vector<GrooveHit> b)
    {
        if (a.size() != b.size()) return false;
        sortHits (a);
        sortHits (b);
        for (size_t i = 0; i < a.size(); ++i)
            if (std::abs (a[i].beat - b[i].beat) > 1.0e-6 || a[i].slot != b[i].slot || std::abs (a[i].velocity - b[i].velocity) > 1.0e-3f)
                return false;
        return true;
    }

    juce::var hitsToVar (const std::vector<GrooveHit>& hits)
    {
        juce::Array<juce::var> a;
        for (const auto& h : hits)
            a.add (juce::Array<juce::var> { juce::var (h.beat), juce::var (slotId (h.slot)), juce::var ((double) h.velocity) });
        return a;
    }

    std::vector<GrooveHit> hitsFromVar (const juce::var& v, const Groove& original)
    {
        std::vector<GrooveHit> out;
        if (auto* a = v.getArray())
            for (const auto& item : *a)
            {
                if (out.size() >= 1024) break;
                auto* h = item.getArray();
                if (h == nullptr || h->size() < 3) continue;
                auto isNumber = [] (const juce::var& x) { return x.isDouble() || x.isInt() || x.isInt64(); };
                if (! isNumber ((*h)[0]) || ! isNumber ((*h)[2])) continue;
                const double beat = (*h)[0];
                const auto slot = slotFromId ((*h)[1].toString());
                const double vel = (*h)[2];
                if (! std::isfinite (beat) || beat < 0 || beat >= original.lengthBeats() || slot == Slot::count || ! std::isfinite (vel)) continue;
                out.push_back ({ beat, slot, (float) juce::jlimit (0.05, 1.0, vel) });
            }
        sortHits (out);
        return out;
    }
}
