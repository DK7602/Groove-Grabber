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
        void rebuild();
        GrooveGrabberProcessor& proc;
        Segmented view { { "SAVED", "RECENT" } };
        juce::Viewport viewport;
        juce::Component content;
        juce::OwnedArray<juce::Component> rows;
        juce::TextButton openFile { "OPEN KIT FILE..." };
        std::unique_ptr<juce::FileChooser> chooser;
        int lastCount = -1;
    };
}
