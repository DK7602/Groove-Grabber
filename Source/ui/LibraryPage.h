// Groove Grabber - LIBRARY: the folders you point it at, the scan, harvesting hits from a drum bounce,
// and every analysed sound (search, filter, listen, correct its type, favourite or ban it).
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    class LibraryPage : public juce::Component,
                        public juce::FileDragAndDropTarget,
                        private juce::TableListBoxModel,
                        private juce::Timer
    {
    public:
        explicit LibraryPage (GrooveGrabberProcessor&);
        ~LibraryPage() override;
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;
        bool isInterestedInFileDrag (const juce::StringArray&) override;
        void fileDragEnter (const juce::StringArray&, int, int) override   { dropHover = true; repaint(); }
        void fileDragExit (const juce::StringArray&) override              { dropHover = false; repaint(); }
        void filesDropped (const juce::StringArray&, int x, int y) override;

    private:
        // table
        int getNumRows() override;
        void paintRowBackground (juce::Graphics&, int row, int w, int h, bool selected) override;
        void paintCell (juce::Graphics&, int row, int col, int w, int h, bool selected) override;
        void cellClicked (int row, int col, const juce::MouseEvent&) override;
        void sortOrderChanged (int columnId, bool forwards) override;
        void selectedRowsChanged (int row) override;

        void timerCallback() override;
        void applyFilter();
        void rebuildSources();
        void harvest (const juce::File&);
        void addFolder (gg::Source kind);
        void showSoundMenu (int row);

        GrooveGrabberProcessor& proc;
        juce::TableListBox table { "sounds", this };
        juce::TextEditor search;
        juce::OwnedArray<juce::TextButton> catChips;
        std::vector<int> chipCats;
        juce::TextButton addPacks { "+ ADD DRUM FOLDER" }, addRecordings { "+ MY RECORDINGS" }, rescan { "RESCAN" };
        juce::OwnedArray<juce::Component> sourceRows;
        juce::Viewport sourcesView;          // the folder list scrolls when there are many
        juce::Component sourcesContent;
        double progress = 0;
        juce::ProgressBar progressBar { progress };
        std::unique_ptr<juce::FileChooser> chooser;

        gg::SoundListPtr all;
        std::vector<int> shown;
        int catFilter = -1;          // -1 = all
        int sortColumn = 2;
        bool sortForwards = true, dropHover = false;
        int lastVersion = -1;
        juce::String harvestStatus;
        juce::Rectangle<int> sourcesArea, harvestArea, statsArea;
    };
}
