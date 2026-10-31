// Draws the whole window (dark and light, every page) to PNGs, so the look can be checked without a DAW.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

namespace
{
    void save (juce::Component& c, const juce::File& f)
    {
        auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
        f.deleteFile();
        juce::FileOutputStream out (f);
        juce::PNGImageFormat png;
        png.writeImageToStream (img, out);
        std::cout << "  wrote " << f.getFullPathName() << std::endl;
    }

    void pump (int ms)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
        while (juce::Time::getMillisecondCounter() < end)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }
}

int runScreenshots (const juce::File& outDir)
{
    std::cout << "\n== Screenshots" << std::endl;
    outDir.createDirectory();
    int failures = 0;

    GrooveGrabberProcessor proc;
    proc.prepareToPlay (44100.0, 512);
    // the library the engine tests built lives in the test home
    for (int i = 0; i < 400 && proc.library->isScanning(); ++i) pump (25);
    if (proc.library->getSounds()->empty())
    {
        std::cout << "  (no sounds in the test library - screenshots will be empty)" << std::endl;
        ++failures;
    }
    proc.applyDescription ("Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor");
    proc.options.lowEnd = 1;
    proc.options.percCount = 2;
    proc.generate (false);
    proc.selectedSlot = gg::Slot::bass808;
    for (int i = 0; i < 3 && i < (int) proc.kits.size(); ++i) { auto k = proc.kits[(size_t) i]; k.favorite = i == 0; gg::kitstore::save (k); }
    if (! proc.kits.empty())
    {
        // a few more genres, so MY KITS shows its genre chips
        struct G { gg::Genre genre; const char* style; const char* name; };
        for (const auto& x : { G { gg::Genre::rock, "rock.hard", "Rock Hard Hitter" }, G { gg::Genre::country, "country.classic", "Honky Tonk Classic" },
                               G { gg::Genre::trap, "trap.atlanta", "ATL Trap House" }, G { gg::Genre::rock, "rock.hard", "80s Arena Hard Rock" } })
        {
            auto k = proc.kits.front();
            k.id = juce::Uuid().toString();
            k.genre = x.genre;
            k.styleId = x.style;
            k.name = x.name;
            k.favorite = false;
            gg::kitstore::save (k);
        }
    }
    pump (1500);

    for (bool light : { false, true })
    {
        proc.getSettings().setLightTheme (light);
        std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
        ed->setSize (1440, 900);
        ed->showTab (0);
        ed->refreshAll();
        pump (900);
        ed->refreshAll();
        pump (300);
        const juce::String suffix = light ? "-light" : "";
        save (*ed, outDir.getChildFile ("build" + suffix + ".png"));
        ed->showTab (1);
        pump (300);
        save (*ed, outDir.getChildFile ("library" + suffix + ".png"));
        ed->showTab (2);
        pump (200);
        save (*ed, outDir.getChildFile ("kits" + suffix + ".png"));
        if (! light)
        {
            ed->setSize (1180, 760);
            ed->showTab (0);
            ed->refreshAll();
            pump (300);
            save (*ed, outDir.getChildFile ("build-small.png"));
        }
        ed.reset();
        pump (100);
    }
    proc.getSettings().setLightTheme (false);

    // many folders: the list must scroll, not squash the last one
    {
        auto sources = proc.getSettings().getSources();
        for (int i = 0; i < 12; ++i)
            sources.push_back ({ juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("GG Pack " + juce::String (i + 1)), gg::Source::packs, true });
        proc.getSettings().setSources (sources);
        std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
        ed->setSize (1440, 900);
        ed->showTab (1);
        pump (300);
        save (*ed, outDir.getChildFile ("library-many-folders.png"));
    }

    // saved state: round trip, and a damaged / oversized one doesn't crash
    {
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        GrooveGrabberProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool same = other.kits.size() == proc.kits.size() && other.description == proc.description;
        if (! same) { std::cout << "  FAIL: state round trip" << std::endl; ++failures; }
        auto v = juce::JSON::parse (mb.toString());
        if (auto* a = v["kits"].getArray()) { auto copy = *a; for (auto& k : copy) a->add (k); }   // 8 kits
        const auto big = juce::JSON::toString (v);
        other.setStateInformation (big.toRawUTF8(), (int) big.getNumBytesAsUTF8());
        pump (300);
        if (other.kits.size() > 4) { std::cout << "  FAIL: more than 4 kits loaded" << std::endl; ++failures; }
        const char junk[] = "{\"kits\": [ {\"slots\": {\"kick\": {\"path\": 5, \"tune\": \"abc\"}}} ], \"active\": 99}";
        other.setStateInformation (junk, (int) sizeof (junk) - 1);
        pump (300);
        std::cout << "  state round trip and damaged state ok" << std::endl;
    }

    // the genre box and the description together
    {
        GrooveGrabberProcessor p;
        auto expect = [&failures] (bool ok, const juce::String& what)
        {
            std::cout << (ok ? "  ok: " : "  FAIL: ") << what << std::endl;
            if (! ok) ++failures;
        };
        p.applyDescription ("90s g-funk");
        p.setGenre (gg::Genre::boomBap);
        expect (p.options.style().id == "hiphop.west", "typed g-funk, then picked Hip-Hop: still West Coast (" + p.options.style().id + ")");
        p.applyDescription ("");
        p.applyDescription ("like Dr. Dre");
        expect (p.options.genre == gg::Genre::boomBap && p.options.style().id == "hiphop.west" && p.options.era == 3
                && p.options.directionFor (0) == gg::directionIndex (p.options.style(), "west.knock"), "like Dr. Dre: 90s West Coast, kit 1 Westside Knock");
        p.setGenre (gg::Genre::rock);
        expect (p.options.genre == gg::Genre::rock && p.options.leadDirection < 0 && p.options.secondaryGenre == (int) gg::Genre::boomBap,
                "Dr. Dre, then Rock picked: rock with a little hip-hop");
        p.applyDescription ("Post Malone");
        expect (p.options.genre == gg::Genre::rock && p.options.directionFor (0) == gg::directionIndex (p.options.style(), "rock.hybrid"),
                "Post Malone with Rock picked: his rock take");
        p.applyDescription ("");
        p.applyDescription ("Morgan Wallen");
        expect (p.options.genre == gg::Genre::country, "cleared, then Morgan Wallen: Country");
    }
    return failures;
}
