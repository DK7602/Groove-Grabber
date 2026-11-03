#include "SaucePage.h"
#include "core/Daws.h"
#include "core/Settings.h"
#include "core/MyGrooves.h"

namespace ggui
{
    //==============================================================================
    PatternGrid::PatternGrid() {}

    juce::String PatternGrid::getTooltip()
    {
        int step = 0;
        if (timelineAt (getMouseXYRelative().toFloat(), step))
            return "Click the count to move the playhead there and play from there. Drag along it to scrub. Play and Space start from here too.";
        return "Click a drum's name to hear it. Click a step to add or take away a hit (drag along a row for several). "
               "Right-click: hard, medium, soft. Ctrl-click (Alt-click on a Mac): 2 quick hits in the step, again for 3 (a triplet roll), again for 1.";
    }

    void PatternGrid::setCue (double beat)
    {
        if (std::abs (beat - cue) < 1.0e-6) return;
        cue = beat;
        repaint();
    }

    bool PatternGrid::timelineAt (juce::Point<float> pos, int& step) const
    {
        const auto geo = geometry();
        if (geo.steps == 0 || pos.x < geo.gridX || pos.x >= geo.area.getRight() || pos.y < geo.header.getY() || pos.y >= geo.area.getY()) return false;
        step = juce::jlimit (0, geo.steps - 1, (int) ((pos.x - geo.gridX) / geo.stepW));
        return true;
    }

    void PatternGrid::clickTimeline (int step)
    {
        if (groove == nullptr || ! juce::isPositiveAndBelow (step, groove->bars * 16)) return;
        if (onSeek) onSeek (step * 0.25);
    }

    void PatternGrid::setGroove (std::shared_ptr<const gg::Groove> g, std::vector<gg::Slot> l)
    {
        groove = std::move (g);
        lanes = std::move (l);
        if (groove != nullptr && cue >= groove->lengthBeats()) cue = 0;     // (a shorter pattern)
        repaint();
    }

    void PatternGrid::setPlayhead (double beat)
    {
        if (std::abs (beat - playhead) < 1.0e-4) return;
        playhead = beat;
        repaint();
    }

    PatternGrid::Geometry PatternGrid::geometry() const
    {
        Geometry geo;
        if (groove == nullptr || lanes.empty()) return geo;
        auto r = getLocalBounds().toFloat();
        geo.labelW = juce::jlimit (96.0f, 150.0f, r.getWidth() * 0.12f);
        geo.header = r.removeFromTop (22.0f);
        geo.steps = groove->bars * 16;
        geo.gridX = r.getX() + geo.labelW;
        geo.stepW = (r.getWidth() - geo.labelW) / (float) geo.steps;
        geo.laneH = juce::jmin (34.0f, r.getHeight() / (float) lanes.size());
        geo.area = juce::Rectangle<float> (geo.gridX, r.getY(), geo.stepW * (float) geo.steps, geo.laneH * (float) lanes.size());
        return geo;
    }

    bool PatternGrid::cellAt (juce::Point<float> pos, int& lane, int& step) const
    {
        const auto geo = geometry();
        if (geo.steps == 0 || ! geo.area.contains (pos)) return false;
        lane = juce::jlimit (0, (int) lanes.size() - 1, (int) ((pos.y - geo.area.getY()) / geo.laneH));
        step = juce::jlimit (0, geo.steps - 1, (int) ((pos.x - geo.gridX) / geo.stepW));
        return true;
    }

    void PatternGrid::clickStep (int lane, int step, const juce::ModifierKeys& mods)
    {
        if (groove == nullptr || ! juce::isPositiveAndBelow (lane, (int) lanes.size()) || ! juce::isPositiveAndBelow (step, groove->bars * 16)) return;
        const auto slot = lanes[(size_t) lane];
        const auto cur = gg::stepHits (*groove, slot, step);
        std::vector<gg::GrooveHit> next;
        if (mods.isPopupMenu() || mods.isShiftDown())
        {
            // hard -> medium -> soft; an empty step gets a soft ghost note
            next = cur.count == 0 ? gg::withStep (*groove, slot, step, 1, gg::softHit)
                                  : gg::withStep (*groove, slot, step, cur.count, gg::nextLevel (cur.velocity));
        }
        else if (mods.isAltDown() || mods.isCtrlDown())
        {
            // one hit -> a quick double -> a triplet burst -> one hit
            next = cur.count == 0 ? gg::withStep (*groove, slot, step, 2, gg::hardHit)
                                  : gg::withStep (*groove, slot, step, cur.count % 3 + 1, cur.velocity);
        }
        else
        {
            painting = true;
            paintLane = lane;
            lastStep = step;
            paintAdd = cur.count == 0;
            next = gg::withStep (*groove, slot, step, paintAdd ? 1 : 0, gg::hardHit);
        }
        // hear what you put there (a hit added, or made harder / softer / a roll); nothing when you take one away
        const auto after = [&]
        {
            gg::Groove probe;
            probe.bars = groove->bars;
            probe.hits = next;
            return gg::stepHits (probe, slot, step);
        }();
        if (after.count > 0 && onAudition) onAudition (slot, after.velocity);
        if (onEdit) onEdit (std::move (next));
    }

    void PatternGrid::auditionLane (int lane)
    {
        if (juce::isPositiveAndBelow (lane, (int) lanes.size()) && onAudition) onAudition (lanes[(size_t) lane], gg::hardHit);
    }

    void PatternGrid::mouseDown (const juce::MouseEvent& e)
    {
        painting = false;
        seekStep = -1;
        // the timeline (the count over the steps): the playhead goes there and plays from there
        if (int step = 0; timelineAt (e.position, step))
        {
            seekStep = step;
            clickTimeline (step);
            return;
        }
        // a drum's name: hear it
        if (const auto geo = geometry(); geo.steps > 0 && e.position.x < geo.gridX && e.position.y >= geo.area.getY() && e.position.y < geo.area.getBottom())
        {
            pressedLabel = juce::jlimit (0, (int) lanes.size() - 1, (int) ((e.position.y - geo.area.getY()) / geo.laneH));
            auditionLane (pressedLabel);
            repaint();
            return;
        }
        int lane = 0, step = 0;
        if (! cellAt (e.position, lane, step)) return;
        if (onEditStart) onEditStart();
        clickStep (lane, step, e.mods);
    }

    void PatternGrid::mouseDrag (const juce::MouseEvent& e)
    {
        if (seekStep >= 0)
        {
            // scrubbing along the timeline: on from each step the mouse reaches
            const auto geo = geometry();
            if (geo.steps == 0) return;
            const int step = juce::jlimit (0, geo.steps - 1, (int) ((e.position.x - geo.gridX) / geo.stepW));
            if (step != seekStep) { seekStep = step; clickTimeline (step); }
            return;
        }
        if (! painting || groove == nullptr) return;
        // along the row you started on, wherever the mouse wanders up or down
        const auto geo = geometry();
        if (geo.steps == 0) return;
        const int step = juce::jlimit (0, geo.steps - 1, (int) ((e.position.x - geo.gridX) / geo.stepW));
        if (step == lastStep || ! juce::isPositiveAndBelow (paintLane, (int) lanes.size())) return;
        lastStep = step;
        const auto slot = lanes[(size_t) paintLane];
        const auto cur = gg::stepHits (*groove, slot, step);
        if (paintAdd == (cur.count == 0) && onEdit)
            onEdit (gg::withStep (*groove, slot, step, paintAdd ? 1 : 0, gg::hardHit));
    }

    void PatternGrid::mouseUp (const juce::MouseEvent&)
    {
        painting = false;
        seekStep = -1;
        if (pressedLabel >= 0) { pressedLabel = -1; repaint(); }
    }

