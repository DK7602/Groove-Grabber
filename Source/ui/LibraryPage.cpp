#include "LibraryPage.h"
#include "core/Harvester.h"
#include "core/Settings.h"

namespace ggui
{
    namespace
    {
        enum Col { colRating = 1, colName, colType, colPack, colLength, colPitch, colTraits, colSource };

        class SourceRow : public juce::Component
        {
        public:
            SourceRow (GrooveGrabberProcessor& p, int i, const gg::SourceFolder& s) : proc (p), index (i), folder (s.folder)
            {
                for (int k = 0; k < (int) gg::Source::count; ++k) kind.addItem (gg::sourceName ((gg::Source) k), k + 1);
                kind.setSelectedId ((int) s.kind + 1, juce::dontSendNotification);
                kind.onChange = [this] { proc.library->setSourceKind (index, (gg::Source) (kind.getSelectedId() - 1)); };
                kind.setTooltip ("What kind of sounds this folder holds (the BUILD page can include or leave out each kind)");
                addAndMakeVisible (kind);
                enabled.setToggleState (s.enabled, juce::dontSendNotification);
                enabled.setTooltip ("Scan and use this folder");
                enabled.onClick = [this] { proc.library->setSourceEnabled (index, enabled.getToggleState()); };
                addAndMakeVisible (enabled);
                remove.setTooltip ("Stop using this folder (your files are not touched)");
                remove.onClick = [this]
                {
                    const int i2 = index;
                    auto* lib = &proc.library.get();
                    juce::MessageManager::callAsync ([lib, i2] { lib->removeSource (i2); });
                };
                addAndMakeVisible (remove);
                reveal.setTooltip ("Show the folder");
                reveal.onClick = [this] { folder.revealToUser(); };
                addAndMakeVisible (reveal);
            }
            void resized() override
            {
                auto r = getLocalBounds().reduced (2);
                remove.setBounds (r.removeFromRight (26));
                reveal.setBounds (r.removeFromRight (26));
                enabled.setBounds (r.removeFromRight (40));
                kind.setBounds (r.removeFromRight (136).reduced (0, 3));
            }
            void paint (juce::Graphics& g) override
            {
                const auto& p = pal();
                auto r = getLocalBounds().reduced (2).withTrimmedRight (236);
                g.setColour (p.text);
                g.setFont (uiFont (15.0f));
                g.drawFittedText (folder.getFileName(), r.removeFromTop (r.getHeight() / 2 + 2), juce::Justification::bottomLeft, 1, 0.7f);
                g.setColour (p.dim);
                g.setFont (uiFont (12.5f, false));
                g.drawFittedText (folder.getParentDirectory().getFullPathName(), r, juce::Justification::topLeft, 1, 0.6f);
                if (! folder.isDirectory())
                {
                    g.setColour (p.red);
                    g.drawText ("missing", getLocalBounds().withTrimmedRight (236), juce::Justification::centredRight);
                }
            }
        private:
            GrooveGrabberProcessor& proc;
            int index;
            juce::File folder;
            juce::ComboBox kind;
            juce::ToggleButton enabled;
            IconButton remove { "remove", Icon::close }, reveal { "reveal", Icon::folder };
        };
    }

