#include "FxPage.h"
#include "core/Settings.h"

namespace ggui
{
    namespace
    {
        enum Col { colName = 1, colType, colLength, colBars, colPitch, colFit, colPack };

        juce::Colour groupColour (gg::fx::Group g)
        {
            const auto& p = pal();
            switch (g)
            {
                case gg::fx::Group::buildUp: return p.orange;
                case gg::fx::Group::hit:     return p.red;
                case gg::fx::Group::move:    return p.blue;
                case gg::fx::Group::texture: return p.amber;
                case gg::fx::Group::accent:  return p.purple;
                default:                     return p.dim;
            }
        }

        juce::Colour fitColour (int level)
        {
            const auto& p = pal();
            return level == 2 ? p.green : level == 1 ? p.blue : p.faint;
        }

        // its note, when it holds one (a riser or a tape stop slides: no note to give)
        juce::String steadyNote (const gg::fx::Shape& sh)
        {
            if (sh.pitchShare <= 0.45f || sh.pitchHz <= 0 || std::abs (sh.pitchTrend) > 2.5f || sh.wobble > 2.0f) return {};
            return gg::noteName ((int) std::round (12.0 * std::log2 (sh.pitchHz / 440.0) + 69.0));
        }

        juce::String barsText (double bars)
        {
            if (bars <= 0) return {};
            if (bars < 0.24) return juce::String (bars * 4.0, 1) + " beats";
            return juce::String (bars, bars < 10 ? 2 : 1) + (std::abs (bars - 1.0) < 0.005 ? " bar" : " bars");
        }
    }

    //==============================================================================
    // The kinds, grouped (Build-ups, Hits, Falls and moves, Textures, Accents), each with how many you have
    class FxPage::TypeList : public juce::Component
    {
    public:
        explicit TypeList (FxPage& o) : owner (o) {}

        struct Item { bool header; int type; juce::String text; gg::fx::Group group; };
        std::vector<Item> items() const
        {
            std::vector<Item> v;
            v.push_back ({ false, -1, "Every effect", gg::fx::Group::other });
            for (int g = 0; g < (int) gg::fx::Group::count; ++g)
            {
                const auto group = (gg::fx::Group) g;
                if (group != gg::fx::Group::other) v.push_back ({ true, -2, gg::fx::groupName (group).toUpperCase(), group });
                for (int t = 0; t < gg::fx::numTypes; ++t)
                    if (gg::fx::groupOf ((gg::fx::Type) t) == group) v.push_back ({ false, t, gg::fx::typeName ((gg::fx::Type) t), group });
            }
            return v;
        }

        float rowH (bool header) const
        {
            int nh = 0, ni = 0;
            for (const auto& i : items()) (i.header ? nh : ni)++;
            const float unit = (float) getHeight() / ((float) ni + 0.85f * (float) nh);
            return juce::jlimit (16.0f, 30.0f, unit) * (header ? 0.85f : 1.0f);
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = pal();
            float y = 0;
            for (const auto& it : items())
            {
                const float h = rowH (it.header);
                auto r = juce::Rectangle<float> (0, y, (float) getWidth(), h);
                y += h;
                if (it.header)
                {
                    g.setColour (groupColour (it.group));
                    g.setFont (headFont (juce::jmin (10.5f, h * 0.5f)).withExtraKerningFactor (0.12f));
                    g.drawText (it.text, r.withTrimmedLeft (4).withTrimmedTop (h * 0.25f), juce::Justification::centredLeft);
                    continue;
                }
                const bool on = owner.typeFilter == it.type;
                const int count = owner.countOf (it.type);
                if (on)
                {
                    g.setColour (p.blue.withAlpha (0.18f));
                    g.fillRoundedRectangle (r.reduced (0, 1.5f), 6.0f);
                    g.setColour (p.blue.withAlpha (0.6f));
                    g.drawRoundedRectangle (r.reduced (0.5f, 2.0f), 6.0f, 1.0f);
                }
                else if (hover == it.type)
                {
                    g.setColour (p.text.withAlpha (0.05f));
                    g.fillRoundedRectangle (r.reduced (0, 1.5f), 6.0f);
                }
                auto in = r.reduced (10.0f, 0);
                if (it.type >= 0)
                {
                    g.setColour (groupColour (it.group).withAlpha (count > 0 ? 1.0f : 0.35f));
                    g.fillEllipse (in.removeFromLeft (7.0f).withSizeKeepingCentre (7.0f, 7.0f));
                    in.removeFromLeft (8.0f);
                }
                g.setFont (uiFont (juce::jmin (16.0f, h * 0.62f), it.type < 0 || on));
                g.setColour (count > 0 || it.type < 0 ? (on ? p.text : p.text.withAlpha (0.88f)) : p.faint);
                g.drawText (it.text, in, juce::Justification::centredLeft, true);
                g.setColour (count > 0 ? p.dim : p.faint);
                g.drawText (juce::String (count), in, juce::Justification::centredRight, false);
            }
        }

        juce::Rectangle<int> rowOf (int type) const
        {
            float y = 0;
            for (const auto& it : items())
            {
                const float h = rowH (it.header);
                if (! it.header && it.type == type) return juce::Rectangle<float> (0, y, (float) getWidth(), h).toNearestInt();
                y += h;
            }
            return {};
        }

