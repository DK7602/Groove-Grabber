// Groove Grabber - the plug-in / app: holds the song, the four kits, the sound engine and the library.
#pragma once

#include "core/Library.h"
#include "core/KitGenerator.h"
#include "core/KitPlayer.h"
#include "core/SongContext.h"
#include "core/Exporter.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

class GrooveGrabberProcessor : public juce::AudioProcessor,
                               public juce::ChangeBroadcaster,
                               private juce::ChangeListener
{
public:
    GrooveGrabberProcessor();
    ~GrooveGrabberProcessor() override;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                    { return true; }
    const juce::String getName() const override        { return "Groove Grabber"; }
    bool acceptsMidi() const override                  { return true; }
    bool producesMidi() const override                 { return false; }
    bool isMidiEffect() const override                 { return false; }
    double getTailLengthSeconds() const override       { return 0.0; }
    int getNumPrograms() override                      { return 1; }
    int getCurrentProgram() override                   { return 0; }
    void setCurrentProgram (int) override              {}
    const juce::String getProgramName (int) override   { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- the song ----
    gg::KitOptions options;
    juce::String description;
    gg::ParsedDescription understood;
    juce::String songName;                         // from Key Compass, if picked there
    bool genreByHand = false;                      // picked in the genre box: artists are read inside it
    void applyDescription (const juce::String& text);
    void setGenre (gg::Genre);
    void setStyle (int subStyle);
    void setEra (int era);
    void setKey (int key);
    void setBpm (double bpm);
    juce::String firstGroove() const;              // kit 1's preview groove
    // A song Song Savant sent: its description, genre, style, era, key and tempo, then four kits for it
    void useSongSavantSong (const gg::SongSavantSong&);

    // ---- the four kits ----
    std::vector<gg::Kit> kits;
    int activeKit = 0;
    gg::Slot selectedSlot = gg::Slot::kick;
    void generate (bool surprise);                 // all four (locked pads kept)
    void regenerateKit (int index, bool surprise); // one kit's unlocked pads
    void setActiveKit (int index);
    gg::Kit* getActiveKit();
    void kitChanged (int index);                   // after editing a pad: re-render, notify
    void setSlotSound (int kitIndex, gg::Slot, const juce::String& path);
    void setAllLocked (int kitIndex, bool locked);
    juce::StringArray alternativesFor (int kitIndex, gg::Slot, int count = 5);
    void loadKit (const gg::Kit&, int intoIndex);
    juce::String getStatusMessage() const          { return statusMessage; }

    // ---- listening ----
    gg::KitPlayer player;
    juce::String grooveId = "boombap1";
    void setGrooveId (const juce::String&);

    // ---- grooves: the built-in ones, or your edit of one (from SAUCE, kept with the song) ----
    std::shared_ptr<const gg::Groove> getGroove (const juce::String& id) const;
    bool isGrooveEdited (const juce::String& id) const;
    void editGroove (const juce::String& id, std::vector<gg::GrooveHit> hits);   // the original's hits reset it
    void resetGroove (const juce::String& id);

    // ---- my grooves: patterns you saved as your own, for every song (core/MyGrooves) ----
    juce::String saveMyGroove (const juce::String& fromId, const juce::String& name, gg::Genre, const juce::String& styleId);
    void renameMyGroove (const juce::String& id, const juce::String& name);
    void deleteMyGroove (const juce::String& id);
    void auditionSlot (gg::Slot);
    void previewFile (const juce::String& path, gg::Cat);
    gg::PlayableKit::Ptr getPlayable (int kitIndex) const;
    bool isStandalone() const noexcept             { return wrapperType == wrapperType_Standalone; }

    // ---- library & settings ----
    juce::SharedResourcePointer<gg::Library> library;
    gg::Settings& getSettings()                    { return library->getSettings(); }

    // run something off the message / audio threads (rendering, export, harvest)
    void runInBackground (std::function<void()> job);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::String makeState();
    void applyState (const juce::String&);
    void renderKits();
    void notify (const juce::String& message = {});
    gg::KitGenerator makeGenerator() const;
    void resetCharacter();          // the sliders back to style + era + what the description said

    juce::ThreadPool worker { juce::ThreadPoolOptions{}.withThreadName ("GG render").withNumberOfThreads (2)
                                                       .withDesiredThreadPriority (juce::Thread::Priority::low) };
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
    std::array<gg::PlayableKit::Ptr, 4> playables;
    std::atomic<int> renderGeneration { 0 };
    juce::uint32 seed = 1;
    juce::String statusMessage;
    juce::AudioBuffer<float> monoScratch;            // for hosts that pick a mono output
    void swapGroove (const juce::String& id, std::shared_ptr<const gg::Groove> edit);
    void retireGroove (std::shared_ptr<const gg::Groove>);      // kept a while: the audio thread may still be reading it
    void repointPlayer (const juce::String& id);                 // the player plays the current version of this groove
    std::map<juce::String, std::shared_ptr<const gg::Groove>> grooveEdits;
    // replaced edits, kept a while: the audio thread may still be reading the one it had
    std::vector<std::pair<std::shared_ptr<const gg::Groove>, juce::uint32>> retiredGrooves;

    juce::CriticalSection snapshotLock;
    juce::String stateSnapshot;                      // for hosts that ask for the state off the message thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGrabberProcessor)
};
