#include "Exporter.h"
#include "KitPlayer.h"
#include "Grooves.h"
#include "MpcTemplate.h"
#include "Settings.h"
#include "Daws.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace gg
{
    juce::String padFileName (int i, bool empty)
    {
        const auto& p = padLayout()[(size_t) juce::jlimit (0, numPads - 1, i)];
        return juce::String (i + 1).paddedLeft ('0', 2) + " " + (empty ? juce::String ("Empty") : juce::String (p.label));
    }

    juce::MemoryBlock makeMpcProgram (const std::array<juce::String, numPads>& names)
    {
        juce::MemoryBlock mb (mpc::templateData, (size_t) mpc::templateSize);
        auto* d = static_cast<juce::uint8*> (mb.getData());
        constexpr int headerSize = 24, padSize = 164, layerSize = 24, numMpcPads = 64;
        constexpr int padNotesAt = headerSize + numMpcPads * padSize;     // 64 bytes: pad -> note
        constexpr int notePadsAt = padNotesAt + 64;                       // 128 bytes: note -> pad

        for (int pad = 0; pad < numMpcPads; ++pad)
        {
            auto* p = d + headerSize + pad * padSize;
            // clear all four layers' names
            for (int layer = 0; layer < 4; ++layer)
                std::fill (p + layer * layerSize, p + layer * layerSize + 16, (juce::uint8) 0);

            if (pad < numPads && names[(size_t) pad].isNotEmpty())
            {
                const auto name = names[(size_t) pad].substring (0, 16).toRawUTF8();
                for (int c = 0; c < 16 && name[c] != 0; ++c) p[c] = (juce::uint8) name[c];
                p[17] = 100;     // level
                p[18] = 0;       // velocity range low
                p[19] = 127;     // velocity range high
                p[20] = 0; p[21] = 0;   // tuning (the file is already tuned)
                p[22] = 0;       // one shot
                const auto& def = padLayout()[(size_t) pad];
                auto* tail = p + 4 * layerSize;
                tail[2] = def.chokeGroup != 0 ? 1 : 0;      // hats: mono
                tail[3] = (juce::uint8) def.chokeGroup;     // ...and in one mute group
            }
        }

        // pads 1..22 on notes 36..57, the rest out of the way above
        for (int pad = 0; pad < numMpcPads; ++pad)
            d[padNotesAt + pad] = (juce::uint8) (pad < numPads ? firstPadNote + pad : juce::jmin (127, 58 + (pad - numPads)));
        for (int note = 0; note < 128; ++note) d[notePadsAt + note] = 64;
        for (int pad = 0; pad < numMpcPads; ++pad) d[notePadsAt + d[padNotesAt + pad]] = (juce::uint8) pad;
        return mb;
    }

    namespace
    {
        bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr, int bits)
        {
            f.deleteFile();
            std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
            if (os == nullptr) return false;
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, (unsigned int) juce::jmax (1, b.getNumChannels()), bits, {}, 0));
            if (w == nullptr) return false;
            os.release();
            return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
    }

    ExportResult exportKit (const Kit& kit, const ExportOptions& o)
    {
        ExportResult r;
        auto base = o.destination.isDirectory() || o.destination.createDirectory() ? o.destination : paths::exportsDir();
        if (o.genreFolder)
            if (auto g = base.getChildFile (genreFolderName (kit.genre)); g.isDirectory() || g.createDirectory())
                base = g;
        const auto kitName = paths::safeFileName (kit.name.isNotEmpty() ? kit.name : "Groove Grabber Kit", 48);
        r.folder = base.getNonexistentChildFile (kitName, {}, true);
        if (! r.folder.createDirectory())
        {
            r.error = "Couldn't make the folder " + r.folder.getFullPathName();
            return r;
        }

        const auto& daw = dawGuide (chosenDaw (o.daw));
        std::array<juce::String, numPads> mpcNames;
        juce::String table;
        for (int i = 0; i < numPads; ++i)
        {
            const auto& pad = padLayout()[(size_t) i];
            const auto& st = kit[pad.slot];
            auto audio = renderPad (kit, pad, o.sampleRate);
            const bool empty = audio == nullptr || audio->getNumSamples() == 0;
            if (empty && ! o.placeholders) continue;

            const auto name = padFileName (i, empty);
            const auto file = r.folder.getChildFile (name + ".wav");
            bool ok;
            if (empty)
            {
                juce::AudioBuffer<float> silence (2, juce::jmax (1, (int) (o.sampleRate * 0.01)));
                silence.clear();
                ok = writeWav (file, silence, o.sampleRate, o.bitDepth);
            }
            else
            {
                ok = writeWav (file, *audio, o.sampleRate, o.bitDepth);
                ++r.soundsWritten;
                mpcNames[(size_t) i] = name;
            }
            if (! ok)
            {
                r.error = "Couldn't write " + file.getFullPathName();
                return r;
            }
            r.padFiles.add (file);

            table << dawNoteName (pad.note, daw).paddedRight (' ', 5) << juce::String (pad.note).paddedRight (' ', 5)
                  << (name + ".wav").paddedRight (' ', 22);
            if (empty) table << "(empty)";
            else
            {
                const float tune = st.tune (kit.tuneToKey) + pad.tuneOffset;
                table << st.path;
                if (std::abs (tune) > 0.005f) table << "   [tuned " << (tune > 0 ? "+" : "") << juce::String (tune, 2) << " st]";
                if (st.reverse) table << "   [reversed]";
            }
            table << "\n";
        }

        if (o.mpcProgram)
        {
            r.programFile = r.folder.getChildFile (kitName.substring (0, 16).trim() + ".pgm");
            r.programFile.replaceWithData (makeMpcProgram (mpcNames).getData(), (size_t) mpc::templateSize);
        }

        const auto& groove = o.groove != nullptr ? *o.groove : grooveFor (o.grooveId);
        const double bpm = o.bpm > 0 ? o.bpm : (kit.bpm > 0 ? kit.bpm : groove.bpm);
        if (o.grooveMidi)
        {
            r.midiFile = r.folder.getChildFile (kitName + " Groove.mid");
            const auto midi = grooveToMidi (groove, bpm, o.swing, 2);
            r.midiFile.replaceWithData (midi.getData(), midi.getSize());
        }

        juce::String info;
        info << "GROOVE GRABBER KIT\n"
             << "==================\n\n"
             << "Kit:      " << kit.name << (kit.blurb.isNotEmpty() ? "  (" + kit.blurb + ")" : juce::String()) << "\n"
             << "Genre:    " << genreName (kit.genre) << "\n";
        if (kit.description.isNotEmpty()) info << "Song:     " << kit.description << "\n";
        info << "Key:      " << (kit.key >= 0 ? keyName (kit.key) + (kit.tuneToKey ? " (kick, 808 and toms tuned to it)" : "") : juce::String ("-")) << "\n"
             << "Tempo:    " << juce::String (bpm, 0) << " BPM\n"
             << "Exported: " << juce::Time::getCurrentTime().toString (true, true, false) << "\n"
             << "Format:   " << o.bitDepth << "-bit, " << juce::String (o.sampleRate / 1000.0, 1) << " kHz WAV\n\n"
             << "PADS (note, file, original sound)\n"
             << "---------------------------------\n" << table << "\n"
             << "LOAD INTO " << (daw.id == "other" ? juce::String ("YOUR SAMPLER") : daw.sampler.toUpperCase() + " (" + daw.name.toUpperCase() + ")") << "\n"
             << "----------------------\n"
             << dawGuideText (daw, numPads, padFileName (numPads - 1, false)) << "\n"
             << (daw.id != "other" ? "Note names: " + daw.name + " calls MIDI note 36 " + dawNoteName (36, daw) + ".\n" : juce::String())
             << "Another DAW? Every DAW's steps are in Groove Grabber's manual (F1 > Exporting to your DAW).\n\n"
             << "Other options: " << (o.mpcProgram ? "'" + r.programFile.getFileName() + "' is an MPC 1000 program with every pad and note\n"
                                                    "already set, for MPCs and samplers that import MPC programs.\n" : juce::String())
             << (o.grooveMidi ? "'" + r.midiFile.getFileName() + "' is the preview groove on the same notes (your edit of it, if you made one on SAUCE).\n" : juce::String())
             << "\nThe original sounds are listed above so you always know which pack each one came from\n"
             << "(check each pack's licence before releasing music made with it).\n";
        r.infoFile = r.folder.getChildFile ("Kit Info.txt");
        r.infoFile.replaceWithText (info);

        // remember the kit itself next to the sounds, so it can be opened again in Groove Grabber
        r.folder.getChildFile (kitName + ".ggkit").replaceWithText (juce::JSON::toString (kit.toVar()));
        r.ok = true;
        return r;
    }
}
