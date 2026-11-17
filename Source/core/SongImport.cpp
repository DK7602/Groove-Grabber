// Groove Grabber - IMPORT SONG (see SongImport.h)
#include "SongImport.h"
#include "SongAnalysis.h"
#include "Grooves.h"

namespace gg
{
    namespace
    {
        const juce::StringArray audioTypes { "wav", "aif", "aiff", "flac", "mp3", "ogg" };
        const juce::StringArray midiTypes { "mid", "midi" };

        juce::String extOf (const juce::File& f)   { return f.getFileExtension().trimCharactersAtStart (".").toLowerCase(); }

        // a track the drums are on, by its name (what DAWs and drum plug-ins call them)
        bool drumTrackName (const juce::String& name)
        {
            const auto n = name.toLowerCase();
            for (auto* w : { "drum", "beat", "perc", "kit", "groove agent", "groove grabber", "battery", "ezdrummer",
                             "superior", "fpc", "kong", "impact", "808", "hats", "kick", "snare" })
                if (n.contains (w)) return true;
            return false;
        }
    }

    double otherTempo (double bpm)
    {
        if (bpm <= 0) return 0;
        if (bpm < 95 && bpm * 2 <= 200) return bpm * 2;
        if (bpm > 135 && bpm / 2 >= 50) return bpm / 2;
        return 0;
    }

    bool canImport (const juce::File& f)
    {
        const auto e = extOf (f);
        return f.existsAsFile() && (audioTypes.contains (e) || midiTypes.contains (e) || e == "sssong" || e == "kcsong");
    }

    juce::String importPatterns()
    {
        juce::StringArray p;
        for (const auto& e : audioTypes) p.add ("*." + e);
        for (const auto& e : midiTypes) p.add ("*." + e);
        p.add ("*.sssong");
        p.add ("*.kcsong");
        return p.joinIntoString (";");
    }

    juce::String ImportedSong::summary() const
    {
        if (songSavant) return songSavant->summary();
        juce::StringArray bits;
        if (key >= 0) bits.add (keyName (key) + (keySure ? juce::String() : juce::String (" (a guess)")));
        if (bpm > 0)
        {
            auto t = juce::String (bpm, std::abs (bpm - std::round (bpm)) < 0.05 ? 0 : 1) + " BPM";
            if (otherBpm > 0) t << " (or " << juce::String (otherBpm, 0) << ")";
            bits.add (t);
        }
        if (bits.isEmpty()) return hasDrums() ? "a drum part, no key" : "no key or tempo in it";
        return bits.joinIntoString (", ");
    }

    juce::String ImportedSong::title() const
    {
        if (from == From::songSavant) return "Song Savant sent \"" + name + "\"";
        if (from == From::keyCompass) return "Key Compass: \"" + name + "\"";
        return "\"" + (file != juce::File() ? file.getFileName() : name) + "\"";
    }

    //==============================================================================
    ImportedSong songFromSongSavant (const SongSavantSong& s)
    {
        ImportedSong r;
        r.from = ImportedSong::From::songSavant;
        r.ok = true;
        r.name = s.name;
        r.file = s.file;
        r.bpm = s.bpm;
        r.key = s.key;
        r.keyFrom = "Song Savant";
        r.songSavant = s;
        return r;
    }

    ImportedSong songFromKeyCompass (const KeyCompassSong& s)
    {
        ImportedSong r;
        r.from = ImportedSong::From::keyCompass;
        r.ok = true;
        r.name = s.name;
        r.file = s.file;
        r.bpm = s.bpm;
        r.key = s.key;
        r.keyFrom = "Key Compass";
        return r;
    }

