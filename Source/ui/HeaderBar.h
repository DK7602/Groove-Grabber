// Groove Grabber - the top bar: logo, pages, the song's key and tempo, IMPORT SONG, theme, settings.
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

    class HeaderBar : public juce::Component, public juce::FileDragAndDropTarget
    {
    public:
        explicit HeaderBar (GrooveGrabberProcessor&);
        void refresh();
        void setTab (int);
        std::function<void (int)> onTab;
        std::function<void()> onThemeChanged;
        std::function<void (int)> onHelp;              // 0 = the manual, 1 = About
        const juce::TextButton& getTabButton (int page) const   { return *tabs[page]; }   // (for the tests)
        juce::Rectangle<int> getLogoArea() const noexcept        { return logoArea; }
        int getControlsLeft() const noexcept                     { return keyLabel.getX(); }   // where KEY starts
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void paintOverChildren (juce::Graphics&) override;
        // a song file dropped on IMPORT SONG
        bool isInterestedInFileDrag (const juce::StringArray&) override;
        void fileDragEnter (const juce::StringArray&, int, int) override     { dropHover = true; repaint(); }
        void fileDragExit (const juce::StringArray&) override                 { dropHover = false; repaint(); }
        void filesDropped (const juce::StringArray&, int, int) override;

    private:
        void toggleTheme();
        void showImportMenu();
        void showSettingsMenu();
        GrooveGrabberProcessor& proc;
        juce::OwnedArray<juce::TextButton> tabs;                      // by page: BUILD, LIBRARY, MY KITS, SAUCE, FX
        static constexpr int tabOrder[] = { 0, 3, 4, 2, 1 };           // left to right: BUILD, SAUCE, FX, MY KITS, LIBRARY
        juce::ComboBox key;
        DragNumber bpm;
        IconButton importSong { "Import song", Icon::music }, theme { "theme", Icon::moon }, gear { "settings", Icon::gear },
                   help { "manual", Icon::help };
        std::unique_ptr<juce::FileChooser> chooser;
        juce::Rectangle<int> logoArea, keyLabel, bpmLabel;
        float titleH = 46.0f, grabberH = 20.0f;                       // GROOVE and GRABBER (font heights)
        int tab = 0;
        bool dropHover = false;
    };
}
