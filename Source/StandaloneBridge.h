// Groove Grabber - the bits of the standalone app (audio / MIDI settings) the plug-in can't see directly.
#pragma once

namespace juce { class Component; class DocumentWindow; }

namespace gg::standalone
{
    bool isStandalone();
    void showAudioSettings();                     // the audio / MIDI device dialog (standalone only)
    void hideTitleBarOptionsButton (juce::Component& editor);   // our gear menu does that job, bigger
    // The app's window: minimise, maximise and close (JUCE's standalone window has no maximise of its own;
    // double-clicking the title bar maximises too), and no "Options" button. Does nothing in a DAW.
    void tidyTitleBar (juce::Component& editor);
    void addMaximiseButton (juce::DocumentWindow&);
}
