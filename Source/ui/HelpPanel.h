// Groove Grabber - the manual and About, read inside the app or plug-in: contents on the left, the page on the
// right, search across it. Opens from the ? button, the gear menu or F1.
#pragma once

#include "Widgets.h"
#include "../PluginProcessor.h"

namespace ggui
{
    //==============================================================================
    // A page of simple markdown: # and ## headings, paragraphs with **bold**, - and 1. lists (one level of
    // nesting), | tables |, ``` code ```. Laid out once per width, then only painted.
    class DocView : public juce::Component
    {
    public:
        void setMarkdown (const juce::String&);
        void setLogo (const juce::Image& img)          { logo = img; laidOutWidth = -1; }
        int layoutFor (int width);                       // lays out (if the width changed), returns the height

        struct Section { juce::String title; int y = 0; };
        const std::vector<Section>& getSections() const { return sections; }

        int setSearch (const juce::String&);             // highlights the blocks that contain it; returns how many
        int getMatchY (int index) const;                 // where match `index` starts, -1 if none
        int numBlocks() const                            { return (int) blocks.size(); }
        void paint (juce::Graphics&) override;
        void lookAndFeelChanged() override               { laidOutWidth = -1; }

    private:
        struct Cell { juce::String text; juce::TextLayout layout; juce::Rectangle<float> box; };
        struct Block
        {
            enum Type { h1, h2, h3, para, bullet, number, table, code } type = para;
            juce::String text;
            int level = 0, ordinal = 0;
            std::vector<std::vector<Cell>> rows;          // a table's rows (row 0 = the header)
            std::vector<float> rowTops;
            juce::TextLayout layout;
            juce::Rectangle<float> box;
            bool match = false;
        };
        std::vector<Block> blocks;
        std::vector<Section> sections;
        std::vector<int> matches;
        juce::Image logo;
        juce::Rectangle<float> logoBox;
        int laidOutWidth = -1, laidOutHeight = 0;
    };

    //==============================================================================
    class HelpPanel : public juce::Component
    {
    public:
        explicit HelpPanel (GrooveGrabberProcessor&);
        ~HelpPanel() override;
        void show (int page);                            // 0 = manual, 1 = about
        void showPage (int page);
        int getPage() const noexcept                     { return page; }
        void scrollToSection (int index);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;
        void lookAndFeelChanged() override;

        DocView& getManual()                             { return manual; }   // (for the tests)

    private:
        class Contents;
        class Scroller;
        void layoutDoc();
        void nextMatch();
        void updateCurrentSection();

        GrooveGrabberProcessor& proc;
        DocView manual, about;
        std::unique_ptr<Scroller> viewport;
        std::unique_ptr<Contents> contents;
        Segmented tabs { { "MANUAL", "ABOUT" } };
        juce::TextEditor search;
        juce::TextButton close { "CLOSE" };
        juce::Rectangle<int> card, titleArea, matchArea;
        int page = 0, matchCount = 0, matchIndex = -1;
    };
}
