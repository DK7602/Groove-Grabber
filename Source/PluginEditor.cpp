#include "PluginEditor.h"

GrooveGrabberEditor::GrooveGrabberEditor (GrooveGrabberProcessor& p)
    : AudioProcessorEditor (p), proc (p), header (p), build (p), library (p), kitsPage (p), exporter (p)
{
    ggui::setLight (proc.getSettings().isLightTheme());
    lnf->refreshColours();
    setLookAndFeel (&lnf.get());
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf.get());

    addAndMakeVisible (header);
    addChildComponent (build);
    addChildComponent (library);
    addChildComponent (kitsPage);
    addChildComponent (exporter);

    header.onTab = [this] (int t) { showTab (t); };
    header.onThemeChanged = [this] { themeChanged(); };
    build.onExport = [this] { if (auto* k = proc.getActiveKit()) exporter.show (*k); };
    kitsPage.onExport = [this] (const gg::Kit& k) { exporter.show (k); };
    kitsPage.onLoaded = [this] { showTab (0); };

    proc.addChangeListener (this);
    setWantsKeyboardFocus (true);

    // first time, with nothing to build from: start on the library
    const bool empty = proc.library->getSounds()->empty() && ! proc.library->isScanning();
    showTab (empty && proc.kits.empty() ? 1 : 0);

    setResizable (true, true);
    setResizeLimits (1180, 760, 2600, 1700);
    setSize (1440, 900);
    refreshAll();
}

GrooveGrabberEditor::~GrooveGrabberEditor()
{
    proc.removeChangeListener (this);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void GrooveGrabberEditor::showTab (int t)
{
    tab = juce::jlimit (0, 2, t);
    header.setTab (tab);
    build.setVisible (tab == 0);
    library.setVisible (tab == 1);
    kitsPage.setVisible (tab == 2);
    if (tab == 1) library.refresh();
    if (tab == 2) kitsPage.refresh();
}

void GrooveGrabberEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    triggerAsyncUpdate();
}

void GrooveGrabberEditor::handleAsyncUpdate()
{
    refreshAll();
}

void GrooveGrabberEditor::refreshAll()
{
    header.refresh();
    if (tab == 0) build.refresh();
    if (tab == 1) library.refresh();
    if (tab == 2) kitsPage.refresh();
}

void GrooveGrabberEditor::themeChanged()
{
    lnf->refreshColours();
    sendLookAndFeelChange();
    build.refresh();
    library.refresh();
    kitsPage.refresh();
    header.refresh();
    repaint();
}

bool GrooveGrabberEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey)
    {
        proc.player.setGroovePlaying (! proc.player.isGroovePlaying());
        proc.sendChangeMessage();
        return true;
    }
    if (k == juce::KeyPress::escapeKey && exporter.isVisible())
    {
        exporter.setVisible (false);
        return true;
    }
    return false;
}

void GrooveGrabberEditor::paint (juce::Graphics& g)
{
    const auto& p = ggui::pal();
    g.fillAll (p.bg);
    if (! p.light)
    {
        // a faint glow behind the content, like the logo's stage light
        auto r = getLocalBounds().toFloat();
        juce::ColourGradient glow (p.purple.withAlpha (0.10f), r.getCentreX(), r.getY() + 60,
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom(), true);
        g.setGradientFill (glow);
        g.fillRect (r);
    }
}

void GrooveGrabberEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (66));
    for (auto* page : std::initializer_list<juce::Component*> { &build, &library, &kitsPage })
        page->setBounds (r);
    exporter.setBounds (getLocalBounds());
}