    //==============================================================================
    ImportedSong songFromMidi (const juce::MemoryBlock& data, const juce::String& name)
    {
        ImportedSong r;
        r.from = ImportedSong::From::midi;
        r.name = name;
        juce::MemoryInputStream in (data, false);
        juce::MidiFile mf;
        if (! mf.readFrom (in) || mf.getTimeFormat() <= 0)
        {
            r.error = "It isn't a MIDI file Groove Grabber can read.";
            return r;
        }
        const double tpq = mf.getTimeFormat();

        // what the file says about itself
        double tempoSecsPerQuarter = 0;
        int sharps = 0, num = 4, den = 4;
        bool hasKeySig = false, keyMajor = true, hasTimeSig = false;
        juce::String drumsTag;
        struct Note { double beat, length; int note, channel, track, velocity; };
        std::vector<Note> notes;
        std::vector<bool> drumTrack ((size_t) mf.getNumTracks(), false);
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            auto* seq = const_cast<juce::MidiMessageSequence*> (mf.getTrack (t));
            seq->updateMatchedPairs();
            int onTen = 0, onOther = 0;
            for (int i = 0; i < seq->getNumEvents(); ++i)
            {
                const auto* e = seq->getEventPointer (i);
                const auto& m = e->message;
                if (m.isTempoMetaEvent() && tempoSecsPerQuarter <= 0 && m.getTempoSecondsPerQuarterNote() > 0)
                    tempoSecsPerQuarter = m.getTempoSecondsPerQuarterNote();
                else if (m.isKeySignatureMetaEvent() && ! hasKeySig)
                {
                    hasKeySig = true;
                    sharps = m.getKeySignatureNumberOfSharpsOrFlats();
                    keyMajor = m.isKeySignatureMajorKey();
                }
                else if (m.isTimeSignatureMetaEvent() && ! hasTimeSig)
                {
                    hasTimeSig = true;
                    m.getTimeSignatureInfo (num, den);
                }
                else if (m.isTrackNameEvent())
                {
                    if (drumTrackName (m.getTextFromTextMetaEvent())) drumTrack[(size_t) t] = true;
                }
                else if (m.isTextMetaEvent())
                {
                    const auto text = m.getTextFromTextMetaEvent().trim();
                    if (text.startsWithIgnoreCase ("drums:")) { drumsTag = text; drumTrack[(size_t) t] = true; }
                }
                else if (m.isNoteOn())
                {
                    const double on = m.getTimeStamp() / tpq;
                    const double off = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() / tpq : on + 0.5;
                    notes.push_back ({ on, juce::jmax (0.05, off - on), m.getNoteNumber(), m.getChannel(), t, (int) m.getVelocity() });
                    (m.getChannel() == 10 ? onTen : onOther)++;
                }
            }
            if (onTen > 0 && onOther == 0) drumTrack[(size_t) t] = true;
        }
        if (tempoSecsPerQuarter > 0) r.bpm = std::round (6000.0 / tempoSecsPerQuarter) / 100.0;
        bool anyDrum = false;
        for (const auto& n : notes) anyDrum = anyDrum || n.channel == 10 || drumTrack[(size_t) n.track];
        // a drum pattern with nothing to say which notes are drums but its "drums:" tag (Groove Grabber's, Song Savant's)
        const bool allDrums = ! anyDrum && drumsTag.isNotEmpty();
        auto isDrum = [&] (const Note& n) { return allDrums || n.channel == 10 || drumTrack[(size_t) n.track]; };

        // the key: the signature, or the notes (a C major signature with nothing else set is often a DAW's default)
        std::array<double, 12> chroma {};
        int melodic = 0;
        for (const auto& n : notes)
            if (! isDrum (n)) { chroma[(size_t) (n.note % 12)] += n.length * (n.note < 52 ? 1.5 : 1.0); ++melodic; }
        float conf = 0;
        int fromNotes = melodic >= 8 ? keyFromChroma (chroma, conf) : -1;
        // a major key and its relative minor share their notes (C major, A minor): the bass the song starts on decides
        if (fromNotes >= 0)
        {
            double first = 1.0e9;
            for (const auto& n : notes) if (! isDrum (n)) first = juce::jmin (first, n.beat);
            int bass = 128;
            for (const auto& n : notes) if (! isDrum (n) && n.beat < first + 0.25) bass = juce::jmin (bass, n.note);
            const int pc = fromNotes % 12;
            const bool minor = fromNotes >= 12;
            const int relPc = minor ? (pc + 3) % 12 : (pc + 9) % 12;
            if (bass < 128 && bass % 12 == relPc) fromNotes = minor ? relPc : 12 + relPc;
        }
        if (hasKeySig)
        {
            const int majorPc = ((sharps * 7) % 12 + 12) % 12;
            const int sigKey = keyMajor ? majorPc : 12 + (majorPc + 9) % 12;
            if (sharps != 0 || ! keyMajor) { r.key = sigKey; r.keyFrom = "its key signature"; }
            else if (fromNotes == 0 || fromNotes == 21) { r.key = fromNotes; r.keyFrom = "its key signature"; }
            else if (fromNotes >= 0) { r.key = fromNotes; r.keyFrom = "its notes"; r.keySure = conf >= 0.3f; }
        }
        else if (fromNotes >= 0)
        {
            r.key = fromNotes;
            r.keyFrom = "its notes";
            r.keySure = conf >= 0.3f;
        }

