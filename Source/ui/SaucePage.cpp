#include "SaucePage.h"
#include "core/Daws.h"
#include "core/Settings.h"

namespace ggui
{
    //==============================================================================
    void PatternGrid::setGroove (const gg::Groove* g)
    {
        groove = g;
        lanes.clear();
        if (g != nullptr)
        {
            for (const auto& h : g->hits)
                if (std::find (lanes.begin(), lanes.end(), h.slot) == lanes.end()) lanes.push_back (h.slot);
            std::sort (lanes.begin(), lanes.end(), [] (gg::Slot a, gg::Slot b) { return (int) a < (int) b; });
        }
        repaint();
    }

    void PatternGrid::setPlayhead (double beat)
    {
        if (std::abs (beat - playhead) < 1.0e-4) return;
        playhead = beat;
        repaint();
    }

    void PatternGrid::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        if (groove == nullptr || lanes.empty()) return;

        auto r = getLocalBounds().toFloat();
        const float labelW = juce::jlimit (96.0f, 150.0f, r.getWidth() * 0.12f);
        auto header = r.removeFromTop (22.0f);
        const int steps = groove->bars * 16;
        const float gridX = r.getX() + labelW;
        const float stepW = (r.getWidth() - labelW) / (float) steps;
        const float laneH = juce::jmin (34.0f, r.getHeight() / (float) lanes.size());
        const float gridH = laneH * (float) lanes.size();

