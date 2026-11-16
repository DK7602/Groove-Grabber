// Groove Grabber - look: black glass with electric blue, purple and orange (and a light version),
// Orbitron for titles, Rajdhani for everything else.
#pragma once

#include "core/SoundTypes.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace ggui
{
    struct Palette
    {
        bool light = false;
        juce::Colour bg, panel, raised, sunken, line, text, dim, faint;
        juce::Colour blue, purple, orange, amber, red, green;
        juce::Colour glow;      // soft shadow / glow colour
    };

    const Palette& pal();
    void setLight (bool);
    bool isLight();

    juce::Colour catColour (gg::Cat);
    juce::Colour slotColour (gg::Slot);

    juce::Font titleFont (float height);        // Orbitron Black
    juce::Font headFont (float height);         // Orbitron Bold
    juce::Font uiFont (float height, bool bold = true);   // Rajdhani SemiBold / Bold
    juce::Font bodyFont (float height);         // Rajdhani Medium

    // How much bigger to draw everything in a window of this size: 1 up to the size it opens at (1440 x 920), then
    // growing with the window (a maximised window has bigger words, buttons and pads). The pages are laid out at the
    // window's size divided by this, never smaller than zoomBase (they fit down to 1180 x 780).
    constexpr int zoomBaseWidth = 1440, zoomBaseHeight = 820;
    float zoomFor (int width, int height);

    juce::ColourGradient brandGradient (juce::Rectangle<float>, float alpha = 1.0f);

    void drawPanel (juce::Graphics&, juce::Rectangle<float>, float radius = 12.0f, bool highlight = false);
    void drawSectionTitle (juce::Graphics&, juce::Rectangle<int>, const juce::String&, juce::Colour accent = {});
    void drawGlowOutline (juce::Graphics&, juce::Rectangle<float>, float radius, float strength = 1.0f);
    juce::Image badgeImage();

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void refreshColours();

        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        juce::Font getPopupMenuFont() override;
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
        void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive, bool isHighlighted,
                                bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                                const juce::Drawable* icon, const juce::Colour* textColour) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
        void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
        void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
        void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                            int thumbSize, bool over, bool down) override;
        int getDefaultScrollbarWidth() override   { return 10; }
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        void drawLabel (juce::Graphics&, juce::Label&) override;
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;
        void drawProgressBar (juce::Graphics&, juce::ProgressBar&, int w, int h, double progress, const juce::String&) override;
    };
}
