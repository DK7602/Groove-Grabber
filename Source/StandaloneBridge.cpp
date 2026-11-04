#include "StandaloneBridge.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#if defined (JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

namespace gg::standalone
{
    bool isStandalone()
    {
       #if defined (JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
        return juce::StandalonePluginHolder::getInstance() != nullptr;
       #else
        return false;
       #endif
    }

    void showAudioSettings()
    {
       #if defined (JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->showAudioSettingsDialog();
       #endif
    }

    void addMaximiseButton (juce::DocumentWindow& w)
    {
        if (w.getMaximiseButton() != nullptr) return;
        w.setTitleBarButtonsRequired (juce::DocumentWindow::minimiseButton | juce::DocumentWindow::maximiseButton
                                        | juce::DocumentWindow::closeButton, false);
        // and tell the system, which made the window without a maximise box: on Windows that's what Win+Up,
        // dragging the window to the top of the screen and the snap layouts over the button go by. (Not on
        // Linux, where the button needs nothing more, and X refuses the hint with no window manager running.)
       #if JUCE_WINDOWS || JUCE_MAC
        if (auto* peer = w.getPeer(); peer != nullptr && (peer->getStyleFlags() & juce::ComponentPeer::windowHasMaximiseButton) == 0)
            w.addToDesktop (w.getDesktopWindowStyleFlags());
       #endif
    }

    void tidyTitleBar (juce::Component& editor)
    {
        if (! isStandalone()) return;
        if (auto* w = dynamic_cast<juce::DocumentWindow*> (editor.getTopLevelComponent()); w != nullptr && w != &editor)
            addMaximiseButton (*w);
        hideTitleBarOptionsButton (editor);
    }

    void hideTitleBarOptionsButton (juce::Component& editor)
    {
        if (! isStandalone()) return;
        auto* top = editor.getTopLevelComponent();
        if (top == nullptr || top == &editor) return;
        for (auto* c : top->getChildren())
            if (auto* b = dynamic_cast<juce::Button*> (c))
                if (b->getButtonText() == "Options")
                    b->setVisible (false);
    }
}
