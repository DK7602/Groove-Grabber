#include "BuildPage.h"
#include "core/MyGrooves.h"

namespace ggui
{
    //==============================================================================
    SongPanel::SongPanel (GrooveGrabberProcessor& p) : proc (p)
    {
        description.setMultiLine (true, true);
        description.setReturnKeyStartsNewLine (false);
        description.setFont (bodyFont (16.5f));
        description.setTextToShowWhenEmpty ("Describe your song...  e.g. \"dark 90s boom bap, punchy but warm, 88 BPM in A minor\"  or  \"like Dr. Dre\", \"Bon Jovi drums\"",
                                            pal().faint);
        description.setBorder (juce::BorderSize<int> (8, 10, 8, 10));
        description.onReturnKey = [this] { syncDescription(); proc.generate (false); };
        description.onFocusLost = [this] { if (description.getText() != proc.description) proc.applyDescription (description.getText()); };
        description.onTextChange = [this] { proc.applyDescription (description.getText()); };
        description.setTooltip ("Genre, style, decade, feel, tempo and key in your own words. Name an artist or a song too "
                                "(\"like Dr. Dre\", \"Metro Boomin type beat\", \"Still D.R.E.\", \"Morgan Wallen\"): kit 1 takes their sound, "
                                "inside the genre you pick.");
        addAndMakeVisible (description);

        for (int i = 0; i < gg::numGenres; ++i) genre.addItem (gg::genreName ((gg::Genre) i), i + 1);
        genre.onChange = [this] { if (! updating) proc.setGenre ((gg::Genre) (genre.getSelectedId() - 1)); };
        genre.setTooltip ("The genre");
        addAndMakeVisible (genre);

        styleBox.onChange = [this] { if (! updating && styleBox.getSelectedId() > 0) proc.setStyle (styleBox.getSelectedId() - 1); };
        styleBox.setTooltip ("The style within the genre: picks the four kit directions, the tempo and the preview groove "
                             "(type it too: \"g-funk\", \"uk drill\", \"honky tonk\", \"pop rock\"...)");
        addAndMakeVisible (styleBox);

        era.addItem ("Any era", 1);
        for (int e = 0; e < gg::numEras; ++e) era.addItem (gg::eraName (e), e + 2);
        era.onChange = [this] { if (! updating) proc.setEra (era.getSelectedId() - 2); };
        era.setTooltip ("The decade: 70s = dry, warm, live; 80s = gated rooms and drum machines; 90s = punchy samplers; "
                        "today = clean, hard, polished. Type it too (\"80s pop rock\").");
        addAndMakeVisible (era);

        static const char* tips[] = {
            "Punch: sharp, fast attacks vs soft ones", "Warmth: full low-mids vs thin", "Brightness: crisp top end vs dark",
            "Room: long, roomy sounds vs short and dry", "Vintage: dusty, vinyl, sampler grit vs clean and modern",
            "Aggression: hard and compressed vs gentle", "Live feel: real drums vs programmed / drum machine" };
        for (int i = 0; i < gg::Traits::num; ++i)
        {
            auto* s = traits.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
            s->setRange (0.0, 100.0, 1.0);
            s->setTooltip (tips[i]);
            s->setDoubleClickReturnValue (true, 50.0);
            s->onValueChange = [this] { pushOptions(); };
            addAndMakeVisible (s);
        }

        feel.setTooltips ({ "Let the genre decide", "Real, acoustic drums", "A mix of both", "Drum machines and programmed sounds" });
        lowEnd.setTooltips ({ "Let the genre decide", "Just a kick", "A kick and an 808" });
        perc.setTooltips ({ "Let the genre decide", "No percussion", "One percussion sound", "Two", "Three" });
        for (auto* s : { &feel, &lowEnd, &perc })
        {
            s->onChange = [this] (int) { pushOptions(); };
            addAndMakeVisible (*s);
        }
        for (auto* t : { &fx, &tuneKey })
        {
            t->onClick = [this] { pushOptions(); };
            addAndMakeVisible (*t);
        }
        fx.setTooltip ("Add two FX pads: risers, impacts, sweeps...");
        tuneKey.setTooltip ("Tune the kick and 808 to the song's root and the toms to its scale (needs a key)");

        for (int i = 0; i < (int) gg::Source::count; ++i)
        {
            auto* t = sources.add (new juce::ToggleButton (gg::sourceName ((gg::Source) i)));
            t->onClick = [this] { pushOptions(); };
            addAndMakeVisible (t);
        }

        generate.getProperties().set ("style", "primary");
        generate.setTooltip ("Build four kits for this song (locked pads stay)");
        generate.onClick = [this] { syncDescription(); proc.generate (false); };
        addAndMakeVisible (generate);
        surprise.setTooltip ("Surprise me: four wilder kits");
        surprise.setFilled (true);
        surprise.setAccent (pal().purple);
        surprise.onClick = [this] { syncDescription(); proc.generate (true); };
        addAndMakeVisible (surprise);
    }

