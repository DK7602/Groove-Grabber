// Groove Grabber - the window.
#pragma once

#include "PluginProcessor.h"
#include "ui/HeaderBar.h"
#include "ui/BuildPage.h"
#include "ui/LibraryPage.h"
#include "ui/KitsPage.h"
#include "ui/ExportPanel.h"
#include "ui/HelpPanel.h"

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

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void handleAsyncUpdate() override;
    void themeChanged();

    GrooveGrabberProcessor& proc;
    juce::SharedResourcePointer<ggui::LookAndFeel> lnf;
    juce::TooltipWindow tooltips { this, 600 };
    ggui::HeaderBar header;
    ggui::BuildPage build;
    ggui::LibraryPage library;
    ggui::KitsPage kitsPage;
    ggui::ExportPanel exporter;
    ggui::HelpPanel help;
    int tab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrooveGrabberEditor)
};