        int typeAt (float yPos) const
        {
            float y = 0;
            for (const auto& it : items())
            {
                const float h = rowH (it.header);
                if (yPos >= y && yPos < y + h) return it.header ? -2 : it.type;
                y += h;
            }
            return -2;
        }

        void mouseMove (const juce::MouseEvent& e) override   { const int t = typeAt (e.position.y); if (t != hover) { hover = t; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override     { hover = -3; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            const int t = typeAt (e.position.y);
            if (t != -2) owner.showType (t);
        }

    private:
        FxPage& owner;
        int hover = -3;
    };

    //==============================================================================
    FxPage::FxPage (GrooveGrabberProcessor& p) : proc (p)
    {
        types = std::make_unique<TypeList> (*this);
        addAndMakeVisible (*types);

        auto& h = table.getHeader();
        h.addColumn ("NAME", colName, 220, 120, 600);
        h.addColumn ("KIND", colType, 128, 90, 200);
        h.addColumn ("LENGTH", colLength, 66, 50, 100);
        h.addColumn ("BARS", colBars, 86, 60, 120);
        h.addColumn ("NOTE", colPitch, 54, 40, 80);
        h.addColumn ("FIT", colFit, 92, 70, 140);
        h.addColumn ("PACK / FOLDER", colPack, 140, 70, 400);
        h.setSortColumnId (colName, true);
        h.setStretchToFitActive (true);
        table.setRowHeight (30);
        table.setMultipleSelectionEnabled (false);
        addAndMakeVisible (table);

        search.setTextToShowWhenEmpty ("Search effects, packs, folders...", pal().faint);
        search.setFont (uiFont (16.0f, false));
        search.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        search.onTextChange = [this] { applyFilter(); };
        search.onEscapeKey = [this] { search.clear(); applyFilter(); };
        addAndMakeVisible (search);

        fitsFirst.getProperties().set ("style", "toggle");
        fitsFirst.setClickingTogglesState (true);
        fitsFirst.setTooltip ("Put the effects that fit your song (BUILD's genre and decade) at the top");
        fitsFirst.onClick = [this] { sortByFit = fitsFirst.getToggleState(); applyFilter(); };
        addAndMakeVisible (fitsFirst);

        play.setTooltip ("Hear it on its own");
        play.onClick = [this] { playSelected(); };
        addAndMakeVisible (play);
        wave.setEmptyText ("Pick an effect");
        addAndMakeVisible (wave);

        inTime.getProperties().set ("style", "accent");
        inTime.setTooltip ("Hear it over BUILD's groove and kit, where it goes: a build-up ends on the downbeat, a hit lands on it");
        inTime.onClick = [this] { hearInTime(); };
        addAndMakeVisible (inTime);

        {
            const juce::String how = " at your tempo. A build-up ends on the downbeat, a hit starts on it: too long, it's trimmed "
                                     "(a riser from its start); too short, stretched without changing its pitch. HEAR IT IN TIME, DRAG, "
                                     "ADD TO SONG FX and PUT ON THIS KIT use it.";
            fit.setTooltips ({ "As it is: its own length", "Make it 1 bar" + how, "Make it 2 bars" + how, "Make it 4 bars" + how,
                               "Make it 8 bars" + how });
        }
        fit.onChange = [this] (int i) { static const int bars[] { 0, 1, 2, 4, 8 }; setFitBars (bars[juce::jlimit (0, 4, i)]); };
        addAndMakeVisible (fit);

        addBank.getProperties().set ("style", "accent");
        addBank.onClick = [this] { addToSongFx(); };
        addAndMakeVisible (addBank);
        onKit.getProperties().set ("style", "ghost");
        onKit.onClick = [this] { putOnKit(); };
        addAndMakeVisible (onKit);

        proc.fxCatalog->addChangeListener (this);
        startTimerHz (4);
    }

    FxPage::~FxPage()
    {
        proc.fxCatalog->removeChangeListener (this);
        table.setModel (nullptr);
    }

    void FxPage::refresh()
    {
        search.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        search.applyColourToAllText (pal().text);
        search.setTextToShowWhenEmpty ("Search effects, packs, folders...", pal().faint);
        inTime.getProperties().set ("accent", pal().orange.toString());
        addBank.getProperties().set ("accent", pal().amber.toString());
        if (proc.library->getVersion() != lastVersion)
        {
            lastVersion = proc.library->getVersion();
            all = proc.library->getSounds();
            proc.fxCatalog->update (all);
        }
        applyFilter();
        repaint();
    }

    void FxPage::visibilityChanged()
    {
        if (isVisible()) refresh();
    }

    void FxPage::changeListenerCallback (juce::ChangeBroadcaster*)
    {
        catalogChanged = true;      // (picked up by the timer: the catalog can report often while it sorts)
    }

    void FxPage::timerCallback()
    {
        if (! isVisible()) return;
        if (catalogChanged)
        {
            catalogChanged = false;
            applyFilter();
        }
        if (proc.library->getVersion() != lastVersion) refresh();
        const bool playingInTime = juce::Time::getMillisecondCounterHiRes() < inTimeUntil;
        const auto text = playingInTime ? juce::String ("STOP") : juce::String ("HEAR IT IN TIME");
        if (inTime.getButtonText() != text) inTime.setButtonText (text);
        updateButtons();
        repaint (statusArea);
    }

    //==============================================================================
    void FxPage::applyFilter()
    {
        if (all == nullptr) all = proc.library->getSounds();
        const auto& o = proc.options;
        const auto words = juce::StringArray::fromTokens (search.getText().toLowerCase(), " ", "");
        counts.assign ((size_t) gg::fx::numTypes, 0);
        rows.clear();
        for (int i = 0; i < (int) all->size(); ++i)
        {
            const auto& s = (*all)[(size_t) i];
            if (s.cat != gg::Cat::fx || s.hidden || s.rating < 0) continue;
            const auto item = proc.fxCatalog->get (s);
            ++counts[(size_t) item.type];
            if (typeFilter >= 0 && (int) item.type != typeFilter) continue;
            if (! words.isEmpty())
            {
                const auto hay = (s.path + " " + gg::fx::typeName (item.type) + " " + gg::fx::groupName (gg::fx::groupOf (item.type))).toLowerCase();
                bool ok = true;
                for (auto& w : words) if (w.isNotEmpty() && ! hay.contains (w)) { ok = false; break; }
                if (! ok) continue;
            }
            rows.push_back ({ i, item, gg::fx::fitFor (item.type, o.genre, o.era) });
        }
        const auto& list = *all;
        auto len = [] (const Row& r, const gg::SoundInfo& s) { return r.item.shape.valid ? r.item.shape.duration : s.duration; };
        auto key = [&] (const Row& a, const Row& b)
        {
            const auto& x = list[(size_t) a.index];
            const auto& y = list[(size_t) b.index];
            switch (sortColumn)
            {
                case colType:   return a.item.type != b.item.type ? (int) a.item.type < (int) b.item.type : x.fileName().compareNatural (y.fileName()) < 0;
                case colLength:
                case colBars:   return len (a, x) < len (b, y);
                case colPitch:  return a.item.shape.pitchHz < b.item.shape.pitchHz;
                case colFit:    return a.fit.level > b.fit.level;
                case colPack:   return x.pack != y.pack ? x.pack.compareNatural (y.pack) < 0 : x.fileName().compareNatural (y.fileName()) < 0;
                default:        return x.fileName().compareNatural (y.fileName()) < 0;
            }
        };
        std::stable_sort (rows.begin(), rows.end(), [&] (const Row& a, const Row& b) { return sortForwards ? key (a, b) : key (b, a); });
        if (sortByFit)
            std::stable_sort (rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.fit.level > b.fit.level; });
        table.updateContent();
        // keep the one picked picked
        int keep = -1;
        for (int r = 0; r < (int) rows.size(); ++r)
            if ((*all)[(size_t) rows[(size_t) r].index].path == shownPath) keep = r;
        if (keep >= 0) table.selectRow (keep, true, true);
        else table.deselectAllRows();
        table.repaint();
        types->repaint();
        showSelected();
    }