    //==============================================================================
    LibraryPage::LibraryPage (GrooveGrabberProcessor& p) : proc (p)
    {
        auto& h = table.getHeader();
        h.addColumn ("", colRating, 30, 30, 30, juce::TableHeaderComponent::notResizableOrSortable);
        h.addColumn ("NAME", colName, 220, 120, 600);
        h.addColumn ("TYPE", colType, 100, 70, 160);
        h.addColumn ("PACK / FOLDER", colPack, 160, 80, 400);
        h.addColumn ("LENGTH", colLength, 70, 50, 100);
        h.addColumn ("NOTE", colPitch, 60, 40, 90);
        h.addColumn ("CHARACTER", colTraits, 180, 120, 400, juce::TableHeaderComponent::notSortable);
        h.addColumn ("FROM", colSource, 96, 60, 160);
        h.setSortColumnId (colName, true);
        table.setRowHeight (30);
        table.getHeader().setStretchToFitActive (true);
        table.setMultipleSelectionEnabled (false);
        addAndMakeVisible (table);

        search.setTextToShowWhenEmpty ("Search sounds, packs, folders...", pal().faint);
        search.setFont (uiFont (16.0f, false));
        search.onTextChange = [this] { applyFilter(); };
        search.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        addAndMakeVisible (search);

        auto addChip = [this] (const juce::String& label, int cat)
        {
            auto* b = catChips.add (new juce::TextButton (label));
            chipCats.push_back (cat);
            b->getProperties().set ("style", "segment");
            b->onClick = [this, cat] { catFilter = cat; applyFilter(); refresh(); };
            addAndMakeVisible (b);
        };
        addChip ("ALL", -1);
        for (int c = 0; c < gg::numCats; ++c)
            if (c != (int) gg::Cat::unknown) addChip (gg::catName ((gg::Cat) c).toUpperCase(), c);

        addPacks.getProperties().set ("style", "accent");
        addPacks.setTooltip ("Add a folder of drum kits / one shots (sub-folders are included)");
        addPacks.onClick = [this] { addFolder (gg::Source::packs); };
        addAndMakeVisible (addPacks);
        addRecordings.getProperties().set ("style", "ghost");
        addRecordings.setTooltip ("Add a folder of your own recorded hits");
        addRecordings.onClick = [this] { addFolder (gg::Source::recordings); };
        addAndMakeVisible (addRecordings);
        rescan.getProperties().set ("style", "ghost");
        rescan.setTooltip ("Look for new, changed or removed files (only new ones are analysed)");
        rescan.onClick = [this] { proc.library->startScan(); };
        addAndMakeVisible (rescan);
        addAndMakeVisible (progressBar);

        startTimerHz (4);
        refresh();
    }

    LibraryPage::~LibraryPage()
    {
        table.setModel (nullptr);
    }

