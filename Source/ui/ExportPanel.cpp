#include "ExportPanel.h"
#include "core/Settings.h"
#include "core/Daws.h"

namespace ggui
{
    namespace
    {
        // Wrapped text at the top of an area; returns the height used
        int drawWrapped (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour, juce::Rectangle<int> area)
        {
            juce::AttributedString as;
            as.append (text, font, colour);
            juce::TextLayout tl;
            tl.createLayout (as, (float) area.getWidth());
            const int h = (int) std::ceil (tl.getHeight());
            tl.draw (g, area.withHeight (h).toFloat());
            return h;
        }

        // The DAW's loading steps (and the hats tip), numbered, **bold** where marked; returns the height used
        int drawSteps (juce::Graphics& g, const gg::DawGuide& guide, juce::Rectangle<int> area, int numPads)
        {
            const auto& p = pal();
            juce::StringArray items;
            for (auto st : guide.steps)
                items.add (st.replace ("{FIRST}", gg::dawNoteName (36, guide)).replace ("{LAST}", gg::padFileName (numPads - 1, false))
                             .replace ("{PADS}", juce::String (numPads)));
            if (guide.hats.isNotEmpty()) items.add (guide.hats);
            const int top = area.getY();
            const auto normal = uiFont (14.0f, false), bold = uiFont (14.0f);
            for (int i = 0; i < items.size() && area.getHeight() > 16; ++i)
            {
                juce::AttributedString as;
                as.setLineSpacing (1.0f);
                as.append (juce::String (i + 1) + ".  ", bold, p.orange);
                bool isBold = false;
                for (const auto& piece : juce::StringArray::fromTokens (items[i], "*", ""))
                {
                    if (piece.isEmpty()) { isBold = ! isBold; continue; }     // "**" gives an empty token between the stars
                    as.append (piece, isBold ? bold : normal, isBold ? p.text : p.dim);
                }
                juce::TextLayout tl;
                tl.createLayout (as, (float) area.getWidth());
                const int h = (int) std::ceil (tl.getHeight());
                tl.draw (g, area.removeFromTop (h).toFloat());
                area.removeFromTop (5);
            }
            return area.getY() - top;
        }
    }