    int FxPage::countOf (int type) const
    {
        if (type < 0)
        {
            int n = 0;
            for (int c : counts) n += c;
            return n;
        }
        return juce::isPositiveAndBelow (type, (int) counts.size()) ? counts[(size_t) type] : 0;
    }

    void FxPage::showType (int type)
    {
        typeFilter = type;
        applyFilter();
        resized();
        repaint();
    }

    void FxPage::setFitsFirst (bool b)
    {
        fitsFirst.setToggleState (b, juce::dontSendNotification);
        sortByFit = b;
        applyFilter();
    }

    const gg::SoundInfo* FxPage::soundAt (int row) const
    {
        if (all == nullptr || ! juce::isPositiveAndBelow (row, (int) rows.size())) return nullptr;
        return &(*all)[(size_t) rows[(size_t) row].index];
    }

    juce::String FxPage::fitTextOf (int row) const
    {
        return juce::isPositiveAndBelow (row, (int) rows.size()) ? rows[(size_t) row].fit.why : juce::String();
    }

    void FxPage::selectRow (int row)
    {
        table.selectRow (row);
        selectedRowsChanged (row);
    }

    juce::String FxPage::getSelectedPath() const
    {
        const auto* s = soundAt (table.getSelectedRow());
        return s != nullptr ? s->path : juce::String();
    }

    gg::fx::Type FxPage::getSelectedType() const
    {
        const int r = table.getSelectedRow();
        return juce::isPositiveAndBelow (r, (int) rows.size()) ? rows[(size_t) r].item.type : gg::fx::Type::other;
    }

    //==============================================================================
    int FxPage::getNumRows()   { return (int) rows.size(); }

    void FxPage::paintRowBackground (juce::Graphics& g, int row, int w, int h, bool selected)
    {
        const auto& p = pal();
        if (selected) { g.setColour (p.blue.withAlpha (0.16f)); g.fillRect (0, 0, w, h); }
        else if (row % 2 == 1) { g.setColour (p.text.withAlpha (0.025f)); g.fillRect (0, 0, w, h); }
        g.setColour (p.line.withAlpha (0.5f));
        g.fillRect (0, h - 1, w, 1);
    }

