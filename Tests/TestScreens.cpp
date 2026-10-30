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
    return failures;
}
