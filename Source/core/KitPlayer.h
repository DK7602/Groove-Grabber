// Groove Grabber - the sound engine: plays a kit from MIDI (General MIDI drum notes, the same layout
// as the exports), from the on-screen pads, and as a preview groove - free-running at the song's tempo,
// or locked to your DAW's transport so you hear the kit inside your song.
#pragma once

#include "Kit.h"
#include "Grooves.h"
#include "SampleRender.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <atomic>

namespace gg
{
    // A kit rendered for playback: one buffer per pad (note 36 upwards)
    struct PlayableKit : public juce::ReferenceCountedObject
    {
        using Ptr = juce::ReferenceCountedObjectPtr<PlayableKit>;
        std::array<std::shared_ptr<const juce::AudioBuffer<float>>, numPads> pads;
        juce::String kitId;
        double sampleRate = 44100;
    };

    // Renders every pad of a kit (shared cache, so switching kits is instant the second time). Any thread.
    PlayableKit::Ptr buildPlayable (const Kit&, double sampleRate);
    std::shared_ptr<const juce::AudioBuffer<float>> renderPad (const Kit&, const PadDef&, double sampleRate);
    std::shared_ptr<const juce::AudioBuffer<float>> renderSlotPreview (const Kit&, Slot, double sampleRate);
    void clearRenderCache();

    struct PreviewSound : public juce::ReferenceCountedObject
    {
        using Ptr = juce::ReferenceCountedObjectPtr<PreviewSound>;
        std::shared_ptr<const juce::AudioBuffer<float>> audio;
    };

    class KitPlayer : private juce::Timer
    {
    public:
        KitPlayer();
        ~KitPlayer() override;

        void prepare (double sampleRate, int maxBlockSize);
        double getSampleRate() const noexcept      { return sampleRate.load(); }

        // ---- message thread ----
        void setKit (PlayableKit::Ptr);
        PlayableKit::Ptr getKit() const;
        void playPreview (std::shared_ptr<const juce::AudioBuffer<float>>);   // a single sound (library, alternatives)
        void setGroove (const Groove* g)           { groove.store (g); }
        const Groove* getGroove() const noexcept   { return groove.load(); }
        void setGroovePlaying (bool);
        bool isGroovePlaying() const noexcept      { return groovePlaying.load(); }
        // play on from this beat (SAUCE's timeline), or start there if it's about to play. While your DAW drives
        // the groove (Follow DAW), the DAW's position leads.
        void seekGroove (double beat);
        void setFollowHost (bool b)                { followHost = b; }
        bool isFollowingHost() const noexcept      { return followHost.load(); }
        void setTempo (double bpm)                 { tempo = juce::jlimit (40.0, 250.0, bpm); }
        double getTempo() const noexcept           { return tempo.load(); }
        void setSwing (float s)                    { swing = juce::jlimit (0.0f, 0.7f, s); }
        float getSwing() const noexcept            { return swing.load(); }
        void setVolume (float v)                   { volume = juce::jlimit (0.0f, 2.0f, v); }
        float getVolume() const noexcept           { return volume.load(); }
        double getGrooveBeat() const noexcept      { return displayBeat.load(); }   // for the UI playhead
        bool isHostDriving() const noexcept        { return hostDriving.load(); }

        // ---- any thread ----
        void trigger (int note, float velocity);
        std::uint32_t getPadFlash (int padIndex) const noexcept;   // counter that ticks when a pad plays

        // ---- audio thread ----
        struct HostInfo { bool playing = false; double ppq = 0, bpm = 0; bool valid = false; };
        void process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const HostInfo& host);
        void allNotesOff();

    private:
        struct Voice
        {
            const float* l = nullptr; const float* r = nullptr;
            int length = 0, pos = 0, note = -1, choke = 0, startOffset = 0;
            float gain = 0, fade = 1, fadeStep = 0;
            bool active = false;
            juce::uint64 age = 0;
        };

        void startVoice (int note, float velocity, int offset);
        void startPreviewVoice();
        void render (juce::AudioBuffer<float>&, int start, int num);
        void scheduleGroove (double beatStart, double beatEnd, int blockSamples, double samplesPerBeat);
        void timerCallback() override;

        std::atomic<double> sampleRate { 44100.0 };
        std::atomic<double> tempo { 100.0 };
        std::atomic<float> swing { 0.0f }, volume { 0.9f };
        std::atomic<bool> groovePlaying { false }, followHost { true }, hostDriving { false }, resetGroove { false };
        std::atomic<const Groove*> groove { nullptr };
        std::atomic<double> displayBeat { 0.0 }, seekTo { -1.0 };
        double internalBeat = 0;
        bool wasHostPlaying = false;

        juce::SpinLock kitLock;
        PlayableKit::Ptr pendingKit, currentKit;
        PreviewSound::Ptr pendingPreview, currentPreview;
        std::atomic<bool> previewFlag { false };

        std::array<Voice, 40> voices;
        juce::uint64 voiceCounter = 0;
        std::array<std::atomic<std::uint32_t>, numPads> padFlash {};

        struct Trig { int note; float vel; };
        juce::AbstractFifo fifo { 128 };
        std::array<Trig, 128> trigs;
        juce::SpinLock fifoWriteLock;

        // released on the message thread, never the audio thread
        struct Kept { juce::ReferenceCountedObjectPtr<juce::ReferenceCountedObject> object; int idleTicks = 0; };
        std::vector<Kept> keepAlive;
        void keep (juce::ReferenceCountedObject*);
    };
}