    ExportPanel::ExportPanel (GrooveGrabberProcessor& p) : proc (p)
    {
        name.setFont (uiFont (17.0f));
        name.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        name.onTextChange = [this] { repaint(); };
        addAndMakeVisible (name);
        for (auto* t : { &mpc, &midi, &placeholders, &genreFolder })
        {
            t->setToggleState (true, juce::dontSendNotification);
            addAndMakeVisible (*t);
        }
        genreFolder.onClick = [this] { proc.getSettings().setExportGenreFolders (genreFolder.getToggleState()); repaint(); };
        addAndMakeVisible (bits);
        addAndMakeVisible (rate);
        daw.setTooltip ("Which DAW the loading steps are for. Automatic picks the DAW Groove Grabber is running in.");
        daw.onChange = [this]
        {
            const int id = daw.getSelectedId();
            const auto& all = gg::dawGuides();
            proc.getSettings().set ("daw", id <= 1 ? juce::String() : all[(size_t) juce::jlimit (0, (int) all.size() - 1, id - 2)].id);
            if (gg::chosenDaw (proc.getSettings().get ("daw")) == "mpc") bits.setSelected (0);     // MPC hardware needs 16-bit
            repaint();
        };
        addAndMakeVisible (daw);
        go.getProperties().set ("style", "primary");
        go.onClick = [this] { doExport(); };
        addAndMakeVisible (go);
        for (auto* b : { &close, &reveal, &change })
        {
            b->getProperties().set ("style", "ghost");
            addAndMakeVisible (*b);
        }
        close.onClick = [this] { setVisible (false); };
        reveal.onClick = [this] { if (result.folder.isDirectory()) result.folder.revealToUser(); };
        change.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Where exported kits go", proc.getSettings().getExportDir());
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                  [this, safe = juce::Component::SafePointer<ExportPanel> (this)] (const juce::FileChooser& fc)
            {
                if (safe != nullptr && fc.getResult().isDirectory()) { proc.getSettings().setExportDir (fc.getResult()); repaint(); }
            });
        };
    }

    void ExportPanel::fillDaws()
    {
        daw.clear (juce::dontSendNotification);
        const auto found = gg::detectedDaw();
        daw.addItem (found.isNotEmpty() ? "Automatic: " + gg::dawGuide (found).name : juce::String ("Automatic (no DAW found: any sampler)"), 1);
        const auto& all = gg::dawGuides();
        for (int i = 0; i < (int) all.size(); ++i)
            daw.addItem (all[(size_t) i].name + (all[(size_t) i].id != "other" ? "  -  " + all[(size_t) i].sampler : juce::String()), i + 2);
        const auto setting = proc.getSettings().get ("daw");
        int selected = 1;
        for (int i = 0; i < (int) all.size(); ++i)
            if (setting.isNotEmpty() && all[(size_t) i].id == setting) selected = i + 2;
        daw.setSelectedId (selected, juce::dontSendNotification);
    }

    void ExportPanel::show (const gg::Kit& k)
    {
        fillDaws();
        kit = k;
        result = {};
        name.setText (k.name, false);
        name.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        name.applyColourToAllText (pal().text);
        bits.setSelected (proc.getSettings().getExportBitDepth() == 16 || gg::chosenDaw (proc.getSettings().get ("daw")) == "mpc" ? 0 : 1);
        rate.setSelected (proc.getSettings().getExportSampleRate() > 46000 ? 1 : 0);
        genreFolder.setToggleState (proc.getSettings().getExportGenreFolders(), juce::dontSendNotification);
        genreFolder.setButtonText ("In a genre folder (Exports > " + gg::genreFolderName (k.genre) + " > your kit)");
        reveal.setVisible (false);
        setVisible (true);
        toFront (true);
        resized();
        repaint();
    }

    void ExportPanel::doExport()
    {
        if (busy) return;
        busy = true;
        go.setEnabled (false);
        auto k = kit;
        k.name = name.getText().trim().isNotEmpty() ? name.getText().trim() : kit.name;
        proc.getSettings().setExportBitDepth (bits.getSelected() == 0 ? 16 : 24);
        proc.getSettings().setExportSampleRate (rate.getSelected() == 1 ? 48000.0 : 44100.0);
        gg::ExportOptions o;
        o.destination = proc.getSettings().getExportDir();
        o.mpcProgram = mpc.getToggleState();
        o.grooveMidi = midi.getToggleState();
        o.placeholders = placeholders.getToggleState();
        o.genreFolder = genreFolder.getToggleState();
        o.bitDepth = bits.getSelected() == 0 ? 16 : 24;
        o.sampleRate = rate.getSelected() == 1 ? 48000.0 : 44100.0;
        o.grooveId = proc.grooveId;
        o.groove = proc.getGroove (proc.grooveId);     // your edit of it, if you made one on SAUCE
        o.bpm = proc.options.bpm > 0 ? proc.options.bpm : proc.player.getTempo();
        o.swing = proc.player.getSwing();
        o.daw = gg::chosenDaw (proc.getSettings().get ("daw"));     // (resolved here: the export runs on another thread)
        proc.runInBackground ([this, k, o, safe = juce::Component::SafePointer<ExportPanel> (this)]
        {
            auto r = gg::exportKit (k, o);
            juce::MessageManager::callAsync ([safe, r]
            {
                if (safe == nullptr) return;
                safe->result = r;
                safe->busy = false;
                safe->go.setEnabled (true);
                safe->reveal.setVisible (r.ok);
                safe->resized();
                safe->repaint();
            });
        });
        repaint();
    }

    void ExportPanel::resized()
    {
        card = getLocalBounds().withSizeKeepingCentre (juce::jmin (660, getWidth() - 40), juce::jmin (660, getHeight() - 40));
        auto r = card.reduced (26, 20);
        r.removeFromTop (34);
        name.setBounds (r.removeFromTop (36));
        r.removeFromTop (10);
        destRow = r.removeFromTop (30);
        change.setBounds (destRow.removeFromRight (100));
        r.removeFromTop (8);
        dawRow = r.removeFromTop (32);
        daw.setBounds (dawRow.withTrimmedLeft (44));
        r.removeFromTop (12);
        auto fmt = r.removeFromTop (30);
        bits.setBounds (fmt.removeFromLeft (fmt.getWidth() / 2 - 6));
        fmt.removeFromLeft (12);
        rate.setBounds (fmt);
        r.removeFromTop (10);
        for (auto* t : { &mpc, &midi, &placeholders, &genreFolder }) { t->setBounds (r.removeFromTop (26)); }
        r.removeFromTop (14);
        auto buttons = r.removeFromBottom (44);
        close.setBounds (buttons.removeFromRight (100));
        buttons.removeFromRight (10);
        reveal.setBounds (buttons.removeFromRight (140));
        buttons.removeFromRight (10);
        go.setBounds (buttons.removeFromLeft (220));
        r.removeFromBottom (12);
        dragHandle = r;
    }

    void ExportPanel::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        g.fillAll (juce::Colours::black.withAlpha (p.light ? 0.35f : 0.6f));
        drawPanel (g, card.toFloat(), 16.0f, true);
        auto r = card.reduced (26, 20);
        const auto& guide = gg::dawGuide (gg::chosenDaw (proc.getSettings().get ("daw")));
        drawSectionTitle (g, r.removeFromTop (24), guide.id == "other" ? juce::String ("Export for your sampler") : "Export for " + guide.sampler, p.orange);
        g.setColour (p.dim);
        g.setFont (headFont (10.0f).withExtraKerningFactor (0.12f));
        g.drawText ("FOR", dawRow.withWidth (40), juce::Justification::centredLeft);

        g.setColour (p.dim);
        g.setFont (uiFont (14.5f, false));
        auto dest = proc.getSettings().getExportDir();
        if (genreFolder.getToggleState()) dest = dest.getChildFile (gg::genreFolderName (kit.genre));
        g.drawFittedText ("To:  " + dest.getChildFile (gg::paths::safeFileName (name.getText(), 48)).getFullPathName(),
                          destRow.withTrimmedRight (110), juce::Justification::centredLeft, 1, 0.6f);

        // the result / drag area
        auto d = dragHandle.toFloat();
        if (busy)
        {
            g.setColour (p.dim);
            g.setFont (uiFont (16.0f));
            g.drawText ("Exporting...", d, juce::Justification::centred);
            return;
        }
        if (result.folder == juce::File() && ! result.ok)
        {
            auto a = dragHandle.reduced (4, 0);
            a.removeFromTop (drawWrapped (g, "You get one WAV per pad, numbered in General MIDI order from note 36 (01 Kick, 02 Rim, 03 Snare...), "
                                             "already tuned, trimmed and level matched. Then, in " + (guide.id == "other" ? juce::String ("your DAW") : guide.name) + ":",
                                          uiFont (14.5f, false), p.dim, a));
            a.removeFromTop (10);
            drawSteps (g, guide, a, gg::numPads);
            return;
        }
        if (! result.ok)
        {
            g.setColour (p.red);
            g.setFont (uiFont (15.0f));
            g.drawFittedText (result.error, d.toNearestInt(), juce::Justification::centred, 3);
            return;
        }
        // success: the drag handle
        g.setColour (dragging ? p.orange.withAlpha (0.2f) : p.sunken);
        g.fillRoundedRectangle (d, 12.0f);
        drawGlowOutline (g, d.reduced (1.0f), 12.0f, 0.8f);
        auto in = dragHandle.reduced (18, 12);
        auto handle = in.removeFromLeft (64).toFloat();
        drawIcon (g, Icon::drag, handle.withSizeKeepingCentre (44, 44), p.orange);
        g.setColour (p.text);
        g.setFont (headFont (14.0f).withExtraKerningFactor (0.06f));
        g.drawText (guide.dragLabel.replace ("{FIRST}", gg::dawNoteName (36, guide)), in.removeFromTop (24), juce::Justification::centredLeft);
        g.setColour (p.dim);
        g.setFont (uiFont (14.5f, false));
        in.removeFromTop (drawWrapped (g, juce::String (result.soundsWritten) + " sounds written to " + result.folder.getFileName()
                                          + (result.programFile.existsAsFile() ? ", plus an MPC program" : "") + (result.midiFile.existsAsFile() ? " and the groove MIDI" : "")
                                          + ". Grab here and drop all " + juce::String (result.padFiles.size()) + " files at once:",
                                       uiFont (14.5f, false), p.dim, in));
        in.removeFromTop (10);
        drawSteps (g, guide, in, result.padFiles.size());
    }

    void ExportPanel::mouseDown (const juce::MouseEvent& e)
    {
        dragging = false;
        if (! card.contains (e.getPosition())) setVisible (false);
    }

    void ExportPanel::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragging || ! result.ok || ! dragHandle.contains (e.getMouseDownPosition()) || e.getDistanceFromDragStart() < 6) return;
        dragging = true;
        repaint();
        juce::StringArray files;
        for (auto& f : result.padFiles) files.add (f.getFullPathName());
        juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this, [safe = juce::Component::SafePointer<ExportPanel> (this)]
        {
            if (safe != nullptr) { safe->dragging = false; safe->repaint(); }
        });
    }
}
