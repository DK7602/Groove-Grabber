// Groove Grabber - the export panel: write the kit for your DAW's drum sampler (picked for the DAW it's running
// in, or by hand), then drag it straight onto the sampler's first pad.
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
        void fillDaws();
        GrooveGrabberProcessor& proc;
        juce::ComboBox daw;
        gg::Kit kit;
        juce::TextEditor name;
        juce::ToggleButton mpc { "MPC program (.pgm) for MPCs and samplers that import them" }, midi { "Groove MIDI file (the preview groove)" },
                           placeholders { "Silent files for empty pads (keeps the numbering)" },
                           genreFolder { "In a genre folder (Exports > Hip-Hop > your kit)" };
        Segmented bits { { "16-BIT", "24-BIT" } }, rate { { "44.1 kHz", "48 kHz" } };
        juce::TextButton go { "EXPORT KIT" }, close { "CLOSE" }, reveal { "SHOW FOLDER" }, change { "CHANGE..." };
        std::unique_ptr<juce::FileChooser> chooser;
        gg::ExportResult result;
        bool busy = false, dragging = false;
        juce::Rectangle<int> card, dragHandle, destRow, dawRow;
    };
}
