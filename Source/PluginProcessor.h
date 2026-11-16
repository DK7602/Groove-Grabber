// Groove Grabber - the plug-in / app: holds the song, the four kits, the sound engine and the library.
#pragma once

#include "core/Library.h"
#include "core/KitGenerator.h"
#include "core/KitPlayer.h"
#include "core/SongContext.h"
#include "core/Exporter.h"
#include "core/Fx.h"
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

    // ---- the Song FX bank: up to 8 effects for the song (FX tab > ADD TO SONG FX). Every kit plays them on its Song FX
    // pads (notes 69 up; 4, 6 or 8 by its size). Saved with the song; GENERATE, REGENERATE and LOAD never touch it. ----
    std::array<gg::SlotState, gg::numBankSlots> songFx;
    int songFxPads() const;                              // the bank pads the kit you're on has (4, 6 or 8)
    int songFxWaiting() const;                           // sounds in the bank past them (a smaller kit size)
    // (fitBars: FIT, 1 2 4 or 8 bars at the song's tempo, 0 = as it is; what lands on the bar line comes from its kind)
    int addSongFx (const juce::String& path, int fitBars = 0);            // on the next free pad; -1: they're full
    void setSongFx (int index, const juce::String& path, int fitBars = 0);   // empty: clears it
    gg::Slot freeKitFx() const;                          // the kit's first empty FX pad (Slot::count: none)
    bool putOnKit (const juce::String& path, gg::Slot fxSlot, int fitBars = 0);   // on the kit you're on, one of its own FX pads
    void setFit (gg::SlotState&, int fitBars) const;     // FIT on a pad, with what lands on the bar line for its kind
    // Pitched FX tuned to the key (with "Tune ... to key" on): the steps for this effect (0: none, or it can't be)
    float fxTuneFor (const juce::String& path) const;
    void retuneFx (bool render = true);                  // every FX pad (the kits' and the bank) to the key; re-renders when it changed
    void loadKit (const gg::Kit&, int intoIndex);
    juce::String getStatusMessage() const          { return statusMessage; }

    // ---- SAUCE's kit: one of MY GROOVES can play with the kit it was saved with ("its original kit"), on SAUCE only.
    // BUILD's four kits, and the one picked there, stay as they are, and BUILD always plays BUILD's kit. ----
    void setSauceKit (std::shared_ptr<const gg::Kit>);     // nullptr: SAUCE plays BUILD's kit again
    std::shared_ptr<const gg::Kit> getSauceKit() const noexcept   { return sauceKit; }
    const gg::Kit* kitOnSauce();                           // what SAUCE plays: its own kit, or BUILD's (nullptr: none yet)
    void setSauceShowing (bool);                           // SAUCE on screen: its kit plays (its pads, your DAW's notes)
    gg::PlayableKit::Ptr getSaucePlayable() const          { return saucePlayable; }

    // ---- listening ----
    gg::KitPlayer player;
    juce::String grooveId = "boombap1";
    void setGrooveId (const juce::String&);

    // ---- grooves: the built-in ones, or your edit of one (from SAUCE, kept with the song) ----
    std::shared_ptr<const gg::Groove> getGroove (const juce::String& id) const;
    bool isGrooveEdited (const juce::String& id) const;
    // changes wait (with the song) until saved: a built-in groove keeps them as your edit for this song; one of
    // your grooves keeps them until SAVE (saveGrooveChanges) or SAVE AS COPY. The saved hits again clear them.
    void editGroove (const juce::String& id, std::vector<gg::GrooveHit> hits);
    void editGroove (const juce::String& id, std::vector<gg::GrooveHit> hits, float swing);
    void setGrooveSwing (const juce::String& id, float swing);   // 0 = straight .. 0.5 = a full triplet (SAUCE's SWING)
    void resetGroove (const juce::String& id);

    // ---- my grooves: patterns you saved as your own, for every song (core/MyGrooves) ----
    juce::String saveMyGroove (const juce::String& fromId, const juce::String& name, gg::Genre, const juce::String& styleId);
    void renameMyGroove (const juce::String& id, const juce::String& name);
    bool saveGrooveChanges (const juce::String& id);   // your changes go into one of your grooves (SAVE)
    juce::String restoreMyGroove (const juce::File& backup);   // a groove back from My Grooves/Backups
    // A MIDI pattern dropped on SAUCE, saved as one of your grooves (with this genre and style; `bpm` when the file
    // has no tempo). Returns its id, or empty (with a message saying why).
    juce::String importMyGroove (const juce::File& midi, gg::Genre, const juce::String& styleId, double bpm);
    void deleteMyGroove (const juce::String& id);

    // COACH MY BEAT's ON PURPOSE tips, kept after the app closes: one of MY GROOVES keeps them in the settings
    // (so they follow it into every song), any other pattern (an edit, BLANK) with the song
    juce::StringArray getCoachOnPurpose (const juce::String& grooveId);
    void setCoachOnPurpose (const juce::String& grooveId, const juce::StringArray& tipIds);
    void auditionSlot (gg::Slot);
    void previewFile (const juce::String& path, gg::Cat);
    gg::PlayableKit::Ptr getPlayable (int kitIndex) const;
    bool isStandalone() const noexcept             { return wrapperType == wrapperType_Standalone; }

    // ---- FX: your effects, typed (core/Fx), and heard in time with the groove ----
    juce::SharedResourcePointer<gg::fx::Catalog> fxCatalog;
    double songBpm() const;                        // the song's tempo (or its style's)
    double dawBpm() const noexcept                 { return hostBpm.load(); }   // the DAW's tempo (0: none, or standalone)
    // Plays the effect over BUILD's groove and kit, placed where it goes: a build-up ends on a downbeat, a hit starts
    // on one, a whoosh peaks on one. With no kits yet, over a click. Returns how long it plays (seconds).
    double hearFxInTime (const juce::String& path, gg::fx::Type, int fitBars = 0);
    // The effect as it plays here (tuned to the key, fit to the bars), written to Documents/Groove Grabber/FX Renders
    // for DRAG; the original file when it's played as it is
    juce::File fxFileToDrag (const juce::String& path, gg::fx::Type, int fitBars);
    void stopPreview();                            // a sound playing on its own (a library sound, an FX) stops

    // ---- library & settings ----
    juce::SharedResourcePointer<gg::Library> library;
    gg::Settings& getSettings()                    { return library->getSettings(); }

    // run something off the message / audio threads (rendering, export, harvest)
    void runInBackground (std::function<void()> job);

