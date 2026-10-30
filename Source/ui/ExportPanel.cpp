#include "ExportPanel.h"
#include "core/Settings.h"

namespace ggui
{
    ExportPanel::ExportPanel (GrooveGrabberProcessor& p) : proc (p)
    {
        name.setFont (uiFont (17.0f));
        name.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        addAndMakeVisible (name);
        for (auto* t : { &mpc, &midi, &placeholders })
        {
            t->setToggleState (true, juce::dontSendNotification);
            addAndMakeVisible (*t);
        }
        addAndMakeVisible (bits);
        addAndMakeVisible (rate);
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

    void ExportPanel::show (const gg::Kit& k)
    {
        kit = k;
        result = {};
        name.setText (k.name, false);
        name.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        name.applyColourToAllText (pal().text);
        bits.setSelected (proc.getSettings().getExportBitDepth() == 16 ? 0 : 1);
        rate.setSelected (proc.getSettings().getExportSampleRate() > 46000 ? 1 : 0);
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
        o.bitDepth = bits.getSelected() == 0 ? 16 : 24;
        o.sampleRate = rate.getSelected() == 1 ? 48000.0 : 44100.0;
        o.grooveId = proc.grooveId;
        o.bpm = proc.options.bpm > 0 ? proc.options.bpm : proc.player.getTempo();
        o.swing = proc.player.getSwing();
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
        card = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 40), juce::jmin (600, getHeight() - 40));
        auto r = card.reduced (26, 20);
        r.removeFromTop (34);
        name.setBounds (r.removeFromTop (36));
        r.removeFromTop (10);
        destRow = r.removeFromTop (30);
        change.setBounds (destRow.removeFromRight (100));
        r.removeFromTop (12);
        auto fmt = r.removeFromTop (30);
        bits.setBounds (fmt.removeFromLeft (fmt.getWidth() / 2 - 6));
        fmt.removeFromLeft (12);
        rate.setBounds (fmt);
        r.removeFromTop (10);
        for (auto* t : { &mpc, &midi, &placeholders }) { t->setBounds (r.removeFromTop (26)); }
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
        drawSectionTitle (g, r.removeFromTop (24), "Export for Groove Agent", p.orange);

        g.setColour (p.dim);
        g.setFont (uiFont (14.5f, false));
        g.drawFittedText ("To:  " + proc.getSettings().getExportDir().getFullPathName() + "/" + gg::paths::safeFileName (name.getText(), 48),
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
            g.setColour (p.dim);
            g.setFont (uiFont (15.0f, false));
            g.drawFittedText ("You get one WAV per pad, numbered in General MIDI order from C1 (01 Kick, 02 Rim, 03 Snare...), "
                              "already tuned, trimmed and level matched. Drop them all on Groove Agent's first pad and every "
                              "sound lands on the right note.", d.toNearestInt(), juce::Justification::centred, 4);
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
        g.drawText ("DRAG TO GROOVE AGENT  -  PAD 1 (C1)", in.removeFromTop (24), juce::Justification::centredLeft);
        g.setColour (p.dim);
        g.setFont (uiFont (14.5f, false));
        g.drawFittedText ("Grab here and drop on Groove Agent's first pad: all " + juce::String (result.padFiles.size())
                          + " pads fill in order. Then right-click the kit > Export Kit with Samples to keep it.\n"
                          + juce::String (result.soundsWritten) + " sounds written to " + result.folder.getFileName()
                          + (result.programFile.existsAsFile() ? ", plus an MPC program" : "") + (result.midiFile.existsAsFile() ? " and the groove MIDI" : "") + ".",
                          in, juce::Justification::topLeft, 4);
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
