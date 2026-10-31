#include "KitsPage.h"

namespace ggui
{
    namespace
    {
        class KitRow : public juce::Component
        {
        public:
            KitRow (GrooveGrabberProcessor& p, const gg::Kit& k, const juce::File& f, KitsPage& page,
                    std::function<void (const gg::Kit&)> exportFn, std::function<void()> loadedFn)
                : proc (p), kit (k), file (f), onExport (std::move (exportFn)), onLoaded (std::move (loadedFn))
            {
                juce::ignoreUnused (page);
                load.getProperties().set ("style", "accent");
                load.getProperties().set ("accent", pal().blue.toString());
                load.setTooltip ("Open this kit in the BUILD page (it takes the place of the selected kit)");
                load.onClick = [this]
                {
                    proc.loadKit (kit, proc.activeKit);
                    if (onLoaded) onLoaded();
                };
                exp.getProperties().set ("style", "ghost");
                exp.onClick = [this] { if (onExport) onExport (kit); };
                second.getProperties().set ("style", "ghost");
                if (file.existsAsFile())
                {
                    second.setButtonText ("DELETE");
                    second.onClick = [this]
                    {
                        auto f2 = file;
                        auto* pr = &proc;
                        juce::MessageManager::callAsync ([f2, pr] { gg::kitstore::remove (f2); pr->sendChangeMessage(); });
                    };
                }
                else
                {
                    second.setButtonText ("SAVE");
                    second.onClick = [this] { auto k2 = kit; k2.favorite = true; gg::kitstore::save (k2); proc.sendChangeMessage(); };
                }
                for (auto* b : { &load, &exp, &second }) addAndMakeVisible (*b);
            }

            void resized() override
            {
                auto r = getLocalBounds().reduced (12, 14);
                second.setBounds (r.removeFromRight (84));
                r.removeFromRight (6);
                exp.setBounds (r.removeFromRight (84));
                r.removeFromRight (6);
                load.setBounds (r.removeFromRight (84));
            }

            void paint (juce::Graphics& g) override
            {
                const auto& p = pal();
                auto r = getLocalBounds().toFloat().reduced (2.0f, 3.0f);
                g.setColour (p.raised);
                g.fillRoundedRectangle (r, 10.0f);
                g.setColour (p.line);
                g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
                auto in = getLocalBounds().reduced (16, 10).withTrimmedRight (290);
                auto strip = in.removeFromRight (180).reduced (0, 12).toFloat();
                if (kit.favorite) drawIcon (g, Icon::heartFilled, in.removeFromLeft (22).toFloat().withSizeKeepingCentre (16, 16), p.red);
                else in.removeFromLeft (22);
                auto top = in.removeFromTop (in.getHeight() / 2);
                g.setColour (p.text);
                g.setFont (headFont (14.0f));
                g.drawFittedText (kit.name.toUpperCase(), top, juce::Justification::bottomLeft, 1, 0.7f);
                g.setColour (p.dim);
                g.setFont (uiFont (14.0f, false));
                juce::String info = gg::genreName (kit.genre);
                if (const int si = gg::styleIndex (kit.genre, kit.styleId); si > 0) info << ": " << gg::styleFor (kit.genre, si).name;
                if (kit.era >= 0) info << "  -  " << gg::eraName (kit.era);
                if (kit.key >= 0) info << "  -  " << gg::keyName (kit.key);
                if (kit.bpm > 0) info << "  -  " << juce::String (kit.bpm, 0) << " BPM";
                info << "  -  " << kit.numFilled() << " pads";
                if (kit.created > 0) info << "  -  " << juce::Time (kit.created).toString (true, true, false);
                g.drawFittedText (info, in, juce::Justification::topLeft, 1, 0.7f);

                const float w = strip.getWidth() / gg::numSlots;
                for (int i = 0; i < gg::numSlots; ++i)
                {
                    auto cell = juce::Rectangle<float> (strip.getX() + (float) i * w, strip.getY(), w - 2.0f, strip.getHeight());
                    g.setColour (kit.slots[(size_t) i].isEmpty() ? p.line.withAlpha (0.5f) : slotColour ((gg::Slot) i));
                    g.fillRoundedRectangle (kit.slots[(size_t) i].isEmpty() ? cell.withTop (cell.getBottom() - 3) : cell, 1.5f);
                }
            }

        private:
            GrooveGrabberProcessor& proc;
            gg::Kit kit;
            juce::File file;
            std::function<void (const gg::Kit&)> onExport;
            std::function<void()> onLoaded;
            juce::TextButton load { "LOAD" }, exp { "EXPORT" }, second;
        };
    }