    void FxPage::paintCell (juce::Graphics& g, int row, int col, int w, int h, bool)
    {
        const auto* s = soundAt (row);
        if (s == nullptr) return;
        const auto& r = rows[(size_t) row];
        const auto& p = pal();
        juce::Rectangle<int> box (6, 0, w - 10, h);
        g.setFont (uiFont (15.0f, false));
        const float secs = r.item.shape.valid ? r.item.shape.duration : s->duration;
        switch (col)
        {
            case colName:
                g.setColour (p.text);
                g.drawText (s->fileName(), box, juce::Justification::centredLeft, true);
                break;
            case colType:
            {
                const auto c = groupColour (gg::fx::groupOf (r.item.type));
                auto chip = box.withSizeKeepingCentre (box.getWidth(), 20).withX (box.getX()).toFloat();
                g.setColour (c.withAlpha (0.16f));
                g.fillRoundedRectangle (chip, 5.0f);
                g.setColour (c);
                g.setFont (uiFont (13.5f));
                g.drawText (gg::fx::typeName (r.item.type) + (r.item.byUser ? " *" : (r.item.analysed || r.item.byName ? "" : " ...")),
                            chip.toNearestInt().reduced (4, 0), juce::Justification::centred, true);
                break;
            }
            case colLength:
                g.setColour (p.dim);
                g.drawText (juce::String (secs, secs < 10 ? 1 : 0) + " s", box, juce::Justification::centredLeft, true);
                break;
            case colBars:
                g.setColour (p.dim);
                g.drawText (barsText (gg::fx::barsAt (secs, proc.songBpm())), box, juce::Justification::centredLeft, true);
                break;
            case colPitch:
                g.setColour (p.dim);
                g.drawText (steadyNote (r.item.shape), box, juce::Justification::centredLeft, true);
                break;
            case colFit:
                g.setColour (fitColour (r.fit.level));
                g.setFont (uiFont (14.5f, r.fit.level == 2));
                g.drawText (r.fit.word, box, juce::Justification::centredLeft, true);
                break;
            case colPack:
                g.setColour (p.dim);
                g.drawText (s->pack, box, juce::Justification::centredLeft, true);
                break;
            default: break;
        }
    }

    void FxPage::cellClicked (int row, int, const juce::MouseEvent& e)
    {
        if (soundAt (row) == nullptr) return;
        if (e.mods.isPopupMenu()) { table.selectRow (row); showTypeMenu (row); return; }
        playSelected();
    }

    void FxPage::selectedRowsChanged (int row)
    {
        if (const auto* s = soundAt (row))
        {
            const bool changed = s->path != shownPath;
            shownPath = s->path;
            showSelected();
            if (changed && ! juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown()) playSelected();   // arrow keys audition as you go
        }
    }

    void FxPage::sortOrderChanged (int col, bool fwd)
    {
        sortColumn = col;
        sortForwards = fwd;
        applyFilter();
    }