        // its drums: the busiest stretch of 4 bars (not an intro's lone crash), on its own, as a MIDI file
        std::vector<const Note*> drums;
        for (const auto& n : notes) if (isDrum (n)) drums.push_back (&n);
        if (! drums.empty())
        {
            if (hasTimeSig && ! (num == 4 && den == 4))
                r.why = "Its drums are in " + juce::String (num) + "/" + juce::String (den) + ", and MY GROOVES are in 4/4.";
            else
            {
                int lastBar = 0;
                for (auto* n : drums) lastBar = juce::jmax (lastBar, (int) (n->beat / 4.0 + 0.01));
                std::vector<int> perBar ((size_t) lastBar + 1, 0);
                for (auto* n : drums) ++perBar[(size_t) juce::jlimit (0, lastBar, (int) (n->beat / 4.0 + 0.01))];
                auto windowAt = [&] (int s) { int c = 0; for (int b = s; b < s + 4 && b <= lastBar; ++b) c += perBar[(size_t) b]; return c; };
                int most = 0;
                for (int s = 0; s <= lastBar; ++s) most = juce::jmax (most, windowAt (s));
                // from a full bar (an intro's lone crash isn't the groove)
                int fullest = 0;
                for (auto c : perBar) fullest = juce::jmax (fullest, c);
                int start = 0;
                while (start < lastBar && (windowAt (start) < most * 0.7 || perBar[(size_t) start] < fullest / 2)) ++start;
                const double from = start * 4.0, to = from + 16.0;

                juce::MidiMessageSequence seq;
                const double secs = tempoSecsPerQuarter > 0 ? tempoSecsPerQuarter : 0.5;
                seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (secs * 1.0e6)), 0);
                seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
                if (drumsTag.isNotEmpty()) seq.addEvent (juce::MidiMessage::textMetaEvent (1, drumsTag), 0);
                seq.addEvent (juce::MidiMessage::textMetaEvent (3, name + " drums"), 0);
                for (auto* n : drums)
                {
                    if (n->beat < from - 0.03 || n->beat >= to - 0.03) continue;
                    const double at = juce::jmax (0.0, n->beat - from) * tpq;
                    seq.addEvent (juce::MidiMessage::noteOn (n->channel, n->note, (juce::uint8) juce::jlimit (1, 127, n->velocity)), at);
                    seq.addEvent (juce::MidiMessage::noteOff (n->channel, n->note), at + juce::jmin (n->length, 0.24) * tpq);
                }
                seq.addEvent (juce::MidiMessage::endOfTrack(), 16.0 * tpq);
                seq.updateMatchedPairs();
                juce::MidiFile out;
                out.setTicksPerQuarterNote ((int) tpq);
                out.addTrack (seq);
                juce::MemoryOutputStream os;
                out.writeTo (os);
                juce::MemoryBlock block (os.getData(), os.getDataSize());
                const auto g = grooveFromMidi (block, name + " drums");
                if (g.why.isNotEmpty()) r.why = g.why;
                else { r.drums = block; r.drumBars = g.groove.bars; }
            }
        }
        r.ok = true;
        return r;
    }

    //==============================================================================
    ImportedSong importSong (const juce::File& f, const std::function<bool()>& shouldStop)
    {
        const auto e = extOf (f);
        ImportedSong r;
        r.file = f;
        r.name = f.getFileNameWithoutExtension();
        if (! f.existsAsFile())
        {
            r.error = "The file isn't there any more.";
            return r;
        }
        if (e == "sssong")
        {
            if (auto s = readSongSavantSong (f)) return songFromSongSavant (*s);
            r.error = "It isn't a song Song Savant can read back.";
            return r;
        }
        if (e == "kcsong")
        {
            if (auto s = readKeyCompassSong (f)) return songFromKeyCompass (*s);
            r.error = "It isn't a Key Compass song Groove Grabber can read.";
            return r;
        }
        if (midiTypes.contains (e))
        {
            juce::MemoryBlock data;
            if (! f.loadFileAsData (data)) { r.error = "Couldn't open it."; return r; }
            auto m = songFromMidi (data, r.name);
            m.file = f;
            return m;
        }
        if (audioTypes.contains (e))
        {
            r.from = ImportedSong::From::audio;
            const auto facts = analyseSong (f, shouldStop);
            if (! facts.ok)
            {
                r.error = facts.error.isNotEmpty() ? facts.error : juce::String ("Couldn't hear a tempo or key in it.");
                return r;
            }
            r.ok = true;
            r.bpm = facts.bpm > 0 ? std::round (facts.bpm * 10.0) / 10.0 : 0.0;
            r.otherBpm = otherTempo (r.bpm);
            r.key = facts.key;
            r.keyFrom = "the audio";
            r.keySure = facts.keyConfidence >= 0.35f;
            return r;
        }
        r.error = "Groove Grabber can't read this kind of file. Import a bounce (WAV, AIFF, FLAC, MP3), a MIDI file, "
                  "or a song from Song Savant or Key Compass.";
        return r;
    }
}
