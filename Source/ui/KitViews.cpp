#include "KitViews.h"
#include "core/Settings.h"
#include "core/SampleRender.h"

namespace ggui
{
    namespace
    {
        // Writes one slot (as heard) to a temporary WAV, for dragging into a DAW
        juce::File renderSlotToTempFile (const gg::Kit& kit, gg::Slot slot)
        {
            auto audio = gg::renderSlotPreview (kit, slot, 44100.0);
            if (audio == nullptr) return {};
            auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Groove Grabber Drag");
            dir.createDirectory();
            auto f = dir.getChildFile (gg::paths::safeFileName (kit.name + " " + gg::slotName (slot), 60) + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            if (auto os = f.createOutputStream())
            {
                std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 44100.0, 2, 24, {}, 0));
                if (w != nullptr)
                {
                    os.release();
                    w->writeFromAudioSampleBuffer (*audio, 0, audio->getNumSamples());
                    return f;
                }
            }
            return {};
        }

        juce::String shortName (const juce::String& path)
        {
            return juce::File (path).getFileNameWithoutExtension();
        }
    }

    //==============================================================================
    KitCard::KitCard (GrooveGrabberProcessor& p, int i) : proc (p), index (i)
    {
        play.setTooltip ("Hear this kit play the groove (click again to stop)");
        save.setTooltip ("Save this kit to MY KITS");
        regen.setTooltip ("New sounds for this kit's unlocked pads");
        play.setAccent (pal().orange);
        save.setAccent (pal().red);
        addAndMakeVisible (play);
        addAndMakeVisible (save);
        addAndMakeVisible (regen);
        play.onClick = [this]
        {
            const bool playingThis = proc.player.isGroovePlaying() && proc.activeKit == index;
            proc.setActiveKit (index);
            proc.player.setGroovePlaying (! playingThis);
            proc.sendChangeMessage();
        };
        save.onClick = [this]
        {
            if (! juce::isPositiveAndBelow (index, (int) proc.kits.size())) return;
            auto& k = proc.kits[(size_t) index];
            k.favorite = true;
            gg::kitstore::save (k);
            proc.sendChangeMessage();
        };
        regen.onClick = [this] { proc.regenerateKit (index, false); };
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void KitCard::refresh()
    {
        const bool has = juce::isPositiveAndBelow (index, (int) proc.kits.size());
        play.setEnabled (has);
        save.setEnabled (has);
        regen.setEnabled (has);
        play.setToggleState (has && proc.player.isGroovePlaying() && proc.activeKit == index, juce::dontSendNotification);
        save.setToggleState (has && proc.kits[(size_t) index].favorite, juce::dontSendNotification);
        play.setAccent (pal().orange);
        save.setAccent (pal().red);
        repaint();
    }

    void KitCard::resized()
    {
        auto r = getLocalBounds().reduced (12, 10);
        auto top = r.removeFromTop (24);
        save.setBounds (top.removeFromRight (26));
        top.removeFromRight (2);
        regen.setBounds (top.removeFromRight (26));
        play.setBounds (getLocalBounds().reduced (12, 10).removeFromBottom (38).removeFromRight (38));
    }

    void KitCard::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        const bool has = juce::isPositiveAndBelow (index, (int) proc.kits.size());
        const bool active = has && proc.activeKit == index;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        drawPanel (g, r, 12.0f, false);
        if (active) drawGlowOutline (g, r, 12.0f, 1.0f);
        else if (isMouseOver (true))
        {
            g.setColour (p.blue.withAlpha (0.5f));
            g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.2f);
        }

        auto in = getLocalBounds().reduced (14, 10);
        auto top = in.removeFromTop (24);
        g.setFont (titleFont (19.0f));
        g.setColour (active ? p.orange : p.faint);
        g.drawText (juce::String (index + 1).paddedLeft ('0', 2), top.removeFromLeft (34), juce::Justification::centredLeft);

        if (! has)
        {
            g.setColour (p.dim);
            g.setFont (uiFont (15.0f, false));
            g.drawFittedText ("Describe your song and press GENERATE KITS", in, juce::Justification::centred, 2);
            return;
        }
        const auto& k = proc.kits[(size_t) index];
        in.removeFromTop (2);
        g.setColour (p.text);
        g.setFont (headFont (15.0f).withExtraKerningFactor (0.05f));
        g.drawFittedText (k.name.toUpperCase(), in.removeFromTop (20), juce::Justification::centredLeft, 1, 0.8f);
        g.setColour (p.dim);
        g.setFont (uiFont (14.0f, false));
        g.drawFittedText (k.blurb, in.removeFromTop (18), juce::Justification::centredLeft, 1, 0.8f);

        // the kit at a glance: one bar per pad, coloured by kind, height by how hard it hits
        auto strip = in.removeFromBottom (38).withTrimmedRight (46).reduced (0, 4).toFloat();
        const int shown = gg::numSlots;
        const float w = strip.getWidth() / (float) shown;
        auto pk = proc.getPlayable (index);
        for (int i = 0; i < shown; ++i)
        {
            const auto slot = (gg::Slot) i;
            auto cell = juce::Rectangle<float> (strip.getX() + (float) i * w, strip.getY(), w - 2.0f, strip.getHeight());
            if (k[slot].isEmpty())
            {
                g.setColour (p.line.withAlpha (0.6f));
                g.fillRoundedRectangle (cell.withTop (cell.getBottom() - 3), 1.5f);
                continue;
            }
            float h = 0.55f;
            if (pk != nullptr)
                if (auto& b = pk->pads[(size_t) (gg::mainNoteForSlot (slot) - gg::firstPadNote)]; b != nullptr && b->getNumSamples() > 0)
                    h = juce::jlimit (0.25f, 1.0f, std::sqrt ((float) b->getNumSamples() / (44100.0f * 1.2f)));
            auto bar = cell.withTop (cell.getBottom() - cell.getHeight() * h);
            g.setGradientFill (juce::ColourGradient (slotColour (slot), bar.getX(), bar.getY(), slotColour (slot).withAlpha (0.35f), bar.getX(), bar.getBottom(), false));
            g.fillRoundedRectangle (bar, 1.5f);
        }
        if (pk == nullptr)
        {
            g.setColour (p.dim);
            g.setFont (uiFont (12.0f, false));
            g.drawText ("loading...", strip, juce::Justification::centred);
        }
    }

    void KitCard::mouseUp (const juce::MouseEvent& e)
    {
        if (e.mouseWasClicked() && juce::isPositiveAndBelow (index, (int) proc.kits.size()))
            proc.setActiveKit (index);
    }

    //==============================================================================
    PadTile::PadTile (GrooveGrabberProcessor& p, gg::Slot s) : proc (p), slot (s)
    {
        wave.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (wave);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip (gg::slotName (slot) + " - click to hear it, drag it into your DAW, drop a sound file on it to replace it. "
                    "MIDI note " + juce::String (gg::mainNoteForSlot (slot)) + " (" + gg::noteName (gg::mainNoteForSlot (slot)) + ").");
    }

    juce::Rectangle<int> PadTile::lockArea() const
    {
        return getLocalBounds().reduced (8, 6).removeFromTop (18).removeFromRight (18);
    }

    void PadTile::refresh()
    {
        wave.setColour (slotColour (slot));
        const auto pk = proc.getPlayable (proc.activeKit);
        const int padIndex = gg::mainNoteForSlot (slot) - gg::firstPadNote;
        wave.setAudio (pk != nullptr ? pk->pads[(size_t) padIndex] : nullptr);
        repaint();
    }

    bool PadTile::decayFlash()
    {
        if (flashLevel <= 0) return false;
        flashLevel = juce::jmax (0.0f, flashLevel - 0.12f);
        repaint();
        return true;
    }

    void PadTile::resized()
    {
        auto r = getLocalBounds().reduced (8, 6);
        r.removeFromTop (20);
        r.removeFromBottom (32);
        wave.setBounds (r);
    }

    void PadTile::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        const auto* kit = proc.getActiveKit();
        const auto& st = kit != nullptr ? (*kit)[slot] : gg::SlotState {};
        const bool selected = proc.selectedSlot == slot;
        const auto colour = slotColour (slot);
        auto r = getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (p.raised.withAlpha (p.light ? 1.0f : 0.92f));
        g.fillRoundedRectangle (r, 9.0f);
        if (flashLevel > 0)
        {
            g.setColour (colour.withAlpha (0.28f * flashLevel));
            g.fillRoundedRectangle (r, 9.0f);
        }
        g.setColour (selected ? colour : (isMouseOver (true) ? colour.withAlpha (0.6f) : p.line));
        g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, selected ? 2.0f : 1.0f);
        if (selected && ! p.light)
        {
            g.setColour (colour.withAlpha (0.18f));
            g.drawRoundedRectangle (r.expanded (1.5f), 10.5f, 3.0f);
        }
        if (dropHover)
        {
            g.setColour (p.blue.withAlpha (0.2f));
            g.fillRoundedRectangle (r, 9.0f);
        }

        auto in = getLocalBounds().reduced (9, 6);
        auto top = in.removeFromTop (18);
        g.setColour (colour);
        g.setFont (headFont (10.0f).withExtraKerningFactor (0.1f));
        g.drawFittedText (gg::slotName (slot).toUpperCase(), top.withTrimmedRight (20), juce::Justification::centredLeft, 1, 0.6f);
        if (! st.isEmpty())
            drawIcon (g, st.locked ? Icon::lock : Icon::unlock, lockArea().toFloat().reduced (2.0f), st.locked ? p.orange : p.faint);

        auto bottom = in.removeFromBottom (30);
        if (st.isEmpty())
        {
            const bool optionalOff = gg::slotIsOptional (slot);
            g.setColour (p.faint);
            g.setFont (uiFont (13.0f, false));
            g.drawFittedText (optionalOff ? "Off - drop a sound" : "Empty - drop a sound", wave.getBounds(), juce::Justification::centred, 2);
            return;
        }
        g.setColour (p.text);
        g.setFont (uiFont (13.5f, false));
        g.drawFittedText (shortName (st.path), bottom.removeFromTop (16), juce::Justification::centredLeft, 1, 0.7f);
        juce::String tag;
        const float tune = st.tune (kit->tuneToKey);
        if (std::abs (st.autoTune) > 0.01f && kit->tuneToKey)
            tag << (kit->key >= 0 ? "KEY " : "TO 808 ") << (st.autoTune > 0 ? "+" : "") << juce::String (st.autoTune, 1);
        if (std::abs (st.userTune) > 0.01f) tag << (tag.isEmpty() ? "" : "  ") << (tune > 0 ? "+" : "") << juce::String (tune, 1) << " st";
        if (st.reverse) tag << (tag.isEmpty() ? "" : "  ") << "REV";
        if (tag.isNotEmpty())
        {
            g.setColour (p.purple.brighter (p.light ? 0.0f : 0.3f));
            g.setFont (uiFont (12.0f));
            g.drawText (tag, bottom, juce::Justification::centredLeft, true);
        }
    }

    void PadTile::mouseDown (const juce::MouseEvent& e)
    {
        dragStarted = false;
        auto* kit = proc.getActiveKit();
        if (kit == nullptr) return;
        if (e.mods.isPopupMenu()) { proc.selectedSlot = slot; if (onSelect) onSelect(); showMenu(); return; }
        if (lockArea().contains (e.getPosition()) && ! (*kit)[slot].isEmpty())
        {
            (*kit)[slot].locked = ! (*kit)[slot].locked;
            proc.sendChangeMessage();
            return;
        }
        proc.auditionSlot (slot);
        if (onSelect) onSelect();
    }

    void PadTile::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragStarted || e.getDistanceFromDragStart() < 8) return;
        auto* kit = proc.getActiveKit();
        if (kit == nullptr || (*kit)[slot].isEmpty()) return;
        dragStarted = true;
        auto f = renderSlotToTempFile (*kit, slot);
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

    bool PadTile::isInterestedInFileDrag (const juce::StringArray& files)
    {
        return files.size() == 1 && gg::Library::isAudioFile (juce::File (files[0]));
    }

    void PadTile::filesDropped (const juce::StringArray& files, int, int)
    {
        dropHover = false;
        if (files.isEmpty()) return;
        proc.selectedSlot = slot;
        proc.setSlotSound (proc.activeKit, slot, files[0]);
        if (onSelect) onSelect();
    }

    void PadTile::showMenu()
    {
        auto* kit = proc.getActiveKit();
        if (kit == nullptr) return;
        const auto& st = (*kit)[slot];
        juce::PopupMenu m;
        m.addSectionHeader (gg::slotName (slot) + (st.isEmpty() ? juce::String() : " - " + shortName (st.path)));
        m.addItem (1, st.locked ? "Unlock" : "Lock (keep it when regenerating)", ! st.isEmpty());
        m.addItem (2, "Show the file", ! st.isEmpty());
        m.addSeparator();
        m.addItem (3, "Favourite this sound (picked more often)", ! st.isEmpty());
        m.addItem (4, "Never use this sound again", ! st.isEmpty());
        m.addSeparator();
        m.addItem (5, "Reset tune / start / length / gain", ! st.isEmpty());
        m.addItem (6, "Clear this pad", ! st.isEmpty());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, safe = juce::Component::SafePointer<PadTile> (this)] (int r)
        {
            if (safe == nullptr) return;
            auto* k = proc.getActiveKit();
            if (k == nullptr) return;
            auto& s = (*k)[slot];
            switch (r)
            {
                case 1: s.locked = ! s.locked; proc.sendChangeMessage(); break;
                case 2: juce::File (s.path).revealToUser(); break;
                case 3: proc.library->setRating (s.path, 1); break;
                case 4: proc.library->setRating (s.path, -1); proc.setSlotSound (proc.activeKit, slot, {}); break;
                case 5: s.userTune = 0; s.startMs = 0; s.lengthPct = 100; s.gainDb = 0; s.reverse = false; proc.kitChanged (proc.activeKit); break;
                case 6: proc.setSlotSound (proc.activeKit, slot, {}); break;
                default: break;
            }
        });
    }

    //==============================================================================
    PadGrid::PadGrid (GrooveGrabberProcessor& p) : proc (p)
    {
        for (int i = 0; i < gg::numSlots; ++i)
        {
            auto* t = tiles.add (new PadTile (proc, (gg::Slot) i));
            t->onSelect = [this] { refresh(); if (onSelect) onSelect(); };
            addAndMakeVisible (t);
        }
        startTimerHz (30);
    }

    void PadGrid::refresh()
    {
        for (auto* t : tiles) t->refresh();
    }

    void PadGrid::resized()
    {
        // two rows: the kit, then toms / percussion / FX
        auto r = getLocalBounds();
        const int rowH = r.getHeight() / 2;
        auto row1 = r.removeFromTop (rowH);
        auto row2 = r;
        const int perRow = 9;
        const int w = row1.getWidth() / perRow;
        for (int i = 0; i < tiles.size(); ++i)
        {
            auto& row = i < perRow ? row1 : row2;
            tiles[i]->setBounds (row.removeFromLeft (w).reduced (3));
        }
    }

    void PadGrid::timerCallback()
    {
        for (int pad = 0; pad < gg::numPads; ++pad)
        {
            const auto f = proc.player.getPadFlash (pad);
            if (f != lastFlash[(size_t) pad])
            {
                lastFlash[(size_t) pad] = f;
                const auto slot = gg::padLayout()[(size_t) pad].slot;
                tiles[(int) slot]->flash();
            }
        }
        for (auto* t : tiles) t->decayFlash();
    }

    //==============================================================================
    AltTile::AltTile (GrooveGrabberProcessor& p) : proc (p)
    {
        wave.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (wave);
        use.getProperties().set ("style", "accent");
        use.setTooltip ("Put this sound on the pad");
        use.onClick = [this] { if (onUse && path.isNotEmpty()) onUse (path); };
        addChildComponent (use);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void AltTile::setSound (const juce::String& newPath, gg::Cat c)
    {
        path = newPath;
        cat = c;
        wave.setColour (catColour (c));
        wave.setAudio (nullptr);
        use.setVisible (path.isNotEmpty());
        setTooltip (path.isNotEmpty() ? juce::File (path).getFileName() + "\nfrom " + juce::File (path).getParentDirectory().getFileName() + "\nClick to hear it" : juce::String());
        if (path.isEmpty()) { repaint(); return; }
        const double sr = 44100.0;
        proc.runInBackground ([safe = juce::Component::SafePointer<AltTile> (this), newPath, c, sr]
        {
            auto buf = std::make_shared<juce::AudioBuffer<float>> (gg::render::sound (newPath, c, {}, sr));
            juce::MessageManager::callAsync ([safe, buf, newPath]
            {
                if (safe != nullptr && safe->path == newPath) safe->wave.setAudio (buf);
            });
        });
        repaint();
    }

    void AltTile::resized()
    {
        auto r = getLocalBounds().reduced (7, 5);
        use.setBounds (r.removeFromBottom (20).removeFromRight (40));
        r.removeFromTop (16);
        wave.setBounds (r.reduced (0, 2));
    }

    void AltTile::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (p.sunken);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (isMouseOver (true) ? catColour (cat) : p.line);
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
        auto in = getLocalBounds().reduced (7, 5);
        g.setColour (p.dim);
        g.setFont (headFont (9.5f));
        g.drawText (juce::String (number), in.removeFromTop (14), juce::Justification::centredLeft);
        if (path.isEmpty())
        {
            g.setColour (p.faint);
            g.setFont (uiFont (12.0f, false));
            g.drawText ("-", getLocalBounds(), juce::Justification::centred);
            return;
        }
        g.setColour (p.text);
        g.setFont (uiFont (12.5f, false));
        g.drawFittedText (shortName (path), in.removeFromBottom (20).withTrimmedRight (44), juce::Justification::centredLeft, 1, 0.7f);
    }

    void AltTile::mouseUp (const juce::MouseEvent& e)
    {
        if (e.mouseWasClicked() && path.isNotEmpty())
        {
            if (e.getNumberOfClicks() >= 2 && onUse) onUse (path);
            else proc.previewFile (path, cat);
        }
    }

    //==============================================================================
    PadDetail::PadDetail (GrooveGrabberProcessor& p) : proc (p)
    {
        addAndMakeVisible (wave);
        wave.setEmptyText ("Pick a pad");
        for (auto* k : { &tune, &start, &length, &gain })
        {
            addAndMakeVisible (*k);
            k->slider.onValueChange = [this] { applyKnobs(); };
        }
        tune.slider.setTooltip ("Tune this pad (on top of the key tuning). Double-click to reset.");
        start.slider.setTooltip ("Skip the start of the sound");
        length.slider.setTooltip ("Shorten the sound (it fades out at the end)");
        gain.slider.setTooltip ("Level, on top of the automatic level matching");
        tune.setAccent (pal().purple);
        start.setAccent (pal().blue);
        length.setAccent (pal().blue);
        gain.setAccent (pal().orange);
        for (auto* b : { &reverse, &lock })
        {
            b->getProperties().set ("style", "toggle");
            b->setClickingTogglesState (true);
            addAndMakeVisible (*b);
        }
        reverse.onClick = [this] { applyKnobs(); };
        lock.setTooltip ("Keep this pad when you regenerate");
        lock.onClick = [this]
        {
            if (auto* k = proc.getActiveKit()) { (*k)[proc.selectedSlot].locked = lock.getToggleState(); proc.sendChangeMessage(); }
        };
        playBtn.setAccent (pal().orange);
        playBtn.setFilled (true);
        playBtn.setTooltip ("Hear the pad");
        playBtn.onClick = [this] { proc.auditionSlot (proc.selectedSlot); };
        addAndMakeVisible (playBtn);
        for (int i = 0; i < 5; ++i)
        {
            auto* a = alts.add (new AltTile (proc));
            a->number = i + 1;
            a->onUse = [this] (const juce::String& path)
            {
                proc.setSlotSound (proc.activeKit, proc.selectedSlot, path);
                proc.auditionSlot (proc.selectedSlot);
                shownPath = {};     // reload the alternatives around the new sound
                refresh();
            };
            addAndMakeVisible (a);
        }
    }

    void PadDetail::refresh()
    {
        const auto* kit = proc.getActiveKit();
        const auto slot = proc.selectedSlot;
        tune.setAccent (pal().purple);
        start.setAccent (pal().blue);
        length.setAccent (pal().blue);
        gain.setAccent (pal().orange);
        playBtn.setAccent (pal().orange);

        updating = true;
        const bool has = kit != nullptr && ! (*kit)[slot].isEmpty();
        for (auto* k : { &tune, &start, &length, &gain }) k->setEnabled (has);
        reverse.setEnabled (has);
        lock.setEnabled (has);
        playBtn.setEnabled (has);
        if (has)
        {
            const auto& st = (*kit)[slot];
            tune.slider.setValue (st.userTune, juce::dontSendNotification);
            start.slider.setValue (st.startMs, juce::dontSendNotification);
            length.slider.setValue (st.lengthPct, juce::dontSendNotification);
            gain.slider.setValue (st.gainDb, juce::dontSendNotification);
            reverse.setToggleState (st.reverse, juce::dontSendNotification);
            lock.setToggleState (st.locked, juce::dontSendNotification);
        }
        updating = false;

        wave.setColour (slotColour (slot));
        auto pk = proc.getPlayable (proc.activeKit);
        wave.setAudio (pk != nullptr && has ? pk->pads[(size_t) (gg::mainNoteForSlot (slot) - gg::firstPadNote)] : nullptr);
        wave.setEmptyText (kit == nullptr ? "Generate kits to start" : "No sound on this pad - pick one below or drop a file on the pad");

        // notes about the sound (Sample Doctor)
        notes = {};
        source = {};
        if (has)
        {
            const auto& st = (*kit)[slot];
            const auto file = juce::File (st.path);
            source = file.getFileName();
            if (auto info = proc.library->find (st.path))
            {
                source = info->pack + "  /  " + file.getFileName();
                juce::StringArray n;
                if (info->leadSilenceMs > 2.0f) n.add ("trimmed " + juce::String (juce::roundToInt (info->leadSilenceMs)) + " ms of silence");
                n.add ("level matched");
                if (std::abs (st.autoTune) > 0.01f && kit->tuneToKey)
                    n.add ("tuned " + juce::String (st.autoTune > 0 ? "+" : "") + juce::String (st.autoTune, 2) + " st to "
                           + (kit->key >= 0 ? gg::keyShortName (kit->key) : juce::String ("the 808")));
                if (info->hasPitch() && (slot == gg::Slot::bass808 || slot == gg::Slot::kick || gg::slotCat (slot) == gg::Cat::tom))
                    n.add ("note " + gg::noteName ((int) std::round (info->midiPitch() + st.tune (kit->tuneToKey))));
                if (info->clipped) n.add ("clipped in the file");
                if (info->tailFloorDb > -45.0f) n.add ("hiss / vinyl noise");
                notes = n.joinIntoString ("  -  ");
            }
            else
                notes = "your own file - level matched";
        }

        const bool changed = shownSlot != slot || shownKit != proc.activeKit || shownPath != (has ? (*kit)[slot].path : juce::String());
        if (changed)
        {
            shownSlot = slot;
            shownKit = proc.activeKit;
            shownPath = has ? (*kit)[slot].path : juce::String();
            loadAlternatives();
        }
        repaint();
    }

    void PadDetail::loadAlternatives()
    {
        const auto list = proc.kits.empty() ? juce::StringArray() : proc.alternativesFor (proc.activeKit, proc.selectedSlot, alts.size());
        for (int i = 0; i < alts.size(); ++i)
            alts[i]->setSound (list[i], gg::slotCat (proc.selectedSlot));
    }

    void PadDetail::applyKnobs()
    {
        if (updating) return;
        auto* kit = proc.getActiveKit();
        if (kit == nullptr) return;
        auto& st = (*kit)[proc.selectedSlot];
        if (st.isEmpty()) return;
        st.userTune = (float) tune.slider.getValue();
        st.startMs = (float) start.slider.getValue();
        st.lengthPct = (float) length.slider.getValue();
        st.gainDb = (float) gain.slider.getValue();
        st.reverse = reverse.getToggleState();
        proc.kitChanged (proc.activeKit);
    }

    void PadDetail::resized()
    {
        auto r = getLocalBounds().reduced (14, 10);
        r.removeFromTop (26);      // title
        auto knobs = r.removeFromRight (360);
        r.removeFromRight (14);
        auto left = r;
        auto waveRow = left.removeFromTop (juce::jmax (60, left.getHeight() - 92));
        playBtn.setBounds (waveRow.removeFromLeft (34).withSizeKeepingCentre (34, 34));
        waveRow.removeFromLeft (8);
        wave.setBounds (waveRow);
        left.removeFromTop (22);   // "alternatives" label
        const int w = left.getWidth() / alts.size();
        for (auto* a : alts) a->setBounds (left.removeFromLeft (w).reduced (3, 0));

        auto buttons = knobs.removeFromBottom (30);
        lock.setBounds (buttons.removeFromRight (90));
        buttons.removeFromRight (8);
        reverse.setBounds (buttons.removeFromRight (100));
        knobs.removeFromBottom (6);
        const int kw = knobs.getWidth() / 4;
        for (auto* k : { &tune, &start, &length, &gain }) k->setBounds (knobs.removeFromLeft (kw));
    }

    void PadDetail::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        drawPanel (g, getLocalBounds().toFloat().reduced (2.0f), 12.0f);
        auto r = getLocalBounds().reduced (14, 10);
        auto title = r.removeFromTop (22);
        const auto slot = proc.selectedSlot;
        drawSectionTitle (g, title.removeFromLeft (180), gg::slotName (slot), slotColour (slot));
        g.setColour (p.dim);
        g.setFont (uiFont (14.5f, false));
        g.drawFittedText (source, title.removeFromLeft (title.getWidth() - 380), juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (p.green.withAlpha (0.9f));
        g.setFont (uiFont (13.5f, false));
        g.drawFittedText (notes, title, juce::Justification::centredRight, 1, 0.6f);

        // "alternatives" label
        auto alt = alts.isEmpty() ? juce::Rectangle<int>() : alts[0]->getBounds().withY (alts[0]->getY() - 20).withHeight (18);
        g.setColour (p.dim);
        g.setFont (headFont (9.5f).withExtraKerningFactor (0.12f));
        g.drawText ("ALTERNATIVES  -  CLICK TO HEAR, USE TO SWAP", alt.withWidth (400), juce::Justification::centredLeft);
    }
}
