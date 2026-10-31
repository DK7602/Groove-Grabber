// Groove Grabber - the top bar: logo, pages, the song's key and tempo, Key Compass, theme, settings.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    // A number you drag up / down, or double-click to type (the BPM box)
    class DragNumber : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        DragNumber();
        void setValue (double v)                  { value = v; repaint(); }
        double getValue() const noexcept          { return value; }
        std::function<void (double)> onChange;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void resized() override;

    private:
        double value = 100, dragStartValue = 100;
        juce::Label editor;
    };

    class HeaderBar : public juce::Component
    {
    public:
        explicit HeaderBar (GrooveGrabberProcessor&);
        void refresh();
        void setTab (int);
        std::function<void (int)> onTab;
        std::function<void()> onThemeChanged;
        std::function<void (int)> onHelp;              // 0 = the manual, 1 = About
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;

    private:
        void toggleTheme();
        void showKeyCompassMenu();
        void showSettingsMenu();
        GrooveGrabberProcessor& proc;
        juce::OwnedArray<juce::TextButton> tabs;
        juce::ComboBox key;
        DragNumber bpm;
        IconButton keyCompass { "Key Compass", Icon::compass }, theme { "theme", Icon::moon }, gear { "settings", Icon::gear },
                   help { "manual", Icon::help };
        std::unique_ptr<juce::FileChooser> chooser;
        juce::Rectangle<int> logoArea, keyLabel, bpmLabel;
        int tab = 0;
    };
}
