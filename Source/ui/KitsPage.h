// Groove Grabber - MY KITS: the kits you saved, the ones generated recently, and MY GROOVES (patterns you saved
// as your own on SAUCE).
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
        std::function<void (const juce::String&)> onOpenGroove;    // show one of my grooves on SAUCE
        std::function<void()> onUseGroove;                         // after BUILD was set to one of my grooves
        void setView (int v)                       { view.setSelected (v); rebuild (false); }   // 0 saved, 1 recent, 2 my grooves
        int numRows() const noexcept               { return rows.size(); }

    private:
        void rebuild (bool keepScroll = true);
        bool matchesSearch (const gg::Kit&) const;
        GrooveGrabberProcessor& proc;
        Segmented view { { "SAVED", "RECENT", "MY GROOVES" } };
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
