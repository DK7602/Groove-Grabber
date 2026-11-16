// Groove Grabber - effects: what kind each FX sound is (risers, impacts, tape stops, scratches...), which genres and
// decades use it, where it goes against the beat, and the FX tab's catalog of your FX (analysed once, in the
// background, and remembered).
//
// The type comes from the file's name and folders when they say it ("Riser 4 Bars", "Tape Stop", "Vinyl Crackle"),
// otherwise from the sound's shape over its whole length: where it peaks, whether it gets brighter or darker, whether
// its pitch rises, falls or wobbles, how steady and how noisy it is. You can always correct it (right-click on FX).
#pragma once

#include "Library.h"
#include "Styles.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <map>

namespace gg::fx
{
    enum class Type : int
    {
        riser, reverseCymbal, swell,                       // build-ups
        impact, subDrop,                                   // hits
        downlifter, sweep, whoosh, tapeStop, rewind,       // falls and moves
        crackle, noise,                                    // textures
        scratch, zap, laser, blip, glitch, horn,           // accents
        other,
        count
    };
    constexpr int numTypes = (int) Type::count;

    enum class Group : int { buildUp, hit, move, texture, accent, other, count };

    juce::String typeName (Type);          // "Riser", "Reverse cymbal", "Air horn / siren"
    juce::String typeId (Type);            // "riser", "reverseCymbal" (stable: the cache and Fx.md use it)
    Type typeFromId (const juce::String&); // Type::count if unknown
    Group groupOf (Type);
    juce::String groupName (Group);        // "Build-ups", "Hits", "Falls and moves", "Textures", "Accents", "Other"

    // How it lines up with the beat: a build-up ends on the downbeat, a hit starts on it, a whoosh peaks on it,
    // a texture runs under the bar
    enum class Anchor { end, start, peak };
    Anchor anchorOf (Type);

    //==============================================================================
    // The sound over its whole length (up to 30 s), for the audio rules
    struct Shape
    {
        bool valid = false;
        float duration = 0;            // seconds, from the first sound to the last (silence at the ends left out)
        float peakPos = 0;             // where it's loudest, 0 = the start .. 1 = the end
        float startRelDb = 0, endRelDb = 0;   // the first / last sixth against the peak (a riser: very quiet, then 0)
        float holdPos = 0;             // how long it stays within 12 dB after its peak, as a share of its length
        float brightTrend = 0;         // octaves the brightness moves from the start to the end (+ = brighter)
        float brightHz = 0;            // average brightness
        float pitchShare = 0;          // how much of it has a clear pitch
        float pitchHz = 0;             // its middle pitch, when it has one
        float pitchTrend = 0;          // semitones the pitch moves from the start to the end
        float wobble = 0;              // pitch / brightness turning back and forth, per second (scratches)
        float clicks = 0;              // sharp little jumps per second (crackle, glitches)
        float gaps = 0;                // drop-outs per second (stutters)
        float steadyDb = 0;            // how much the level moves (a noise bed hardly does)
        float lowShare = 0;            // energy under 150 Hz
    };

    Shape measureShape (const float* mono, int numSamples, double sampleRate);
    Shape measureShape (const juce::File&, juce::AudioFormatManager&);

    // The type a file's name and folders say ("Type::count": they don't)
    Type fromName (const juce::String& fileName, const juce::String& folders);
    // The type from the sound itself
    Type fromShape (const Shape&, const SoundInfo&);

    //==============================================================================
    // Which genres and decades use it
    struct Fit
    {
        int level = 1;                 // 2 = great fit, 1 = fits, 0 = unusual
        juce::String word;             // "Great fit", "Fits", "Unusual"
        juce::String why;              // "Great fit for Trap today", "Unusual for Country in the 60s"
    };
    Fit fitFor (Type, Genre, int era);
    juce::String bestFor (Type);       // "Best in: EDM, Pop, Trap  -  2000s to today"
    juce::String tip (Type);           // how it's used (Resources/Fx.md)
    juce::StringArray tipKeys();       // (for the tests)

    // How long it is in bars at a tempo
    double barsAt (double seconds, double bpm);

    // Where it goes when you hear it in time: the beat (from the start of the preview) it starts on, the downbeat it
    // lands on, and how many beats the preview runs. One bar of the groove first, two after.
    struct Placement { double startBeat = 0, downbeat = 0, totalBeats = 0; };
    Placement place (Type, double lengthSec, double peakSec, double bpm);

    //==============================================================================
    // Your FX: the type of every FX sound in the library, analysed in the background and kept in AppData
    // (fx-types.json). What you set by hand is never re-guessed.
    class Catalog : private juce::Thread, public juce::ChangeBroadcaster
    {
    public:
        Catalog();
        ~Catalog() override;

        struct Item
        {
            Type type = Type::other;
            bool byUser = false, byName = false, analysed = false;
            Shape shape;
        };

        // The library changed: analyse the FX sounds not seen yet (or changed since)
        void update (const SoundListPtr& sounds);
        Item get (const SoundInfo&) const;           // (before it's analysed: the name's type, or "other")
        void setType (const juce::String& path, Type);
        bool isBusy() const noexcept                 { return busy.load(); }
        void waitUntilDone (int timeoutMs);          // (the tests)

    private:
        void run() override;
        void load();
        void save();
        struct Stored { Item item; juce::int64 modTime = 0, size = 0; };
        mutable juce::CriticalSection lock;
        std::map<juce::String, Stored> items;
        std::vector<SoundInfo> pending;
        std::atomic<bool> busy { false };
        juce::File file;
    };
}
