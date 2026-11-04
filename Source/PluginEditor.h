// Groove Grabber - the window.
#pragma once

#include "PluginProcessor.h"
#include "ui/HeaderBar.h"
#include "ui/BuildPage.h"
#include "ui/LibraryPage.h"
#include "ui/KitsPage.h"
#include "ui/ExportPanel.h"
#include "ui/HelpPanel.h"
#include "ui/SaucePage.h"

class GrooveGrabberEditor : public juce::AudioProcessorEditor,
                            private juce::ChangeListener,
                            private juce::AsyncUpdater
{
public:
    explicit GrooveGrabberEditor (GrooveGrabberProcessor&);
    ~GrooveGrabberEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void showTab (int);
    int getTab() const noexcept   { return tab; }
    void refreshAll();
    void parentHierarchyChanged() override;
    void showHelp (int page)      { help.show (page); }
    ggui::HelpPanel& getHelp()    { return help; }
    ggui::SaucePage& getSauce()   { return sauce; }
    ggui::BuildPage& getBuild()   { return build; }
    ggui::HeaderBar& getHeader()  { return header; }
    ggui::ExportPanel& getExporter()   { return exporter; }
    ggui::KitsPage& getKitsPage()      { return kitsPage; }
    float getZoom() const              { return content.getTransform().getScaleFactor(); }
    juce::Component& getContent()      { return content; }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void handleAsyncUpdate() override;
    void themeChanged();

    GrooveGrabberProcessor& proc;
    juce::SharedResourcePointer<ggui::LookAndFeel> lnf;
    // Everything on screen sits in here. A window bigger than the one it opens at zooms it (ggui::zoomFor), so the
    // words, buttons and pads grow with the window; the pages are laid out at the window's size divided by the zoom.
    juce::Component content;
    juce::TooltipWindow tooltips { &content, 600 };
    ggui::HeaderBar header;
    ggui::BuildPage build;
    ggui::LibraryPage library;
    ggui::KitsPage kitsPage;
    ggui::SaucePage sauce;
    ggui::ExportPanel exporter;
    ggui::HelpPanel help;
    int tab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGrabberEditor)
};