    void SongPanel::syncDescription()
    {
        // only when the text changed: never undo slider tweaks made after typing
        if (description.getText() != proc.description)
            proc.applyDescription (description.getText());
    }

    void SongPanel::pushOptions()
    {
        if (updating) return;
        auto& o = proc.options;
        for (int i = 0; i < gg::Traits::num; ++i) o.character[i] = (float) traits[i]->getValue() / 100.0f;
        o.feel = feel.getSelected();
        o.lowEnd = lowEnd.getSelected() - 1;
        o.percCount = perc.getSelected() - 1;
        o.fx = fx.getToggleState();
        for (int i = 0; i < sources.size(); ++i) o.sources[(size_t) i] = sources[i]->getToggleState();
        if (o.tuneToKey != tuneKey.getToggleState())
        {
            o.tuneToKey = tuneKey.getToggleState();
            for (auto& k : proc.kits) k.tuneToKey = o.tuneToKey;
            proc.kitChanged (proc.activeKit);     // re-render with / without the key tuning
        }
        else
            proc.sendChangeMessage();
    }

    void SongPanel::refresh()
    {
        updating = true;
        const auto& o = proc.options;
        if (! description.hasKeyboardFocus (true) && description.getText() != proc.description)
            description.setText (proc.description, false);
        description.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        description.setColour (juce::TextEditor::textColourId, pal().text);
        description.applyColourToAllText (pal().text);
        description.setTextToShowWhenEmpty ("Describe your song...  e.g. \"dark 90s boom bap, punchy but warm, 88 BPM in A minor\"  or  \"like Dr. Dre\", \"Bon Jovi drums\"", pal().faint);
        genre.setSelectedId ((int) o.genre + 1, juce::dontSendNotification);
        if (stylesShownFor != o.genre)
        {
            stylesShownFor = o.genre;
            styleBox.clear (juce::dontSendNotification);
            const auto& list = gg::stylesOf (o.genre);
            for (int i = 0; i < (int) list.size(); ++i)
                styleBox.addItem (i == 0 ? "General " + gg::genreName (o.genre) : list[(size_t) i].name, i + 1);
        }
        styleBox.setSelectedId (o.subStyle + 1, juce::dontSendNotification);
        era.setSelectedId (o.era + 2, juce::dontSendNotification);
        const juce::Colour accents[] = { pal().orange, pal().orange, pal().amber, pal().blue, pal().purple, pal().red, pal().green };
        for (int i = 0; i < traits.size(); ++i)
        {
            traits[i]->setValue (o.character[i] * 100.0, juce::dontSendNotification);
            traits[i]->getProperties().set ("accent", accents[i].toString());
            traits[i]->repaint();
        }
        feel.setSelected (o.feel);
        lowEnd.setSelected (o.lowEnd + 1);
        perc.setSelected (o.percCount + 1);
        fx.setToggleState (o.fx, juce::dontSendNotification);
        tuneKey.setToggleState (o.tuneToKey, juce::dontSendNotification);
        tuneKey.setEnabled (o.key >= 0);
        const auto counts = [&]
        {
            std::array<int, (size_t) gg::Source::count> c {};
            for (auto& s : *proc.library->getSounds()) if (s.isUsable()) ++c[(size_t) s.source];
            return c;
        }();
        for (int i = 0; i < sources.size(); ++i)
        {
            sources[i]->setToggleState (o.sources[(size_t) i], juce::dontSendNotification);
            sources[i]->setButtonText (gg::sourceName ((gg::Source) i) + "  (" + juce::String (counts[(size_t) i]) + ")");
        }
        surprise.setAccent (pal().purple);
        updating = false;
        repaint();
    }

