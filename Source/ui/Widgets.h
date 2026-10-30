// Groove Grabber - small building blocks: vector icons, icon buttons, segmented choices, waveforms, knobs.
#pragma once

#include "Theme.h"
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>

namespace ggui
{
    enum class Icon { play, stop, lock, unlock, heart, heartFilled, dice, refresh, gear, sun, moon, folder, exportIcon,
                      compass, search, close, star, plus, waveform, drag, check, music, trash, eye, minus, library, grid };
    juce::Path iconPath (Icon);
    void drawIcon (juce::Graphics&, Icon, juce::Rectangle<float>, juce::Colour);

    //==============================================================================
    class IconButton : public juce::Button
    {
    public:
        IconButton (const juce::String& name, Icon icon, Icon iconOn = Icon::play, bool hasOnIcon = false);
        void setIcon (Icon i)                     { icon = i; repaint(); }
        void setAccent (juce::Colour c)           { accent = c; repaint(); }
        void setLabel (const juce::String& s)     { label = s; repaint(); }
        void setFilled (bool b)                   { filled = b; repaint(); }
        void paintButton (juce::Graphics&, bool over, bool down) override;

    private:
        Icon icon, iconOn;
        bool hasOnIcon, filled = false;
        juce::Colour accent;
        juce::String label;
    };

    //==============================================================================
    // A row of mutually exclusive choices ("AUTO | LIVE | HYBRID | PROGRAMMED")
    class Segmented : public juce::Component
    {
    public:
        explicit Segmented (const juce::StringArray& labels);
        void setSelected (int index, bool notify = false);
        int getSelected() const noexcept          { return selected; }
        std::function<void (int)> onChange;
        void resized() override;
        void paint (juce::Graphics&) override;
        void setTooltips (const juce::StringArray&);

    private:
        juce::OwnedArray<juce::TextButton> buttons;
        int selected = 0;
    };

    //==============================================================================
    class WaveView : public juce::Component
    {
    public:
        void setAudio (std::shared_ptr<const juce::AudioBuffer<float>>);
        void setColour (juce::Colour c)           { colour = c; repaint(); }
        void setMarkers (float start01, float end01)   { startMark = start01; endMark = end01; repaint(); }
        void setEmptyText (const juce::String& s) { emptyText = s; repaint(); }
        void setPlayhead (float p)                { playhead = p; repaint(); }
        bool hasAudio() const noexcept            { return ! peaks.empty(); }
        void paint (juce::Graphics&) override;
        void resized() override                   { rebuild(); }

    private:
        void rebuild();
        std::shared_ptr<const juce::AudioBuffer<float>> audio;
        std::vector<std::pair<float, float>> peaks;
        juce::Colour colour;
        float startMark = -1, endMark = -1, playhead = -1;
        juce::String emptyText;
    };

    //==============================================================================
    // A knob with its name under it and the value inside the text box
    class LabelledKnob : public juce::Component
    {
    public:
        LabelledKnob (const juce::String& name, double min, double max, double step, double def, const juce::String& suffix);
        juce::Slider slider;
        void resized() override;
        void paint (juce::Graphics&) override;
        void setAccent (juce::Colour);

    private:
        juce::String name;
    };

    //==============================================================================
    // The status line at the bottom: a message that fades after a while
    class Toast : public juce::Component, private juce::Timer
    {
    public:
        void show (const juce::String& text, juce::Colour accent = {});
        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;
        juce::String text;
        juce::Colour accent;
        float alpha = 0;
        int ticks = 0;
    };
}