    KitsPage::KitsPage (GrooveGrabberProcessor& p) : proc (p)
    {
        view.onChange = [this] (int) { rebuild (false); };
        addAndMakeVisible (view);

        search.setTextToShowWhenEmpty ("Search kits: name, style, decade, key, BPM...", pal().faint);
        search.setFont (uiFont (16.0f, false));
        search.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        search.setTooltip ("Every word must match: \"g-funk 90\", \"A minor\", \"westside\", \"92 BPM\"");
        search.onTextChange = [this] { rebuild (false); };
        search.onEscapeKey = [this] { search.clear(); rebuild (false); };
        addAndMakeVisible (search);

        // ALL, then a chip per genre; a chip shows only when that genre has kits here
        for (int i = -1; i < gg::numGenres; ++i)
        {
            auto* b = genreChips.add (new juce::TextButton (i < 0 ? juce::String ("ALL") : gg::genreFolderName ((gg::Genre) i).toUpperCase()));
            b->getProperties().set ("style", "segment");
            b->setTooltip (i < 0 ? juce::String ("Every genre") : "Only " + gg::genreName ((gg::Genre) i) + " kits");
            b->onClick = [this, i] { genreFilter = i; rebuild (false); };
            addChildComponent (b);
        }

        viewport.setViewedComponent (&content, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
        openFile.getProperties().set ("style", "ghost");
        openFile.setTooltip ("Open a .ggkit file (every export folder has one)");
        openFile.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Open a Groove Grabber kit", gg::paths::kitsDir(), "*.ggkit");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this, safe = juce::Component::SafePointer<KitsPage> (this)] (const juce::FileChooser& fc)
            {
                if (safe == nullptr || ! fc.getResult().existsAsFile()) return;
                auto k = gg::Kit::fromVar (juce::JSON::parse (fc.getResult()));
                if (k.isValid()) { proc.loadKit (k, proc.activeKit); if (onLoaded) onLoaded(); }
            });
        };
        addAndMakeVisible (openFile);
        rebuild();
    }

    bool KitsPage::matchesSearch (const gg::Kit& k) const
    {
        const auto words = juce::StringArray::fromTokens (search.getText().toLowerCase(), " ,", "\"");
        if (words.isEmpty()) return true;
        juce::String hay;
        hay << k.name << " " << k.blurb << " " << gg::genreName (k.genre) << " " << gg::genreFolderName (k.genre) << " " << k.description;
        if (const int si = gg::styleIndex (k.genre, k.styleId); si > 0) hay << " " << gg::styleFor (k.genre, si).name;
        if (k.era >= 0) hay << " " << gg::eraName (k.era);
        if (k.key >= 0) hay << " " << gg::keyName (k.key);
        if (k.bpm > 0) hay << " " << juce::String (k.bpm, 0) << " bpm";
        hay = hay.toLowerCase().replaceCharacter ('-', ' ');
        for (auto w : words)
        {
            w = w.replaceCharacter ('-', ' ').trim();
            if (w.isNotEmpty() && ! hay.contains (w)) return false;
        }
        return true;
    }

    void KitsPage::rebuild (bool keepScroll)
    {
        const int scrollY = keepScroll ? viewport.getViewPositionY() : 0;
        rows.clear();

        std::vector<std::pair<juce::File, gg::Kit>> list;
        if (view.getSelected() == 0) list = gg::kitstore::listSaved();
        else for (auto& kit : gg::kitstore::history()) list.emplace_back (juce::File(), kit);

        std::array<int, gg::numGenres> counts {};
        for (auto& [file, kit] : list) ++counts[(size_t) kit.genre];
        if (genreFilter >= 0 && counts[(size_t) genreFilter] == 0) genreFilter = -1;
        kitsInView = (int) list.size();

        for (int i = 0; i < genreChips.size(); ++i)
        {
            const int g = i - 1;
            const int n = g < 0 ? kitsInView : counts[(size_t) g];
            auto* b = genreChips[i];
            b->setButtonText ((g < 0 ? juce::String ("ALL") : gg::genreFolderName ((gg::Genre) g).toUpperCase()) + "  " + juce::String (n));
            b->setVisible (n > 0);
            b->setToggleState (g == genreFilter, juce::dontSendNotification);
        }

        for (auto& [file, kit] : list)
            if ((genreFilter < 0 || (int) kit.genre == genreFilter) && matchesSearch (kit))
                content.addAndMakeVisible (rows.add (new KitRow (proc, kit, file, *this, onExport, onLoaded)));
        resized();
        viewport.setViewPosition (0, scrollY);
        repaint();
    }

    void KitsPage::refresh()
    {
        search.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        search.applyColourToAllText (pal().text);
        search.setTextToShowWhenEmpty ("Search kits: name, style, decade, key, BPM...", pal().faint);
        rebuild();
    }

    void KitsPage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8).reduced (14, 12);
        r.removeFromTop (26);
        auto top = r.removeFromTop (32);
        view.setBounds (top.removeFromLeft (220));
        openFile.setBounds (top.removeFromRight (160));
        top.removeFromRight (12);
        top.removeFromLeft (12);
        search.setBounds (top.removeFromRight (juce::jmin (380, top.getWidth())));
        r.removeFromTop (10);
        auto chips = r.removeFromTop (30);
        for (auto* b : genreChips)
        {
            if (! b->isVisible()) continue;
            const int w = juce::jmin (chips.getWidth(), 28 + 8 * b->getButtonText().length());
            b->setBounds (chips.removeFromLeft (w));
            chips.removeFromLeft (4);
        }
        r.removeFromTop (8);
        viewport.setBounds (r);
        const int rowH = 74;
        content.setSize (r.getWidth() - 12, juce::jmax (r.getHeight(), rowH * rows.size()));
        int y = 0;
        for (auto* row : rows) { row->setBounds (0, y, content.getWidth(), rowH); y += rowH; }
    }

    void KitsPage::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().reduced (10, 8);
        drawPanel (g, r.toFloat(), 14.0f);
        drawSectionTitle (g, r.reduced (14, 12).removeFromTop (20), "My kits", p.orange);
        if (rows.isEmpty())
        {
            g.setColour (p.dim);
            g.setFont (uiFont (16.0f, false));
            const juce::String text = kitsInView > 0 ? "No kits match. Clear the search or pick ALL."
                                    : view.getSelected() == 0 ? "No saved kits yet. Press the heart on a kit, or SAVE KIT, to keep it here."
                                                              : "Kits you generate show up here.";
            g.drawFittedText (text, viewport.getBounds(), juce::Justification::centred, 2);
        }
    }
}