        // beats shaded in turn, bars split by a stronger line
        for (int beat = 0; beat < groove->bars * 4; ++beat)
        {
            const float x = gridX + (float) beat * 4.0f * stepW;
            if (beat % 2 == 1)
            {
                g.setColour (p.text.withAlpha (p.light ? 0.035f : 0.03f));
                g.fillRect (juce::Rectangle<float> (x, r.getY(), stepW * 4.0f, gridH));
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
                    juce::Rectangle<float> (r.getX(), header.getY(), labelW - 8.0f, header.getHeight()), juce::Justification::centredLeft);

        for (int i = 0; i <= steps; ++i)
        {
            const float x = gridX + (float) i * stepW;
            const bool bar = i % 16 == 0, beat = i % 4 == 0;
            g.setColour (bar ? p.dim.withAlpha (0.8f) : beat ? p.line.brighter (0.2f) : p.line.withAlpha (0.45f));
            g.fillRect (juce::Rectangle<float> (x - (bar ? 1.0f : 0.5f), r.getY(), bar ? 2.0f : 1.0f, gridH));
        }

        for (size_t li = 0; li < lanes.size(); ++li)
        {
            const auto slot = lanes[li];
            const auto colour = slotColour (slot);
            const juce::Rectangle<float> lane (r.getX(), r.getY() + (float) li * laneH, r.getWidth(), laneH);
            g.setColour (p.line.withAlpha (0.5f));
            g.fillRect (lane.withHeight (1.0f).withY (lane.getBottom() - 0.5f));

            // the lane's name, and the note it plays on in your DAW
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

            // empty steps
            for (int i = 0; i < steps; ++i)
            {
                auto cell = juce::Rectangle<float> (gridX + (float) i * stepW, lane.getY(), stepW, laneH).reduced (juce::jmin (3.0f, stepW * 0.12f), juce::jmin (4.0f, laneH * 0.14f));
                g.setColour (p.text.withAlpha (p.light ? 0.05f : 0.035f));
                g.fillRoundedRectangle (cell, 3.0f);
            }

            // hits: a full step each, or a slice of it for rolls; harder hits brighter
            std::vector<const gg::GrooveHit*> mine;
            for (const auto& h : groove->hits)
                if (h.slot == slot) mine.push_back (&h);
            for (size_t k = 0; k < mine.size(); ++k)
            {
                const auto& h = *mine[k];
                double len = 0.25;
                if (k + 1 < mine.size()) len = juce::jmin (len, mine[k + 1]->beat - h.beat);
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

        if (playhead >= 0)
        {
            const float x = gridX + (float) (std::fmod (playhead, groove->lengthBeats()) * 4.0) * stepW;
            g.setColour (p.orange);
            g.fillRect (juce::Rectangle<float> (x - 1.0f, r.getY(), 2.0f, gridH));
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

    void SaucePage::refresh()
    {
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
            return;
        }
        fillBoxes();
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
        const auto& style = gg::styleFor (genre, subStyle);
        grooves = gg::sauceGrooves (style);
        grooveChips.clear();
        for (int i = 0; i < (int) grooves.size(); ++i)
        {
            auto* b = grooveChips.add (new juce::TextButton (grooves[(size_t) i]->name.toUpperCase()));
            b->getProperties().set ("style", "toggle");
            b->setTooltip ("Show the " + grooves[(size_t) i]->name + " pattern");
            b->onClick = [this, i] { pickGroove (i); };
            addAndMakeVisible (b);
        }
        pickGroove (juce::jlimit (0, juce::jmax (0, (int) grooves.size() - 1), grooveIndex));

        doc.setMarkdown (gg::sauceMarkdown (genre, subStyle, era));
        resized();
        docView.setViewPosition (0, 0);
        repaint();
    }

    void SaucePage::pickGroove (int index)
    {
        grooveIndex = index;
        for (int i = 0; i < grooveChips.size(); ++i) grooveChips[i]->setToggleState (i == index, juce::dontSendNotification);
        const auto* g = juce::isPositiveAndBelow (index, (int) grooves.size()) ? grooves[(size_t) index] : nullptr;
        grid.setGroove (g);
        if (playingHere && g != nullptr)
        {
            proc.player.setGroove (g);
            proc.player.setSwing (g->swing);
            proc.player.setTempo (previewTempo());
        }
        repaint (patternHeader);
    }

    const gg::Groove* SaucePage::currentGroove() const
    {
        return juce::isPositiveAndBelow (grooveIndex, (int) grooves.size()) ? grooves[(size_t) grooveIndex] : nullptr;
    }

    double SaucePage::previewTempo() const
    {
        // your song's tempo while it's your song's style; each pattern's own tempo when you're reading up on another
        // (some styles borrow a groove that's played at its own speed: a boom bap groove in country rap)
        if (isFollowingBuild())
            return playingHere ? savedTempo : (proc.options.bpm > 0 ? proc.options.bpm : proc.player.getTempo());
        if (const auto* g = currentGroove()) return g->bpm;
        return gg::styleFor (genre, subStyle).bpm;
    }

    void SaucePage::togglePlay()
    {
        if (playingHere) { stopPreview(); return; }
        const auto* g = currentGroove();
        if (g == nullptr) return;
        savedGroove = proc.player.getGroove();
        savedSwing = proc.player.getSwing();
        savedTempo = proc.player.getTempo();
        playingHere = true;
        proc.player.setGroove (g);
        proc.player.setSwing (g->swing);
        proc.player.setTempo (previewTempo());
        proc.player.setGroovePlaying (true);
        play.setToggleState (true, juce::dontSendNotification);
        startTimerHz (30);
        repaint();
    }

    void SaucePage::stopPreview()
    {
        if (! playingHere) return;
        playingHere = false;
        proc.player.setGroovePlaying (false);
        if (savedGroove != nullptr) proc.player.setGroove (savedGroove);
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
        const auto* g = currentGroove();
        if (g == nullptr) return {};
        const auto& style = gg::styleFor (genre, subStyle);
        const double bpm = previewTempo();
        const auto styleName = subStyle == 0 ? gg::genreName (genre) : style.name;
        const auto folder = gg::paths::documentsDir().getChildFile ("Sauce MIDI");
        folder.createDirectory();
        const auto f = folder.getChildFile (gg::paths::safeFileName (styleName + " - " + g->name + " " + juce::String (bpm, 0) + " BPM", 80) + ".mid");
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
        patternArea = r.removeFromTop (juce::jlimit (250, 330, r.getHeight() * 2 / 5));
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
        for (auto* c : grooveChips) { c->setBounds (h.removeFromLeft (juce::jmin (168, h.getWidth())).reduced (2, 3)); h.removeFromLeft (4); }
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
                "Drag the MIDI in, or copy the grid into your piano roll or step sequencer.",
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
            if (grooves.empty())
            {
                g.setColour (p.faint);
                g.setFont (uiFont (15.0f, false));
                g.drawText ("No pattern for this style yet", patternHeader.withTrimmedLeft (140), juce::Justification::centredLeft);
            }
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
            g.drawText ("4 bars at " + juce::String (previewTempo(), 0) + " BPM", in, juce::Justification::topLeft);
        }
        if (proc.kits.empty() && ! grooves.empty())
        {
            g.setColour (p.faint);
            g.setFont (uiFont (13.5f, false));
            g.drawText ("Generate kits on BUILD to hear it with your sounds", patternArea.reduced (18, 6).removeFromBottom (18),
                        juce::Justification::centredRight);
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