    void PatternGrid::mouseMove (const juce::MouseEvent& e)
    {
        int lane = -1, step = -1;
        const bool over = cellAt (e.position, lane, step);
        int label = -1;
        if (const auto geo = geometry(); geo.steps > 0 && e.position.x < geo.gridX && e.position.y >= geo.area.getY() && e.position.y < geo.area.getBottom())
            label = juce::jlimit (0, (int) lanes.size() - 1, (int) ((e.position.y - geo.area.getY()) / geo.laneH));
        int tl = -1;
        if (! timelineAt (e.position, tl)) tl = -1;
        setMouseCursor (over || label >= 0 || tl >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        if (lane != hoverLane || step != hoverStep || label != hoverLabel || tl != hoverTimeline)
        {
            hoverLane = lane; hoverStep = step; hoverLabel = label; hoverTimeline = tl;
            repaint();
        }
    }

    void PatternGrid::mouseExit (const juce::MouseEvent&)
    {
        hoverLane = hoverStep = hoverLabel = hoverTimeline = -1;
        repaint();
    }

    void PatternGrid::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        const auto geo = geometry();
        if (geo.steps == 0) return;
        const auto header = geo.header;
        const float labelX = (float) getLocalBounds().getX(), labelW = geo.labelW, gridX = geo.gridX, stepW = geo.stepW, laneH = geo.laneH;
        const float top = geo.area.getY(), gridH = geo.area.getHeight();
        const int steps = geo.steps;

        // the timeline: the step under the mouse lit up (click it to play from there)
        if (hoverTimeline >= 0)
        {
            g.setColour (p.orange.withAlpha (0.16f));
            g.fillRoundedRectangle (juce::Rectangle<float> (gridX + (float) hoverTimeline * stepW, header.getY(), stepW, header.getHeight()).reduced (1.0f, 1.0f), 4.0f);
        }

        // beats shaded in turn, bars split by a stronger line
        for (int beat = 0; beat < groove->bars * 4; ++beat)
        {
            const float x = gridX + (float) beat * 4.0f * stepW;
            if (beat % 2 == 1)
            {
                g.setColour (p.text.withAlpha (p.light ? 0.035f : 0.03f));
                g.fillRect (juce::Rectangle<float> (x, top, stepW * 4.0f, gridH));
            }
            // the count over each beat: 1 e & a
            g.setFont (headFont (11.0f));
            g.setColour (beat % 4 == 0 ? p.orange : p.text);
            g.drawText (juce::String (beat % 4 + 1), juce::Rectangle<float> (x, header.getY(), stepW, header.getHeight()), juce::Justification::centred);
            g.setFont (uiFont (12.0f, false));
            g.setColour (p.faint);
            const char* sub[] = { "e", "&", "a" };
            for (int s = 1; s < 4; ++s)
                g.drawText (sub[s - 1], juce::Rectangle<float> (x + (float) s * stepW, header.getY(), stepW, header.getHeight()), juce::Justification::centred);
        }
        g.setFont (headFont (10.0f).withExtraKerningFactor (0.1f));
        g.setColour (p.dim);
        g.drawText (juce::String (groove->bars) + (groove->bars == 1 ? " BAR" : " BARS"),
                    juce::Rectangle<float> (labelX, header.getY(), labelW - 8.0f, header.getHeight()), juce::Justification::centredLeft);

        for (int i = 0; i <= steps; ++i)
        {
            const float x = gridX + (float) i * stepW;
            const bool bar = i % 16 == 0, beat = i % 4 == 0;
            g.setColour (bar ? p.dim.withAlpha (0.8f) : beat ? p.line.brighter (0.2f) : p.line.withAlpha (0.45f));
            g.fillRect (juce::Rectangle<float> (x - (bar ? 1.0f : 0.5f), top, bar ? 2.0f : 1.0f, gridH));
        }

        for (size_t li = 0; li < lanes.size(); ++li)
        {
            const auto slot = lanes[li];
            const auto colour = slotColour (slot);
            const juce::Rectangle<float> lane (labelX, top + (float) li * laneH, (float) getWidth() - labelX, laneH);
            g.setColour (p.line.withAlpha (0.5f));
            g.fillRect (lane.withHeight (1.0f).withY (lane.getBottom() - 0.5f));

            // the lane's name (click it to hear the drum), and the note it plays on in your DAW
            if ((int) li == hoverLabel || (int) li == pressedLabel)
            {
                g.setColour (colour.withAlpha ((int) li == pressedLabel ? 0.30f : 0.12f));
                g.fillRoundedRectangle (lane.withWidth (labelW - 6.0f).reduced (0.0f, 2.0f), 5.0f);
            }
            auto label = lane.withWidth (labelW - 8.0f);
            g.setColour (colour);
            g.fillRoundedRectangle (label.removeFromLeft (4.0f).reduced (0.0f, laneH * 0.22f), 2.0f);
            label.removeFromLeft (8.0f);
            g.setFont (uiFont (juce::jmin (15.0f, laneH * 0.58f)));
            g.setColour (p.text);
            const int note = gg::mainNoteForSlot (slot);
            const auto name = slot == gg::Slot::bass808 ? juce::String ("808") : gg::slotName (slot);
            g.drawFittedText (name, label.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
            if (noteNamer && note >= 0 && labelW > 110.0f)
            {
                g.setFont (uiFont (12.0f, false));
                g.setColour (p.faint);
                g.drawText (noteNamer (note), label, juce::Justification::centredRight);
            }

            // empty steps (the one under the mouse lit up)
            for (int i = 0; i < steps; ++i)
            {
                auto cell = juce::Rectangle<float> (gridX + (float) i * stepW, lane.getY(), stepW, laneH).reduced (juce::jmin (3.0f, stepW * 0.12f), juce::jmin (4.0f, laneH * 0.14f));
                const bool hover = (int) li == hoverLane && i == hoverStep;
                g.setColour (hover ? colour.withAlpha (0.28f) : p.text.withAlpha (p.light ? 0.05f : 0.035f));
                g.fillRoundedRectangle (cell, 3.0f);
            }

            // hits: a full step each, or a slice of it for rolls; harder hits brighter
            std::vector<const gg::GrooveHit*> mine;
            for (const auto& h : groove->hits)
                if (h.slot == slot) mine.push_back (&h);
            for (size_t k = 0; k < mine.size(); ++k)
            {
                const auto& h = *mine[k];
                double len = 0.25 - std::fmod (h.beat, 0.25);                 // to the end of its step...
                if (k + 1 < mine.size()) len = juce::jmin (len, mine[k + 1]->beat - h.beat);   // ...or the next hit
                const float x = gridX + (float) (h.beat * 4.0) * stepW;
                const float w = (float) (len * 4.0) * stepW;
                auto cell = juce::Rectangle<float> (x, lane.getY(), w, laneH).reduced (juce::jmin (2.5f, w * 0.12f), juce::jmin (4.0f, laneH * 0.14f));
                const float alpha = juce::jmap (h.velocity, 0.3f, 1.0f, 0.35f, 1.0f);
                g.setColour (colour.withAlpha (alpha));
                g.fillRoundedRectangle (cell, 3.0f);
                if (h.velocity >= 0.95f)
                {
                    g.setColour (juce::Colours::white.withAlpha (p.light ? 0.35f : 0.22f));
                    g.fillRoundedRectangle (cell.withHeight (cell.getHeight() * 0.4f), 3.0f);
                }
            }
        }

        // the playhead: where it plays, or (stopped) where play starts, with a handle on the timeline
        {
            const bool playing = playhead >= 0;
            const double at = playing ? std::fmod (playhead, groove->lengthBeats()) : juce::jlimit (0.0, groove->lengthBeats(), cue);
            const float x = gridX + (float) (at * 4.0) * stepW;
            g.setColour (p.orange.withAlpha (playing ? 1.0f : 0.45f));
            g.fillRect (juce::Rectangle<float> (x - 1.0f, top, 2.0f, gridH));
            juce::Path head;
            const float hy = header.getBottom() - 1.0f, hw = juce::jmin (6.0f, stepW * 0.45f);
            head.addTriangle (x - hw, hy - 8.0f, x + hw, hy - 8.0f, x, hy);
            g.setColour (p.orange.withAlpha (playing ? 1.0f : 0.8f));
            g.fillPath (head);
        }
    }

    //==============================================================================
    SaucePage::SaucePage (GrooveGrabberProcessor& p) : proc (p)
    {
        for (int i = 0; i < gg::numGenres; ++i) genreBox.addItem (gg::genreName ((gg::Genre) i), i + 1);
        genreBox.onChange = [this]
        {
            if (updating) return;
            genre = (gg::Genre) (genreBox.getSelectedId() - 1);
            subStyle = gg::styleForEra (genre, era);
            grooveIndex = 0;
            showChoice();
        };
        genreBox.setTooltip ("The genre to read up on");
        addAndMakeVisible (genreBox);

        styleBox.onChange = [this]
        {
            if (updating || styleBox.getSelectedId() <= 0) return;
            subStyle = styleBox.getSelectedId() - 1;
            grooveIndex = 0;
            showChoice();
        };
        styleBox.setTooltip ("The style within the genre");
        addAndMakeVisible (styleBox);

        eraBox.addItem ("Any era", 1);
        for (int e = 0; e < gg::numEras; ++e) eraBox.addItem (gg::eraName (e), e + 2);
        eraBox.onChange = [this]
        {
            if (updating) return;
            era = eraBox.getSelectedId() - 2;
            showChoice();
        };
        eraBox.setTooltip ("The decade: adds how drums were made and sounded then");
        addAndMakeVisible (eraBox);

        for (auto* b : { &matchBuild, &useOnBuild })
        {
            b->getProperties().set ("style", "ghost");
            addChildComponent (*b);
        }
        matchBuild.setTooltip ("Back to the genre, style and decade picked on BUILD");
        matchBuild.onClick = [this]
        {
            lastBuildGenre = -99;      // forces a re-sync
            refresh();
        };
        useOnBuild.setTooltip ("Set BUILD's genre, style and decade to this, then go to BUILD to generate kits");
        useOnBuild.onClick = [this]
        {
            stopPreview();
            proc.setGenre (genre);
            proc.setStyle (subStyle);
            proc.setEra (era);
            refresh();
            if (onUseOnBuild) onUseOnBuild();
        };

        play.setTooltip ("Play this pattern with the kit you're editing on BUILD (Space)");
        play.onClick = [this] { togglePlay(); };
        addAndMakeVisible (play);

        addAndMakeVisible (grid);
        grid.setNoteNamer ([this] (int note) { return dawNoteName (note); });
        grid.onEditStart = [this] { pushUndo(); };
        grid.onEdit = [this] (std::vector<gg::GrooveHit> hits) { applyHits (std::move (hits)); };
        grid.onAudition = [this] (gg::Slot slot, float velocity)
        {
            if (const int note = gg::mainNoteForSlot (slot); note >= 0) proc.player.trigger (note, velocity);
        };
        grid.onSeek = [this] (double beat) { playFrom (beat); };

        // which of BUILD's four kits plays the pattern
        for (int i = 0; i < 4; ++i)
        {
            auto* b = kitButtons.add (new juce::TextButton (juce::String (i + 1).paddedLeft ('0', 2)));
            b->getProperties().set ("style", "toggle");
            b->onClick = [this, i] { proc.setActiveKit (i); updateKitButtons(); };
            addAndMakeVisible (b);
        }

        blankChip.getProperties().set ("style", "toggle");
        blankChip.setTooltip ("A blank pattern: build one from scratch with the sounds of the kit you picked");
        blankChip.onClick = [this] { openGroove ("blank"); };
        addAndMakeVisible (blankChip);

        swingSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        swingSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        swingSlider.setRange (50.0, 66.0, 1.0);
        swingSlider.setDoubleClickReturnValue (true, 50.0);
        swingSlider.setTooltip ("Swing: 50% is straight, 54 to 58% light, 60 to 62% heavy, 66% a full triplet. Double-click for straight.");
        swingSlider.getProperties().set ("accent", pal().purple.toString());
        swingSlider.onDragStart = [this] { pushUndo(); };
        swingSlider.onValueChange = [this]
        {
            if (! swingSlider.isMouseButtonDown()) pushUndo();          // (a scroll or a double-click: one step each)
            setSwingPercent ((int) swingSlider.getValue());
        };
        addAndMakeVisible (swingSlider);

        for (auto* b : { &addDrum, &undo, &reset })
        {
            b->getProperties().set ("style", "ghost");
            addAndMakeVisible (*b);
        }
        addDrum.setTooltip ("Add a row for a drum this pattern doesn't use yet");
        addDrum.onClick = [this] { showAddDrum(); };
        undo.setTooltip ("Take back your last change to this pattern");
        undo.onClick = [this]
        {
            const auto id = currentId();
            auto& steps = undoSteps[id];
            if (steps.empty()) return;
            auto snap = std::move (steps.back());
            steps.pop_back();
            proc.editGroove (id, std::move (snap.hits), snap.swing);     // (the saved version again clears the change)
            showGroove();
        };
        reset.setTooltip ("Back to the original pattern (UNDO brings your version back)");
        reset.onClick = [this]
        {
            const auto id = currentId();
            if (showingMine())
            {
                // changes not saved yet: REVERT them (UNDO brings them back)
                if (proc.isGrooveEdited (id))
                {
                    pushUndo();
                    addedLanes.erase (id);
                    proc.resetGroove (id);
                    showGroove();
                    return;
                }
                // DELETE, once you've clicked it twice (it's kept in Backups too)
                if (! deleteArmed)
                {
                    deleteArmed = true;
                    showGroove();
                    disarmDelete.start (4000, [this] { if (deleteArmed) { deleteArmed = false; showGroove(); } });
                    return;
                }
                deleteArmed = false;
                disarmDelete.cancel();
                myShown = {};
                undoSteps.erase (id);
                addedLanes.erase (id);
                proc.deleteMyGroove (id);
                pickGroove (0);
                return;
            }
            if (! proc.isGrooveEdited (id) && addedLanes[id].empty()) return;
            pushUndo();
            addedLanes.erase (id);
            proc.resetGroove (id);
            showGroove();
        };

        saveMine.getProperties().set ("style", "accent");
        saveMine.getProperties().set ("accent", pal().blue.toString());
        saveMine.setTooltip ("Keep this pattern as your own, under a name: it's in every song, here and in BUILD's groove list");
        saveMine.onClick = [this] { startNaming(); };
        addAndMakeVisible (saveMine);

        saveChanges.getProperties().set ("style", "accent");
        saveChanges.getProperties().set ("accent", pal().orange.toString());
        saveChanges.setTooltip ("Put your changes into this groove (the version before is kept in Backups)");
        saveChanges.onClick = [this]
        {
            const auto id = currentId();
            if (proc.saveGrooveChanges (id)) addedLanes.erase (id);
            showGroove();
        };
        addChildComponent (saveChanges);
        renameMine.getProperties().set ("style", "ghost");
        renameMine.setTooltip ("Give this groove a new name (nothing else changes)");
        renameMine.onClick = [this] { startNaming (true); };
        addChildComponent (renameMine);

        myChip.getProperties().set ("style", "toggle");
        myChip.setTooltip ("Your own grooves, saved with SAVE AS MY GROOVE");
        myChip.onClick = [this] { showMyGroovesMenu(); };
        addAndMakeVisible (myChip);

        nameEdit.setFont (uiFont (16.0f));
        nameEdit.setBorder (juce::BorderSize<int> (2, 8, 2, 8));
        nameEdit.setInputRestrictions (60);
        nameEdit.onReturnKey = [this] { finishNaming (true); };
        nameEdit.onEscapeKey = [this] { finishNaming (false); };
        addChildComponent (nameEdit);
        nameOk.getProperties().set ("style", "accent");
        nameOk.onClick = [this] { finishNaming (true); };
        nameOk.setTooltip ("Save it in MY GROOVES. The pattern you started from stays as it was on BUILD.");
        nameReplace.getProperties().set ("style", "accent");
        nameReplace.onClick = [this] { finishNaming (true, {}, true); };
        nameReplace.setTooltip ("Save it in MY GROOVES, and BUILD's preview plays it in place of the pattern you started from");
        addChildComponent (nameReplace);
        nameCancel.getProperties().set ("style", "ghost");
        nameCancel.onClick = [this] { finishNaming (false); };
        addChildComponent (nameOk);
        addChildComponent (nameCancel);

        docView.setViewedComponent (&doc, false);
        docView.setScrollBarsShown (true, false);
        docView.setSingleStepSizes (0, 48);
        addAndMakeVisible (docView);
    }

    SaucePage::~SaucePage()
    {
        stopTimer();
        if (playingHere) stopPreview();
        docView.setViewedComponent (nullptr, false);
    }

    juce::String SaucePage::dawNoteName (int note) const
    {
        const auto& daw = gg::dawGuide (gg::chosenDaw (proc.getSettings().get ("daw")));
        return gg::dawNoteName (note, daw) + " (" + juce::String (note) + ")";
    }

    bool SaucePage::isFollowingBuild() const
    {
        const auto& o = proc.options;
        return o.genre == genre && o.subStyle == subStyle && o.era == era;
    }

    void SaucePage::choose (gg::Genre g, int sub, int e)
    {
        genre = g;
        subStyle = juce::jlimit (0, (int) gg::stylesOf (g).size() - 1, sub);
        era = juce::jlimit (-1, gg::numEras - 1, e);
        grooveIndex = 0;
        showChoice();
    }

    void SaucePage::setSwingPercent (int percent)
    {
        const auto id = currentId();
        if (id.isEmpty()) return;
        proc.setGrooveSwing (id, (float) (juce::jlimit (50, 66, percent) - 50) * 3.0f / 100.0f);
        if (playingHere) if (const auto g = currentGroove()) proc.player.setSwing (g->swing);
        showGroove();
        repaint (dragArea);
    }

    void SaucePage::updateKitButtons()
    {
        for (int i = 0; i < kitButtons.size(); ++i)
        {
            auto* b = kitButtons[i];
            const bool has = i < (int) proc.kits.size();
            b->setEnabled (has);
            b->setToggleState (has && i == proc.activeKit, juce::dontSendNotification);
            b->setTooltip (has ? "Kit " + juce::String (i + 1) + ": " + proc.kits[(size_t) i].name + " (plays the pattern; the same as clicking it on BUILD)"
                               : juce::String ("Generate kits on BUILD to hear the pattern with them"));
        }
    }

    void SaucePage::refresh()
    {
        updateKitButtons();
        // BUILD changed since we last looked: follow it. Otherwise keep what you picked here.
        const auto& o = proc.options;
        if ((int) o.genre != lastBuildGenre || o.subStyle != lastBuildStyle || o.era != lastBuildEra)
        {
            lastBuildGenre = (int) o.genre;
            lastBuildStyle = o.subStyle;
            lastBuildEra = o.era;
            if (! (o.genre == genre && o.subStyle == subStyle && o.era == era)) grooveIndex = 0;
            genre = o.genre;
            subStyle = o.subStyle;
            era = o.era;
            showChoice();
            openGroove (proc.grooveId);          // the pattern BUILD is playing
            return;
        }
        fillBoxes();
        showGroove();
        resized();
        repaint();
    }

    void SaucePage::fillBoxes()
    {
        const juce::ScopedValueSetter<bool> busy (updating, true);
        genreBox.setSelectedId ((int) genre + 1, juce::dontSendNotification);
        if (stylesShownFor != genre)
        {
            stylesShownFor = genre;
            styleBox.clear (juce::dontSendNotification);
            const auto& list = gg::stylesOf (genre);
            for (int i = 0; i < (int) list.size(); ++i)
                styleBox.addItem (i == 0 ? "General " + gg::genreName (genre) : list[(size_t) i].name, i + 1);
        }
        styleBox.setSelectedId (subStyle + 1, juce::dontSendNotification);
        eraBox.setSelectedId (era + 2, juce::dontSendNotification);
        const bool following = isFollowingBuild();
        matchBuild.setVisible (! following);
        useOnBuild.setVisible (! following);
    }

    void SaucePage::showChoice()
    {
        fillBoxes();
        myShown = {};
        if (naming) finishNaming (false);
        const auto& style = gg::styleFor (genre, subStyle);
        grooveIds.clear();
        for (const auto* g : gg::sauceGrooves (style)) grooveIds.add (g->id);
        grooveChips.clear();
        for (int i = 0; i < grooveIds.size(); ++i)
        {
            const auto& name = gg::grooveFor (grooveIds[i]).name;
            auto* b = grooveChips.add (new juce::TextButton (name.toUpperCase()));
            b->getProperties().set ("style", "toggle");
            b->setTooltip ("Show the " + name + " pattern");
            b->onClick = [this, i] { pickGroove (i); };
            addAndMakeVisible (b);
        }
        pickGroove (juce::jlimit (0, juce::jmax (0, grooveIds.size() - 1), grooveIndex));

        doc.setMarkdown (gg::sauceMarkdown (genre, subStyle, era));
        resized();
        docView.setViewPosition (0, 0);
        repaint();
    }

    void SaucePage::pickGroove (int index)
    {
        grooveIndex = index;
        myShown = {};
        deleteArmed = false;
        for (int i = 0; i < grooveChips.size(); ++i) grooveChips[i]->setToggleState (i == index, juce::dontSendNotification);
        showGroove();
        if (playingHere)
            if (const auto g = currentGroove())
            {
                proc.player.setGroove (g.get());
                proc.player.setSwing (g->swing);
                proc.player.setTempo (previewTempo());
            }
    }

    juce::String SaucePage::currentId() const
    {
        if (myShown.isNotEmpty()) return myShown;
        return juce::isPositiveAndBelow (grooveIndex, grooveIds.size()) ? grooveIds[grooveIndex] : juce::String();
    }

    bool SaucePage::showingMine() const
    {
        return myShown.isNotEmpty();
    }

    void SaucePage::pickMine (const juce::String& id)
    {
        if (gg::mygrooves::find (id) == nullptr) return;
        myShown = id;
        deleteArmed = false;
        for (auto* c : grooveChips) c->setToggleState (false, juce::dontSendNotification);
        showGroove();
        if (playingHere)
            if (const auto g = currentGroove())
            {
                proc.player.setGroove (g.get());
                proc.player.setSwing (g->swing);
                proc.player.setTempo (previewTempo());
            }
        resized();
    }

    void SaucePage::openGroove (const juce::String& id)
    {
        if (gg::mygrooves::isMine (id)) { pickMine (id); return; }
        if (gg::grooveFor (id).id != id) return;
        int index = grooveIds.indexOf (id);
        if (index < 0 && id == "blank")
        {
            grooveIds.add (id);                  // (the BLANK button stands for it)
            index = grooveIds.size() - 1;
        }
        if (index < 0)
        {
            // a groove from another style: a button for it next to this style's
            grooveIds.add (id);
            const auto& name = gg::grooveFor (id).name;
            auto* b = grooveChips.add (new juce::TextButton (name.toUpperCase()));
            b->getProperties().set ("style", "toggle");
            b->setTooltip ("BUILD's groove: " + name);
            const int i = grooveIds.size() - 1;
            b->onClick = [this, i] { pickGroove (i); };
            addAndMakeVisible (b);
            index = i;
        }
        pickGroove (index);
        resized();
    }

    void SaucePage::showMyGroovesMenu()
    {
        const auto all = gg::mygrooves::list();
        juce::PopupMenu m, others;
        std::array<juce::PopupMenu, gg::numGenres> byGenre;
        int here = 0;
        for (int i = 0; i < (int) all.size(); ++i)
        {
            const auto& e = all[(size_t) i];
            const auto text = e.groove->name + "   (" + juce::String (e.groove->bpm, 0) + ")";
            if (e.genre == genre) { if (here++ == 0) m.addSectionHeader ("MY " + gg::genreName (genre).toUpperCase() + " GROOVES"); m.addItem (i + 1, text, true, e.groove->id == myShown); }
            else byGenre[(size_t) e.genre].addItem (i + 1, text, true, e.groove->id == myShown);
        }
        for (int g = 0; g < gg::numGenres; ++g)
            if (byGenre[(size_t) g].getNumItems() > 0) others.addSubMenu (gg::genreName ((gg::Genre) g), byGenre[(size_t) g]);
        if (others.getNumItems() > 0)
        {
            if (here > 0) m.addSeparator();
            m.addSubMenu (here > 0 ? "Other genres" : "From other genres", others);
        }
        if (all.empty()) m.addItem (-1, "None yet. Change a pattern, then press SAVE AS MY GROOVE.", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&myChip), [this, all, safe = juce::Component::SafePointer<SaucePage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0 || r > (int) all.size()) return;
            pickMine (all[(size_t) r - 1].groove->id);
        });
    }

    void SaucePage::startNaming (bool rename)
    {
        const auto g = currentGroove();
        if (g == nullptr) return;
        renaming = rename && showingMine();
        juce::String name = g->name;
        if (! renaming)
        {
            // a name that isn't taken yet: My Boom Bap, My Boom Bap 2... and for a copy of yours, the next number
            // ("DK Trap 1" -> "DK Trap 2"), or "... 2"
            juce::StringArray taken;
            for (const auto& e : gg::mygrooves::list()) taken.add (e.groove->name);
            juce::String stem = showingMine() ? g->name : "My " + g->name;
            int n = 1;
            const auto digits = stem.getTrailingIntValue();
            if (showingMine() && digits > 0 && stem.endsWith (juce::String (digits)))
            {
                n = digits;
                stem = stem.dropLastCharacters (juce::String (digits).length()).trimEnd();
            }
            name = showingMine() ? stem + " " + juce::String (n + 1) : stem;
            for (int k = n + 1; taken.contains (name, true) && k < n + 200; ++k) name = stem + " " + juce::String (k + (showingMine() ? 1 : 0));
        }
        naming = true;
        nameEdit.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        nameEdit.setColour (juce::TextEditor::outlineColourId, pal().blue);
        nameEdit.setColour (juce::TextEditor::focusedOutlineColourId, pal().blue);
        nameEdit.setText (name, false);
        nameEdit.applyColourToAllText (pal().text);
        nameOk.setButtonText (renaming ? "RENAME" : "SAVE");
        nameOk.setTooltip (renaming ? "Give this groove its new name"
                                    : "Save it in MY GROOVES. The pattern you started from stays as it was on BUILD.");
        nameOk.getProperties().set ("accent", pal().blue.toString());
        nameReplace.getProperties().set ("accent", pal().orange.toString());
        showGroove();                  // the name box in, the edit buttons out
        resized();
        nameEdit.grabKeyboardFocus();
        nameEdit.selectAll();
        repaint (patternArea);
    }

    void SaucePage::finishNaming (bool save, const juce::String& givenName, bool replaceOnBuild)
    {
        const auto name = (givenName.isNotEmpty() ? givenName : nameEdit.getText()).trim();
        naming = false;
        if (save && name.isNotEmpty())
        {
            if (renaming && showingMine())
                proc.renameMyGroove (myShown, name);
            else if (! renaming)
            {
                const auto from = currentId();
                const auto& style = gg::styleFor (genre, subStyle);
                const auto id = proc.saveMyGroove (from, name, genre, style.id);
                if (id.isNotEmpty())
                {
                    // your version lives on under its new name; the pattern you started from goes back to how it
                    // was saved (UNDO on it brings your changes back)
                    if (proc.isGrooveEdited (from))
                    {
                        auto& steps = undoSteps[from];
                        steps.push_back ({ proc.getGroove (from)->hits, proc.getGroove (from)->swing });
                        proc.resetGroove (from);
                    }
                    if (replaceOnBuild) proc.setGrooveId (id);          // BUILD plays yours in its place
                    pickMine (id);
                }
            }
        }
        resized();
        showGroove();
    }

    std::shared_ptr<const gg::Groove> SaucePage::currentGroove() const
    {
        const auto id = currentId();
        return id.isEmpty() ? nullptr : proc.getGroove (id);
    }

    std::vector<gg::Slot> SaucePage::lanesFor (const juce::String& id) const
    {
        // the original's drums, any you've added hits for, and any you added rows for
        std::vector<gg::Slot> lanes;
        auto add = [&lanes] (gg::Slot s) { if (std::find (lanes.begin(), lanes.end(), s) == lanes.end()) lanes.push_back (s); };
        if (! gg::mygrooves::isMine (id)) for (const auto& h : gg::grooveFor (id).hits) add (h.slot);
        if (id == "blank")
        {
            // a blank pad: a row for every sound in the kit you picked (or the usual drums, before there are kits)
            if (const auto* k = const_cast<GrooveGrabberProcessor&> (proc).getActiveKit())
            {
                for (int i = 0; i < gg::numSlots; ++i)
                    if (! k->slots[(size_t) i].isEmpty()) add ((gg::Slot) i);
            }
            if (lanes.empty())
                for (auto s : { gg::Slot::kick, gg::Slot::snare, gg::Slot::clap, gg::Slot::closedHat, gg::Slot::openHat, gg::Slot::perc1 }) add (s);
        }
        if (const auto g = proc.getGroove (id)) for (const auto& h : g->hits) add (h.slot);
        if (const auto it = addedLanes.find (id); it != addedLanes.end()) for (auto s : it->second) add (s);
        std::sort (lanes.begin(), lanes.end(), [] (gg::Slot a, gg::Slot b) { return (int) a < (int) b; });
        return lanes;
    }

    void SaucePage::showGroove()
    {
        if (showingMine() && gg::mygrooves::find (myShown) == nullptr) { myShown = {}; deleteArmed = false; }   // deleted meanwhile
        const auto id = currentId();
        const bool mine = showingMine();
        grid.setGroove (currentGroove(), id.isEmpty() ? std::vector<gg::Slot>() : lanesFor (id));
        const auto it = undoSteps.find (id);
        undo.setEnabled (it != undoSteps.end() && ! it->second.empty());
        const bool changed = proc.isGrooveEdited (id);
        if (changed) deleteArmed = false;
        reset.setButtonText (mine ? (changed ? "REVERT" : (deleteArmed ? "SURE? DELETE" : "DELETE")) : "RESET");
        reset.getProperties().set ("style", mine && deleteArmed ? "accent" : "ghost");
        reset.setTooltip (mine ? (changed ? "Back to your groove as it's saved (UNDO brings your changes back)"
                                          : "Delete this groove from MY GROOVES (click twice; a copy stays in Backups)")
                               : "Back to the original pattern (UNDO brings your version back)");
        reset.setEnabled (mine || changed || (addedLanes.count (id) > 0 && ! addedLanes.at (id).empty()));
        addDrum.setEnabled (id.isNotEmpty() && (int) lanesFor (id).size() < gg::numSlots);
        saveMine.setButtonText (mine ? "SAVE AS COPY" : "SAVE AS MY GROOVE");
        saveMine.setTooltip (mine ? "Save this as a new groove of its own; this one stays as it's saved"
                                  : "Keep this pattern as your own, under a name: it's in every song, here and in BUILD's groove list");
        saveMine.setEnabled (id.isNotEmpty());
        saveMine.getProperties().set ("accent", pal().blue.toString());

        const auto count = (int) gg::mygrooves::list().size();
        juce::String chip = "MY GROOVES" + (count > 0 ? " (" + juce::String (count) + ")" : juce::String());
        if (mine) if (const auto g = currentGroove()) chip = "MY: " + g->name.toUpperCase();
        myChip.setButtonText (chip);
        myChip.setToggleState (mine, juce::dontSendNotification);
        blankChip.setToggleState (id == "blank", juce::dontSendNotification);
        if (const auto g = currentGroove())
            swingSlider.setValue (gg::swingPercent (*g), juce::dontSendNotification);
        swingSlider.setEnabled (id.isNotEmpty());
        swingSlider.setVisible (! naming);

        for (auto* b : { &saveMine, &addDrum, &undo, &reset }) b->setVisible (! naming);
        const bool relayout = renameMine.isVisible() != (! naming && mine) || saveChanges.isVisible() != (! naming && mine && changed);
        renameMine.setVisible (! naming && mine);
        saveChanges.setVisible (! naming && mine && changed);
        if (relayout) resized();          // SAVE and RENAME come and go: the edit bar makes room for them
        for (auto* c : std::initializer_list<juce::Component*> { &nameEdit, &nameOk, &nameCancel }) c->setVisible (naming);
        nameReplace.setVisible (naming && ! renaming);
        repaint (patternArea);
    }

    void SaucePage::pushUndo()
    {
        const auto id = currentId();
        if (id.isEmpty()) return;
        auto& steps = undoSteps[id];
        const auto g = proc.getGroove (id);
        steps.push_back ({ g->hits, g->swing });
        if (steps.size() > 64) steps.erase (steps.begin());
    }

    void SaucePage::applyHits (std::vector<gg::GrooveHit> hits)
    {
        const auto id = currentId();
        if (id.isEmpty()) return;
        proc.editGroove (id, std::move (hits));       // the same as the original resets it
        showGroove();
    }

    void SaucePage::showAddDrum()
    {
        const auto id = currentId();
        if (id.isEmpty()) return;
        const auto lanes = lanesFor (id);
        juce::PopupMenu m;
        m.addSectionHeader ("ADD A ROW FOR");
        for (int i = 0; i < gg::numSlots; ++i)
        {
            const auto slot = (gg::Slot) i;
            if (std::find (lanes.begin(), lanes.end(), slot) != lanes.end()) continue;
            m.addItem (i + 1, (slot == gg::Slot::bass808 ? juce::String ("808") : gg::slotName (slot)) + "   " + dawNoteName (gg::mainNoteForSlot (slot)));
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addDrum), [this, id, safe = juce::Component::SafePointer<SaucePage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0 || id != currentId()) return;
            addedLanes[id].push_back ((gg::Slot) (r - 1));
            showGroove();
        });
    }

    double SaucePage::previewTempo() const
    {
        // your song's tempo while it's your song's style; each pattern's own tempo when you're reading up on another
        // (some styles borrow a groove that's played at its own speed: a boom bap groove in country rap)
        if (isFollowingBuild())
            return playingHere ? savedTempo : (proc.options.bpm > 0 ? proc.options.bpm : proc.player.getTempo());
        if (const auto g = currentGroove()) return g->bpm;
        return gg::styleFor (genre, subStyle).bpm;
    }

    juce::String SaucePage::dropHint() const
    {
        const auto& daw = gg::dawGuide (gg::chosenDaw (proc.getSettings().get ("daw")));
        return "DRAG MIDI, then drop it " + daw.groove + ".";
    }

    juce::String SaucePage::getTooltip()
    {
        return dragArea.contains (getMouseXYRelative()) ? dropHint() : juce::String();
    }

    void SaucePage::togglePlay()
    {
        if (playingHere) { stopPreview(); return; }
        const auto g = currentGroove();
        if (g == nullptr) return;
        savedSwing = proc.player.getSwing();
        savedTempo = proc.player.getTempo();
        playingHere = true;
        proc.player.setGroove (g.get());       // (an edit stays alive in the processor while it's the current one)
        proc.player.setSwing (g->swing);
        proc.player.setTempo (previewTempo());
        proc.player.setGroovePlaying (true);
        // from where the playhead rests (the last place you clicked on the timeline)
        const double from = juce::jlimit (0.0, g->lengthBeats() - 0.25, grid.getCue());
        proc.player.seekGroove (from);
        grid.setPlayhead (from);
        play.setToggleState (true, juce::dontSendNotification);
        startTimerHz (30);
        repaint();
    }

    void SaucePage::playFrom (double beat)
    {
        const auto g = currentGroove();
        if (g == nullptr) return;
        beat = juce::jlimit (0.0, g->lengthBeats() - 0.25, beat);
        grid.setCue (beat);
        if (! playingHere) { togglePlay(); return; }
        proc.player.seekGroove (beat);
        grid.setPlayhead (beat);
    }

    void SaucePage::stopPreview()
    {
        if (! playingHere) return;
        playingHere = false;
        proc.player.setGroovePlaying (false);
        proc.player.setGroove (proc.getGroove (proc.grooveId).get());     // BUILD's preview groove again
        proc.player.setSwing (savedSwing);
        proc.player.setTempo (savedTempo);
        play.setToggleState (false, juce::dontSendNotification);
        grid.setPlayhead (-1);
        stopTimer();
        proc.sendChangeMessage();
        repaint();
    }

    void SaucePage::timerCallback()
    {
        if (! playingHere) { stopTimer(); return; }
        if (! proc.player.isGroovePlaying()) { stopPreview(); return; }     // stopped elsewhere (Space on another page...)
        grid.setPlayhead (proc.player.getGrooveBeat());
    }

    void SaucePage::visibilityChanged()
    {
        if (! isVisible()) stopPreview();
    }

    void SaucePage::lookAndFeelChanged()
    {
        layoutDoc();
        repaint();
    }

    void SaucePage::layoutDoc()
    {
        if (docView.getWidth() <= 0) return;
        const int w = docView.getWidth() - docView.getScrollBarThickness() - 4;
        const int h = doc.layoutFor (w);
        doc.setSize (w, juce::jmax (h, docView.getHeight()));
        doc.repaint();
    }

    juce::File SaucePage::writeMidi() const
    {
        const auto g = currentGroove();
        if (g == nullptr) return {};
        const auto& style = gg::styleFor (genre, subStyle);
        const double bpm = previewTempo();
        const auto styleName = subStyle == 0 ? gg::genreName (genre) : style.name;
        const auto folder = gg::paths::documentsDir().getChildFile ("Sauce MIDI");
        folder.createDirectory();
        const auto f = folder.getChildFile (gg::paths::safeFileName (styleName + " - " + g->name + (proc.isGrooveEdited (g->id) ? " (my edit)" : "")
                                                                     + " " + juce::String (bpm, 0) + " BPM", 80) + ".mid");
        const auto midi = gg::grooveToMidi (*g, bpm, g->swing, 2);   // (a DAW places the notes by beat, at the song's tempo)
        f.replaceWithData (midi.getData(), midi.getSize());
        return f;
    }

    void SaucePage::mouseMove (const juce::MouseEvent& e)
    {
        setMouseCursor (dragArea.contains (e.getPosition()) ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void SaucePage::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragging || ! dragArea.contains (e.getMouseDownPosition()) || e.getDistanceFromDragStart() < 6) return;
        const auto f = writeMidi();
        if (! f.existsAsFile()) return;
        dragging = true;
        repaint (dragArea);
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this,
                                                                    [safe = juce::Component::SafePointer<SaucePage> (this)]
        {
            if (safe != nullptr) { safe->dragging = false; safe->repaint (safe->dragArea); }
        });
    }

    //==============================================================================
    void SaucePage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8);
        leftArea = r.removeFromLeft (juce::jlimit (300, 360, r.getWidth() / 4));
        r.removeFromLeft (10);
        patternArea = r.removeFromTop (juce::jlimit (290, 380, r.getHeight() * 45 / 100));
        r.removeFromTop (8);
        sauceArea = r;

        // left: the recipe
        auto l = leftArea.reduced (14, 12);
        l.removeFromTop (28);
        auto row = l.removeFromTop (32);
        eraBox.setBounds (row.removeFromRight (row.getWidth() * 40 / 100));
        row.removeFromRight (8);
        genreBox.setBounds (row);
        l.removeFromTop (8);
        styleBox.setBounds (l.removeFromTop (32));
        l.removeFromTop (10);
        followArea = l.removeFromTop (40);
        if (matchBuild.isVisible())
        {
            auto b = l.removeFromTop (32);
            matchBuild.setBounds (b.removeFromLeft (b.getWidth() / 2 - 4));
            b.removeFromLeft (8);
            useOnBuild.setBounds (b);
        }
        l.removeFromTop (16);
        factsArea = l.removeFromTop (24 + 4 * 26);
        l.removeFromTop (14);
        legendArea = l.removeFromTop (juce::jmin (l.getHeight(), 176));
        l.removeFromTop (12);
        buildItArea = l;

        // the pattern
        auto pa = patternArea.reduced (14, 10);
        patternHeader = pa.removeFromTop (34);
        auto h = patternHeader;
        h.removeFromLeft (130);
        play.setBounds (h.removeFromRight (40).reduced (0, 1));
        h.removeFromRight (8);
        dragArea = h.removeFromRight (176);
        h.removeFromRight (12);
        kitArea = h.removeFromRight (34 + 4 * 36);
        {
            auto ka = kitArea.withTrimmedLeft (34);
            for (auto* b : kitButtons) { b->setBounds (ka.removeFromLeft (36).reduced (2, 3)); }
        }
        h.removeFromRight (12);
        {
            // BLANK, the style's grooves and MY GROOVES share what's left (narrower together on a small window)
            // (BLANK keeps room for its word)
            std::vector<std::pair<juce::Component*, int>> chips;
            for (auto* c : grooveChips) chips.push_back ({ c, 150 });
            chips.push_back ({ &myChip, showingMine() ? 230 : 160 });
            int want = 0;
            for (auto& c : chips) want += c.second;
            const int room = juce::jmax (0, h.getWidth() - 4 * (int) chips.size());
            const double scale = want + 76 > room ? (double) juce::jmax (0, room - 64) / want : 1.0;
            blankChip.setBounds (h.removeFromLeft (scale < 1.0 ? 64 : 76).reduced (2, 3));
            h.removeFromLeft (4);
            for (auto& c : chips)
            {
                c.first->setBounds (h.removeFromLeft (juce::roundToInt (c.second * scale)).reduced (2, 3));
                h.removeFromLeft (4);
            }
        }
        editBar = pa.removeFromBottom (30);
        {
            auto eb = editBar;
            const bool mine = showingMine();
            swingArea = eb.withWidth (210);
            swingSlider.setBounds (swingArea.withTrimmedLeft (52).withTrimmedRight (44).reduced (0, 4));
            reset.setBounds (eb.removeFromRight (mine ? 112 : 84).reduced (0, 1));
            eb.removeFromRight (6);
            undo.setBounds (eb.removeFromRight (70).reduced (0, 1));
            eb.removeFromRight (6);
            addDrum.setBounds (eb.removeFromRight (108).reduced (0, 1));
            eb.removeFromRight (6);
            if (mine)
            {
                renameMine.setBounds (eb.removeFromRight (86).reduced (0, 1));
                eb.removeFromRight (6);
            }
            saveMine.setBounds (eb.removeFromRight (mine ? 124 : 166).reduced (0, 1));
            if (mine && saveChanges.isVisible())
            {
                eb.removeFromRight (6);
                saveChanges.setBounds (eb.removeFromRight (74).reduced (0, 1));
            }

            // naming a groove: the name box in place of the hint and the buttons
            auto nb = editBar;
            nameCancel.setBounds (nb.removeFromRight (90).reduced (0, 1));
            nb.removeFromRight (6);
            if (! renaming)
            {
                nameReplace.setBounds (nb.removeFromRight (206).reduced (0, 1));
                nb.removeFromRight (6);
            }
            nameOk.setBounds (nb.removeFromRight (100).reduced (0, 1));
            nb.removeFromRight (10);
            nb.removeFromLeft (juce::jmin (200, nb.getWidth() / 3));
            nameEdit.setBounds (nb.reduced (0, 1));
        }
        pa.removeFromBottom (8);
        pa.removeFromTop (10);
        grid.setBounds (pa);

        // the write-up
        auto sa = sauceArea.reduced (16, 10);
        sauceTitle = sa.removeFromTop (46);
        sa.removeFromTop (6);
        docView.setBounds (sa);
        layoutDoc();
    }

    void SaucePage::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        const auto& style = gg::styleFor (genre, subStyle);

        // ---- left: the recipe ----
        drawPanel (g, leftArea.toFloat().reduced (2.0f), 14.0f);
        auto l = leftArea.reduced (14, 12);
        drawSectionTitle (g, l.removeFromTop (20), "The recipe", p.orange);
        {
            auto f = followArea;
            const bool following = isFollowingBuild();
            g.setColour (following ? p.green : p.amber);
            g.fillEllipse (f.removeFromLeft (16).withSizeKeepingCentre (8, 8).toFloat());
            f.removeFromLeft (4);
            g.setFont (uiFont (14.0f, false));
            g.setColour (p.dim);
            juce::String text;
            if (following) text = "Showing your song's genre, style and decade from BUILD.";
            else
            {
                const auto& o = proc.options;
                text = "Reading up on another style. BUILD is on " + gg::genreName (o.genre);
                if (o.subStyle > 0) text << ": " << o.style().name;
                if (o.era >= 0) text << ", " << gg::eraName (o.era);
                text << ".";
            }
            g.drawFittedText (text, f, juce::Justification::centredLeft, 2, 0.9f);
        }

        drawSectionTitle (g, factsArea.withHeight (20), "At a glance", p.blue);
        {
            auto fa = factsArea.withTrimmedTop (26);
            auto fact = [&] (const juce::String& label, const juce::String& value)
            {
                auto line = fa.removeFromTop (26);
                g.setFont (uiFont (14.5f));
                g.setColour (p.dim);
                g.drawText (label, line.removeFromLeft (96), juce::Justification::centredLeft);
                g.setColour (p.text);
                g.drawFittedText (value, line, juce::Justification::centredLeft, 1, 0.75f);
            };
            fact ("Tempo", juce::String (style.bpm, 0) + " BPM to start from");
            fact ("Low end", gg::sauceLowEnd (style));
            fact ("Percussion", gg::saucePercussion (style));
            fact ("Known for", gg::sauceEras (style));
        }

        if (legendArea.getHeight() > 60)
        {
            auto la = legendArea;
            drawSectionTitle (g, la.removeFromTop (20), "Reading the grid", p.purple);
            la.removeFromTop (6);
            const auto& daw = gg::dawGuide (gg::chosenDaw (proc.getSettings().get ("daw")));
            juce::AttributedString as;
            as.setLineSpacing (3.0f);
            as.append ("16 steps a bar. The beats are steps 1, 5, 9 and 13, counted 1 e & a. Brighter = harder hits, faded = soft "
                       "ghost notes, split steps = rolls.\n\nDrag the pattern into your DAW as MIDI. The notes match every Groove Grabber kit: ",
                       uiFont (14.0f, false), p.dim);
            as.append ("kick " + gg::dawNoteName (36, daw) + ", snare " + gg::dawNoteName (38, daw) + ", closed hat " + gg::dawNoteName (42, daw)
                       + ", 808 " + gg::dawNoteName (52, daw), uiFont (14.0f), p.text);
            as.append (daw.id != "other" ? " (as " + daw.name + " names them)." : ".", uiFont (14.0f, false), p.dim);
            as.draw (g, la.toFloat());
        }

        if (buildItArea.getHeight() > 120)
        {
            auto ba = buildItArea;
            drawSectionTitle (g, ba.removeFromTop (20), "Build it in your DAW", p.green);
            ba.removeFromTop (8);
            const auto& daw = gg::dawGuide (gg::chosenDaw (proc.getSettings().get ("daw")));
            const juce::StringArray steps {
                "Put a kit on a drum track: Groove Grabber itself, or your exported kit in " + daw.sampler + ".",
                dropHint(),
                "Set the tempo and swing from the table under THE SAUCE.",
                "Shape the velocities: a strong backbeat, softer hats and ghost notes.",
                "Copy it for the hook and make the changes from Fills and changes." };
            for (int i = 0; i < steps.size() && ba.getHeight() > 20; ++i)
            {
                juce::AttributedString as;
                as.setLineSpacing (2.0f);
                as.append (juce::String (i + 1) + ".  ", uiFont (14.0f), p.green);
                as.append (steps[i], uiFont (14.0f, false), p.dim);
                juce::TextLayout tl;
                tl.createLayout (as, (float) ba.getWidth());
                const int h = (int) std::ceil (tl.getHeight());
                tl.draw (g, ba.removeFromTop (h).toFloat());
                ba.removeFromTop (6);
            }
        }

        // ---- the pattern ----
        drawPanel (g, patternArea.toFloat().reduced (2.0f), 14.0f);
        {
            auto h = patternHeader;
            drawSectionTitle (g, h.removeFromLeft (130).withSizeKeepingCentre (130, 20), "The pattern", p.purple);
            if (grooveIds.isEmpty())
            {
                g.setColour (p.faint);
                g.setFont (uiFont (15.0f, false));
                g.drawText ("No pattern for this style yet", patternHeader.withTrimmedLeft (140), juce::Justification::centredLeft);
            }
            g.setColour (p.dim);
            g.setFont (headFont (10.0f).withExtraKerningFactor (0.12f));
            g.drawText ("KIT", kitArea.withWidth (32), juce::Justification::centredRight);
            // the drag handle
            auto d = dragArea.toFloat().reduced (0.0f, 1.0f);
            g.setColour (dragging ? p.orange.withAlpha (0.2f) : p.sunken);
            g.fillRoundedRectangle (d, 8.0f);
            g.setColour (p.orange.withAlpha (0.8f));
            g.drawRoundedRectangle (d, 8.0f, 1.2f);
            auto in = d.reduced (8.0f, 2.0f);
            drawIcon (g, Icon::drag, in.removeFromLeft (24.0f).withSizeKeepingCentre (20.0f, 20.0f), p.orange);
            in.removeFromLeft (6.0f);
            g.setColour (p.text);
            g.setFont (headFont (11.0f).withExtraKerningFactor (0.08f));
            g.drawText ("DRAG MIDI", in.removeFromTop (in.getHeight() * 0.55f), juce::Justification::bottomLeft);
            g.setColour (p.dim);
            g.setFont (uiFont (12.5f, false));
            const auto cg = currentGroove();
            g.drawText ("4 bars, " + juce::String (previewTempo(), 0) + " BPM" + (cg != nullptr ? ", swing " + juce::String (gg::swingPercent (*cg)) + "%" : juce::String()),
                        in, juce::Justification::topLeft);
        }
        if (naming)
        {
            g.setColour (p.blue);
            g.setFont (headFont (11.0f).withExtraKerningFactor (0.1f));
            g.drawText (renaming ? "NEW NAME FOR THIS GROOVE" : "NAME YOUR GROOVE", editBar.withRight (nameEdit.getX() - 10), juce::Justification::centredLeft);
        }
        else
        {
            // SWING, at the left
            g.setColour (p.dim);
            g.setFont (headFont (10.0f).withExtraKerningFactor (0.12f));
            g.drawText ("SWING", swingArea.withWidth (50), juce::Justification::centredLeft);
            g.setColour (p.text);
            g.setFont (uiFont (14.5f));
            g.drawText (juce::String ((int) swingSlider.getValue()) + "%", swingArea.withTrimmedLeft (swingArea.getWidth() - 42), juce::Justification::centredRight);
            auto eb = editBar.withRight ((saveChanges.isVisible() ? saveChanges.getX() : saveMine.getX()) - 12).withTrimmedLeft (swingArea.getWidth() + 14);
            const auto id = currentId();
            if (showingMine())
            {
                // your groove: saved as it is, or with changes waiting for SAVE
                const bool changed = proc.isGrooveEdited (id);
                const auto colour = changed ? p.orange : p.blue;
                auto tag = eb.removeFromLeft (96).withSizeKeepingCentre (96, 22).toFloat();
                g.setColour (colour.withAlpha (0.18f));
                g.fillRoundedRectangle (tag, 6.0f);
                g.setColour (colour);
                g.drawRoundedRectangle (tag, 6.0f, 1.0f);
                g.setFont (headFont (10.5f).withExtraKerningFactor (0.1f));
                g.drawText (changed ? "CHANGED" : "MY GROOVE", tag, juce::Justification::centred);
                eb.removeFromLeft (10);
                if (eb.getWidth() > 150)
                {
                    g.setColour (p.dim);
                    g.setFont (uiFont (14.0f, false));
                    g.drawFittedText (changed ? "Not saved yet. SAVE puts it into this groove, SAVE AS COPY makes a new one, REVERT drops it."
                                              : "Saved. Changes wait here until you press SAVE or SAVE AS COPY.",
                                      eb, juce::Justification::centredLeft, 2, 0.8f);
                }
            }
            else if (proc.isGrooveEdited (id))
            {
                auto tag = eb.removeFromLeft (70).withSizeKeepingCentre (70, 22).toFloat();
                g.setColour (p.orange.withAlpha (0.18f));
                g.fillRoundedRectangle (tag, 6.0f);
                g.setColour (p.orange);
                g.drawRoundedRectangle (tag, 6.0f, 1.0f);
                g.setFont (headFont (10.5f).withExtraKerningFactor (0.1f));
                g.drawText ("EDITED", tag, juce::Justification::centred);
                eb.removeFromLeft (10);
                g.setColour (p.dim);
                g.setFont (uiFont (14.0f, false));
                g.drawFittedText ("Your version is saved with this song. BUILD's preview and the export's Groove MIDI use it.",
                                  eb, juce::Justification::centredLeft, 1, 0.8f);
            }
            else if (! id.isEmpty() && eb.getWidth() > 200)
            {
                juce::AttributedString as;
                as.setLineSpacing (1.0f);
                const auto key = uiFont (13.5f), plainText = uiFont (13.5f, false);
                as.append ("Click", key, p.text);
                as.append (" a step: add / take away (drag for several).   ", plainText, p.dim);
                as.append ("Right-click", key, p.text);
                as.append (": hard, medium, soft.   ", plainText, p.dim);
                as.append ("Ctrl-click", key, p.text);
                as.append (": 2 quick hits, again for 3 (a triplet roll).", plainText, p.dim);
                if (proc.kits.empty()) as.append ("   Generate kits on BUILD to hear it with your sounds.", plainText, p.faint);
                as.setJustification (juce::Justification::centredLeft);
                as.draw (g, eb.toFloat());
            }
        }

        // ---- the write-up ----
        drawPanel (g, sauceArea.toFloat().reduced (2.0f), 14.0f);
        {
            auto t = sauceTitle;
            auto top = t.removeFromTop (26);
            drawSectionTitle (g, top.removeFromLeft (110).withSizeKeepingCentre (110, 20), "The sauce", p.orange);
            g.setColour (p.text);
            g.setFont (headFont (17.0f).withExtraKerningFactor (0.04f));
            g.drawFittedText ((subStyle == 0 ? gg::genreName (genre) : style.name).toUpperCase(), top, juce::Justification::centredLeft, 1, 0.7f);
            g.setColour (p.dim);
            g.setFont (uiFont (14.5f, false));
            juce::String sub = gg::genreName (genre);
            if (subStyle > 0) sub << "  -  " << style.name;
            sub << "  -  " << (era >= 0 ? gg::eraName (era) : juce::String ("any decade (pick one for how drums sounded then)"));
            g.drawFittedText (sub, t.withTrimmedLeft (110), juce::Justification::centredLeft, 1, 0.8f);
        }
    }
}