    void FxPage::showTypeMenu (int row)
    {
        const auto* s = soundAt (row);
        if (s == nullptr) return;
        const auto path = s->path;
        const auto current = rows[(size_t) row].item.type;
        juce::PopupMenu kinds;
        for (int g = 0; g < (int) gg::fx::Group::count; ++g)
        {
            if (g > 0) kinds.addSeparator();
            for (int t = 0; t < gg::fx::numTypes; ++t)
                if ((int) gg::fx::groupOf ((gg::fx::Type) t) == g)
                    kinds.addItem (100 + t, gg::fx::typeName ((gg::fx::Type) t), true, (gg::fx::Type) t == current);
        }
        juce::PopupMenu m;
        m.addSectionHeader (s->fileName());
        m.addItem (1, "Listen");
        m.addItem (2, "Hear it in time");
        m.addSubMenu ("It's really a...", kinds);
        m.addItem (3, "Show the file");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&table).withMousePosition(),
                         [this, path, safe = juce::Component::SafePointer<FxPage> (this)] (int r)
        {
            if (safe == nullptr) return;
            if (r == 1) playSelected();
            else if (r == 2) hearInTime();
            else if (r == 3) juce::File (path).revealToUser();
            else if (r >= 100) proc.fxCatalog->setType (path, (gg::fx::Type) (r - 100));
        });
    }

    //==============================================================================
    void FxPage::playSelected()
    {
        if (const auto* s = soundAt (table.getSelectedRow()))
        {
            inTimeUntil = 0;
            proc.previewFile (s->path, gg::Cat::fx);
        }
    }

    void FxPage::hearInTime()
    {
        if (juce::Time::getMillisecondCounterHiRes() < inTimeUntil)
        {
            inTimeUntil = 0;
            proc.stopPreview();
            return;
        }
        const auto path = getSelectedPath();
        if (path.isEmpty()) return;
        const double secs = proc.hearFxInTime (path, getSelectedType(), getFitBars());
        inTimeUntil = juce::Time::getMillisecondCounterHiRes() + secs * 1000.0 + 400.0;
    }

    juce::Rectangle<int> FxPage::tutorialArea (const juce::String& which) const
    {
        if (which == "kinds") return leftArea;
        if (which == "detail") return rightArea;
        if (which == "facts") return factsArea;
        if (which == "drag") return dragArea;
        if (which == "fit") return fitRow;
        if (which.startsWith ("kind."))
        {
            const auto t = gg::fx::typeFromId (which.fromFirstOccurrenceOf (".", false, false));
            if (t != gg::fx::Type::count) return types->rowOf ((int) t) + types->getPosition();
        }
        return {};
    }

    int FxPage::getFitBars() const
    {
        const auto it = fitChoice.find (getSelectedPath());
        return it != fitChoice.end() ? it->second : 0;
    }

    void FxPage::setFitBars (int bars)
    {
        const auto path = getSelectedPath();
        if (path.isEmpty()) return;
        fitChoice[path] = bars;
        static const int all[] { 0, 1, 2, 4, 8 };
        for (int i = 0; i < 5; ++i) if (all[i] == bars) fit.setSelected (i);
        repaint (rightArea);
    }

    juce::String FxPage::fitWords (const gg::fx::Catalog::Item& item, const gg::SoundInfo& s) const
    {
        const int bars = getFitBars();
        if (bars <= 0) return {};
        const double bpm = proc.songBpm(), target = bars * 240.0 / bpm;
        const double len = item.shape.valid ? item.shape.duration : s.duration;
        const auto set = gg::fx::fitSettings (item.type);
        const double anchor = set.anchor == 1 ? len * item.shape.peakPos : len;
        juce::String w = juce::String (bars) + (bars == 1 ? " bar" : " bars") + ": ";
        if (anchor > target * 1.01)
        {
            if (set.anchor == 2) return w + (anchor < target * 1.05 ? "a touch cut, faded out" : "cut short, faded out");
            return w + (anchor < target * 1.05 ? "a touch trimmed from its start" : "trimmed from its start");
        }
        if (anchor < target * 0.99)
        {
            if (! set.stretch) return w + "starts on the downbeat and rings as it is";
            const double factor = target / juce::jmax (0.01, anchor);
            if (factor < 1.05) return w + "stretched a touch, pitch kept";
            if (factor > 4.0) return w + "stretched x4 (the most), pitch kept" + juce::String (set.anchor == 2 ? ", then silence" : ", silence before it");
            return w + "stretched x" + juce::String (factor, 1) + ", pitch kept";
        }
        return w + "fits as it is";
    }

    int FxPage::bankIndexOf (const juce::String& path) const
    {
        for (int i = 0; i < gg::numBankSlots; ++i)
            if (path.isNotEmpty() && proc.songFx[(size_t) i].path == path) return i;
        return -1;
    }

    juce::String FxPage::whereItIs (const juce::String& path) const
    {
        juce::StringArray where;
        if (const int b = bankIndexOf (path); b >= 0)
            where.add (b < proc.songFxPads() ? "Song FX " + juce::String (b + 1) + " (note " + juce::String (gg::firstBankNote + b) + ", every kit)"
                                             : "Song FX " + juce::String (b + 1) + " (waiting for a bigger KIT SIZE)");
        for (int k = 0; k < (int) proc.kits.size(); ++k)
            for (auto s : { gg::Slot::fx1, gg::Slot::fx2, gg::Slot::fx3, gg::Slot::fx4 })
                if (gg::slotInKit (s, proc.kits[(size_t) k].size) && proc.kits[(size_t) k][s].path == path)
                    where.add ("Kit " + juce::String (k + 1) + "'s " + gg::slotName (s));
        return where.joinIntoString (", ");
    }

    void FxPage::updateButtons()
    {
        const auto path = getSelectedPath();
        const bool any = path.isNotEmpty();
        const bool inBank = bankIndexOf (path) >= 0;
        const auto addText = inBank ? juce::String ("REMOVE FROM SONG FX") : juce::String ("ADD TO SONG FX");
        if (addBank.getButtonText() != addText) addBank.setButtonText (addText);
        addBank.setEnabled (any);
        addBank.setTooltip (inBank ? "Take it out of the song's FX bank (every kit's Song FX pads)"
                                   : "Put it on the next free Song FX pad: every kit plays it (notes " + juce::String (gg::firstBankNote) + " up), and it stays when you generate again");
        const auto* k = proc.getActiveKit();
        onKit.setEnabled (any && k != nullptr);
        onKit.setTooltip (k == nullptr ? juce::String ("Generate kits on BUILD first")
                                       : "Put it on one of " + k->name + "'s own FX pads (this kit only)");
    }

    void FxPage::addToSongFx()
    {
        const auto path = getSelectedPath();
        if (path.isEmpty()) return;
        if (const int b = bankIndexOf (path); b >= 0)
        {
            proc.setSongFx (b, {});
            updateButtons();
            repaint (rightArea);
            return;
        }
        if (proc.addSongFx (path, getFitBars()) >= 0) { updateButtons(); repaint (rightArea); return; }
        // full: which one does it replace?
        juce::PopupMenu m;
        m.addSectionHeader ("Song FX is full: replace...");
        for (int i = 0; i < proc.songFxPads(); ++i)
            m.addItem (i + 1, "Song FX " + juce::String (i + 1) + ": " + juce::File (proc.songFx[(size_t) i].path).getFileNameWithoutExtension());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addBank), [this, path, safe = juce::Component::SafePointer<FxPage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            proc.setSongFx (r - 1, path, getFitBars());
            updateButtons();
            repaint (rightArea);
        });
    }

    void FxPage::putOnKit()
    {
        const auto path = getSelectedPath();
        const auto* k = proc.getActiveKit();
        if (path.isEmpty() || k == nullptr) return;
        if (const auto free = proc.freeKitFx(); free != gg::Slot::count)
        {
            proc.putOnKit (path, free, getFitBars());
            repaint (rightArea);
            return;
        }
        juce::PopupMenu m;
        m.addSectionHeader (k->name + "'s FX pads are full: replace...");
        for (auto s : { gg::Slot::fx1, gg::Slot::fx2, gg::Slot::fx3, gg::Slot::fx4 })
            if (gg::slotInKit (s, k->size))
                m.addItem ((int) s + 1, gg::slotName (s) + ": " + juce::File ((*k)[s].path).getFileNameWithoutExtension());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&onKit), [this, path, safe = juce::Component::SafePointer<FxPage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            proc.putOnKit (path, (gg::Slot) (r - 1), getFitBars());
            repaint (rightArea);
        });
    }

    juce::String FxPage::inTimeWords (gg::fx::Type t) const
    {
        switch (gg::fx::anchorOf (t))
        {
            case gg::fx::Anchor::end:  return "Ends on the downbeat";
            case gg::fx::Anchor::peak: return "Peaks on the downbeat";
            default:                   return gg::fx::groupOf (t) == gg::fx::Group::texture ? "Runs under the bar from the downbeat" : "Starts on the downbeat";
        }
    }

    void FxPage::showSelected()
    {
        const int row = table.getSelectedRow();
        const auto* s = soundAt (row);
        const bool any = s != nullptr;
        play.setEnabled (any);
        inTime.setEnabled (any);
        fit.setEnabled (any);
        {
            static const int all[] { 0, 1, 2, 4, 8 };
            const int bars = getFitBars();
            for (int i = 0; i < 5; ++i) if (all[i] == bars) fit.setSelected (i);
        }
        updateButtons();
        if (! any)
        {
            if (shownPath.isNotEmpty() && (all == nullptr || std::none_of (all->begin(), all->end(), [this] (const gg::SoundInfo& x) { return x.path == shownPath; })))
                shownPath = {};
            wave.setAudio (nullptr);
            repaint (rightArea);
            return;
        }
        const auto colour = groupColour (gg::fx::groupOf (rows[(size_t) row].item.type));
        wave.setColour (colour);
        if (s->path != wave.getProperties()["path"].toString())
        {
            wave.getProperties().set ("path", s->path);
            wave.setAudio (nullptr);
            const int gen = ++loadGeneration;
            const auto path = s->path;
            proc.runInBackground ([this, safe = juce::Component::SafePointer<FxPage> (this), gen, path]
            {
                auto buf = std::make_shared<juce::AudioBuffer<float>> (gg::render::sound (path, gg::Cat::fx, {}, 22050.0));
                juce::MessageManager::callAsync ([safe, gen, buf]
                {
                    if (safe != nullptr && gen == safe->loadGeneration) safe->wave.setAudio (buf);
                });
            });
        }
        repaint (rightArea);
    }

    //==============================================================================
    void FxPage::mouseDown (const juce::MouseEvent&) {}

    void FxPage::mouseMove (const juce::MouseEvent& e)
    {
        setMouseCursor (dragArea.contains (e.getPosition()) && getSelectedPath().isNotEmpty() ? juce::MouseCursor::DraggingHandCursor
                                                                                             : juce::MouseCursor::NormalCursor);
    }

    void FxPage::mouseDrag (const juce::MouseEvent& e)
    {
        const auto path = getSelectedPath();
        if (dragging || path.isEmpty() || ! dragArea.contains (e.getMouseDownPosition()) || e.getDistanceFromDragStart() < 6) return;
        dragging = true;
        repaint (dragArea);
        // (tuned or fit: a file of it as it plays here; as it is: the original)
        const auto file = proc.fxFileToDrag (path, getSelectedType(), getFitBars());
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                    [safe = juce::Component::SafePointer<FxPage> (this)]
        {
            if (safe != nullptr) { safe->dragging = false; safe->repaint (safe->dragArea); }
        });
    }

    //==============================================================================
    void FxPage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8);
        leftArea = r.removeFromLeft (juce::jlimit (210, 260, r.getWidth() / 6));
        r.removeFromLeft (10);
        rightArea = r.removeFromRight (juce::jlimit (330, 440, r.getWidth() * 34 / 100));
        r.removeFromRight (10);
        midArea = r;

        auto l = leftArea.reduced (12, 12);
        l.removeFromTop (30);
        types->setBounds (l);

        auto m = midArea.reduced (14, 12);
        m.removeFromTop (26);
        auto top = m.removeFromTop (32);
        search.setBounds (top.removeFromRight (juce::jmin (300, top.getWidth() / 2)));
        top.removeFromRight (8);
        fitsFirst.setBounds (top.removeFromRight (176).reduced (0, 1));
        statusArea = top;
        m.removeFromTop (10);
        table.setBounds (m);

        auto d = rightArea.reduced (16, 12);
        d.removeFromTop (30);
        d.removeFromTop (74);                         // name, folder, kind
        auto waveRow = d.removeFromTop (96);
        play.setBounds (waveRow.removeFromLeft (40).withSizeKeepingCentre (40, 40));
        waveRow.removeFromLeft (8);
        wave.setBounds (waveRow);
        d.removeFromTop (12);
        auto bottom = d.removeFromBottom (52);
        dragArea = bottom.removeFromRight (bottom.getWidth() / 2 - 4);
        bottom.removeFromRight (8);
        inTime.setBounds (bottom.reduced (0, 4));
        auto use = d.removeFromBottom (44);
        d.removeFromBottom (6);
        fitRow = d.removeFromBottom (32);
        fit.setBounds (fitRow.withTrimmedLeft (44).reduced (0, 2));
        onKit.setBounds (use.removeFromRight (use.getWidth() / 2 - 4).reduced (0, 2));
        use.removeFromRight (8);
        addBank.setBounds (use.reduced (0, 2));
        d.removeFromBottom (10);
        detailText = d;
    }

    void FxPage::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        drawPanel (g, leftArea.toFloat(), 14.0f);
        drawPanel (g, midArea.toFloat(), 14.0f);
        drawPanel (g, rightArea.toFloat(), 14.0f);
        drawSectionTitle (g, leftArea.reduced (12, 12).removeFromTop (20), "Kinds", p.orange);
        drawSectionTitle (g, midArea.reduced (14, 12).removeFromTop (20), "Your effects", p.blue);
        drawSectionTitle (g, rightArea.reduced (16, 12).removeFromTop (20), "The effect", p.purple);

        // the status: how many, sorting, the song they're matched to
        {
            g.setColour (p.dim);
            g.setFont (uiFont (14.5f, false));
            juce::String info = juce::String ((int) rows.size()) + " shown";
            if (proc.fxCatalog->isBusy()) info << "  -  sorting your effects...";
            else info << "  -  fit for " << gg::genreName (proc.options.genre)
                      << (juce::isPositiveAndBelow (proc.options.era, gg::numEras) ? " " + gg::eraName (proc.options.era) : juce::String())
                      << ", " << juce::String (proc.songBpm(), 0) << " BPM";
            g.drawFittedText (info, statusArea, juce::Justification::centredLeft, 1, 0.8f);
        }
        if (rows.empty())
        {
            g.setColour (p.dim);
            g.setFont (uiFont (16.0f, false));
            const auto msg = countOf (-1) == 0
                ? juce::String ("No effects in your library yet. Add the folders where your FX live on the LIBRARY page "
                                "(risers, impacts, sweeps...), or right-click a sound there and pick FX.")
                : juce::String ("Nothing here: pick another kind, or clear the search.");
            g.drawFittedText (msg, table.getBounds().reduced (30, 60).withHeight (120), juce::Justification::centredTop, 4);
        }

        // ---- the effect ----
        auto d = rightArea.reduced (16, 12);
        d.removeFromTop (30);
        const int row = table.getSelectedRow();
        const auto* s = soundAt (row);
        if (s == nullptr)
        {
            g.setColour (p.dim);
            g.setFont (uiFont (15.5f, false));
            g.drawFittedText ("Click an effect to hear it. Pick a kind on the left to see only those. Each one shows where it fits, "
                              "how long it runs at your tempo, and where it goes against the beat.",
                              d.removeFromTop (110), juce::Justification::topLeft, 5);
            return;
        }
        const auto& r = rows[(size_t) row];
        const auto colour = groupColour (gg::fx::groupOf (r.item.type));
        auto head = d.removeFromTop (74);
        g.setColour (p.text);
        g.setFont (uiFont (21.0f));
        g.drawFittedText (s->fileName(), head.removeFromTop (28), juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (p.dim);
        g.setFont (uiFont (13.5f, false));
        g.drawFittedText (s->pack, head.removeFromTop (18), juce::Justification::centredLeft, 1, 0.7f);
        {
            auto chips = head.removeFromTop (26).withTrimmedTop (4);
            g.setFont (uiFont (14.0f));
            const auto kind = gg::fx::typeName (r.item.type);
            auto chip = chips.removeFromLeft ((int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), kind) + 22).toFloat();
            g.setColour (colour.withAlpha (0.18f));
            g.fillRoundedRectangle (chip, 5.0f);
            g.setColour (colour);
            g.drawText (kind, chip.toNearestInt(), juce::Justification::centred);
            chips.removeFromLeft (10);
            g.setColour (p.faint);
            g.setFont (uiFont (13.5f, false));
            g.drawText (gg::fx::groupName (gg::fx::groupOf (r.item.type)) + "  -  "
                        + (r.item.byUser ? "set by you" : r.item.byName ? "from its name" : r.item.analysed ? "from its sound" : "sorting..."),
                        chips, juce::Justification::centredLeft, true);
        }

        // facts, fit, tip
        auto t = detailText;
        const float secs = r.item.shape.valid ? r.item.shape.duration : s->duration;
        const double bpm = proc.songBpm();
        juce::AttributedString facts;
        facts.setLineSpacing (3.0f);
        auto add = [&facts, &p] (const juce::String& k, const juce::String& v)
        {
            facts.append (k, uiFont (15.0f), p.dim);
            facts.append (v + "\n", uiFont (15.0f), p.text);
        };
        add ("Length   ", juce::String (secs, 2) + " s  -  " + barsText (gg::fx::barsAt (secs, bpm)) + " at " + juce::String (bpm, 0) + " BPM");
        if (const auto note = steadyNote (r.item.shape); note.isNotEmpty()) add ("Note   ", note);
        add ("In time   ", inTimeWords (r.item.type));
        if (const auto f = fitWords (r.item, *s); f.isNotEmpty())
        {
            add ("Fit   ", f);
            if (const double daw = proc.dawBpm(); daw > 0 && std::abs (daw - bpm) > 0.05)
                add ("Tempo   ", "your DAW is at " + juce::String (daw, daw == std::round (daw) ? 0 : 1) + " BPM (IMPORT SONG > Use my DAW's tempo)");
        }
        if (proc.options.tuneToKey && proc.options.key >= 0)
        {
            const float t = proc.fxTuneFor (s->path);
            if (std::abs (t) >= 0.005f)
            {
                juce::String to;
                if (r.item.shape.pitchHz > 20.0f)     // (the note it lands on)
                    to = gg::noteName ((int) std::round (69.0 + 12.0 * std::log2 (r.item.shape.pitchHz / 440.0) + t)) + ", ";
                add ("Key   ", "tuned " + juce::String (t > 0 ? "+" : "") + juce::String (t, 1) + " st to " + to + gg::keyName (proc.options.key));
            }
            else if (gg::fx::hasSteadyPitch (r.item.shape)) add ("Key   ", "in " + gg::keyName (proc.options.key) + " already");
        }
        if (const auto where = whereItIs (s->path); where.isNotEmpty()) add ("In your song   ", where);
        juce::TextLayout factsLayout;
        factsLayout.createLayout (facts, (float) t.getWidth());
        factsArea = t.withHeight ((int) std::ceil (factsLayout.getHeight()));
        factsLayout.draw (g, t.removeFromTop ((int) std::ceil (factsLayout.getHeight())).toFloat());
        t.removeFromTop (8);

        // the fit, against BUILD's genre and decade
        {
            auto badge = t.removeFromTop (54).toFloat();
            const auto fc = fitColour (r.fit.level);
            g.setColour (fc.withAlpha (0.12f));
            g.fillRoundedRectangle (badge, 8.0f);
            g.setColour (fc.withAlpha (0.7f));
            g.drawRoundedRectangle (badge.reduced (0.5f), 8.0f, 1.0f);
            auto in = badge.reduced (12.0f, 6.0f);
            g.setColour (r.fit.level == 0 ? p.dim : fc);
            g.setFont (uiFont (16.0f));
            g.drawFittedText (r.fit.why, in.removeFromTop (in.getHeight() / 2).toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);
            g.setColour (p.dim);
            g.setFont (uiFont (13.5f, false));
            g.drawFittedText (gg::fx::bestFor (r.item.type), in.toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);
        }
        t.removeFromTop (10);
        {
            juce::AttributedString tipText;
            tipText.setLineSpacing (2.0f);
            tipText.append (gg::fx::tip (r.item.type), bodyFont (15.5f), p.text.withAlpha (0.9f));
            juce::TextLayout layout;
            layout.createLayout (tipText, (float) t.getWidth());
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (t);
            layout.draw (g, t.toFloat().withHeight (juce::jmax ((float) t.getHeight(), layout.getHeight())));
        }

        // FIT
        g.setColour (p.dim);
        g.setFont (headFont (10.5f).withExtraKerningFactor (0.1f));
        g.drawText ("FIT", fitRow.withWidth (40), juce::Justification::centredLeft);

        // DRAG
        {
            auto dr = dragArea.toFloat().reduced (0.0f, 4.0f);
            g.setColour (dragging ? p.orange.withAlpha (0.2f) : p.sunken);
            g.fillRoundedRectangle (dr, 8.0f);
            g.setColour (p.orange.withAlpha (0.8f));
            g.drawRoundedRectangle (dr, 8.0f, 1.2f);
            auto in = dr.reduced (8.0f, 2.0f);
            drawIcon (g, Icon::drag, in.removeFromLeft (24.0f).withSizeKeepingCentre (20.0f, 20.0f), p.orange);
            in.removeFromLeft (6.0f);
            g.setColour (p.text);
            g.setFont (headFont (11.0f).withExtraKerningFactor (0.08f));
            g.drawText ("DRAG", in.removeFromTop (in.getHeight() * 0.55f), juce::Justification::bottomLeft);
            g.setColour (p.dim);
            g.setFont (uiFont (12.5f, false));
            g.drawFittedText ("onto an audio track", in.toNearestInt(), juce::Justification::topLeft, 1, 0.7f);
        }
    }
}
