// Groove Grabber - listens to a one-shot and works out what it is and what it sounds like.
//
// Every file is measured (envelope, spectrum, pitch, noise, clipping), its name and folders are read for
// clues ("Kicks/BD_Dusty_03.wav"), and the two are combined into a category with a confidence and the
// seven character traits the kit builder aims at. The name wins when it's explicit; the audio decides
// when the name says nothing ("Sound 12.wav") or is vague ("Hat" that rings for a second = open hat).
#pragma once

#include "SoundInfo.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace gg::analysis
{
    struct NameGuess
    {
        Cat cat = Cat::unknown;
        float strength = 0;          // 0 = no idea, 0.6 = vague word or folder, 0.9 = explicit
        PercKind perc = PercKind::none;
        float vintage = 0;           // -1..1 from words like "dusty", "vinyl" / "clean", "modern"
        float organic = 0;           // -1..1 from words like "acoustic", "live" / "808", "synth"
        bool loopWord = false;
        bool says808 = false, saysKick = false, saysHat = false, saysOpen = false, saysClosed = false;
        bool saysMelodic = false;     // vocal / instrument words ("Vocal - Loops", "Synth", "Keys")
    };

    // fileName without extension; folders = the folders between the source root and the file
    NameGuess guessFromName (const juce::String& fileName, const juce::String& folders);

    // Measure mono audio (any length; only the first few seconds matter) into 'info'
    void measure (const float* mono, int numSamples, double sampleRate, SoundInfo& info);

    // Category from the measurements alone, with a 0..1 confidence
    Cat classifyAudio (const SoundInfo&, float& confidence);

    // Name + audio -> category, perc kind and traits (leaves user-set categories alone)
    void finish (SoundInfo&, const NameGuess&);
    void computeTraits (SoundInfo&, const NameGuess&);

    // Everything: read the file, measure, classify. False if the file can't be read.
    bool analyzeFile (const juce::File&, const juce::String& folders, juce::AudioFormatManager&, SoundInfo&);

    // YIN pitch on a stretch of audio; 0 if none. confidence 0..1
    float detectPitch (const float* x, int n, double sr, float minHz, float maxHz, float& confidence);

    // Loudest 50 ms RMS of a buffer in dBFS (used to balance kits)
    float shortTermLoudnessDb (const float* const* channels, int numChannels, int numSamples, double sr);
}
