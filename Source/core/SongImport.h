// Groove Grabber - IMPORT SONG: what a song can tell Groove Grabber, from wherever it comes.
//
//   a bounce or rough mix (WAV, AIFF, FLAC, MP3, OGG)   its tempo (or double / half) and key, worked out from the audio
//   a MIDI file from any DAW                            its tempo, its key (its key signature, or worked out from the
//                                                       notes), and its drum part, to keep as one of MY GROOVES
//   a song Song Savant sent (.sssong)                   everything: description, genre, style, decade, key, tempo
//   a Key Compass song (.kcsong)                        its key (or from its chords) and tempo
//
// No UI here: the processor offers what was found (BUILD shows it under the description) before anything changes.
#pragma once

#include "SongContext.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>
#include <optional>

namespace gg
{
    struct ImportedSong
    {
        enum class From { audio, midi, songSavant, keyCompass };
        From from = From::audio;
        bool ok = false;
        juce::String error;              // when it couldn't be read: what's wrong, in plain words
        juce::String name;               // the song's name (or the file's)
        juce::File file;
        double bpm = 0;                  // 0: it didn't say
        double otherBpm = 0;             // audio: the tempo it could also be (double or half the one heard), 0 = none
        int key = -1;                    // 0..11 major, 12..23 minor, -1 = none
        bool keySure = true;             // false: worked out, and it's a guess
        juce::String keyFrom;            // "its key signature", "its notes", "its chords", "the audio"
        std::optional<SongSavantSong> songSavant;   // the whole song, when Song Savant sent it
        juce::MemoryBlock drums;         // MIDI: its drum part on its own (a MIDI file, up to 4 bars), empty = none
        int drumBars = 0;                // how many bars of them
        juce::String why;                // MIDI: why its drums can't be kept (empty when they can, or it has none)

        bool hasDrums() const noexcept   { return drums.getSize() > 0; }
        // "A Minor, 92 BPM (or 184)": what USE IT sets
        juce::String summary() const;
        // where it came from: "Song Savant sent \"Back Roads\"", "\"Late Night.wav\""
        juce::String title() const;
    };

    bool canImport (const juce::File&);          // a file IMPORT SONG reads
    juce::String importPatterns();               // for a file chooser: "*.wav;*.mid;..."

    // Reads a song file (audio is analysed: a few seconds; shouldStop is checked now and then)
    ImportedSong importSong (const juce::File&, const std::function<bool()>& shouldStop = {});
    ImportedSong songFromMidi (const juce::MemoryBlock&, const juce::String& name);
    ImportedSong songFromSongSavant (const SongSavantSong&);
    ImportedSong songFromKeyCompass (const KeyCompassSong&);

    // A tempo heard in audio is easily double or half the one you count: the other one worth offering (0 = none)
    double otherTempo (double bpm);
}
