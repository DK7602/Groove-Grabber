// Groove Grabber - the bits of the standalone app (audio / MIDI settings) the plug-in can't see directly.
#pragma once

namespace juce { class Component; }

namespace gg::standalone
{
    bool isStandalone();
    void showAudioSettings();                     // the audio / MIDI device dialog (standalone only)
    void hideTitleBarOptionsButton (juce::Component& editor);   // our gear menu does that job, bigger
}
