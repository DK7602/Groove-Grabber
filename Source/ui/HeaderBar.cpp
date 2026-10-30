#include "HeaderBar.h"
#include "core/SongAnalysis.h"
#include "core/Settings.h"
#include "GGBinaryData.h"

namespace ggui
{
    //==============================================================================
    DragNumber::DragNumber()
    {
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        editor.setEditable (false, true, false);
        editor.setJustificationType (juce::Justification::centred);
        editor.onTextChange = [this]
        {
            const double v = editor.getText().getDoubleValue();
            if (v >= 40 && v <= 250 && onChange) onChange (v);
            editor.setVisible (false);
        };
        editor.onEditorHide = [this] { editor.setVisible (false); };
        addChildComponent (editor);
        setTooltip ("Song tempo: drag up / down, scroll, or double-click to type");
    }

    void DragNumber::resized()   { editor.setBounds (getLocalBounds()); }

    void DragNumber::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (p.sunken);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (isMouseOverOrDragging() ? p.blue : p.line);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        if (! editor.isVisible())
        {
            g.setColour (p.text);
            g.setFont (uiFont (21.0f));
            g.drawText (juce::String (value, std::abs (value - std::round (value)) > 0.01 ? 1 : 0), r, juce::Justification::centred);
        }
    }

    void DragNumber::mouseDown (const juce::MouseEvent&)    { dragStartValue = value; }

    void DragNumber::mouseDrag (const juce::MouseEvent& e)
    {
        const double v = juce::jlimit (40.0, 250.0, std::round (dragStartValue - e.getDistanceFromDragStartY() * 0.25));
        if (std::abs (v - value) > 0.001 && onChange) onChange (v);
    }

    void DragNumber::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
    {
        const double v = juce::jlimit (40.0, 250.0, std::round (value + (w.deltaY > 0 ? 1 : -1)));
        if (onChange) onChange (v);
    }

    void DragNumber::mouseDoubleClick (const juce::MouseEvent&)
    {
        editor.setColour (juce::Label::textColourId, pal().text);
        editor.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        editor.setColour (juce::TextEditor::textColourId, pal().text);
        editor.setFont (headFont (15.0f));
        editor.setText (juce::String (value, 0), juce::dontSendNotification);
        editor.setVisible (true);
        editor.showEditor();
    }

    //==============================================================================
    HeaderBar::HeaderBar (GrooveGrabberProcessor& p) : proc (p)
    {
        const char* names[] = { "BUILD", "LIBRARY", "MY KITS" };
        for (int i = 0; i < 3; ++i)
        {
            auto* b = tabs.add (new juce::TextButton (names[i]));
            b->getProperties().set ("style", "tab");
            b->onClick = [this, i] { setTab (i); if (onTab) onTab (i); };
            addAndMakeVisible (b);
        }
        tabs[0]->setTooltip ("Describe the song, generate and edit kits");
        tabs[1]->setTooltip ("Your sound folders, scanning, and every analysed sound");
        tabs[2]->setTooltip ("Kits you saved, and recent ones");

        key.addItem ("No key", 1);
        for (int k = 0; k < 24; ++k)
            key.addItem (gg::keyName (k < 12 ? k + 12 : k - 12), k + 2);   // minor keys first (more common in these genres)
        key.onChange = [this]
        {
            const int id = key.getSelectedId();
            if (id <= 1) { proc.setKey (-1); return; }
            const int idx = id - 2;
            proc.setKey (idx < 12 ? idx + 12 : idx - 12);
        };
        key.setTooltip ("The song's key: the kick, 808 and toms are tuned to it");
        addAndMakeVisible (key);

        bpm.onChange = [this] (double v) { proc.setBpm (v); };
        addAndMakeVisible (bpm);

        keyCompass.setLabel ("KEY COMPASS");
        keyCompass.setTooltip ("Take the key and tempo from a Key Compass song, or from a song file");
        keyCompass.onClick = [this] { showKeyCompassMenu(); };
        addAndMakeVisible (keyCompass);

        theme.setTooltip ("Light / dark");
        theme.onClick = [this]
        {
            proc.getSettings().setLightTheme (! isLight());
            setLight (! isLight());
            if (onThemeChanged) onThemeChanged();
        };
        addAndMakeVisible (theme);
        gear.setTooltip ("Settings");
        gear.onClick = [this] { showSettingsMenu(); };
        addAndMakeVisible (gear);
        setTab (0);
    }

    void HeaderBar::setTab (int t)
    {
        tab = t;
        for (int i = 0; i < tabs.size(); ++i) tabs[i]->setToggleState (i == t, juce::dontSendNotification);
    }

    void HeaderBar::refresh()
    {
        const int k = proc.options.key;
        key.setSelectedId (k < 0 ? 1 : (k >= 12 ? k - 12 + 2 : k + 12 + 2), juce::dontSendNotification);
        bpm.setValue (proc.options.bpm > 0 ? proc.options.bpm : proc.player.getTempo());
        theme.setIcon (isLight() ? Icon::moon : Icon::sun);
        keyCompass.setAccent (pal().blue);
        repaint();
    }

    void HeaderBar::resized()
    {
        auto r = getLocalBounds().reduced (14, 10);
        const bool narrow = getWidth() < 1340;
        logoArea = r.removeFromLeft (narrow ? 236 : 300);
        r.removeFromLeft (narrow ? 4 : 10);
        for (auto* t : tabs) { t->setBounds (r.removeFromLeft (t == tabs[2] ? (narrow ? 96 : 104) : (narrow ? 86 : 96)).reduced (2, 2)); }

        gear.setBounds (r.removeFromRight (38).reduced (2));
        r.removeFromRight (2);
        theme.setBounds (r.removeFromRight (38).reduced (2));
        r.removeFromRight (10);
        keyCompass.setLabel (narrow ? juce::String() : juce::String ("KEY COMPASS"));
        keyCompass.setBounds (r.removeFromRight (narrow ? 40 : 150).reduced (0, 3));
        r.removeFromRight (10);
        bpm.setBounds (r.removeFromRight (64).reduced (0, 5));
        bpmLabel = r.removeFromRight (36);
        r.removeFromRight (6);
        key.setBounds (r.removeFromRight (narrow ? 124 : 140).reduced (0, 5));
        keyLabel = r.removeFromRight (34);
    }

    void HeaderBar::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto bounds = getLocalBounds().toFloat();
        juce::ColourGradient bg (p.light ? juce::Colours::white : juce::Colour (0xff0b0e16), 0, 0,
                                 p.light ? p.bg.brighter (0.02f) : juce::Colour (0xff06070b), 0, bounds.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRect (bounds);
        // the brand line under the bar
        g.setGradientFill (brandGradient (bounds, p.light ? 0.9f : 1.0f));
        g.fillRect (bounds.removeFromBottom (2.0f));

        // logo: the claw badge + GROOVE GRABBER
        auto la = logoArea;
        auto badge = la.removeFromLeft (la.getHeight()).toFloat();
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path clip;
            clip.addRoundedRectangle (badge, 9.0f);
            g.reduceClipRegion (clip);
            g.drawImage (badgeImage(), badge, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        }
        if (! p.light)
        {
            g.setColour (p.orange.withAlpha (0.35f));
            g.drawRoundedRectangle (badge.expanded (1.0f), 10.0f, 1.0f);
        }
        la.removeFromLeft (12);
        auto groove = la.removeFromTop (la.getHeight() * 62 / 100).toFloat();
        auto grabber = la.toFloat();
        {
            const auto font = titleFont (28.0f).withExtraKerningFactor (0.04f);
            juce::GlyphArrangement ga;
            ga.addLineOfText (font, "GROOVE", groove.getX(), groove.getBottom() - 4);
            juce::Path text;
            ga.createPath (text);
            const auto tb = text.getBounds();
            juce::ColourGradient chrome (p.light ? juce::Colour (0xff3a4052) : juce::Colour (0xffffffff), 0, tb.getY(),
                                         p.light ? juce::Colour (0xff10131c) : juce::Colour (0xff8e9ab3), 0, tb.getBottom(), false);
            chrome.addColour (0.5, p.light ? juce::Colour (0xff6d7590) : juce::Colour (0xffdfe6f3));
            chrome.addColour (0.56, p.light ? juce::Colour (0xff1a1e2a) : juce::Colour (0xff6c7790));
            if (! p.light)
            {
                g.setColour (p.blue.withAlpha (0.25f));
                g.strokePath (text, juce::PathStrokeType (2.5f));
            }
            g.setGradientFill (chrome);
            g.fillPath (text);
        }
        g.setColour (p.orange);
        g.setFont (headFont (13.0f).withExtraKerningFactor (0.62f));
        g.drawText ("GRABBER", grabber.withTrimmedLeft (2), juce::Justification::topLeft, false);

        g.setColour (p.dim);
        g.setFont (headFont (10.0f).withExtraKerningFactor (0.12f));
        g.drawText ("KEY", keyLabel, juce::Justification::centredRight);
        g.drawText ("BPM", bpmLabel, juce::Justification::centredRight);
    }

    //==============================================================================
    void HeaderBar::showKeyCompassMenu()
    {
        const auto folder = proc.getSettings().getKeyCompassDir();
        const auto songs = gg::listKeyCompassSongs (folder, 12);
        juce::PopupMenu m;
        m.addSectionHeader ("KEY COMPASS SONGS");
        if (songs.empty())
            m.addItem (-1, folder.isDirectory() ? "No songs in Key Compass's My Songs yet" : "Key Compass's My Songs folder wasn't found", false);
        for (size_t i = 0; i < songs.size(); ++i)
        {
            const auto& s = songs[i];
            juce::String text = s.name + "    " + (s.key >= 0 ? gg::keyName (s.key) : juce::String ("no key"));
            if (s.bpm > 0) text << "  -  " << juce::String (s.bpm, 0) << " BPM";
            m.addItem ((int) i + 1, text);
        }
        m.addSeparator();
        m.addItem (100, "Detect key and tempo from a song file...");
        m.addItem (101, "Choose the Key Compass songs folder...");

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&keyCompass), [this, songs, safe = juce::Component::SafePointer<HeaderBar> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r > 0 && r <= (int) songs.size())
            {
                const auto& s = songs[(size_t) r - 1];
                proc.songName = s.name;
                if (s.bpm > 0) proc.setBpm (s.bpm);
                proc.setKey (s.key);
                return;
            }
            if (r == 100)
            {
                chooser = std::make_unique<juce::FileChooser> ("Pick your song (a bounce or rough mix)", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
                chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, safe] (const juce::FileChooser& fc)
                {
                    if (safe == nullptr || fc.getResult() == juce::File()) return;
                    const auto f = fc.getResult();
                    proc.runInBackground ([this, f, a = safe]
                    {
                        const auto facts = gg::analyseSong (f);
                        juce::MessageManager::callAsync ([this, facts, a, f]
                        {
                            if (a == nullptr) return;
                            if (! facts.ok) return;
                            proc.songName = f.getFileNameWithoutExtension();
                            if (facts.bpm > 0) proc.setBpm (facts.bpm);
                            if (facts.key >= 0) proc.setKey (facts.key);
                        });
                    });
                });
            }
            if (r == 101)
            {
                chooser = std::make_unique<juce::FileChooser> ("Key Compass's songs folder", proc.getSettings().getKeyCompassDir());
                chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this, safe] (const juce::FileChooser& fc)
                {
                    if (safe != nullptr && fc.getResult().isDirectory()) proc.getSettings().setKeyCompassDir (fc.getResult());
                });
            }
        });
    }

    void HeaderBar::showSettingsMenu()
    {
        auto& s = proc.getSettings();
        juce::PopupMenu m;
        m.addSectionHeader ("EXPORT");
        m.addItem (1, "Export folder: " + s.getExportDir().getFullPathName());
        m.addItem (2, "24-bit WAV", true, s.getExportBitDepth() == 24);
        m.addItem (3, "16-bit WAV (MPC and older samplers)", true, s.getExportBitDepth() == 16);
        m.addItem (4, "44.1 kHz", true, s.getExportSampleRate() < 46000);
        m.addItem (5, "48 kHz", true, s.getExportSampleRate() > 46000);
        m.addSeparator();
        m.addItem (6, "Open the Groove Grabber folder");
        m.addItem (7, "About Groove Grabber " GG_VERSION);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gear), [this, safe = juce::Component::SafePointer<HeaderBar> (this)] (int r)
        {
            if (safe == nullptr) return;
            auto& st = proc.getSettings();
            switch (r)
            {
                case 1:
                    chooser = std::make_unique<juce::FileChooser> ("Where exported kits go", st.getExportDir());
                    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this, safe] (const juce::FileChooser& fc)
                    {
                        if (safe != nullptr && fc.getResult().isDirectory()) proc.getSettings().setExportDir (fc.getResult());
                    });
                    break;
                case 2: st.setExportBitDepth (24); break;
                case 3: st.setExportBitDepth (16); break;
                case 4: st.setExportSampleRate (44100.0); break;
                case 5: st.setExportSampleRate (48000.0); break;
                case 6: gg::paths::documentsDir().revealToUser(); break;
                case 7:
                {
                    auto notices = juce::String::fromUTF8 (GGBinary::THIRD_PARTY_NOTICES_txt, GGBinary::THIRD_PARTY_NOTICES_txtSize);
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "Groove Grabber " GG_VERSION,
                        "Your sounds. Your style. Your groove.\n\nBuilds drum kits from your own library for the song you're making.\n\n" + notices);
                    break;
                }
                default: break;
            }
        });
    }
}
