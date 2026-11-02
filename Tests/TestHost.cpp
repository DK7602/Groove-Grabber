// Groove Grabber tests - load the built VST3 the way a DAW does when it scans and uses plug-ins:
// read its description, create it, play MIDI through it, save and restore its state, open and close its window,
// keep it alive for a while, and delete it again - several times in a row.
//
//   GGTests --host "<path>/Groove Grabber.vst3"
#include <juce_audio_processors/juce_audio_processors.h>
#include "core/Analyzer.h"
#include <iostream>
#include <atomic>
#include <thread>
#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace
{
    void pumpMessages (int ms)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
        while (juce::Time::getMillisecondCounter() < end)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }
}

int runHostScan (const juce::File& plugin)
{
    int failures = 0;
    auto fail = [&failures] (const juce::String& what) { std::cout << "  FAIL: " << what << std::endl; ++failures; };
    std::cout << "\n== Loading " << plugin.getFullPathName() << " like a DAW" << std::endl;
    if (! plugin.exists()) { fail ("no plug-in there"); return failures; }

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> types;
    auto t0 = juce::Time::getMillisecondCounterHiRes();
    format.findAllTypesForFile (types, plugin.getFullPathName());
    std::cout << "  scan: " << types.size() << " plug-in(s) in " << (int) (juce::Time::getMillisecondCounterHiRes() - t0) << " ms" << std::endl;
    if (types.isEmpty()) { fail ("the scan found nothing in the bundle"); return failures; }
    const auto& desc = *types[0];
    std::cout << "  name: " << desc.name << "  maker: " << desc.manufacturerName << "  category: " << desc.category
              << "  instrument: " << (desc.isInstrument ? "yes" : "no") << "  version: " << desc.version
              << "  outputs: " << desc.numOutputChannels << std::endl;
    if (! desc.isInstrument) fail ("not seen as an instrument");

    for (int round = 0; round < 3; ++round)
    {
        juce::String error;
        t0 = juce::Time::getMillisecondCounterHiRes();
        auto inst = format.createInstanceFromDescription (desc, 48000.0, 512, error);
        const auto createMs = juce::Time::getMillisecondCounterHiRes() - t0;
        if (inst == nullptr) { fail ("couldn't create it: " + error); return failures; }
        std::cout << "  round " << (round + 1) << ": created in " << (int) createMs << " ms" << std::endl;
        if (createMs > 5000) fail ("creating it took over 5 seconds");

        inst->enableAllBuses();
        inst->prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (juce::jmax (2, inst->getTotalNumOutputChannels()), 512);
        float peak = 0.0f;
        for (int block = 0; block < 200; ++block)
        {
            juce::MidiBuffer midi;
            if (block % 20 == 0) midi.addEvent (juce::MidiMessage::noteOn (10, 36 + (block / 20) % 6, (juce::uint8) 110), 0);
            buffer.clear();
            inst->processBlock (buffer, midi);
            peak = juce::jmax (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        }
        std::cout << "    played 200 blocks (peak " << peak << ")" << std::endl;
        if (! std::isfinite (peak)) fail ("audio went bad");

        juce::MemoryBlock state;
        inst->getStateInformation (state);
        inst->setStateInformation (state.getData(), (int) state.getSize());
        std::cout << "    state: " << state.getSize() << " bytes, saved and restored" << std::endl;

        if (round == 1 && inst->hasEditor())
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (inst->createEditorIfNeeded());
            if (ed == nullptr) fail ("no window");
            else
            {
                std::cout << "    window: " << ed->getWidth() << " x " << ed->getHeight() << std::endl;
                pumpMessages (1500);
            }
        }
        if (round == 2)
        {
            std::cout << "    keeping it open for 6 seconds..." << std::endl;
            pumpMessages (6000);
        }
        inst->releaseResources();
        t0 = juce::Time::getMillisecondCounterHiRes();
        inst.reset();
        const auto deleteMs = juce::Time::getMillisecondCounterHiRes() - t0;
        std::cout << "    deleted in " << (int) deleteMs << " ms" << std::endl;
        if (deleteMs > 3000) fail ("deleting it took over 3 seconds");
        pumpMessages (200);
    }
    std::cout << (failures == 0 ? "  loads, plays, saves and closes cleanly" : "  PROBLEMS loading it") << std::endl;

   #if JUCE_WINDOWS
    // What a DAW does when it quits: unload the plug-in while everything else is still running. Anything the plug-in
    // keeps in a static (fonts, images, threads, timers) is let go right here, and some of that can freeze Windows:
    // Cubase hung on quit with Groove Grabber 1.0.12 (its fonts were statics). A watchdog ends the test if it hangs.
    {
        const auto dll = plugin.isDirectory() ? plugin.getChildFile ("Contents").getChildFile ("x86_64-win").getChildFile (plugin.getFileName())
                                              : plugin;
        auto* module = GetModuleHandleW (dll.getFileName().toWideCharPointer());
        if (module == nullptr)
            fail ("couldn't find the loaded plug-in to unload it");
        else
        {
            std::atomic<bool> unloaded { false };
            std::thread watchdog ([&unloaded]
            {
                for (int i = 0; i < 300 && ! unloaded.load(); ++i) juce::Thread::sleep (100);
                if (! unloaded.load())
                {
                    std::cout << "  FAIL: froze while being unloaded, the way Cubase unloads it when it quits" << std::endl;
                    std::cout.flush();
                    TerminateProcess (GetCurrentProcess(), 3);
                }
            });
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            using ExitDll = bool (*)();
            if (auto exitDll = reinterpret_cast<ExitDll> (reinterpret_cast<void*> (GetProcAddress (module, "ExitDll")))) exitDll();
            while (FreeLibrary (module))
                if (GetModuleHandleW (dll.getFileName().toWideCharPointer()) == nullptr) break;
            unloaded = true;
            watchdog.join();
            std::cout << "  unloaded like a DAW quitting, in " << (int) (juce::Time::getMillisecondCounterHiRes() - t0) << " ms" << std::endl;
            // the host side still thinks the plug-in is loaded: leave now, without the usual shutdown
            std::cout.flush();
            TerminateProcess (GetCurrentProcess(), failures == 0 ? 0 : 1);
        }
    }
   #endif
    return failures;
}

