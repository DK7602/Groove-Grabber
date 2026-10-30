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
        view.onChange = [this] (int) { rebuild(); };
        addAndMakeVisible (view);
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

    void KitsPage::rebuild()
    {
        rows.clear();
        if (view.getSelected() == 0)
        {
            for (auto& [file, kit] : gg::kitstore::listSaved())
                content.addAndMakeVisible (rows.add (new KitRow (proc, kit, file, *this, onExport, onLoaded)));
        }
        else
        {
            for (auto& kit : gg::kitstore::history())
                content.addAndMakeVisible (rows.add (new KitRow (proc, kit, {}, *this, onExport, onLoaded)));
        }
        resized();
        repaint();
    }

    void KitsPage::refresh()
    {
        rebuild();
    }

    void KitsPage::resized()
    {
        auto r = getLocalBounds().reduced (10, 8).reduced (14, 12);
        r.removeFromTop (26);
        auto top = r.removeFromTop (32);
        view.setBounds (top.removeFromLeft (220));
        openFile.setBounds (top.removeFromRight (160));
        r.removeFromTop (10);
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
            g.drawFittedText (view.getSelected() == 0 ? "No saved kits yet. Press the heart on a kit, or SAVE KIT, to keep it here."
                                                      : "Kits you generate show up here.",
                              viewport.getBounds(), juce::Justification::centred, 2);
        }
    }
}