    void SongPanel::resized()
    {
        auto r = getLocalBounds().reduced (14, 12);
        r.removeFromTop (24);
        description.setBounds (r.removeFromTop (getHeight() < 760 ? 62 : 86));
        understoodArea = r.removeFromTop (24);
        r.removeFromTop (4);
        {
            auto row = r.removeFromTop (30);
            era.setBounds (row.removeFromRight (row.getWidth() * 40 / 100));
            row.removeFromRight (6);
            genre.setBounds (row);
        }
        r.removeFromTop (6);
        styleBox.setBounds (r.removeFromTop (30));
        r.removeFromTop (10);

        auto bottom = r.removeFromBottom (48);
        surprise.setBounds (bottom.removeFromRight (48));
        bottom.removeFromRight (8);
        generate.setBounds (bottom);
        r.removeFromBottom (10);

        sourcesArea = r.removeFromBottom (22 + 22 * 3);
        {
            auto a = sourcesArea.withTrimmedTop (22);
            for (int i = 0; i < sources.size(); ++i)
            {
                const int col = i % 2, row = i / 2;
                sources[i]->setBounds (a.getX() + col * (a.getWidth() / 2), a.getY() + row * 22, a.getWidth() / 2 - 4, 22);
            }
        }
        r.removeFromBottom (8);

        optionsArea = r.removeFromBottom (22 + 3 * 30 + 2 * 26);
        auto oa = optionsArea.withTrimmedTop (22);
        auto row = [&oa] (int h) { auto x = oa.removeFromTop (h); oa.removeFromTop (4); return x; };
        auto feelRow = row (26);   feelRow.removeFromLeft (64);   feel.setBounds (feelRow);
        auto lowRow = row (26);    lowRow.removeFromLeft (64);    lowEnd.setBounds (lowRow);
        auto percRow = row (26);   percRow.removeFromLeft (64);   perc.setBounds (percRow);
        fx.setBounds (row (22));
        tuneKey.setBounds (oa.removeFromTop (22));

        traitsArea = r.withTrimmedBottom (6);
        auto ta = traitsArea.withTrimmedTop (22);
        const int rowH = juce::jlimit (12, 34, ta.getHeight() / gg::Traits::num);
        for (auto* s : traits)
        {
            auto line = ta.removeFromTop (rowH);
            line.removeFromLeft (84);
            line.removeFromRight (34);
            s->setBounds (line);
        }
    }

    void SongPanel::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        drawPanel (g, getLocalBounds().toFloat().reduced (2.0f), 14.0f);
        auto r = getLocalBounds().reduced (14, 12);
        drawSectionTitle (g, r.removeFromTop (20), "Your song", p.orange);

        // what the description was understood as
        {
            juce::String u;
            if (proc.understood.understood.size() > 0) u = proc.understood.understood.joinIntoString ("  -  ");
            else u = proc.description.isEmpty() ? "Type a genre, a feel, a tempo or a key" : "Using " + gg::genreName (proc.options.genre);
            if (proc.songName.isNotEmpty()) u = "Song: " + proc.songName + "  -  " + u;
            g.setColour (proc.understood.understood.size() > 0 ? p.blue : p.faint);
            g.setFont (uiFont (13.5f, false));
            g.drawFittedText (u, understoodArea, juce::Justification::centredLeft, 1, 0.7f);
        }

        drawSectionTitle (g, traitsArea.withHeight (20), "Kit character", p.blue);
        {
            auto ta = traitsArea.withTrimmedTop (22);
            const int rowH = juce::jlimit (12, 34, ta.getHeight() / gg::Traits::num);
            for (int i = 0; i < traits.size(); ++i)
            {
                auto line = ta.removeFromTop (rowH);
                g.setColour (p.dim);
                g.setFont (uiFont (14.5f));
                g.drawText (gg::Traits::name (i), line.removeFromLeft (82), juce::Justification::centredLeft);
                g.setColour (p.text);
                g.drawText (juce::String (juce::roundToInt (traits[i]->getValue())), line.removeFromRight (30), juce::Justification::centredRight);
            }
        }

