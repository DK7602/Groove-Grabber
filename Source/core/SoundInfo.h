// Groove Grabber - everything the analysis learns about one sound file.
#pragma once

#include "SoundTypes.h"

namespace gg
{
    // The character of a sound, each 0..1. These are what the kit character sliders aim at.
    struct Traits
    {
        float punch = 0.5f, warmth = 0.5f, bright = 0.5f, room = 0.5f, vintage = 0.5f, aggr = 0.5f;
        float organic = 0.5f;   // 0 = programmed / electronic, 1 = live / acoustic

        static constexpr int num = 7;
        float& operator[] (int i) noexcept;
        float operator[] (int i) const noexcept;
        static juce::String name (int i);          // "Punch", "Warmth"...
        juce::var toVar() const;
        static Traits fromVar (const juce::var&);
    };

    enum class Source : int { packs, oneShots, recordings, sampleSnagger, harvested, count };
    juce::String sourceName (Source);

    struct SoundInfo
    {
        // ---- where it is ----
        juce::String path;              // absolute path
        juce::String pack;              // the folder it came from under its source (the pack name)
        Source source = Source::packs;
        juce::int64 fileSize = 0, modTime = 0;

        // ---- what it is ----
        Cat cat = Cat::unknown;
        float catConfidence = 0.0f;     // 0..1
        bool catByUser = false;         // the user picked the category; never re-guessed
        PercKind percKind = PercKind::none;
        juce::uint32 genreTags = 0;     // bit per Genre found in the path ("Trap Kit/...")

        // ---- measured ----
        float duration = 0;             // seconds (whole file)
        double sampleRate = 44100;
        int channels = 1;
        float peakDb = -100, loudDb = -100;       // loudDb = loudest 50 ms, RMS
        float leadSilenceMs = 0;        // silence before the hit (trimmed on the way out)
        float attackMs = 0, decayMs = 0;           // onset -> peak, peak -> -30 dB
        float centroidHz = 0, rolloffHz = 0, flatness = 0;
        float airFlatness = 0;          // how noise-like the top (2 kHz up) is: wires, shakers, cymbals ~ high; bells, rims ~ low
        float ringMs = 0;               // peak -> -40 dB (cymbals and 808s ring on long after a hat or a kick has gone)
        float pitchGlide = 0;           // semitones the pitch falls over the first ~150 ms (kicks sweep, toms barely move)
        float subShare = 0, lowShare = 0, midShare = 0, highShare = 0;   // energy shares of the hit
        float pitchHz = 0, pitchConf = 0;          // fundamental (kicks, 808s, toms)
        int onsets = 1;                 // hits in the file (more than a few = a loop)
        int burstCount = 1;             // separate bursts in the first 60 ms, each after a real gap (claps have 3 or 4)
        float crestDb = 0;              // peak to RMS
        float tailFloorDb = -100;       // noise left once it has died away (vinyl, hiss)
        bool clipped = false;
        int measureVersion = 0;         // which version of the measurements this is (older ones are measured again)
        static constexpr int currentMeasure = 2;
        bool needsMeasure() const noexcept { return measureVersion < currentMeasure; }

        Traits traits;

        // ---- the user's opinion ----
        int rating = 0;                 // 1 = favourite, -1 = never use
        bool hidden = false;

        bool isUsable() const noexcept  { return ! hidden && rating >= 0 && catIsKitSound (cat); }
        juce::String fileName() const   { return juce::File (path).getFileNameWithoutExtension(); }
        float midiPitch() const noexcept;           // fundamental as a (fractional) MIDI note, 0 if none
        bool hasPitch() const noexcept  { return pitchHz > 0 && pitchConf >= 0.45f; }

        void write (juce::OutputStream&) const;
        bool read (juce::InputStream&, int version);
    };

    // Words in a path that mean a genre ("trap", "boom bap", "nashville"...)
    juce::uint32 genreTagsForPath (const juce::String& lowerPath);
}
