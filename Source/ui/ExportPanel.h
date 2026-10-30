// Groove Grabber - the export panel: write the kit for Groove Agent, then drag it straight onto pad 1.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    class ExportPanel : public juce::Component
    {
    public:
        explicit ExportPanel (GrooveGrabberProcessor&);
        void show (const gg::Kit&);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;

    private:
        void doExport();
        GrooveGrabberProcessor& proc;
        gg::Kit kit;
        juce::TextEditor name;
        juce::ToggleButton mpc { "MPC program (.pgm) - Groove Agent can import it" }, midi { "Groove MIDI file (the preview groove)" },
                           placeholders { "Silent files for empty pads (keeps the numbering)" };
        Segmented bits { { "16-BIT", "24-BIT" } }, rate { { "44.1 kHz", "48 kHz" } };
        juce::TextButton go { "EXPORT KIT" }, close { "CLOSE" }, reveal { "SHOW FOLDER" }, change { "CHANGE..." };
        std::unique_ptr<juce::FileChooser> chooser;
        gg::ExportResult result;
        bool busy = false, dragging = false;
        juce::Rectangle<int> card, dragHandle, destRow;
    };
}
