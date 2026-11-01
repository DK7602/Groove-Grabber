#include "Grooves.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

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
            v.push_back (make ("boombap1", "Boom Bap", 90, 0.5f, {
                "K:x-------x-x-----|x------x--x---x-",
                "S:----x-------x---|----x-------x---",
                "H:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "O:----------------|--------------o-",
                "1:--o---o---o---o-|--o---o---o---o-",
                "Y:x---------------|----------------" }));
            v.push_back (make ("boombap2", "Dusty Swing", 88, 0.62f, {
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
                "1:xoxoxoxoxoxoxoxo|xoxoxoxoxoxoxoxo",
                "2:----x-------x---|----x-------x---",
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
            v.push_back (make ("lofi1", "Lo-Fi", 82, 0.6f, {
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
            v.push_back (make ("newjack1", "New Jack Swing", 104, 0.55f, {
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
                "1:x-x-x-x-x-x-x-x-|x-x-x-x-x-x-x-x-",
                "2:----x-------x---|----x-------x---",
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

    juce::MemoryBlock grooveToMidi (const Groove& g, double bpm, float swing, int times)
    {
        constexpr int ppq = 480;
        juce::MidiMessageSequence seq;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (20.0, bpm))), 0);
        seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
        seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Groove Grabber - " + g.name), 0);
        for (int rep = 0; rep < times; ++rep)
            for (auto& h : g.hits)
            {
                const int note = mainNoteForSlot (h.slot);
                if (note < 0) continue;
                const double beat = rep * g.lengthBeats() + swungBeat (h.beat, swing, g.swingEighths);
                const double t = beat * ppq;
                const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (h.velocity * 118.0f + 6.0f));
                seq.addEvent (juce::MidiMessage::noteOn (10, note, vel), t);
                seq.addEvent (juce::MidiMessage::noteOff (10, note), t + ppq / 8.0);
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
}