private:
    std::map<juce::String, juce::StringArray> coachOnPurpose;     // the song's patterns' ON PURPOSE tips
    std::shared_ptr<const gg::Kit> sauceKit;                       // a groove's original kit, playing on SAUCE
    gg::PlayableKit::Ptr saucePlayable;
    bool sauceShowing = false;
    std::atomic<int> sauceGeneration { 0 };
    void renderSauceKit();
    void updatePlayerKit();          // SAUCE's own kit while SAUCE shows it, BUILD's picked kit otherwise
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::String makeState();
    void applyState (const juce::String&);
    void renderKits();
    void pushSongFx();                  // the bank into every kit (the pads its size has; the rest cleared)
    void pullSongFx (int kitIndex);     // changes made on a kit's Song FX pads (its knobs), back into the bank
    void notify (const juce::String& message = {});
    gg::KitGenerator makeGenerator() const;
    void resetCharacter();          // the sliders back to style + era + what the description said

    juce::ThreadPool worker { juce::ThreadPoolOptions{}.withThreadName ("GG render").withNumberOfThreads (2)
                                                       .withDesiredThreadPriority (juce::Thread::Priority::low) };
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
    std::array<gg::PlayableKit::Ptr, 4> playables;
    std::atomic<int> renderGeneration { 0 };
    std::atomic<double> hostBpm { 0.0 };
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
