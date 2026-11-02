// Groove Grabber - exports a kit, ready for your DAW's drum sampler (Groove Agent, Drum Rack, FPC, Drum Machine
// Designer, Impact XT ... or any sampler / drum machine):
//
//   <Kit name>/
//     01 Kick.wav ... 22 FX 2.wav   one file per pad, numbered in General MIDI pad order from C1 (note 36),
//                                    so dropping them all on the sampler's first pad puts every sound on the
//                                    right note. Empty pads get a silent file so the numbering never shifts.
//     <Kit name>.pgm                 an Akai MPC 1000 program with every pad and note set (Groove Agent can
//                                    import it: right-click the kit slot > Import)
//     <Kit name> Groove.mid          the preview groove on the same notes
//     Kit Info.txt                   where every sound came from, its tuning, and the loading steps for your DAW
#pragma once

#include "Kit.h"
#include "Grooves.h"
#include <functional>

namespace gg
{
    struct ExportOptions
    {
        juce::File destination;          // the kit folder is made inside this
        bool mpcProgram = true;
        bool grooveMidi = true;
        bool placeholders = true;        // silent files for empty pads
        bool genreFolder = true;         // inside a folder per genre: Exports/Hip-Hop/<kit>
        int bitDepth = 24;
        double sampleRate = 44100;
        juce::String grooveId;
        std::shared_ptr<const Groove> groove;   // the groove itself (your edit of it, if any); null = the built-in grooveId
        double bpm = 0;
        float swing = 0;
        juce::String daw;                // the DawGuide id the loading steps are written for ("" = automatic)
    };

    struct ExportResult
    {
        bool ok = false;
        juce::String error;
        juce::File folder;
        juce::Array<juce::File> padFiles;    // in pad order (what to drag onto the sampler's first pad)
        juce::File programFile, midiFile, infoFile;
        int soundsWritten = 0;
    };

    ExportResult exportKit (const Kit&, const ExportOptions&);

    // MPC 1000 program bytes. names[i] = sample name for pad i (note 36 + i), empty = no sample
    juce::MemoryBlock makeMpcProgram (const std::array<juce::String, numPads>& names);

    // The file name (no extension) for pad i, e.g. "03 Snare" (always 16 characters or less)
    juce::String padFileName (int padIndex, bool empty);
}
