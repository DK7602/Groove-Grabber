// GGTests --tutorial DIR: walks through Groove Grabber the way a new user would (add a drum folder, describe the
// song, generate, hear and shape a kit, export, SAUCE, MY GROOVES, COACH MY BEAT, FX, MY KITS, the manual) and saves
// each step as a picture of the real window, with where the things to point at are, plus a beat the first kit
// plays. tools/tutorial/make_video.py turns them into the video tutorial.
//
// Run it with HOME set to a throwaway folder (tools/tutorial/make.sh uses /Users/you): it makes its own sample
// library there and its paths show on screen.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TestSounds.h"
#include "core/MyGrooves.h"
#include "ui/KitViews.h"
#include <iostream>

namespace
{
    void pumpFor (int ms)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
        while (juce::Time::getMillisecondCounter() < end)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (15);
    }

    bool shownIn (const juce::Component* c, const juce::Component* root)
    {
        for (auto* p = c; p != nullptr && p != root; p = p->getParentComponent())
            if (! p->isVisible()) return false;
        return true;
    }

    template <typename T>
    std::vector<T*> all (juce::Component& root)
    {
        std::vector<T*> out;
        std::function<void (juce::Component&)> walk = [&] (juce::Component& c)
        {
            if (auto* t = dynamic_cast<T*> (&c); t != nullptr && shownIn (&c, &root)) out.push_back (t);
            for (auto* child : c.getChildren()) walk (*child);
        };
        walk (root);
        return out;
    }

    juce::Button* button (juce::Component& root, const juce::String& text, bool startsWith = false)
    {
        for (auto* b : all<juce::Button> (root))
        {
            const auto t = b->getButtonText().trim();
            if (startsWith ? t.startsWithIgnoreCase (text) : t.equalsIgnoreCase (text)) return b;
        }
        return nullptr;
    }

    void click (juce::Button* b, int settleMs = 250)
    {
        if (b != nullptr) b->triggerClick();
        else std::cout << "  (a button to click wasn't there)" << std::endl;
        pumpFor (settleMs);
    }

    // A menu as the app draws one (its look and feel's menu drawing), placed in the window so the picture has it.
    // (A real menu closes itself at once here: with no window in front, JUCE takes the app for one in the background.)
    struct DrawnMenu : public juce::Component
    {
        struct Item { juce::String text; bool header = false, separator = false, enabled = true, ticked = false, sub = false, lit = false; };
        std::vector<Item> items;
        std::vector<int> heights;

        static std::unique_ptr<DrawnMenu> from (const juce::PopupMenu& m, const juce::String& lit = {})
        {
            auto d = std::make_unique<DrawnMenu>();
            for (juce::PopupMenu::MenuItemIterator it (m); it.next();)
            {
                const auto& i = it.getItem();
                d->items.push_back ({ i.text, i.isSectionHeader, i.isSeparator, i.isEnabled, i.isTicked, i.subMenu != nullptr,
                                      lit.isNotEmpty() && i.text == lit });
            }
            return d;
        }

        void place (juce::Component& parent, juce::Point<int> topLeft)
        {
            parent.addAndMakeVisible (this);
            auto& lf = getLookAndFeel();
            const int border = lf.getPopupMenuBorderSize();
            int w = 120, h = 0;
            heights.clear();
            for (const auto& it : items)
            {
                int iw = 0, ih = 0;
                lf.getIdealPopupMenuItemSize (it.text, it.separator, -1, iw, ih);
                if (it.sub) iw += 20;
                w = juce::jmax (w, iw + 16);
                heights.push_back (ih);
                h += ih;
            }
            setBounds (topLeft.x, topLeft.y, w + 2 * border, h + 2 * border);
            toFront (false);
        }

        int itemY (int index) const
        {
            int y = getLookAndFeel().getPopupMenuBorderSize();
            for (int i = 0; i < index && i < (int) heights.size(); ++i) y += heights[(size_t) i];
            return getY() + y;
        }

        void paint (juce::Graphics& g) override
        {
            auto& lf = getLookAndFeel();
            juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 14, { 0, 4 }).drawForRectangle (g, getLocalBounds());
            lf.drawPopupMenuBackground (g, getWidth(), getHeight());
            const int border = lf.getPopupMenuBorderSize();
            int y = border;
            for (size_t i = 0; i < items.size(); ++i)
            {
                const auto& it = items[i];
                const juce::Rectangle<int> r (border, y, getWidth() - 2 * border, heights[i]);
                if (it.header) lf.drawPopupMenuSectionHeader (g, r, it.text);
                else lf.drawPopupMenuItem (g, r, it.separator, it.enabled, it.lit, it.ticked, it.sub, it.text, {}, nullptr, nullptr);
                y += heights[i];
            }
        }
    };

    struct Capture
    {
        GrooveGrabberEditor& ed;
        juce::File dir;
        float scale = 1.5f;
        juce::DynamicObject::Ptr frames = new juce::DynamicObject();
        juce::DynamicObject::Ptr marks = new juce::DynamicObject();
        juce::StringArray order;

        juce::Rectangle<float> area (juce::Component* c) const
        {
            return ed.getLocalArea (c, c->getLocalBounds().toFloat());
        }

        void mark (const juce::String& key, juce::Rectangle<float> r)
        {
            juce::Array<juce::var> a { r.getX(), r.getY(), r.getWidth(), r.getHeight() };
            marks->setProperty (key, a);
        }

        void mark (const juce::String& key, juce::Component* c)
        {
            if (c != nullptr) mark (key, area (c));        // (a part that isn't on screen now has no mark)
        }

        template <typename Range>
        void markUnion (const juce::String& key, const Range& comps)
        {
            juce::Rectangle<float> u;
            for (auto* c : comps) u = u.isEmpty() ? area (c) : u.getUnion (area (c));
            if (u.isEmpty()) { std::cout << "  (nothing to mark for " << key << ")" << std::endl; return; }
            mark (key, u);
        }

        // a rectangle in one of the window's components, in the window's coordinates
        void markIn (const juce::String& key, juce::Component& c, juce::Rectangle<float> r)
        {
            mark (key, ed.getLocalArea (&c, r));
        }

        void clearMarks()   { marks = new juce::DynamicObject(); }

        void snap (const juce::String& name)
        {
            pumpFor (120);
            auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, scale);
            const auto f = dir.getChildFile ("frames").getChildFile (name + ".png");
            f.getParentDirectory().createDirectory();
            f.deleteFile();
            {
                juce::FileOutputStream out (f);
                juce::PNGImageFormat png;
                png.writeImageToStream (img, out);
            }
            auto* o = new juce::DynamicObject();
            o->setProperty ("file", "frames/" + name + ".png");
            o->setProperty ("marks", juce::var (marks->clone().release()));     // (a copy the frame owns)
            frames->setProperty (name, juce::var (o));
            order.add (name);
            std::cout << "  " << name << std::endl;
        }
    };

    // A library that looks like a producer's: three packs and a folder of downloads with no names worth reading
    juce::File makeDemoLibrary (const juce::File& root)
    {
        root.deleteRecursively();
        ggtest::Synth s;
        auto add = [&] (const juce::String& rel, juce::AudioBuffer<float> b, bool vintage = false)
        {
            if (vintage) s.vintage (b, 4800, -50);
            ggtest::Synth::write (root.getChildFile (rel), b, s.sr);
        };
        // dusty, sampled-sounding drums
        const juce::String crate = "Crate Digger Drums/";
        add (crate + "Kicks/Kick Crate 01.wav", s.kick (56, 0.30f, 0.30f), true);
        add (crate + "Kicks/Kick Crate 02.wav", s.kick (50, 0.36f, 0.25f), true);
        add (crate + "Kicks/Kick Crate 03.wav", s.kick (61, 0.26f, 0.40f), true);
        add (crate + "Kicks/Kick Crate 04.wav", s.kick (53, 0.40f, 0.20f), true);
        add (crate + "Snares/Snare Crate 01.wav", s.snare (182, 0.22f, 0.70f), true);
        add (crate + "Snares/Snare Crate 02.wav", s.snare (200, 0.18f, 0.90f), true);
        add (crate + "Snares/Snare Crate 03.wav", s.snare (170, 0.26f, 0.65f), true);
        add (crate + "Hats/Closed Hat Crate 01.wav", s.hat (0.040f), true);
        add (crate + "Hats/Closed Hat Crate 02.wav", s.hat (0.055f), true);
        add (crate + "Hats/Open Hat Crate 01.wav", s.hat (0.40f), true);
        add (crate + "Perc/Rim Crate.wav", s.rim(), true);
        add (crate + "Perc/Tambourine Crate.wav", s.shaker(), true);
        add (crate + "Cymbals/Crash Crate.wav", s.crash (2.0f), true);
        add (crate + "Cymbals/Ride Crate.wav", s.ride (1.8f), true);
        // clean, electronic
        const juce::String trap = "Midnight Trap Kit/";
        add (trap + "Kicks/Kick Hard 01.wav", s.kick (52, 0.22f, 0.8f));
        add (trap + "Kicks/Kick Hard 02.wav", s.kick (58, 0.20f, 1.0f));
        add (trap + "Kicks/Kick Punch 03.wav", s.kick (60, 0.18f, 0.9f));
        add (trap + "808s/808 Long G.wav", s.bass808 (49.0f, 1.4f));
        add (trap + "808s/808 Long F.wav", s.bass808 (43.65f, 1.5f));
        add (trap + "808s/808 Short C.wav", s.bass808 (65.4f, 0.9f));
        add (trap + "808s/808 Mid A.wav", s.bass808 (55.0f, 1.1f));
        add (trap + "Snares/Snare Crack 01.wav", s.snare (230, 0.14f, 1.0f));
        add (trap + "Snares/Snare Crack 02.wav", s.snare (245, 0.12f, 1.0f));
        add (trap + "Claps/Clap Wide.wav", s.clap (0.14f));
        add (trap + "Claps/Clap Tight.wav", s.clap (0.10f));
        add (trap + "Claps/Clap Stack.wav", s.clap (0.18f));
        add (trap + "Hats/HH Closed 01.wav", s.hat (0.030f));
        add (trap + "Hats/HH Closed 02.wav", s.hat (0.025f));
        add (trap + "Hats/HH Open 01.wav", s.hat (0.32f));
        add (trap + "Perc/Snap 01.wav", s.rim());
        add (trap + "Perc/Cowbell 01.wav", s.cowbell());
        add (trap + "FX/Riser Up.wav", s.riser (2.2f));
        add (trap + "FX/Riser Long.wav", s.riser (3.0f));
        // a live kit
        const juce::String live = "Live Room Session/";
        add (live + "Kick Room 01.wav", s.kick (62, 0.38f, 0.7f));
        add (live + "Kick Room 02.wav", s.kick (66, 0.34f, 0.6f));
        add (live + "Snare Room 01.wav", s.snare (210, 0.30f, 0.85f));
        add (live + "Snare Room 02.wav", s.snare (195, 0.34f, 0.80f));
        add (live + "Tom Floor.wav", s.tom (90));
        add (live + "Tom Rack Low.wav", s.tom (128));
        add (live + "Tom Rack High.wav", s.tom (182));
        add (live + "Hihat Closed 01.wav", s.hat (0.045f));
        add (live + "Hihat Open 01.wav", s.hat (0.45f));
        add (live + "Ride Bell.wav", s.ride());
        add (live + "Crash 18.wav", s.crash());
        add (live + "Crash 20.wav", s.crash (2.8f));
        add (live + "Shaker Live.wav", s.shaker());
        add (live + "Cowbell Live.wav", s.cowbell());
        // downloads with no useful names: the sound decides
        add ("Downloads/Untitled 01.wav", s.kick (55, 0.25f, 0.6f));
        add ("Downloads/Untitled 02.wav", s.bass808 (46.25f, 1.5f));
        add ("Downloads/Untitled 03.wav", s.snare (205, 0.18f, 0.9f));
        add ("Downloads/Untitled 04.wav", s.clap (0.15f));
        add ("Downloads/Untitled 05.wav", s.hat (0.03f));
        add ("Downloads/Untitled 06.wav", s.tom (118));
        // an effects pack (the FX page)
        const juce::String fx = "Hit Factory FX/";
        add (fx + "Risers/Riser Big.wav", s.riser (9.5f));       // (about 3 bars at 88: FIT 2 trims it)
        add (fx + "Transitions/Reverse Crash.wav", s.reverseCymbal (2.0f));
        add (fx + "Transitions/Downlifter Dark.wav", s.downlifter (2.5f));
        add (fx + "Transitions/Whoosh Fast.wav", s.whoosh (1.2f));
        add (fx + "Hits/Cinematic Impact.wav", s.impact (2.5f));
        add (fx + "Hits/Sub Drop.wav", s.subDrop (1.8f));
        add (fx + "Ear Candy/Tape Stop.wav", s.tapeStop (1.2f));
        add (fx + "Ear Candy/Vinyl Crackle.wav", s.crackle (4.0f));
        add (fx + "Ear Candy/Scratch Chirp.wav", s.scratch (1.0f));
        add (fx + "Ear Candy/Laser Zap.wav", s.laser (0.4f));
        add (fx + "Ear Candy/Air Horn.wav", s.horn (1.4f));
        return root;
    }

    // the first kit playing a groove, for the video's music: one bar to start (so the loop has its tails), then 8
    bool renderBeat (const gg::Kit& kit, const juce::String& grooveId, double bpm, const juce::File& out)
    {
        const double sr = 44100.0;
        auto pk = gg::buildPlayable (kit, sr);
        gg::KitPlayer player;
        player.prepare (sr, 512);
        player.setFollowHost (false);
        player.setKit (pk);
        const auto& g = gg::grooveFor (grooveId);
        player.setGroove (&g);
        player.setTempo (bpm);
        player.setSwing (g.swing);
        player.setVolume (0.9f);
        player.setGroovePlaying (true);
        const int barSamples = (int) std::round (4.0 * 60.0 / bpm * sr);
        const int total = barSamples * 9;
        juce::AudioBuffer<float> all (2, total), block (2, 512);
        juce::MidiBuffer none;
        for (int pos = 0; pos < total; pos += 512)
        {
            const int n = juce::jmin (512, total - pos);
            block.setSize (2, n, false, false, true);
            block.clear();
            player.process (block, none, {});
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, pos, block, ch, 0, n);
        }
        juce::AudioBuffer<float> loop (2, barSamples * 8);
        for (int ch = 0; ch < 2; ++ch) loop.copyFrom (ch, 0, all, ch, barSamples, barSamples * 8);
        return ggtest::Synth::write (out, loop, sr);
    }
}

