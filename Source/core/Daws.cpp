#include "Daws.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace gg
{
    namespace
    {
        DawGuide guide (const char* id, const char* name, const char* sampler, const char* dragLabel, const char* dragHint,
                        int middleC, std::initializer_list<const char*> steps, const char* hats, const char* keep, const char* groove)
        {
            DawGuide d;
            d.id = id; d.name = name; d.sampler = sampler; d.dragLabel = dragLabel; d.dragHint = dragHint;
            d.middleCOctave = middleC;
            for (auto* s : steps) d.steps.add (s);
            d.hats = hats; d.keep = keep; d.groove = groove;
            return d;
        }

        std::vector<DawGuide> makeGuides()
        {
            std::vector<DawGuide> v;
            // Steps checked against each maker's own manual where it covers them (2026). {FIRST} = how the DAW names
            // note 36, {LAST} = the last file, {PADS} = how many files.
            v.push_back (guide ("cubase", "Cubase / Nuendo", "Groove Agent SE", "DRAG TO GROOVE AGENT - PAD C1",
                "Drop on Groove Agent's C1 pad, on the lowest of the three drop icons, so the files spread across the pads", 3,
                { "Add an instrument track with **Groove Agent SE** and open it.",
                  "Show **pad group 3**: the **C1** pad is in it (point at a pad to see its note).",
                  "Drag all {PADS} files (01 Kick to {LAST}) over the C1 pad and hold. Three drop icons appear: drop on the **lowest** one, which spreads the files over the pads in order. (The top icon stacks them all on one pad.)",
                  "If Groove Agent asks about free pads, go ahead. Files 13 to 22 land in **pad group 4** (C2 to A2)." },
                "Hats: select the Closed Hat (F#1), Pedal Hat (G#1) and Open Hat (A#1) pads in turn and set **Excl** (Exclusive Group, on the Edit page's Main tab) to the same number, so a closed hat cuts off an open one.",
                "Keep it: right-click the kit and choose **Export Kit with Samples** (or **Save Kit**). It's then a normal Groove Agent kit for any project.",
                "drag the Groove .mid onto the Groove Agent track."));

            v.push_back (guide ("ableton", "Ableton Live", "Drum Rack", "DRAG TO DRUM RACK - PAD C1",
                "Drop on the Drum Rack's C1 pad without holding a key, and the files map upward from it", 3,
                { "Put an empty **Drum Rack** on a MIDI track.",
                  "Drag all {PADS} files onto the **C1** pad. They map upward, one per pad: 01 Kick on C1, 02 Rim on C#1 and so on. Don't hold Alt / Cmd, which stacks them on one pad.",
                  "The rack shows 16 pads at a time: scroll the pad overview up for files 17 to 22 (E2 to A2)." },
                "Hats: show the chain list, open its I/O section and set the Closed Hat, Pedal Hat and Open Hat chains to the same **Choke** group.",
                "Keep it: click the Drum Rack's **Save Preset** button. Live copies the samples into your User Library.",
                "drag the Groove .mid onto the Drum Rack track."));

            v.push_back (guide ("fl", "FL Studio", "FPC", "DRAG TO THE CHANNEL RACK",
                "Drop on FL's Channel Rack for one channel per sound, ready for the step sequencer", 5,
                { "For the step sequencer: drag all {PADS} files onto the **Channel Rack**. Each one becomes its own channel, ready to program.",
                  "For the piano roll and the Groove MIDI: load **FPC** (preset: Empty) and drop each file on the pad that plays its note: 01 Kick on {FIRST} (note 36), 02 Rim on C#3, 03 Snare on D3 and so on up the pad map. Do one file at a time: several files dropped on one FPC pad become layers of that pad." },
                "Hats (FPC): give the Closed Hat, Pedal Hat and Open Hat pads the same number in **CUT** and **BY**, so a closed hat cuts off an open one.",
                "Keep it: FPC's menu > **Save preset as**. Keep the export folder where it is, as the preset points to its files.",
                "drag the Groove .mid into FPC's piano roll. FL calls note 36 C3, so the kick is on C3."));

            v.push_back (guide ("logic", "Logic Pro", "Drum Machine Designer", "DRAG TO LOGIC - DRUM MACHINE DESIGNER",
                "Drop below your last track, on the Drum Machine Designer option, and each file gets its own pad", 3,
                { "Drag all {PADS} files into the empty area below your last track and drop them on **Drum Machine Designer**. Each file gets its own pad, starting from C1 at the lower left.",
                  "Check the order: 01 Kick on C1, 03 Snare on D1, 07 Closed Hat on F#1. There are 16 pads a page; the last files are on page 2.",
                  "To add them to a Drum Machine Designer track you already have, drop them on its main track header instead: they fill the lowest empty pads." },
                "Hats: on each hat pad, open the **Action** pop-up and choose the same **Exclusive Group**.",
                "Keep it: open the Library and click **Save** to keep it as your own kit patch.",
                "drag the Groove .mid onto the Drum Machine Designer track."));

            v.push_back (guide ("garageband", "GarageBand", "Groove Grabber", "DRAG THE KIT FILES",
                "GarageBand has no drum sampler to drop files into: play the kit with Groove Grabber itself", 3,
                { "GarageBand has no drum sampler you can load your own files into, so play the kit with **Groove Grabber itself**: add a Software Instrument track with Groove Grabber (Audio Units > Grabber Audio).",
                  "It plays the kit you're editing on notes C1 to A2, the same layout as the export. Record or draw your part.",
                  "For a full drum sampler, Logic Pro's Drum Machine Designer loads the same export." },
                "",
                "Keep it: SAVE KIT in Groove Grabber. Your GarageBand project remembers the kit too.",
                "drag the Groove .mid onto the Groove Grabber track."));

            v.push_back (guide ("studioone", "Studio One / Studio Pro", "Impact", "DRAG TO IMPACT - HOLD SHIFT, PAD C1",
                "Hold Shift as you drop on Impact's first pad (C1), so the files go to the pads after it", 3,
                { "Add **Impact** (Impact XT in older versions) on an instrument track.",
                  "Drag all {PADS} files over pad 1 of bank A (C1), then **hold Shift** and let go. The first file lands on that pad and the rest on the pads after it, in note order. Without Shift they all stack on one pad.",
                  "Bank A holds 16 pads; files 17 to 22 are in bank B." },
                "Hats: give the Closed Hat, Pedal Hat and Open Hat pads the same **Choke** number.",
                "Keep it: save a preset from the device header (or Export Sampler File to share it).",
                "drag the Groove .mid onto the Impact track."));

            v.push_back (guide ("reaper", "REAPER", "ReaSamplOmatic5000", "DRAG THE KIT FILES",
                "REAPER's sampler takes one sound per copy: the steps are in Kit Info.txt, or play the kit with Groove Grabber itself", 4,
                { "Easiest: play the kit with **Groove Grabber itself** on the track. It plays the kit you're editing on the same notes as the export.",
                  "To use REAPER's own sampler: on one track, add a **ReaSamplOmatic5000** for each sound you want and drop one file into each. Set its **Note range start** and **end** to that file's note (01 Kick = note 36, which REAPER shows as {FIRST}; 03 Snare = 38; 07 Closed Hat = 42; see the pad map) and its **Mode** to Sample (ignores MIDI note). Several files dropped on one copy share it by velocity instead." },
                "Hats: REAPER's sampler has no choke setting. Recent REAPER versions include a MIDI choke group effect you can put first on the track; otherwise keep open hats short.",
                "Keep it: save the track as a track template.",
                "drag the Groove .mid onto the track."));

            v.push_back (guide ("bitwig", "Bitwig Studio", "Drum Machine", "DRAG TO DRUM MACHINE - PAD C1",
                "Drop all the files on the Drum Machine's C1 pad and they fill the pads above it", 3,
                { "Add a **Drum Machine** to an instrument track.",
                  "Drag all {PADS} files onto the **C1** pad: they fill the pads above it in order. Check a few: 01 Kick on C1, 03 Snare on D1, 07 Closed Hat on F#1.",
                  "16 pads show at a time: scroll the pad strip for files 17 to 22 (E2 to A2)." },
                "Hats: right-click each hat pad and use **Choke targets** so each hat cuts off the other two.",
                "Keep it: right-click the device and save it as a preset in your library.",
                "drag the Groove .mid onto the Drum Machine track."));

            v.push_back (guide ("protools", "Pro Tools", "GrooveCell", "DRAG THE KIT FILES",
                "GrooveCell has 16 pads: drop the sounds you need, one per pad", 3,
                { "Pro Tools only loads AAX plug-ins, so use the Groove Grabber app next to it to build and export kits.",
                  "Add **GrooveCell** on an instrument track. It has 16 pads, so load the sounds you need one per pad: 01 Kick, 03 Snare, 07 Closed Hat and 11 Open Hat first.",
                  "Check which note each pad plays before you use the Groove .mid. Any drum sampler you own that loads WAVs or MPC programs works too: the .pgm sets every pad and note." },
                "",
                "Keep it: in the plug-in window, use the Settings menu > Save Settings As.",
                "drag the Groove .mid onto the instrument track."));

            v.push_back (guide ("reason", "Reason", "Kong Drum Designer", "DRAG THE KIT FILES",
                "Kong has 16 pads: drop one file per pad, from pad 1 (C1)", 3,
                { "Create a **Kong Drum Designer**. Its 16 pads play C1 to D#2, pad 1 to pad 16.",
                  "Drop one file per pad: 01 Kick on pad 1 through 16 Ride on pad 16. Several files dropped on one pad become layers of it.",
                  "Kong has no room for files 17 to 22 (808, percussion, FX): put those in a second Kong or an NN-XT." },
                "Hats: put the Closed Hat, Pedal Hat and Open Hat pads in the same **Mute Group**.",
                "Keep it: save the Kong patch. It points to the files, so keep the export folder where it is.",
                "drag the Groove .mid onto the Kong track."));

            v.push_back (guide ("mpc", "MPC (hardware or software)", "MPC program", "DRAG THE KIT FILES",
                "Load the .pgm with its WAVs beside it. MPC hardware needs 16-bit files", 3,
                { "Export at **16-bit** for MPC hardware (the MPC 1000 needs it), with the MPC program on. Keep the .pgm and the WAVs together in one folder.",
                  "MPC hardware: copy the kit folder to the card, LOAD the .pgm and choose **WITH SAMPLES**. Every pad and note is set.",
                  "MPC software: load the .pgm from the Browser, with its WAVs in the same folder." },
                "Hats: the .pgm already puts the hats in one mute group.",
                "Keep it: save the program (MPC software: File > Save Current Program).",
                "load the Groove .mid as a sequence."));

            v.push_back (guide ("other", "Any other DAW or sampler", "your drum sampler", "DRAG THE KIT - FIRST PAD (NOTE 36)",
                "Drop all the files on your sampler's first pad (note 36)", 3,
                { "Open your DAW's drum sampler (any sampler with pads or a key map).",
                  "Put file 01 on note 36, file 02 on note 37 and so on up to {LAST} on note 57. Many samplers do this for you when you drop all the files on the note 36 pad; check your sampler's manual for how it spreads a multi-file drop.",
                  "Or play the kit with Groove Grabber itself on an instrument track: it uses the same notes." },
                "Hats: put the Closed Hat, Pedal Hat and Open Hat (notes 42, 44 and 46) in one choke / mute group.",
                "Keep it: save the kit as a preset in your sampler.",
                "drag the Groove .mid onto the drum track."));
            return v;
        }
    }

    const std::vector<DawGuide>& dawGuides()
    {
        static const auto all = makeGuides();
        return all;
    }

    const DawGuide& dawGuide (const juce::String& id)
    {
        const auto& all = dawGuides();
        for (const auto& d : all)
            if (d.id == id) return d;
        return all.back();
    }

    juce::String detectedDaw()
    {
        if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone) return {};
        const juce::PluginHostType host;
        if (host.isCubase() || host.isNuendo() || host.isSteinberg()) return "cubase";
        if (host.isAbletonLive())  return "ableton";
        if (host.isFruityLoops())  return "fl";
        if (host.isLogic() || host.isMainStage()) return "logic";
        if (host.isGarageBand())   return "garageband";
        if (host.isStudioOne())    return "studioone";
        if (host.isReaper())       return "reaper";
        if (host.isBitwigStudio()) return "bitwig";
        if (host.isProTools())     return "protools";
        if (host.isReason())       return "reason";
        return {};
    }

    juce::String chosenDaw (const juce::String& setting)
    {
        if (setting.isNotEmpty() && setting != "auto")
            for (const auto& d : dawGuides())
                if (d.id == setting) return d.id;
        const auto found = detectedDaw();
        return found.isNotEmpty() ? found : juce::String ("other");
    }

    juce::String dawNoteName (int midiNote, const DawGuide& daw)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int octave = midiNote / 12 - 5 + daw.middleCOctave;
        return juce::String (names[((midiNote % 12) + 12) % 12]) + juce::String (octave);
    }

    juce::String dawGuideText (const DawGuide& daw, int numPads, const juce::String& lastFileName)
    {
        juce::String t;
        int n = 1;
        for (auto s : daw.steps)
        {
            s = s.replace ("{FIRST}", dawNoteName (36, daw)).replace ("{LAST}", lastFileName).replace ("{PADS}", juce::String (numPads))
                 .replace ("**", "");
            t << n++ << ". " << s << "\n";
        }
        if (daw.hats.isNotEmpty())   t << n++ << ". " << daw.hats.replace ("**", "") << "\n";
        if (daw.keep.isNotEmpty())   t << n++ << ". " << daw.keep.replace ("**", "") << "\n";
        if (daw.groove.isNotEmpty()) t << "\nThe groove: " << daw.groove.replace ("**", "") << "\n";
        return t;
    }

    juce::String dawGuidesMarkdown()
    {
        juce::String md;
        for (const auto& daw : dawGuides())
        {
            md << "**" << daw.name << (daw.id != "other" && daw.sampler != "Groove Grabber" ? ": " + daw.sampler : juce::String()) << "**\n\n";
            int n = 1;
            for (auto s : daw.steps)
                md << n++ << ". " << s.replace ("{FIRST}", dawNoteName (36, daw)).replace ("{LAST}", "22 FX 2").replace ("{PADS}", "22") << "\n";
            md << "\n";
            if (daw.hats.isNotEmpty())   md << "- " << daw.hats.replaceFirstOccurrenceOf ("Hats:", "**Hats:**") << "\n";
            if (daw.keep.isNotEmpty())   md << "- " << daw.keep.replaceFirstOccurrenceOf ("Keep it:", "**Keep it:**") << "\n";
            if (daw.groove.isNotEmpty()) md << "- **The groove:** " << daw.groove << "\n";
            if (daw.id != "other")       md << "- **Note names:** " << daw.name << " calls note 36 " << dawNoteName (36, daw) << ".\n";
            md << "\n";
        }
        return md;
    }
}