    void LibraryPage::addFolder (gg::Source kind)
    {
        chooser = std::make_unique<juce::FileChooser> (kind == gg::Source::packs ? "Choose a folder of drum sounds" : "Choose your recordings folder",
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory));
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this, kind, safe = juce::Component::SafePointer<LibraryPage> (this)] (const juce::FileChooser& fc)
        {
            if (safe == nullptr || ! fc.getResult().isDirectory()) return;
            proc.library->addSource (fc.getResult(), kind);
            rebuildSources();
        });
    }

    void LibraryPage::rebuildSources()
    {
        sourceRows.clear();
        const auto src = proc.library->getSources();
        for (int i = 0; i < (int) src.size(); ++i)
            addAndMakeVisible (sourceRows.add (new SourceRow (proc, i, src[(size_t) i])));
        resized();
    }

    void LibraryPage::refresh()
    {
        if ((int) sourceRows.size() != (int) proc.library->getSources().size()) rebuildSources();
        for (auto* r : sourceRows) r->repaint();
        search.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        search.applyColourToAllText (pal().text);
        search.setTextToShowWhenEmpty ("Search sounds, packs, folders...", pal().faint);
        for (int i = 0; i < catChips.size(); ++i)
            catChips[i]->setToggleState (chipCats[(size_t) i] == catFilter, juce::dontSendNotification);
        addPacks.getProperties().set ("accent", pal().orange.toString());
        if (proc.library->getVersion() != lastVersion)
        {
            lastVersion = proc.library->getVersion();
            all = proc.library->getSounds();
            applyFilter();
        }
        repaint();
    }

    void LibraryPage::timerCallback()
    {
        const bool scanning = proc.library->isScanning();
        progress = scanning ? (double) proc.library->getProgress() : 0.0;
        progressBar.setVisible (scanning);
        rescan.setEnabled (! scanning);
        if (scanning) repaint (statsArea);
        if (proc.library->getVersion() != lastVersion) refresh();
    }

    void LibraryPage::applyFilter()
    {
        shown.clear();
        if (all == nullptr) all = proc.library->getSounds();
        const auto words = juce::StringArray::fromTokens (search.getText().toLowerCase(), " ", "");
        for (int i = 0; i < (int) all->size(); ++i)
        {
            const auto& s = (*all)[(size_t) i];
            if (catFilter >= 0 && (int) s.cat != catFilter) continue;
            if (! words.isEmpty())
            {
                const auto hay = (s.path + " " + gg::catName (s.cat)).toLowerCase();
                bool ok = true;
                for (auto& w : words) if (w.isNotEmpty() && ! hay.contains (w)) { ok = false; break; }
                if (! ok) continue;
            }
            shown.push_back (i);
        }
        const auto& list = *all;
        auto key = [this, &list] (int a, int b)
        {
            const auto& x = list[(size_t) a];
            const auto& y = list[(size_t) b];
            switch (sortColumn)
            {
                case colType:   return (int) x.cat != (int) y.cat ? (int) x.cat < (int) y.cat : x.fileName().compareNatural (y.fileName()) < 0;
                case colPack:   return x.pack != y.pack ? x.pack.compareNatural (y.pack) < 0 : x.fileName().compareNatural (y.fileName()) < 0;
                case colLength: return x.decayMs < y.decayMs;
                case colPitch:  return x.midiPitch() < y.midiPitch();
                case colSource: return (int) x.source < (int) y.source;
                default:        return x.fileName().compareNatural (y.fileName()) < 0;
            }
        };
        std::stable_sort (shown.begin(), shown.end(), [&] (int a, int b) { return sortForwards ? key (a, b) : key (b, a); });
        table.updateContent();
        table.repaint();
    }

    int LibraryPage::getNumRows()   { return (int) shown.size(); }

    void LibraryPage::paintRowBackground (juce::Graphics& g, int row, int w, int h, bool selected)
    {
        const auto& p = pal();
        if (selected) { g.setColour (p.blue.withAlpha (0.16f)); g.fillRect (0, 0, w, h); }
        else if (row % 2 == 1) { g.setColour (p.text.withAlpha (0.025f)); g.fillRect (0, 0, w, h); }
        g.setColour (p.line.withAlpha (0.5f));
        g.fillRect (0, h - 1, w, 1);
    }

    void LibraryPage::paintCell (juce::Graphics& g, int row, int col, int w, int h, bool)
    {
        if (! juce::isPositiveAndBelow (row, (int) shown.size()) || all == nullptr) return;
        const auto& s = (*all)[(size_t) shown[(size_t) row]];
        const auto& p = pal();
        juce::Rectangle<int> r (6, 0, w - 10, h);
        g.setFont (uiFont (15.0f, false));
        switch (col)
        {
            case colRating:
                if (s.rating > 0) drawIcon (g, Icon::heartFilled, r.toFloat().reduced (3), p.red);
                else if (s.rating < 0 || s.hidden) drawIcon (g, Icon::close, r.toFloat().reduced (4), p.faint);
                break;
            case colName:
                g.setColour (s.rating < 0 || s.hidden ? p.faint : p.text);
                g.drawText (s.fileName(), r, juce::Justification::centredLeft, true);
                break;
            case colType:
            {
                auto chip = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 92), 20).withX (r.getX()).toFloat();
                const auto c = catColour (s.cat);
                g.setColour (c.withAlpha (0.16f));
                g.fillRoundedRectangle (chip, 5.0f);
                g.setColour (c);
                g.setFont (uiFont (13.5f));
                auto label = s.cat == gg::Cat::perc && s.percKind != gg::PercKind::none ? gg::percKindName (s.percKind) : gg::catName (s.cat);
                if (s.catByUser) label << " *";
                g.drawText (label, chip.toNearestInt(), juce::Justification::centred, true);
                break;
            }
            case colPack:
                g.setColour (p.dim);
                g.drawText (s.pack, r, juce::Justification::centredLeft, true);
                break;
            case colLength:
                g.setColour (p.dim);
                g.drawText (s.cat == gg::Cat::loop ? juce::String (s.duration, 1) + " s" : juce::String (juce::roundToInt (s.decayMs)) + " ms", r, juce::Justification::centredLeft, true);
                break;
            case colPitch:
                g.setColour (p.dim);
                if (s.hasPitch()) g.drawText (gg::noteName ((int) std::round (s.midiPitch())), r, juce::Justification::centredLeft, true);
                break;
            case colTraits:
            {
                // little bars: punch, warmth, brightness, room, vintage, aggression, live
                const juce::Colour cols[] = { p.orange, p.amber, p.blue, p.purple, p.green, p.red, p.text };
                const float bw = (float) (r.getWidth() - 4) / gg::Traits::num;
                for (int t = 0; t < gg::Traits::num; ++t)
                {
                    auto cell = juce::Rectangle<float> ((float) r.getX() + (float) t * bw, 6.0f, bw - 3.0f, (float) h - 12.0f);
                    g.setColour (p.sunken);
                    g.fillRoundedRectangle (cell, 2.0f);
                    g.setColour (cols[t].withAlpha (0.85f));
                    g.fillRoundedRectangle (cell.withTop (cell.getBottom() - cell.getHeight() * s.traits[t]), 2.0f);
                }
                break;
            }
            case colSource:
                g.setColour (p.faint);
                g.setFont (uiFont (13.0f, false));
                g.drawText (gg::sourceName (s.source), r, juce::Justification::centredLeft, true);
                break;
            default: break;
        }
    }

    void LibraryPage::cellClicked (int row, int, const juce::MouseEvent& e)
    {
        if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
        if (e.mods.isPopupMenu()) { table.selectRow (row); showSoundMenu (row); return; }
        const auto& s = (*all)[(size_t) shown[(size_t) row]];
        proc.previewFile (s.path, s.cat);
    }

    void LibraryPage::selectedRowsChanged (int row)
    {
        if (juce::isPositiveAndBelow (row, (int) shown.size()) && juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown() == false)
        {
            const auto& s = (*all)[(size_t) shown[(size_t) row]];
            proc.previewFile (s.path, s.cat);     // arrow keys audition as you go
        }
    }

    void LibraryPage::sortOrderChanged (int col, bool fwd)
    {
        sortColumn = col;
        sortForwards = fwd;
        applyFilter();
    }

    void LibraryPage::showSoundMenu (int row)
    {
        const auto s = (*all)[(size_t) shown[(size_t) row]];
        juce::PopupMenu types;
        for (int c = 0; c < gg::numCats; ++c)
            if (c != (int) gg::Cat::unknown) types.addItem (100 + c, gg::catName ((gg::Cat) c), true, (int) s.cat == c);
        juce::PopupMenu m;
        m.addSectionHeader (s.fileName());
        m.addItem (1, "Listen");
        m.addSubMenu ("It's really a...", types);
        m.addItem (2, s.rating > 0 ? "Un-favourite" : "Favourite (picked more often)");
        m.addItem (3, s.rating < 0 ? "Allow it again" : "Never use this sound");
        m.addItem (5, "Put it on the selected pad (" + gg::slotName (proc.selectedSlot) + ")", ! proc.kits.empty() && gg::slotCat (proc.selectedSlot) == s.cat);
        m.addItem (4, "Show the file");
        m.showMenuAsync ({}, [this, s, safe = juce::Component::SafePointer<LibraryPage> (this)] (int r)
        {
            if (safe == nullptr) return;
            if (r == 1) proc.previewFile (s.path, s.cat);
            else if (r == 2) proc.library->setRating (s.path, s.rating > 0 ? 0 : 1);
            else if (r == 3) proc.library->setRating (s.path, s.rating < 0 ? 0 : -1);
            else if (r == 4) juce::File (s.path).revealToUser();
            else if (r == 5) proc.setSlotSound (proc.activeKit, proc.selectedSlot, s.path);
            else if (r >= 100) proc.library->setCategory (s.path, (gg::Cat) (r - 100));
        });
    }

    //==============================================================================
    bool LibraryPage::isInterestedInFileDrag (const juce::StringArray& files)
    {
        for (auto& f : files)
            if (gg::Library::isAudioFile (juce::File (f)) || juce::File (f).isDirectory()) return true;
        return false;
    }

    void LibraryPage::filesDropped (const juce::StringArray& files, int x, int y)
    {
        dropHover = false;
        repaint();
        for (auto& path : files)
        {
            const juce::File f (path);
            if (f.isDirectory()) proc.library->addSource (f, gg::Source::packs);
            else if (gg::Library::isAudioFile (f) && harvestArea.contains (x, y)) harvest (f);
            else if (gg::Library::isAudioFile (f)) harvest (f);
        }
        rebuildSources();
    }

    void LibraryPage::harvest (const juce::File& f)
    {
        harvestStatus = "Harvesting " + f.getFileName() + "...";
        repaint();
        proc.runInBackground ([this, f, safe = juce::Component::SafePointer<LibraryPage> (this)]
        {
            auto result = gg::harvestHits (f, gg::paths::harvestedDir());
            if (result.ok)
                proc.library->analyseFilesNow (result.files, gg::Source::harvested, gg::paths::harvestedDir());
            juce::MessageManager::callAsync ([safe, result, f]
            {
                if (safe == nullptr) return;
                safe->harvestStatus = result.ok ? "Kept " + juce::String (result.files.size()) + " hits from " + juce::String (result.hitsFound)
                                                  + " in " + f.getFileName() + "  -  they're in Harvested"
                                                : result.error;
                safe->refresh();
            });
        });
    }

    //==============================================================================
    void LibraryPage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8);
        auto left = r.removeFromLeft (juce::jlimit (340, 420, r.getWidth() / 3));
        r.removeFromLeft (10);

        // left: folders, then harvest
        auto l = left.reduced (14, 12);
        l.removeFromTop (26);
        auto buttons = l.removeFromTop (32);
        addPacks.setBounds (buttons.removeFromLeft (buttons.getWidth() * 55 / 100));
        buttons.removeFromLeft (6);
        addRecordings.setBounds (buttons);
        l.removeFromTop (8);
        harvestArea = l.removeFromBottom (150);
        l.removeFromBottom (8);
        auto scanRow = l.removeFromBottom (30);
        rescan.setBounds (scanRow.removeFromRight (90));
        scanRow.removeFromRight (8);
        progressBar.setBounds (scanRow.withSizeKeepingCentre (scanRow.getWidth(), 12));
        statsArea = scanRow;
        l.removeFromBottom (6);
        sourcesArea = l;
        for (auto* row : sourceRows)
            row->setBounds (l.removeFromTop (46));

        // right: chips, search, table
        auto rr = r.reduced (14, 12);
        rr.removeFromTop (26);
        auto top = rr.removeFromTop (32);
        search.setBounds (top.removeFromRight (juce::jmin (320, top.getWidth() / 2)));
        rr.removeFromTop (8);
        auto chips = rr.removeFromTop (26);
        const int cw = chips.getWidth() / juce::jmax (1, catChips.size());
        for (auto* c : catChips) c->setBounds (chips.removeFromLeft (cw).reduced (2, 0));
        rr.removeFromTop (8);
        table.setBounds (rr);
    }

    void LibraryPage::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().reduced (10, 8);
        auto left = r.removeFromLeft (juce::jlimit (340, 420, r.getWidth() / 3));
        r.removeFromLeft (10);
        drawPanel (g, left.toFloat(), 14.0f);
        drawPanel (g, r.toFloat(), 14.0f);

        drawSectionTitle (g, left.reduced (14, 12).removeFromTop (20), "Your sound folders", p.orange);
        if (sourceRows.isEmpty())
        {
            g.setColour (p.dim);
            g.setFont (uiFont (15.0f, false));
            g.drawFittedText ("Add the folders where your drum kits and one shots live (or drop folders here). "
                              "Sub-folders count. Groove Grabber listens to every sound once and remembers it.",
                              sourcesArea.reduced (4), juce::Justification::topLeft, 5);
        }

        // scan status
        g.setColour (p.dim);
        g.setFont (uiFont (14.0f, false));
        if (! proc.library->isScanning())
            g.drawText (proc.library->getStatus().isNotEmpty() ? proc.library->getStatus() : juce::String ((int) (all ? all->size() : 0)) + " sounds",
                        statsArea, juce::Justification::centredLeft, true);

        // harvest drop zone
        {
            auto h = harvestArea.toFloat();
            g.setColour (dropHover ? p.blue.withAlpha (0.14f) : p.sunken);
            g.fillRoundedRectangle (h, 10.0f);
            juce::Path border;
            border.addRoundedRectangle (h.reduced (1.0f), 10.0f);
            juce::Path dashed;
            const float dashes[] = { 6.0f, 5.0f };
            juce::PathStrokeType (1.4f).createDashedStroke (dashed, border, dashes, 2);
            g.setColour (dropHover ? p.blue : p.faint);
            g.fillPath (dashed);
            auto in = harvestArea.reduced (16, 12);
            drawIcon (g, Icon::waveform, in.removeFromTop (30).toFloat().withSizeKeepingCentre (30, 30), p.purple);
            g.setColour (p.text);
            g.setFont (headFont (11.5f).withExtraKerningFactor (0.1f));
            g.drawText ("HARVEST HITS", in.removeFromTop (20), juce::Justification::centred);
            g.setColour (p.dim);
            g.setFont (uiFont (14.0f, false));
            g.drawFittedText (harvestStatus.isNotEmpty() ? harvestStatus
                                                         : "Drop a drums-only bounce, a drum stem or a break here. Every hit is found, sorted, "
                                                           "and the cleanest soft / medium / hard versions are kept.",
                              in, juce::Justification::centred, 4);
        }

        // right header
        auto rr = r.reduced (14, 12);
        drawSectionTitle (g, rr.removeFromTop (20), "Sounds", p.blue);
        auto top = rr.removeFromTop (32);
        g.setColour (p.dim);
        g.setFont (uiFont (15.0f, false));
        juce::String info = juce::String ((int) shown.size()) + " shown";
        if (all != nullptr)
        {
            int usable = 0;
            for (auto& s : *all) usable += s.isUsable() ? 1 : 0;
            info << "  -  " << usable << " usable one-shots in " << (int) all->size() << " files  -  click to listen, right-click for more";
        }
        g.drawText (info, top.withTrimmedRight (search.getWidth() + 10), juce::Justification::centredLeft, true);
    }
}
