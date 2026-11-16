// Groove Grabber - FX: your effects by kind (risers, impacts, tape stops, scratches...), with where each fits (the
// genres and decades that use it, against BUILD's), its length in bars at the song's tempo, a tip on where it goes,
// HEAR IT IN TIME (the effect over BUILD's groove, landing where it belongs), and DRAG to put it on an audio track.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    class FxPage : public juce::Component,
                   private juce::TableListBoxModel,
                   private juce::ChangeListener,
                   private juce::Timer
    {
    public:
        explicit FxPage (GrooveGrabberProcessor&);
        ~FxPage() override;
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void visibilityChanged() override;
        void playSelected();                         // (Space)

        // (for the tests)
        void showType (int type);                    // -1 = every kind
        int getNumShown() const noexcept             { return (int) rows.size(); }
        void selectRow (int row);
        juce::String getSelectedPath() const;
        gg::fx::Type getSelectedType() const;
        int countOf (int type) const;                // -1 = all
        void hearInTime();
        void setFitsFirst (bool);
        juce::String fitTextOf (int row) const;

    private:
        struct Row { int index; gg::fx::Catalog::Item item; gg::fx::Fit fit; };
        class TypeList;

        int getNumRows() override;
        void paintRowBackground (juce::Graphics&, int row, int w, int h, bool selected) override;
        void paintCell (juce::Graphics&, int row, int col, int w, int h, bool selected) override;
        void cellClicked (int row, int col, const juce::MouseEvent&) override;
        void selectedRowsChanged (int row) override;
        void sortOrderChanged (int columnId, bool forwards) override;
        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        void timerCallback() override;

        void applyFilter();
        void showSelected();
        void showTypeMenu (int row);
        const gg::SoundInfo* soundAt (int row) const;
        juce::String inTimeWords (gg::fx::Type) const;

        GrooveGrabberProcessor& proc;
        juce::TableListBox table { "fx", this };
        juce::TextEditor search;
        juce::TextButton fitsFirst { "FITS MY SONG FIRST" }, inTime { "HEAR IT IN TIME" };
        IconButton play { "play", Icon::play };
        WaveView wave;
        std::unique_ptr<TypeList> types;

        gg::SoundListPtr all;
        std::vector<Row> rows;
        std::vector<int> counts;                     // per type, every FX (not just the shown)
        int typeFilter = -1, sortColumn = 1, lastVersion = -1;
        bool sortForwards = true, sortByFit = false, dragging = false, catalogChanged = false;
        juce::String shownPath;                      // the one in the detail panel
        int loadGeneration = 0;
        double inTimeUntil = 0;                      // while HEAR IT IN TIME plays (ms counter)
        juce::Rectangle<int> leftArea, midArea, rightArea, dragArea, detailText, statusArea;
    };
}