        drawSectionTitle (g, optionsArea.withHeight (20), "Options", p.purple);
        {
            auto oa = optionsArea.withTrimmedTop (22);
            g.setColour (p.dim);
            g.setFont (uiFont (14.0f));
            for (auto* label : { "FEEL", "LOW END", "PERC" })
            {
                g.drawText (label, oa.removeFromTop (26).removeFromLeft (64), juce::Justification::centredLeft);
                oa.removeFromTop (4);
            }
        }
        drawSectionTitle (g, sourcesArea.withHeight (20), "Use sounds from", p.green);
    }

    //==============================================================================
    TransportBar::TransportBar (GrooveGrabberProcessor& p) : proc (p)
    {
        play.setAccent (pal().orange);
        play.setFilled (true);
        play.setTooltip ("Play / stop the groove with the selected kit (Space)");
        play.onClick = [this]
        {
            proc.player.setGroovePlaying (! proc.player.isGroovePlaying());
            refresh();
            proc.sendChangeMessage();
        };
        addAndMakeVisible (play);

        fillGrooves();
        groove.onChange = [this]
        {
            const int sel = groove.getSelectedId();
            if (sel >= 1000)
            {
                if (juce::isPositiveAndBelow (sel - 1000, myIds.size())) proc.setGrooveId (myIds[sel - 1000]);
                return;
            }
            const int i = sel - 1;
            if (juce::isPositiveAndBelow (i, (int) gg::allGrooves().size())) proc.setGrooveId (gg::allGrooves()[(size_t) i].id);
        };
        groove.setTooltip ("Preview groove (plays at the song's BPM). Edit it on SAUCE.");
        addAndMakeVisible (groove);

        swing.setSliderStyle (juce::Slider::LinearHorizontal);
        swing.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        swing.setRange (0.0, 70.0, 1.0);
        swing.setTooltip ("Swing");
        swing.onValueChange = [this]
        {
            proc.player.setSwing ((float) swing.getValue() / 100.0f);
            swing.setTooltip ("Swing: " + juce::String (juce::roundToInt (50.0 + swing.getValue() / 3.0)) + "% (50% = straight, 66% = a full triplet feel)");
        };
        swing.getProperties().set ("accent", pal().purple.toString());
        addAndMakeVisible (swing);

        volume.setSliderStyle (juce::Slider::LinearHorizontal);
        volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volume.setRange (0.0, 150.0, 1.0);
        volume.setTooltip ("Volume");
        volume.onValueChange = [this] { proc.player.setVolume ((float) volume.getValue() / 100.0f); };
        addAndMakeVisible (volume);

        follow.setTooltip ("While your DAW plays, lock the groove to its timeline so you hear the kit inside your song");
        follow.onClick = [this] { proc.player.setFollowHost (follow.getToggleState()); };
        addAndMakeVisible (follow);
        follow.setVisible (! proc.isStandalone());
        addAndMakeVisible (toast);
        startTimerHz (20);
    }

    void TransportBar::fillGrooves()
    {
        const auto mine = gg::mygrooves::list();
        juce::String sig;
        for (const auto& e : mine) sig << e.groove->id << ":" << e.groove->name << ":" << e.groove->bpm << ";";
        if (sig == grooveListSignature && groove.getNumItems() > 0) return;
        grooveListSignature = sig;
        groove.clear (juce::dontSendNotification);
        int id = 1;
        for (auto& gr : gg::allGrooves()) groove.addItem (gr.name + "   (" + juce::String (gr.bpm, 0) + ")", id++);
        myIds.clear();
        if (! mine.empty())
        {
            groove.addSectionHeading ("MY GROOVES");
            for (int i = 0; i < (int) mine.size(); ++i)
            {
                groove.addItem (mine[(size_t) i].groove->name + "   (" + juce::String (mine[(size_t) i].groove->bpm, 0) + ")", 1000 + i);
                myIds.add (mine[(size_t) i].groove->id);
            }
        }
    }

    void TransportBar::refresh()
    {
        fillGrooves();
        if (const int mi = myIds.indexOf (proc.grooveId); mi >= 0) groove.setSelectedId (1000 + mi, juce::dontSendNotification);
        play.setToggleState (proc.player.isGroovePlaying(), juce::dontSendNotification);
        play.setAccent (pal().orange);
        for (size_t i = 0; i < gg::allGrooves().size(); ++i)
        {
            const auto& gr = gg::allGrooves()[i];
            const auto text = gr.name + "   (" + juce::String (gr.bpm, 0) + ")" + (proc.isGrooveEdited (gr.id) ? "  - edited" : "");
            if (groove.getItemText ((int) i) != text) groove.changeItemText ((int) i + 1, text);
            if (gr.id == proc.grooveId) groove.setSelectedId ((int) i + 1, juce::dontSendNotification);
        }
        swing.setValue (proc.player.getSwing() * 100.0, juce::dontSendNotification);
        swing.setTooltip ("Swing: " + juce::String (juce::roundToInt (50.0 + proc.player.getSwing() * 100.0 / 3.0)) + "% (50% = straight, 66% = a full triplet feel)");
        volume.setValue (proc.player.getVolume() * 100.0, juce::dontSendNotification);
        follow.setToggleState (proc.player.isFollowingHost(), juce::dontSendNotification);
        swing.getProperties().set ("accent", pal().purple.toString());
        volume.getProperties().set ("accent", pal().blue.toString());
        repaint();
    }

    void TransportBar::timerCallback()
    {
        const double beat = proc.player.getGrooveBeat();
        if (proc.player.isGroovePlaying() && std::abs (beat - lastBeat) > 0.01) { lastBeat = beat; repaint (getLocalBounds().removeFromTop (4)); }
        if (play.getToggleState() != proc.player.isGroovePlaying()) refresh();
    }

    void TransportBar::resized()
    {
        auto r = getLocalBounds().reduced (12, 8);
        play.setBounds (r.removeFromLeft (44));
        r.removeFromLeft (12);
        const bool narrow = getWidth() < 1000;
        groove.setBounds (r.removeFromLeft (narrow ? 170 : 200).withSizeKeepingCentre (narrow ? 170 : 200, 30));
        r.removeFromLeft (16);
        auto sw = r.removeFromLeft (narrow ? 140 : 170);
        sw.removeFromLeft (52);
        swing.setBounds (sw);
        r.removeFromLeft (14);
        if (follow.isVisible()) { follow.setBounds (r.removeFromLeft (130)); r.removeFromLeft (10); }
        auto vol = r.removeFromRight (narrow ? 140 : 180);
        vol.removeFromLeft (40);
        volume.setBounds (vol);
        r.removeFromRight (12);
        toast.setBounds (r);
    }

    void TransportBar::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        drawPanel (g, getLocalBounds().toFloat().reduced (2.0f), 12.0f);
        // where the groove is
        if (proc.player.isGroovePlaying())
            if (auto* gr = proc.player.getGroove())
            {
                const double pos = std::fmod (proc.player.getGrooveBeat(), gr->lengthBeats()) / gr->lengthBeats();
                auto bar = getLocalBounds().reduced (14, 0).removeFromTop (3).toFloat();
                g.setColour (p.line);
                g.fillRect (bar);
                g.setGradientFill (brandGradient (bar));
                g.fillRect (bar.withWidth (bar.getWidth() * (float) juce::jlimit (0.0, 1.0, pos)));
            }
        g.setColour (p.dim);
        g.setFont (uiFont (14.0f));
        g.drawText ("SWING", swing.getBounds().withX (swing.getX() - 52).withWidth (50), juce::Justification::centredLeft);
        g.setColour (p.dim);
        g.drawText ("VOL", volume.getBounds().withX (volume.getX() - 40).withWidth (38), juce::Justification::centredLeft);
        if (proc.player.isHostDriving())
        {
            g.setColour (p.green);
            g.setFont (uiFont (12.0f));
            g.drawText ("LOCKED TO DAW", follow.getBounds().withY (follow.getBottom() - 4).withHeight (12), juce::Justification::centredLeft);
        }
    }

    //==============================================================================
    BuildPage::BuildPage (GrooveGrabberProcessor& p) : proc (p), song (p), grid (p), detail (p), transport (p)
    {
        addAndMakeVisible (song);
        for (int i = 0; i < 4; ++i) addAndMakeVisible (cards.add (new KitCard (proc, i)));
        addAndMakeVisible (grid);
        addAndMakeVisible (detail);
        addAndMakeVisible (transport);
        grid.onSelect = [this] { detail.refresh(); };

        for (auto* b : { &regenerate, &lockAll, &saveKit })
        {
            b->getProperties().set ("style", "ghost");
            addAndMakeVisible (*b);
        }
        exportKit.getProperties().set ("style", "accent");
        addAndMakeVisible (exportKit);
        regenerate.setTooltip ("New sounds for every unlocked pad of this kit");
        lockAll.setTooltip ("Lock / unlock every pad of this kit");
        saveKit.setTooltip ("Save this kit to MY KITS");
        exportKit.setTooltip ("Export this kit for your DAW's drum sampler (numbered WAVs, MPC program, groove MIDI)");
        regenerate.onClick = [this] { proc.regenerateKit (proc.activeKit, false); };
        lockAll.onClick = [this]
        {
            if (auto* k = proc.getActiveKit())
            {
                bool anyUnlocked = false;
                for (auto& s : k->slots) if (! s.isEmpty() && ! s.locked) anyUnlocked = true;
                proc.setAllLocked (proc.activeKit, anyUnlocked);
            }
        };
        saveKit.onClick = [this]
        {
            if (auto* k = proc.getActiveKit())
            {
                k->favorite = true;
                gg::kitstore::save (*k);
                transport.toast.show ("Saved \"" + k->name + "\" to MY KITS", pal().green);
                proc.sendChangeMessage();
            }
        };
        exportKit.onClick = [this] { if (onExport) onExport(); };
    }

    void BuildPage::refresh()
    {
        song.refresh();
        for (auto* c : cards) c->refresh();
        grid.refresh();
        detail.refresh();
        transport.refresh();
        const bool has = proc.getActiveKit() != nullptr;
        for (auto* b : { &regenerate, &lockAll, &saveKit, &exportKit }) b->setEnabled (has);
        if (auto* k = proc.getActiveKit())
        {
            bool anyUnlocked = false;
            for (auto& s : k->slots) if (! s.isEmpty() && ! s.locked) anyUnlocked = true;
            lockAll.setButtonText (anyUnlocked ? "LOCK ALL" : "UNLOCK ALL");
        }
        exportKit.getProperties().set ("accent", pal().orange.toString());
        static juce::String lastMessage;
        if (proc.getStatusMessage() != lastMessage)
        {
            lastMessage = proc.getStatusMessage();
            transport.toast.show (lastMessage);
        }
        repaint();
    }

    void BuildPage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8);
        auto left = r.removeFromLeft (juce::jlimit (300, 360, r.getWidth() / 4));
        song.setBounds (left);
        r.removeFromLeft (10);

        transport.setBounds (r.removeFromBottom (56));
        r.removeFromBottom (8);

        labArea = r.removeFromTop (juce::jlimit (128, 150, r.getHeight() / 5));
        const int cw = labArea.getWidth() / 4;
        auto la = labArea;
        for (auto* c : cards) c->setBounds (la.removeFromLeft (cw).reduced (4, 0));
        r.removeFromTop (8);

        detail.setBounds (r.removeFromBottom (juce::jlimit (210, 260, r.getHeight() * 2 / 5)));
        r.removeFromBottom (8);

        editorHeader = r.removeFromTop (34);
        auto h = editorHeader.reduced (4, 2);
        exportKit.setBounds (h.removeFromRight (110));
        h.removeFromRight (8);
        saveKit.setBounds (h.removeFromRight (100));
        h.removeFromRight (6);
        lockAll.setBounds (h.removeFromRight (110));
        h.removeFromRight (6);
        regenerate.setBounds (h.removeFromRight (120));
        grid.setBounds (r);
    }

    void BuildPage::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto h = editorHeader.reduced (6, 2);
        const auto* k = proc.getActiveKit();
        drawSectionTitle (g, h.removeFromLeft (110), "Kit editor", p.orange);
        if (k != nullptr)
        {
            g.setFont (headFont (14.0f));
            g.setColour (p.text);
            const auto name = k->name.toUpperCase();
            const int nameW = juce::jmin (220, juce::jmax (80, regenerate.getX() - h.getX() - 20));
            g.drawFittedText (name, h.removeFromLeft (nameW), juce::Justification::centredLeft, 1, 0.7f);
            g.setColour (p.dim);
            g.setFont (uiFont (14.0f, false));
            juce::String info = gg::genreName (k->genre);
            if (const int si = gg::styleIndex (k->genre, k->styleId); si > 0) info << ": " << gg::styleFor (k->genre, si).name;
            if (k->era >= 0) info << "  -  " << gg::eraName (k->era);
            if (k->key >= 0) info << "  -  " << gg::keyName (k->key);
            if (k->bpm > 0) info << "  -  " << juce::String (k->bpm, 0) << " BPM";
            info << "  -  " << k->numFilled() << " pads";
            auto infoArea = h.withRight (regenerate.getX() - 10);
            if (infoArea.getWidth() > 160)
                g.drawFittedText (info, infoArea, juce::Justification::centredLeft, 1, 0.8f);
        }
    }
}
