// Groove Grabber - MY KITS: the kits you saved, and the ones generated recently.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    class KitsPage : public juce::Component
    {
    public:
        explicit KitsPage (GrooveGrabberProcessor&);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        std::function<void (const gg::Kit&)> onExport;
        std::function<void()> onLoaded;

    private:
        void rebuild (bool keepScroll = true);
        bool matchesSearch (const gg::Kit&) const;
        GrooveGrabberProcessor& proc;
        Segmented view { { "SAVED", "RECENT" } };
        juce::TextEditor search;
        juce::OwnedArray<juce::TextButton> genreChips;     // [0] = ALL, then one per genre (hidden when it has no kits)
        int genreFilter = -1;                               // -1 = all
        int kitsInView = 0;
        juce::Viewport viewport;
        juce::Component content;
        juce::OwnedArray<juce::Component> rows;
        juce::TextButton openFile { "OPEN KIT FILE..." };
        std::unique_ptr<juce::FileChooser> chooser;
    };
}