int runTutorial (const juce::File& outDir)
{
    std::cout << "Groove Grabber tutorial pictures -> " << outDir.getFullPathName() << std::endl;
    outDir.getChildFile ("frames").deleteRecursively();
    outDir.createDirectory();
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const auto drums = makeDemoLibrary (home.getChildFile ("Music").getChildFile ("Drum Samples"));

    GrooveGrabberProcessor proc;
    proc.prepareToPlay (44100.0, 512);
    proc.getSettings().setLightTheme (false);
    pumpFor (400);

    auto ed = std::unique_ptr<GrooveGrabberEditor> (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
    ed->setSize (1920, 1080);
    Capture cap { *ed, outDir };
    auto& content = ed->getContent();
    juce::var info (new juce::DynamicObject());

    // ---- LIBRARY: add your drum folder ----
    ed->showTab (1);
    ed->refreshAll();
    pumpFor (500);
    cap.markUnion ("tabs", std::vector<juce::Component*> { const_cast<juce::TextButton*> (&ed->getHeader().getTabButton (0)),
                                                           const_cast<juce::TextButton*> (&ed->getHeader().getTabButton (1)) });
    for (int t = 0; t < 5; ++t)
        cap.mark ("tab" + juce::String (t), const_cast<juce::TextButton*> (&ed->getHeader().getTabButton (t)));
    cap.mark ("addFolder", button (content, "+ ADD DRUM FOLDER"));
    cap.snap ("lib-empty");

    proc.library->addSource (drums, gg::Source::packs);
    pumpFor (300);
    for (int i = 0; i < 600 && proc.library->isScanning(); ++i) pumpFor (25);
    ed->refreshAll();
    pumpFor (600);
    cap.mark ("addFolder", button (content, "+ ADD DRUM FOLDER"));
    if (auto tables = all<juce::TableListBox> (content); ! tables.empty()) cap.mark ("table", tables.front());
    {
        std::vector<juce::Component*> chips;
        for (auto* t : { "ALL", "KICK", "808", "SNARE", "CLAP", "RIM", "CLOSED HAT", "OPEN HAT", "TOM", "CRASH", "RIDE", "PERC", "FX" })
            if (auto* b = button (content, t)) chips.push_back (b);
        cap.markUnion ("chips", chips);
        if (auto* b = button (content, "808")) cap.mark ("chip808", b);
    }
    if (auto eds = all<juce::TextEditor> (content); ! eds.empty()) cap.mark ("search", eds.front());
    cap.snap ("lib-full");

    // the 808 chip: only the 808s
    click (button (content, "808"), 400);
    cap.snap ("lib-808");
    click (button (content, "ALL"), 300);

    // files with no useful names: the sound sorted them
    auto* search = all<juce::TextEditor> (content).front();
    search->setText ("untitled", true);
    pumpFor (500);
    cap.snap ("lib-untitled");

    // right-click a sound: fix its type, favourite it, never use it
    if (auto tables = all<juce::TableListBox> (content); ! tables.empty())
    {
        auto* table = tables.front();
        juce::PopupMenu types;
        for (int c = 0; c < gg::numCats; ++c)
            if (c != (int) gg::Cat::unknown) types.addItem (100 + c, gg::catName ((gg::Cat) c), true, c == (int) gg::Cat::kick);
        juce::PopupMenu m;
        m.addSectionHeader ("Untitled 01.wav");
        m.addItem (1, "Listen");
        m.addSubMenu ("It's really a...", types);
        m.addItem (2, "Favourite (picked more often)");
        m.addItem (3, "Never use this sound");
        m.addItem (5, "Put it on the selected pad (Kick)", false);
        m.addItem (4, "Show the file");
        const auto row = table->getRowPosition (0, true);
        cap.markIn ("row", *table, row.toFloat());
        const auto at = content.getLocalArea (table, row).getCentre() + juce::Point<int> (-40, 8);
        auto menu = DrawnMenu::from (m, "It's really a...");
        menu->place (content, at);
        auto sub = DrawnMenu::from (types, "Kick");
        sub->place (content, { menu->getRight() - 4, menu->itemY (2) - 6 });
        cap.mark ("menu", menu.get());
        cap.mark ("submenu", sub.get());
        cap.snap ("lib-menu");
    }
    search->setText ({}, true);
    pumpFor (300);

    // ---- BUILD: describe the song ----
    ed->showTab (0);
    ed->refreshAll();
    pumpFor (400);
    cap.clearMarks();
    auto* description = all<juce::TextEditor> (ed->getBuild()).front();
    cap.mark ("description", description);
    cap.mark ("generate", button (content, "GENERATE KITS"));
    cap.snap ("build-empty");
    const juce::String words = "Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor";
    const juce::StringArray stops { "Dark 90s", "Dark 90s boom bap,", "Dark 90s boom bap, punchy but warm,",
                                    "Dark 90s boom bap, punchy but warm, vinyl vibe,", "Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM", words };
    for (int i = 0; i < stops.size(); ++i)
    {
        proc.applyDescription (stops[i]);      // (what typing it does)
        ed->refreshAll();
        pumpFor (250);
        cap.snap ("type-" + juce::String (i + 1));
    }
    {
        auto combos = all<juce::ComboBox> (ed->getBuild());
        std::vector<juce::Component*> songBoxes;
        for (auto* c : combos) if (c->getY() < 400 && cap.area (c).getX() < 500) songBoxes.push_back (c);
        cap.markUnion ("songBoxes", songBoxes);
        const auto d = cap.area (description);
        if (! songBoxes.empty())
            cap.mark ("understood", juce::Rectangle<float> (d.getX(), d.getBottom(), d.getWidth(), cap.area (songBoxes.front()).getY() - d.getBottom()));
        std::vector<juce::Component*> header;
        for (auto* c : all<juce::ComboBox> (ed->getHeader())) header.push_back (c);
        for (auto* c : all<ggui::DragNumber> (ed->getHeader())) header.push_back (c);
        cap.markUnion ("keyBpm", header);
        std::vector<juce::Component*> sliders;
        for (auto* sl : all<juce::Slider> (ed->getBuild())) if (cap.area (sl).getX() < 500) sliders.push_back (sl);
        cap.markUnion ("character", sliders);
        std::vector<juce::Component*> segs;
        for (auto* sg : all<ggui::Segmented> (ed->getBuild())) segs.push_back (sg);
        cap.markUnion ("options", segs);
        if (! segs.empty()) cap.mark ("kitSize", segs.front());
        std::vector<juce::Component*> toggles;
        for (auto* t : all<juce::ToggleButton> (ed->getBuild())) if (cap.area (t).getX() < 500) toggles.push_back (t);
        cap.markUnion ("toggles", toggles);
    }
    cap.mark ("generate", button (content, "GENERATE KITS"));
    cap.snap ("build-described");

    // ---- GENERATE: four kits ----
    click (button (content, "GENERATE KITS"), 300);
    for (int i = 0; i < 80; ++i) pumpFor (25);
    ed->refreshAll();
    pumpFor (800);
    auto markCards = [&]
    {
        auto cards = all<ggui::KitCard> (ed->getBuild());
        std::sort (cards.begin(), cards.end(), [] (auto* a, auto* b) { return a->getX() < b->getX(); });
        cap.markUnion ("cards", cards);
        for (int i = 0; i < (int) cards.size(); ++i) cap.mark ("card" + juce::String (i), cards[(size_t) i]);
        cap.markUnion ("pads", all<ggui::PadTile> (ed->getBuild()));
        if (auto d = all<ggui::PadDetail> (ed->getBuild()); ! d.empty()) cap.mark ("detail", d.front());
        cap.markUnion ("alts", all<ggui::AltTile> (ed->getBuild()));
        cap.markUnion ("knobs", all<ggui::LabelledKnob> (ed->getBuild()));
        cap.mark ("transport", &ed->getBuild().getTransport());
        cap.mark ("regenerate", button (content, "REGENERATE"));
        cap.mark ("lockAll", button (content, "LOCK ALL"));
        cap.mark ("saveKit", button (content, "SAVE KIT"));
        cap.mark ("export", button (content, "EXPORT"));
        cap.mark ("generate", button (content, "GENERATE KITS"));
        cap.mark ("swing", &ed->getBuild().getTransport().getSwingSlider());
        if (auto c = all<juce::ComboBox> (ed->getBuild().getTransport()); ! c.empty()) cap.mark ("grooveBox", c.front());
        if (auto* f = button (ed->getBuild().getTransport(), "Follow DAW")) cap.mark ("follow", f);
        if (auto* e = button (ed->getBuild().getTransport(), "EDIT IN SAUCE")) cap.mark ("editInSauce", e);
        for (auto* b : all<ggui::IconButton> (ed->getBuild().getTransport()))
            if (b->getButtonText() == "play") cap.mark ("transportPlay", b);
    };
    markCards();
    cap.snap ("build-kits");
    {
        juce::Array<juce::var> names;
        for (auto& k : proc.kits) names.add (k.name);
        info.getDynamicObject()->setProperty ("kits", names);
    }
    auto beatKit = proc.kits.front();

    // click a kit: it plays the groove
    proc.setActiveKit (1);
    proc.player.setGroovePlaying (true);
    ed->refreshAll();
    pumpFor (700);
    markCards();
    cap.snap ("build-kit2");

    // a pad: its sound, knobs and alternatives
    proc.player.setGroovePlaying (false);
    proc.auditionSlot (gg::Slot::snare);
    ed->refreshAll();
    pumpFor (500);
    markCards();
    for (auto* t : all<ggui::PadTile> (ed->getBuild())) if (t->getSlot() == gg::Slot::snare) cap.mark ("snarePad", t);
    cap.mark ("reverse", button (content, "REVERSE"));
    cap.mark ("lock", button (content, "LOCK"));
    cap.snap ("build-pad");

    // REGENERATE: new sounds where nothing is locked
    click (button (content, "REGENERATE"), 300);
    for (int i = 0; i < 40; ++i) pumpFor (25);
    ed->refreshAll();
    pumpFor (500);
    markCards();
    cap.snap ("build-regen");
    click (button (content, "SAVE KIT"), 300);         // (for MY KITS later)
    proc.setActiveKit (0);
    ed->refreshAll();
    pumpFor (400);
    click (button (content, "SAVE KIT"), 300);

    // ---- EXPORT ----
    markCards();
    cap.snap ("build-before-export");
    click (button (content, "EXPORT"), 500);
    cap.clearMarks();
    auto& ex = ed->getExporter();
    if (auto c = all<juce::ComboBox> (ex); ! c.empty()) cap.mark ("forBox", c.front());
    cap.mark ("exportKit", button (ex, "EXPORT KIT"));
    cap.mark ("panel", &ex);
    {
        std::vector<juce::Component*> t;
        for (auto* b : all<juce::ToggleButton> (ex)) t.push_back (b);
        cap.markUnion ("exportToggles", t);
        std::vector<juce::Component*> s;
        for (auto* b : all<ggui::Segmented> (ex)) s.push_back (b);
        cap.markUnion ("format", s);
    }
    cap.snap ("export");
    if (auto c = all<juce::ComboBox> (ex); ! c.empty())
    {
        auto* box = c.front();
        int pick = -1;
        for (int i = 0; i < box->getNumItems(); ++i)
            if (box->getItemText (i).contains ("Groove Agent")) pick = i;
        auto menu = DrawnMenu::from (*box->getRootMenu(), pick >= 0 ? box->getItemText (pick) : juce::String());
        const auto b = content.getLocalArea (box, box->getLocalBounds());
        menu->place (content, { b.getX(), b.getBottom() + 2 });
        if (menu->getWidth() < b.getWidth()) menu->setSize (b.getWidth(), menu->getHeight());
        cap.mark ("menu", menu.get());
        cap.snap ("export-daws");
        menu.reset();
        if (pick >= 0) box->setSelectedItemIndex (pick, juce::sendNotificationSync);
        pumpFor (300);
        cap.clearMarks();
        if (auto cc = all<juce::ComboBox> (ex); ! cc.empty()) cap.mark ("forBox", cc.front());
        cap.mark ("exportKit", button (ex, "EXPORT KIT"));
        cap.mark ("panel", &ex);
        cap.snap ("export-cubase");
        info.getDynamicObject()->setProperty ("daw", box->getText());
    }
    click (button (ex, "EXPORT KIT"), 500);
    for (int i = 0; i < 200 && button (ex, "SHOW FOLDER") == nullptr; ++i) pumpFor (50);
    pumpFor (300);
    cap.mark ("showFolder", button (ex, "SHOW FOLDER"));
    cap.snap ("export-done");
    click (button (ex, "CLOSE"), 300);

    // ---- SAUCE ----
    ed->showTab (3);
    ed->refreshAll();
    pumpFor (600);
    cap.clearMarks();
    auto& sauce = ed->getSauce();
    auto markSauce = [&]
    {
        std::vector<juce::Component*> boxes;
        for (auto* c : all<juce::ComboBox> (sauce)) boxes.push_back (c);
        cap.markUnion ("recipe", boxes);
        cap.mark ("grid", &sauce.getGrid());
        std::vector<juce::Component*> chips;
        for (auto* t : { "BLANK", "BOOM BAP", "DUSTY SWING" }) if (auto* b = button (sauce, t)) chips.push_back (b);
        if (auto* b = button (sauce, "MY", true)) { chips.push_back (b); cap.mark ("myChip", b); }
        cap.markUnion ("grooveChips", chips);
        if (auto* b = button (sauce, "BLANK")) cap.mark ("blank", b);
        std::vector<juce::Component*> kitRow;
        for (auto* t : { "ORIGINAL", "01", "02", "03", "04" }) if (auto* b = button (sauce, t)) kitRow.push_back (b);
        cap.markUnion ("kitRow", kitRow);
        if (auto* b = button (sauce, "ORIGINAL")) cap.mark ("original", b);
        for (auto* b : all<ggui::IconButton> (sauce)) if (b->getButtonText() == "play") cap.mark ("play", b);
        cap.markIn ("drag", sauce, sauce.getDragArea().toFloat());
        if (auto v = all<juce::Viewport> (sauce); ! v.empty()) cap.mark ("doc", v.front());
        if (auto* b = button (sauce, "COACH", true)) cap.mark ("coachBtn", b);
        if (auto sl = all<juce::Slider> (sauce); ! sl.empty()) cap.mark ("swingSlider", sl.front());
        if (auto* b = button (sauce, "SAVE AS MY GROOVE")) cap.mark ("saveMine", b);
        if (auto* b = button (sauce, "SAVE AS COPY")) cap.mark ("saveCopy", b);
        cap.mark ("addDrum", button (sauce, "+ ADD DRUM"));
        cap.mark ("undo", button (sauce, "UNDO"));
    };
    markSauce();
    cap.markIn ("left", sauce, sauce.getLocalBounds().withWidth (juce::jlimit (300, 360, (sauce.getWidth() - 20) / 4) + 10).toFloat());
    cap.snap ("sauce");
    sauce.togglePlay();
    pumpFor (1700);
    markSauce();
    cap.snap ("sauce-playing");
    sauce.togglePlay();
    pumpFor (200);

    // change the pattern: a kick, a ghost note, a hat roll
    {
        auto& grid = sauce.getGrid();
        auto cell = [&] (const juce::String& key, int lane, int step) { cap.markIn (key, grid, grid.cellBounds (lane, step)); };
        cell ("cell1", 0, 10);
        grid.clickStep (0, 10, {});
        pumpFor (150);
        markSauce();
        cap.snap ("sauce-edit-1");
        cell ("cell2", 1, 7);
        grid.clickStep (1, 7, juce::ModifierKeys (juce::ModifierKeys::rightButtonModifier));
        pumpFor (150);
        cap.snap ("sauce-edit-2");
        cell ("cell3", 2, 14);
        grid.clickStep (2, 14, juce::ModifierKeys (juce::ModifierKeys::ctrlModifier));
        pumpFor (150);
        cap.snap ("sauce-edit-3");
        sauce.setSwingPercent (58);
        pumpFor (150);
        markSauce();
        cap.snap ("sauce-swing");
    }

    // SAVE AS MY GROOVE
    sauce.startNaming();
    pumpFor (200);
    if (auto eds = all<juce::TextEditor> (sauce); ! eds.empty())
    {
        eds.front()->setText ("Late Kick Bounce", false);
        cap.mark ("nameBox", eds.front());
    }
    cap.snap ("sauce-naming");
    sauce.finishNaming (true, "Late Kick Bounce");
    pumpFor (300);
    const auto mine = sauce.getGrooveId();
    markSauce();
    cap.snap ("sauce-mine");

    // later, with another kit on BUILD: MY GROOVES offers its original kit
    proc.setActiveKit (2);
    sauce.openGroove ("boombap1");
    pumpFor (300);
    cap.clearMarks();
    markSauce();
    if (auto* chip = button (sauce, "MY GROOVES", true))
    {
        // as SAUCE's MY GROOVES menu shows it: the groove, and how to load it
        const auto* e = gg::mygrooves::entry (mine);
        const auto* buildKit = proc.getActiveKit();
        juce::PopupMenu load, m;
        load.addItem (1, "With its original kit: " + (e != nullptr && e->kit != nullptr ? e->kit->name : juce::String()));
        load.addItem (2, "Onto BUILD's current kit: " + juce::String (proc.activeKit + 1).paddedLeft ('0', 2) + " " + (buildKit != nullptr ? buildKit->name : juce::String()));
        m.addSectionHeader ("MY HIP-HOP GROOVES");
        const auto text = (e != nullptr ? e->groove->name : juce::String ("Late Kick Bounce")) + "   (" + juce::String (e != nullptr ? e->groove->bpm : 88.0, 0) + ")";
        m.addSubMenu (text, load);
        auto menu = DrawnMenu::from (m, text);
        const auto b = content.getLocalArea (chip, chip->getLocalBounds());
        menu->place (content, { b.getX(), b.getBottom() + 2 });
        auto sub = DrawnMenu::from (load, load.getNumItems() > 0 ? juce::String ("With its original kit: ") + (e != nullptr && e->kit != nullptr ? e->kit->name : juce::String()) : juce::String());
        sub->place (content, { menu->getRight() - 4, menu->itemY (1) - 6 });
        cap.mark ("menu", menu.get());
        cap.mark ("submenu", sub.get());
        cap.snap ("sauce-mygrooves-menu");
    }
    sauce.pickMine (mine, true);
    pumpFor (600);
    cap.clearMarks();
    markSauce();
    cap.snap ("sauce-original");

    // BLANK, then COACH MY BEAT
    sauce.openGroove ("blank");
    pumpFor (300);
    cap.clearMarks();
    markSauce();
    cap.snap ("sauce-blank-empty");
    {
        auto& grid = sauce.getGrid();
        for (int step : { 0, 8, 10 }) grid.clickStep (0, step, {});        // the kick
        // the snare's row: BLANK has a row per sound of the kit, in the pads' order (Kick, 808, Snare...)
        int snareLane = 0, row = 0;
        if (const auto* k = proc.kitOnSauce())
            for (auto sl : k->shownSlots())
                if (! (*k)[sl].isEmpty()) { if (sl == gg::Slot::snare) snareLane = row; ++row; }
        for (int step : { 4, 12 }) grid.clickStep (snareLane, step, {});
        pumpFor (200);
        markSauce();
        cap.snap ("sauce-blank");
    }
    sauce.coachMyBeat();
    pumpFor (500);
    markSauce();
    cap.mark ("coach", &sauce.getCoach());
    // TRY IT on "Add the hats" (a change you can see), or the first tip
    int tip = 0;
    {
        const auto& fixes = sauce.getCoach().getReport().fixes;
        for (int i = 0; i < (int) fixes.size(); ++i) if (fixes[(size_t) i].id == "hats.none" && fixes[(size_t) i].hasFix) tip = i;
        std::vector<juce::Button*> tries, purposes;
        for (auto* b : all<juce::Button> (sauce.getCoach()))
        {
            if (b->getButtonText() == "TRY IT") tries.push_back (b);
            if (b->getButtonText() == "ON PURPOSE") purposes.push_back (b);
        }
        auto byY = [this_ = &cap] (juce::Button* a, juce::Button* b) { return this_->area (a).getY() < this_->area (b).getY(); };
        std::sort (tries.begin(), tries.end(), byY);
        std::sort (purposes.begin(), purposes.end(), byY);
        // scroll the list to that tip
        if (tip < (int) tries.size())
            if (auto vps = all<juce::Viewport> (sauce.getCoach()); ! vps.empty())
                if (auto* viewed = vps.front()->getViewedComponent())
                {
                    const int y = viewed->getLocalPoint (tries[(size_t) tip], juce::Point<int>()).y;
                    vps.front()->setViewPosition (0, juce::jmax (0, y - 40));
                    pumpFor (150);
                }
        if (tip < (int) tries.size()) cap.mark ("tryIt", tries[(size_t) tip]);
        if (tip < (int) purposes.size()) cap.mark ("onPurpose", purposes[(size_t) tip]);
    }
    cap.snap ("sauce-coach");
    sauce.getCoach().tryTip (tip);
    pumpFor (600);
    markSauce();
    cap.mark ("coach", &sauce.getCoach());
    cap.snap ("sauce-coach-after");

    // ---- FX: your effects by kind, heard in time, fit to the bars, on every kit ----
    ed->showTab (4);
    proc.fxCatalog->waitUntilDone (30000);
    pumpFor (400);
    ed->refreshAll();
    pumpFor (400);
    cap.clearMarks();
    auto& fxp = ed->getFxPage();
    auto markFx = [&]
    {
        fxp.createComponentSnapshot (fxp.getLocalBounds(), true, 1.0f);      // (painted: the facts' height is known)
        for (const auto* k : { "kinds", "facts", "drag", "fit" })
            if (const auto r = fxp.tutorialArea (k); ! r.isEmpty())
                cap.markIn (juce::String ("fx.") + k, fxp, r.toFloat());
        cap.markIn ("fxDetail", fxp, fxp.tutorialArea ("detail").toFloat());
        cap.markIn ("kindRiser", fxp, fxp.tutorialArea ("kind.riser").toFloat());
        cap.markIn ("kindHorn", fxp, fxp.tutorialArea ("kind.horn").toFloat());
        if (auto tables = all<juce::TableListBox> (fxp); ! tables.empty()) cap.mark ("fxTable", tables.front());
        if (auto* b = button (fxp, "HEAR IT IN TIME")) cap.mark ("inTime", b);
        if (auto* b = button (fxp, "STOP")) cap.mark ("inTime", b);
        if (auto* b = button (fxp, "ADD TO SONG FX")) cap.mark ("addBank", b);
        if (auto* b = button (fxp, "REMOVE FROM SONG FX")) cap.mark ("addBank", b);
        cap.mark ("onKit", button (fxp, "PUT ON THIS KIT"));
        cap.mark ("fitsFirst", button (fxp, "FITS MY SONG FIRST"));
        if (auto sg = all<ggui::Segmented> (fxp); ! sg.empty())
        {
            cap.mark ("fitBars", sg.front());
            if (auto* two = button (*sg.front(), "2")) cap.mark ("fit2", two);
        }
    };
    fxp.showType (-1);
    pumpFor (200);
    markFx();
    cap.snap ("fx");
    // a riser: its length in bars, its fit for the song
    fxp.showType ((int) gg::fx::Type::riser);
    pumpFor (150);
    fxp.selectRow (0);
    pumpFor (600);
    markFx();
    cap.snap ("fx-riser");
    // HEAR IT IN TIME
    fxp.hearInTime();
    pumpFor (900);
    markFx();
    cap.snap ("fx-intime");
    fxp.hearInTime();      // (stop)
    pumpFor (300);
    // FIT: 2 bars
    markFx();
    fxp.setFitBars (2);
    pumpFor (300);
    markFx();
    cap.snap ("fx-fit");
    // ADD TO SONG FX (with its FIT): on every kit
    fxp.addToSongFx();
    pumpFor (600);
    markFx();
    cap.snap ("fx-added");
    // an air horn: tuned to the key
    fxp.showType ((int) gg::fx::Type::horn);
    pumpFor (150);
    fxp.selectRow (0);
    pumpFor (600);
    markFx();
    cap.snap ("fx-tuned");
    proc.stopPreview();

    // BUILD: the Song FX row, and FIT in the pad editor
    ed->showTab (0);
    proc.selectedSlot = gg::Slot::sfx1;
    ed->refreshAll();
    pumpFor (700);
    cap.clearMarks();
    markCards();
    {
        std::vector<juce::Component*> bank;
        for (auto* t : all<ggui::PadTile> (ed->getBuild())) if (gg::isBankSlot (t->getSlot())) bank.push_back (t);
        cap.markUnion ("songFxRow", bank);
        for (auto* t : all<ggui::PadTile> (ed->getBuild())) if (t->getSlot() == gg::Slot::sfx1) cap.mark ("songFx1", t);
    }
    cap.mark ("fitBtn", button (content, "FIT:", true));
    cap.snap ("build-songfx");

    // ---- MY KITS ----
    ed->showTab (2);
    ed->refreshAll();
    pumpFor (600);
    cap.clearMarks();
    if (auto sg = all<ggui::Segmented> (ed->getKitsPage()); ! sg.empty()) cap.mark ("view", sg.front());
    if (auto v = all<juce::Viewport> (ed->getKitsPage()); ! v.empty()) cap.mark ("list", v.front());
    cap.mark ("load", button (ed->getKitsPage(), "LOAD"));
    if (auto eds = all<juce::TextEditor> (ed->getKitsPage()); ! eds.empty()) cap.mark ("search", eds.front());
    cap.snap ("kits");
    if (auto sg = all<ggui::Segmented> (ed->getKitsPage()); ! sg.empty())
    {
        sg.front()->setSelected (2, true);
        pumpFor (500);
        cap.mark ("open", button (ed->getKitsPage(), "OPEN"));
        cap.mark ("useOnBuild", button (ed->getKitsPage(), "USE ON BUILD"));
        cap.snap ("kits-grooves");
        sg.front()->setSelected (0, true);
    }

    // ---- the manual, and the light theme ----
    ed->showTab (0);
    ed->refreshAll();
    pumpFor (300);
    cap.clearMarks();
    cap.mark ("help", button (ed->getHeader(), "manual"));
    cap.mark ("theme", button (ed->getHeader(), "theme"));
    cap.mark ("gear", button (ed->getHeader(), "settings"));
    markCards();
    cap.snap ("build-final");
    ed->showHelp (0);
    pumpFor (600);
    cap.snap ("manual");
    ed->getHelp().setVisible (false);
    pumpFor (200);
    click (button (ed->getHeader(), "theme"), 300);
    ed->refreshAll();
    pumpFor (600);
    cap.snap ("build-light");
    click (button (ed->getHeader(), "theme"), 300);
    pumpFor (300);

    // the music: the first kit playing the groove BUILD played
    const bool beat = renderBeat (beatKit, "boombap1", 88.0, outDir.getChildFile ("beat.wav"));
    std::cout << (beat ? "  beat.wav" : "  (couldn't write the beat)") << std::endl;
    info.getDynamicObject()->setProperty ("version", GG_VERSION);
    info.getDynamicObject()->setProperty ("myGroove", mine);

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("width", ed->getWidth());
    root->setProperty ("height", ed->getHeight());
    root->setProperty ("scale", cap.scale);
    root->setProperty ("frames", juce::var (cap.frames.get()));
    root->setProperty ("info", info);
    outDir.getChildFile ("frames.json").replaceWithText (juce::JSON::toString (juce::var (root.get())));

    ed.reset();
    return 0;
}