// GGTests --classify <folder>: what Groove Grabber hears in every sound under a folder (for audits), as tab-separated lines
int runClassify (const juce::File& folder)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::cout << "path\tcat\tconf\tdur\tpitch\tdecayMs\tattackMs\tsub\tlow\tmid\thigh\tcentroid\tflat\tonsets\tbursts\tloud\tpunch\twarmth\tbright\troom\tvintage\taggr\torganic\tpconf\tcrest\trolloff\ttailfloor\tpeak\tairflat\tring\tglide" << std::endl;
    for (const auto& e : juce::RangedDirectoryIterator (folder, true, "*", juce::File::findFiles))
    {
        const auto f = e.getFile();
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext != ".wav" && ext != ".aif" && ext != ".aiff" && ext != ".flac" && ext != ".mp3" && ext != ".ogg") continue;
        gg::SoundInfo s;
        const auto folders = f.getParentDirectory().getRelativePathFrom (folder).replaceCharacter ('\\', '/');
        if (! gg::analysis::analyzeFile (f, folders, fm, s)) { std::cout << f.getFullPathName() << "\tFAILED" << std::endl; continue; }
        std::cout << f.getFullPathName() << "\t" << gg::catName (s.cat) << "\t" << s.catConfidence << "\t" << s.duration << "\t" << s.pitchHz
                  << "\t" << s.decayMs << "\t" << s.attackMs << "\t" << s.subShare << "\t" << s.lowShare << "\t" << s.midShare << "\t" << s.highShare
                  << "\t" << s.centroidHz << "\t" << s.flatness << "\t" << s.onsets << "\t" << s.burstCount << "\t" << s.loudDb
                  << "\t" << s.traits.punch << "\t" << s.traits.warmth << "\t" << s.traits.bright << "\t" << s.traits.room
                  << "\t" << s.traits.vintage << "\t" << s.traits.aggr << "\t" << s.traits.organic
                  << "\t" << s.pitchConf << "\t" << s.crestDb << "\t" << s.rolloffHz << "\t" << s.tailFloorDb << "\t" << s.peakDb
                  << "\t" << s.airFlatness << "\t" << s.ringMs << "\t" << s.pitchGlide << std::endl;
    }
    return 0;
}
