// Groove Grabber - SAUCE: how the drums go in a genre, style and decade, so you can build your own
// patterns in your DAW. Opens on BUILD's genre, style and decade; pick any other to read up on it.
// The style's grooves show as a step grid you can play with your kit and drag into your DAW as MIDI.
#pragma once

#include "HelpPanel.h"
#include "core/Sauce.h"

namespace ggui
{
    //==============================================================================
    // A groove as a step grid: one lane per drum, 16 steps a bar, rolls split their step
    class PatternGrid : public juce::Component
    {
    public:
        void setGroove (const gg::Groove*);
        void setPlayhead (double beat);          // -1 = none
        void setNoteNamer (std::function<juce::String (int)> f)   { noteNamer = std::move (f); repaint(); }
        int numLanes() const noexcept            { return (int) lanes.size(); }
        void paint (juce::Graphics&) override;

    private:
        const gg::Groove* groove = nullptr;
        std::vector<gg::Slot> lanes;
        double playhead = -1;
        std::function<juce::String (int)> noteNamer;
    };

    //==============================================================================
    class SaucePage : public juce::Component, private juce::Timer
    {
    public:
        explicit SaucePage (GrooveGrabberProcessor&);
        ~SaucePage() override;
        void refresh();                               // follows BUILD unless you've picked something else since
        void togglePlay();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void visibilityChanged() override;
        void lookAndFeelChanged() override;
        std::function<void()> onUseOnBuild;           // after BUILD was set to this style

        // (for the tests)
        void choose (gg::Genre, int subStyle, int era);
        bool isFollowingBuild() const;
        const PatternGrid& getGrid() const noexcept   { return grid; }
        int getDocHeight() const noexcept             { return doc.getHeight(); }

    private:
        void timerCallback() override;
        void showChoice();
        void fillBoxes();
        void layoutDoc();
        void pickGroove (int index);
        void stopPreview();
        double previewTempo() const;                 // what the pattern plays and exports at
        const gg::Groove* currentGroove() const;
        juce::File writeMidi() const;
        juce::String dawNoteName (int note) const;

        GrooveGrabberProcessor& proc;
        gg::Genre genre = gg::Genre::boomBap;
        int subStyle = 0, era = -1, grooveIndex = 0;
        int lastBuildGenre = -99, lastBuildStyle = -99, lastBuildEra = -99;
        std::vector<const gg::Groove*> grooves;

        bool playingHere = false;
        const gg::Groove* savedGroove = nullptr;
        double savedTempo = 100;
        float savedSwing = 0;

        juce::ComboBox genreBox, styleBox, eraBox;
        gg::Genre stylesShownFor = gg::Genre::count;
        juce::TextButton matchBuild { "MATCH BUILD" }, useOnBuild { "USE ON BUILD" };
        juce::OwnedArray<juce::TextButton> grooveChips;
        IconButton play { "play", Icon::play, Icon::stop, true };
        PatternGrid grid;
        DocView doc;
        juce::Viewport docView;
        juce::Rectangle<int> leftArea, factsArea, legendArea, buildItArea, followArea, patternArea, patternHeader, dragArea, sauceArea, sauceTitle;
        bool updating = false, dragging = false;
    };
}
