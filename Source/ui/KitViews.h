// Groove Grabber - the kit cards (Groove Lab), the pad grid and the selected pad's editor.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    //==============================================================================
    class KitCard : public juce::Component
    {
    public:
        KitCard (GrooveGrabberProcessor&, int index);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override   { repaint(); }
        void mouseExit (const juce::MouseEvent&) override    { repaint(); }

    private:
        GrooveGrabberProcessor& proc;
        const int index;
        IconButton play { "play", Icon::play, Icon::stop, true }, save { "save", Icon::heart, Icon::heartFilled, true },
                   regen { "regen", Icon::refresh };
    };

    //==============================================================================
    class PadTile : public juce::Component, public juce::FileDragAndDropTarget, public juce::SettableTooltipClient
    {
    public:
        PadTile (GrooveGrabberProcessor&, gg::Slot);
        void refresh();
        void flash()                                           { flashLevel = 1.0f; repaint(); }
        bool decayFlash();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override     { repaint(); }
        void mouseExit (const juce::MouseEvent&) override      { repaint(); }
        bool isInterestedInFileDrag (const juce::StringArray&) override;
        void fileDragEnter (const juce::StringArray&, int, int) override   { dropHover = true; repaint(); }
        void fileDragExit (const juce::StringArray&) override              { dropHover = false; repaint(); }
        void filesDropped (const juce::StringArray&, int, int) override;
        gg::Slot getSlot() const noexcept                      { return slot; }
        std::function<void()> onSelect;

    private:
        void showMenu();
        juce::Rectangle<int> lockArea() const;
        GrooveGrabberProcessor& proc;
        const gg::Slot slot;
        WaveView wave;
        float flashLevel = 0;
        bool dropHover = false, dragStarted = false;
    };

    //==============================================================================
    class PadGrid : public juce::Component, private juce::Timer
    {
    public:
        explicit PadGrid (GrooveGrabberProcessor&);
        void refresh();
        void resized() override;
        std::function<void()> onSelect;

    private:
        void timerCallback() override;
        GrooveGrabberProcessor& proc;
        juce::OwnedArray<PadTile> tiles;
        std::array<std::uint32_t, gg::numPads> lastFlash {};
    };

    //==============================================================================
    class AltTile : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        AltTile (GrooveGrabberProcessor&);
        void setSound (const juce::String& path, gg::Cat);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override   { repaint(); }
        void mouseExit (const juce::MouseEvent&) override    { repaint(); }
        std::function<void (const juce::String&)> onUse;
        int number = 1;

    private:
        GrooveGrabberProcessor& proc;
        juce::String path;
        gg::Cat cat = gg::Cat::unknown;
        WaveView wave;
        juce::TextButton use { "USE" };
    };

    //==============================================================================
    class PadDetail : public juce::Component
    {
    public:
        explicit PadDetail (GrooveGrabberProcessor&);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void applyKnobs();
        void loadAlternatives();
        GrooveGrabberProcessor& proc;
        WaveView wave;
        LabelledKnob tune { "Tune", -12.0, 12.0, 0.1, 0.0, " st" }, start { "Start", 0.0, 250.0, 1.0, 0.0, " ms" },
                     length { "Length", 5.0, 100.0, 1.0, 100.0, " %" }, gain { "Gain", -24.0, 12.0, 0.1, 0.0, " dB" };
        juce::TextButton reverse { "REVERSE" }, lock { "LOCK" };
        IconButton playBtn { "play", Icon::play };
        juce::OwnedArray<AltTile> alts;
        juce::String shownPath;
        gg::Slot shownSlot = gg::Slot::count;
        int shownKit = -1;
        juce::String notes, source;
        bool updating = false;
    };
}
