// Groove Grabber - SAUCE: how the drums go in a genre, style and decade, so you can build your own
// patterns in your DAW. Opens on BUILD's genre, style and decade; pick any other to read up on it.
// The style's grooves show as a step grid you can play with your kit and drag into your DAW as MIDI.
#pragma once

#include "HelpPanel.h"
#include "core/Sauce.h"
#include <map>

namespace ggui
{
    //==============================================================================
    // A groove as a step grid you can edit: one lane per drum, 16 steps a bar, rolls split their step.
    // Click a step to add or take away a hit (drag along the lane to do several), right-click for hard /
    // medium / soft, Ctrl-click (Alt on a Mac) for a roll.
    class PatternGrid : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        PatternGrid();
        void setGroove (std::shared_ptr<const gg::Groove>, std::vector<gg::Slot> lanes);
        void setPlayhead (double beat);          // -1 = none
        void setNoteNamer (std::function<juce::String (int)> f)   { noteNamer = std::move (f); repaint(); }
        int numLanes() const noexcept            { return (int) lanes.size(); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;

        std::function<void()> onEditStart;                             // before the first change of a click or drag
        std::function<void (std::vector<gg::GrooveHit>)> onEdit;       // the groove's new hits
        std::function<void (gg::Slot, float velocity)> onAudition;     // play one sound (a drum's name tapped, a hit added)
        void auditionLane (int lane);                                  // tap a drum's name: hear it

        // what a click on a lane's step does (the mouse, and the tests)
        void clickStep (int lane, int step, const juce::ModifierKeys&);

    private:
        struct Geometry { juce::Rectangle<float> header, area; float labelW = 0, gridX = 0, stepW = 0, laneH = 0; int steps = 0; };
        Geometry geometry() const;
        bool cellAt (juce::Point<float>, int& lane, int& step) const;
        std::shared_ptr<const gg::Groove> groove;
        std::vector<gg::Slot> lanes;
        double playhead = -1;
        std::function<juce::String (int)> noteNamer;
        bool painting = false, paintAdd = true;
        int paintLane = -1, lastStep = -1, hoverLane = -1, hoverStep = -1, hoverLabel = -1, pressedLabel = -1;
    };

    //==============================================================================
    class SaucePage : public juce::Component, public juce::TooltipClient, private juce::Timer
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
        juce::String getTooltip() override;
        std::function<void()> onUseOnBuild;           // after BUILD was set to this style

        // (for the tests)
        void choose (gg::Genre, int subStyle, int era);
        bool isFollowingBuild() const;
        PatternGrid& getGrid() noexcept               { return grid; }
        juce::String getGrooveId() const              { return currentId(); }
        void pickMine (const juce::String& id);       // show one of my grooves
        void openGroove (const juce::String& id);     // show this groove (BUILD's EDIT IN SAUCE): mine, the style's, or any other
        void startNaming (bool rename = false);       // SAVE AS MY GROOVE / SAVE AS COPY (or RENAME): the name box
        // save (a new groove: the original pattern goes back to how it was), and with replaceOnBuild BUILD plays
        // yours in its place
        void finishNaming (bool save, const juce::String& name = {}, bool replaceOnBuild = false);
        int getDocHeight() const noexcept             { return doc.getHeight(); }

    private:
        void timerCallback() override;
        void showChoice();
        void fillBoxes();
        void layoutDoc();
        void pickGroove (int index);
        void showGroove();                           // the grid, the EDITED tag and the buttons, for the groove shown
        void applyHits (std::vector<gg::GrooveHit>);
        void pushUndo();
        void showAddDrum();
        void showMyGroovesMenu();
        bool showingMine() const;
        juce::String currentId() const;
        std::vector<gg::Slot> lanesFor (const juce::String& id) const;
        juce::String dropHint() const;               // where to drop the MIDI in your DAW
        void stopPreview();
        double previewTempo() const;                 // what the pattern plays and exports at
        std::shared_ptr<const gg::Groove> currentGroove() const;
        juce::File writeMidi() const;
        juce::String dawNoteName (int note) const;

        GrooveGrabberProcessor& proc;
        gg::Genre genre = gg::Genre::boomBap;
        int subStyle = 0, era = -1, grooveIndex = 0;
        int lastBuildGenre = -99, lastBuildStyle = -99, lastBuildEra = -99;
        juce::StringArray grooveIds;
        juce::String myShown;                         // one of my grooves, when it's the one shown
        std::map<juce::String, std::vector<std::vector<gg::GrooveHit>>> undoSteps;   // this session's, per groove
        std::map<juce::String, std::vector<gg::Slot>> addedLanes;                     // drums added with + ADD DRUM

        bool playingHere = false;
        double savedTempo = 100;
        float savedSwing = 0;

        juce::ComboBox genreBox, styleBox, eraBox;
        gg::Genre stylesShownFor = gg::Genre::count;
        juce::TextButton matchBuild { "MATCH BUILD" }, useOnBuild { "USE ON BUILD" };
        juce::OwnedArray<juce::TextButton> grooveChips;
        IconButton play { "play", Icon::play, Icon::stop, true };
        juce::TextButton addDrum { "+ ADD DRUM" }, undo { "UNDO" }, reset { "RESET" }, saveMine { "SAVE AS MY GROOVE" },
                         myChip { "MY GROOVES" }, nameOk { "SAVE" }, nameReplace { "SAVE & REPLACE ON BUILD" },
                         nameCancel { "CANCEL" };
        juce::TextButton saveChanges { "SAVE" }, renameMine { "RENAME" };
        juce::OwnedArray<juce::TextButton> kitButtons;   // 01..04: which of BUILD's kits plays the pattern
        juce::Rectangle<int> kitArea;
        void updateKitButtons();
        OneShotTimer disarmDelete;
        juce::TextEditor nameEdit;
        bool naming = false, renaming = false, deleteArmed = false;
        PatternGrid grid;
        DocView doc;
        juce::Viewport docView;
        juce::Rectangle<int> leftArea, factsArea, legendArea, buildItArea, followArea, patternArea, patternHeader, editBar, dragArea, sauceArea, sauceTitle;
        bool updating = false, dragging = false;
    };
}
