// Groove Grabber - BUILD: describe the song, shape the character, generate four kits, edit them, listen.
#pragma once

#include "KitViews.h"

namespace ggui
{
    class SongPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit SongPanel (GrooveGrabberProcessor&);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        std::function<void()> onGenerate;
        void checkSongSavant();                 // a song Song Savant just sent: offer it (also the tests)
        bool isOfferingSongSavant() const       { return waiting.has_value(); }

    private:
        void timerCallback() override           { checkSongSavant(); }
        void pushOptions();
        void syncDescription();
        GrooveGrabberProcessor& proc;
        std::optional<gg::SongSavantSong> waiting;
        juce::TextButton useSongSavant, skipSongSavant { "X" };
        juce::TextEditor description;
        juce::ComboBox genre, styleBox, era;
        gg::Genre stylesShownFor = gg::Genre::count;
        juce::OwnedArray<juce::Slider> traits;
        Segmented feel { { "AUTO", "LIVE", "HYBRID", "PROG" } }, lowEnd { { "AUTO", "KICK", "KICK + 808" } },
                  perc { { "AUTO", "0", "1", "2", "3" } };
        juce::ToggleButton fx { "FX / sound effects" }, tuneKey { "Tune kick, 808, toms to key" };
        juce::OwnedArray<juce::ToggleButton> sources;
        juce::TextButton generate { "GENERATE KITS" };
        IconButton surprise { "surprise", Icon::dice };
        juce::Rectangle<int> traitsArea, optionsArea, sourcesArea, understoodArea;
        bool updating = false;
    };

    class TransportBar : public juce::Component, private juce::Timer
    {
    public:
        explicit TransportBar (GrooveGrabberProcessor&);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        Toast toast;
        std::function<void()> onEditGroove;     // EDIT IN SAUCE: the groove playing here, open on SAUCE

    private:
        void timerCallback() override;
        GrooveGrabberProcessor& proc;
        IconButton play { "play", Icon::play, Icon::stop, true };
        juce::ComboBox groove;
        juce::TextButton editGroove { "EDIT IN SAUCE" };
        juce::Slider swing, volume;
        juce::ToggleButton follow { "Follow DAW" };
        double lastBeat = -1;
        void fillGrooves();                     // the built-in grooves, then MY GROOVES
        juce::StringArray myIds;                // my grooves in the list (item id 1000 + index)
        juce::String grooveListSignature;
    };

    class BuildPage : public juce::Component
    {
    public:
        explicit BuildPage (GrooveGrabberProcessor&);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        std::function<void()> onExport;
        std::function<void()> onEditGroove;     // open BUILD's groove on SAUCE

    private:
        GrooveGrabberProcessor& proc;
        SongPanel song;
        juce::OwnedArray<KitCard> cards;
        PadGrid grid;
        PadDetail detail;
        TransportBar transport;
        juce::TextButton regenerate { "REGENERATE" }, lockAll { "LOCK ALL" }, saveKit { "SAVE KIT" }, exportKit { "EXPORT" };
        juce::Rectangle<int> labArea, editorHeader;
    };
}
