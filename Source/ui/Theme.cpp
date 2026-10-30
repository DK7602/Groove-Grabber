#include "Theme.h"
#include "GGBinaryData.h"

namespace ggui
{
    namespace
    {
        Palette makeDark()
        {
            Palette p;
            p.light = false;
            p.bg      = juce::Colour (0xff05060a);
            p.panel   = juce::Colour (0xff0d1018);
            p.raised  = juce::Colour (0xff151a27);
            p.sunken  = juce::Colour (0xff080a10);
            p.line    = juce::Colour (0xff232b3d);
            p.text    = juce::Colour (0xffeaf0fa);
            p.dim     = juce::Colour (0xff8d97ad);
            p.faint   = juce::Colour (0xff4b5468);
            p.blue    = juce::Colour (0xff35c3ff);
            p.purple  = juce::Colour (0xff9d5cff);
            p.orange  = juce::Colour (0xffff7a1a);
            p.amber   = juce::Colour (0xffffb547);
            p.red     = juce::Colour (0xffff4d6d);
            p.green   = juce::Colour (0xff3ddc97);
            p.glow    = juce::Colour (0xff35c3ff);
            return p;
        }

        Palette makeLight()
        {
            Palette p;
            p.light = true;
            p.bg      = juce::Colour (0xffe9edf4);
            p.panel   = juce::Colour (0xffffffff);
            p.raised  = juce::Colour (0xfff3f5fa);
            p.sunken  = juce::Colour (0xffe2e7f0);
            p.line    = juce::Colour (0xffd3dae6);
            p.text    = juce::Colour (0xff111520);
            p.dim     = juce::Colour (0xff5b6680);
            p.faint   = juce::Colour (0xffa3acc0);
            p.blue    = juce::Colour (0xff0b8fd9);
            p.purple  = juce::Colour (0xff7a3ee6);
            p.orange  = juce::Colour (0xfff0650a);
            p.amber   = juce::Colour (0xffd98a00);
            p.red     = juce::Colour (0xffe0304f);
            p.green   = juce::Colour (0xff12a66a);
            p.glow    = juce::Colour (0xff7a3ee6);
            return p;
        }

        Palette& current()
        {
            static Palette p = makeDark();
            return p;
        }

        struct Fonts
        {
            juce::Typeface::Ptr black, bold, med, semi, rbold;
            Fonts()
            {
                black = juce::Typeface::createSystemTypefaceFor (GGBinary::OrbitronBlack_ttf, GGBinary::OrbitronBlack_ttfSize);
                bold  = juce::Typeface::createSystemTypefaceFor (GGBinary::OrbitronBold_ttf, GGBinary::OrbitronBold_ttfSize);
                med   = juce::Typeface::createSystemTypefaceFor (GGBinary::RajdhaniMedium_ttf, GGBinary::RajdhaniMedium_ttfSize);
                semi  = juce::Typeface::createSystemTypefaceFor (GGBinary::RajdhaniSemiBold_ttf, GGBinary::RajdhaniSemiBold_ttfSize);
                rbold = juce::Typeface::createSystemTypefaceFor (GGBinary::RajdhaniBold_ttf, GGBinary::RajdhaniBold_ttfSize);
            }
        };

        Fonts& fonts()
        {
            static Fonts f;
            return f;
        }

        juce::Font withTypeface (juce::Typeface::Ptr tf, float h)
        {
            return juce::Font (juce::FontOptions (tf).withHeight (h));
        }
    }

    const Palette& pal()     { return current(); }
    void setLight (bool l)   { current() = l ? makeLight() : makeDark(); }
    bool isLight()           { return current().light; }

    juce::Colour catColour (gg::Cat c)
    {
        const auto& p = pal();
        switch (c)
        {
            case gg::Cat::kick:      return p.orange;
            case gg::Cat::bass808:   return p.purple;
            case gg::Cat::snare:     return p.blue;
            case gg::Cat::clap:      return p.light ? juce::Colour (0xff0aa5b8) : juce::Colour (0xff4ee6f2);
            case gg::Cat::rim:       return p.light ? juce::Colour (0xff3a86c8) : juce::Colour (0xff7fb2ff);
            case gg::Cat::closedHat: return p.amber;
            case gg::Cat::openHat:   return p.light ? juce::Colour (0xffc76a00) : juce::Colour (0xffffcf6b);
            case gg::Cat::tom:       return p.light ? juce::Colour (0xffb0389d) : juce::Colour (0xffd36bff);
            case gg::Cat::crash:     return p.light ? juce::Colour (0xff4a6fe0) : juce::Colour (0xff8fa8ff);
            case gg::Cat::ride:      return p.light ? juce::Colour (0xff5b5fd6) : juce::Colour (0xffa59bff);
            case gg::Cat::perc:      return p.green;
            case gg::Cat::fx:        return p.light ? juce::Colour (0xffd0287a) : juce::Colour (0xffff5fb0);
            default:                 return p.dim;
        }
    }

    juce::Colour slotColour (gg::Slot s)   { return catColour (gg::slotCat (s)); }

    juce::Font titleFont (float h)           { return withTypeface (fonts().black, h); }
    juce::Font headFont (float h)            { return withTypeface (fonts().bold, h); }
    juce::Font uiFont (float h, bool bold)   { return withTypeface (bold ? fonts().rbold : fonts().semi, h); }
    juce::Font bodyFont (float h)            { return withTypeface (fonts().med, h); }

    juce::ColourGradient brandGradient (juce::Rectangle<float> r, float alpha)
    {
        const auto& p = pal();
        juce::ColourGradient g (p.blue.withMultipliedAlpha (alpha), r.getX(), r.getCentreY(),
                                p.orange.withMultipliedAlpha (alpha), r.getRight(), r.getCentreY(), false);
        g.addColour (0.5, p.purple.withMultipliedAlpha (alpha));
        return g;
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius, bool highlight)
    {
        const auto& p = pal();
        // soft drop shadow
        {
            juce::DropShadow ds (juce::Colours::black.withAlpha (p.light ? 0.10f : 0.55f), p.light ? 12 : 18, { 0, p.light ? 3 : 6 });
            juce::Path path;
            path.addRoundedRectangle (r, radius);
            ds.drawForPath (g, path);
        }
        juce::ColourGradient fill (p.panel.brighter (p.light ? 0.0f : 0.06f), r.getX(), r.getY(),
                                   p.panel.darker (p.light ? 0.015f : 0.25f), r.getX(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (r, radius);
        g.setColour (p.line.withAlpha (p.light ? 1.0f : 0.9f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        if (! p.light)
        {
            // glassy top edge
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + radius, r.getRight() - radius);
        }
        if (highlight) drawGlowOutline (g, r, radius, 1.0f);
    }

    void drawGlowOutline (juce::Graphics& g, juce::Rectangle<float> r, float radius, float strength)
    {
        const auto& p = pal();
        for (int i = 3; i >= 1; --i)
        {
            g.setGradientFill (brandGradient (r, strength * (p.light ? 0.10f : 0.14f) * (float) (4 - i)));
            g.drawRoundedRectangle (r.expanded ((float) i * 1.2f), radius + (float) i, 1.6f);
        }
        g.setGradientFill (brandGradient (r, strength));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.6f);
    }

    void drawSectionTitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, juce::Colour accent)
    {
        const auto& p = pal();
        if (accent.isTransparent()) accent = p.blue;
        g.setColour (accent);
        g.fillRoundedRectangle (juce::Rectangle<float> ((float) r.getX(), (float) r.getCentreY() - 6.0f, 3.0f, 12.0f), 1.5f);
        g.setFont (headFont (11.5f).withExtraKerningFactor (0.12f));
        g.setColour (p.text.withAlpha (0.92f));
        g.drawText (text.toUpperCase(), r.withTrimmedLeft (10), juce::Justification::centredLeft, false);
    }

    juce::Image badgeImage()
    {
        static juce::Image img = juce::ImageCache::getFromMemory (GGBinary::badge_256_png, GGBinary::badge_256_pngSize);
        return img;
    }

    //==============================================================================
    LookAndFeel::LookAndFeel()
    {
        refreshColours();
    }

    void LookAndFeel::refreshColours()
    {
        const auto& p = pal();
        setColour (juce::ResizableWindow::backgroundColourId, p.bg);
        setColour (juce::TextButton::buttonColourId, p.raised);
        setColour (juce::TextButton::buttonOnColourId, p.blue);
        setColour (juce::TextButton::textColourOffId, p.text);
        setColour (juce::TextButton::textColourOnId, p.light ? juce::Colours::white : juce::Colour (0xff04121c));
        setColour (juce::ComboBox::backgroundColourId, p.sunken);
        setColour (juce::ComboBox::textColourId, p.text);
        setColour (juce::ComboBox::outlineColourId, p.line);
        setColour (juce::ComboBox::arrowColourId, p.dim);
        setColour (juce::PopupMenu::backgroundColourId, p.raised);
        setColour (juce::PopupMenu::textColourId, p.text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, p.blue.withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, p.text);
        setColour (juce::Label::textColourId, p.text);
        setColour (juce::TextEditor::backgroundColourId, p.sunken);
        setColour (juce::TextEditor::textColourId, p.text);
        setColour (juce::TextEditor::outlineColourId, p.line);
        setColour (juce::TextEditor::focusedOutlineColourId, p.blue);
        setColour (juce::TextEditor::highlightColourId, p.blue.withAlpha (0.3f));
        setColour (juce::TextEditor::highlightedTextColourId, p.text);
        setColour (juce::CaretComponent::caretColourId, p.blue);
        setColour (juce::Slider::textBoxTextColourId, p.text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::thumbColourId, p.blue);
        setColour (juce::ScrollBar::thumbColourId, p.faint);
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TooltipWindow::backgroundColourId, p.raised);
        setColour (juce::TooltipWindow::textColourId, p.text);
        setColour (juce::ToggleButton::textColourId, p.text);
        setColour (juce::ToggleButton::tickColourId, p.blue);
        setColour (juce::ProgressBar::foregroundColourId, p.blue);
        setColour (juce::ProgressBar::backgroundColourId, p.sunken);
        setColour (juce::TableHeaderComponent::backgroundColourId, p.raised);
        setColour (juce::TableHeaderComponent::textColourId, p.dim);
        setColour (juce::TableHeaderComponent::outlineColourId, p.line);
        setColour (juce::TableHeaderComponent::highlightColourId, p.blue.withAlpha (0.15f));
        setColour (juce::AlertWindow::backgroundColourId, p.panel);
        setColour (juce::AlertWindow::textColourId, p.text);
        setColour (juce::AlertWindow::outlineColourId, p.line);
        setColour (juce::DirectoryContentsDisplayComponent::highlightColourId, p.blue.withAlpha (0.25f));
        setColour (juce::DirectoryContentsDisplayComponent::textColourId, p.text);
        setColour (juce::FileChooserDialogBox::titleTextColourId, p.text);
        setColour (juce::ResizableWindow::backgroundColourId, p.bg);
        setColour (juce::DocumentWindow::textColourId, p.text);
    }

    juce::Typeface::Ptr LookAndFeel::getTypefaceForFont (const juce::Font& f)
    {
        // anything JUCE draws by itself uses Rajdhani too
        if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
            return f.isBold() ? fonts().rbold : fonts().semi;
        return LookAndFeel_V4::getTypefaceForFont (f);
    }

    void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
    {
        const auto& p = pal();
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const auto style = b.getProperties()["style"].toString();
        const float radius = juce::jmin (8.0f, r.getHeight() * 0.32f);
        const bool on = b.getToggleState();
        const float alpha = b.isEnabled() ? 1.0f : 0.4f;

        if (style == "primary")
        {
            // the big gradient button (GENERATE KITS)
            juce::DropShadow (p.purple.withAlpha (0.45f * alpha), over ? 22 : 14, {}).drawForRectangle (g, r.toNearestInt());
            g.setGradientFill (brandGradient (r, alpha));
            g.fillRoundedRectangle (r, radius + 2);
            g.setColour (juce::Colours::white.withAlpha (down ? 0.0f : (over ? 0.22f : 0.12f)));
            g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.5f), radius + 2);
            g.setColour (juce::Colours::white.withAlpha (0.35f * alpha));
            g.drawRoundedRectangle (r, radius + 2, 1.0f);
            return;
        }
        if (style == "accent")
        {
            const auto c = b.getProperties().contains ("accent") ? juce::Colour::fromString (b.getProperties()["accent"].toString()) : p.orange;
            g.setColour (c.withAlpha ((down ? 0.38f : over ? 0.3f : 0.2f) * alpha));
            g.fillRoundedRectangle (r, radius);
            g.setColour (c.withAlpha (0.85f * alpha));
            g.drawRoundedRectangle (r, radius, 1.2f);
            return;
        }
        if (style == "tab")
        {
            if (on)
            {
                g.setColour (p.orange.withAlpha (p.light ? 0.14f : 0.18f));
                g.fillRoundedRectangle (r, radius);
                g.setColour (p.orange);
                g.drawRoundedRectangle (r, radius, 1.4f);
            }
            else if (over)
            {
                g.setColour (p.text.withAlpha (0.06f));
                g.fillRoundedRectangle (r, radius);
            }
            return;
        }
        if (style == "segment")
        {
            if (on)
            {
                g.setGradientFill (brandGradient (r, p.light ? 0.9f : 0.85f));
                g.fillRoundedRectangle (r, radius);
            }
            else
            {
                g.setColour (over ? p.text.withAlpha (0.08f) : juce::Colours::transparentBlack);
                g.fillRoundedRectangle (r, radius);
            }
            return;
        }
        if (style == "toggle")
        {
            if (on)
            {
                g.setGradientFill (brandGradient (r, alpha));
                g.fillRoundedRectangle (r, radius);
            }
            else
            {
                g.setColour (p.text.withAlpha ((down ? 0.14f : over ? 0.08f : 0.03f) * alpha));
                g.fillRoundedRectangle (r, radius);
                g.setColour (p.line.withMultipliedAlpha (alpha));
                g.drawRoundedRectangle (r, radius, 1.0f);
            }
            return;
        }
        if (style == "ghost")
        {
            g.setColour (p.text.withAlpha (down ? 0.14f : over ? 0.08f : 0.0f));
            g.fillRoundedRectangle (r, radius);
            g.setColour (p.line);
            g.drawRoundedRectangle (r, radius, 1.0f);
            return;
        }

        // default
        auto base = on ? p.blue.withAlpha (0.25f) : p.raised;
        if (down) base = base.darker (0.1f);
        else if (over) base = base.brighter (p.light ? 0.02f : 0.12f);
        g.setColour (base.withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (r, radius);
        g.setColour ((on ? p.blue : p.line).withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (r, radius, 1.0f);
    }

    juce::Font LookAndFeel::getTextButtonFont (juce::TextButton& b, int h)
    {
        const auto style = b.getProperties()["style"].toString();
        if (style == "primary") return headFont (juce::jmin (16.0f, h * 0.36f)).withExtraKerningFactor (0.08f);
        if (style == "tab") return headFont (juce::jmin (12.5f, h * 0.36f)).withExtraKerningFactor (0.1f);
        return uiFont (juce::jmin (15.5f, h * 0.52f));
    }

    void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool)
    {
        const auto& p = pal();
        const auto style = b.getProperties()["style"].toString();
        const bool on = b.getToggleState();
        juce::Colour c = p.text;
        if (style == "primary") c = juce::Colours::white;
        else if ((style == "segment" || style == "toggle") && on) c = juce::Colours::white;
        else if (style == "tab") c = on ? p.orange : (over ? p.text : p.dim);
        else if (style == "accent") c = b.getProperties().contains ("accent") ? juce::Colour::fromString (b.getProperties()["accent"].toString()) : p.orange;
        else if (style == "segment") c = over ? p.text : p.dim;
        if (! b.isEnabled()) c = c.withAlpha (0.4f);
        g.setColour (c);
        g.setFont (getTextButtonFont (b, b.getHeight()));
        auto r = b.getLocalBounds().reduced (6, 0);
        if (b.getProperties().contains ("iconLeft"))
            r.removeFromLeft (r.getHeight() - 4);
        g.drawFittedText (b.getButtonText(), r, juce::Justification::centred, 1, 0.8f);
    }

    void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
    {
        const auto& p = pal();
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        // a small switch
        auto sw = r.removeFromLeft (36.0f).withSizeKeepingCentre (30.0f, 16.0f).withX (r.getX() - 36.0f);
        r.removeFromLeft (6.0f);
        g.setColour (on ? p.blue.withAlpha (0.9f) : p.sunken);
        g.fillRoundedRectangle (sw, 8.0f);
        g.setColour (on ? p.blue : p.line);
        g.drawRoundedRectangle (sw, 8.0f, 1.0f);
        const auto knob = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ on ? sw.getRight() - 8.0f : sw.getX() + 8.0f, sw.getCentreY() });
        g.setColour (on ? juce::Colours::white : (over ? p.text : p.dim));
        g.fillEllipse (knob);
        g.setColour (b.isEnabled() ? p.text : p.dim);
        g.setFont (uiFont (14.5f, false));
        g.drawText (b.getButtonText(), r, juce::Justification::centredLeft, true);
    }

    void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
    {
        const auto& p = pal();
        juce::Rectangle<float> r (0, 0, (float) w, (float) h);
        r = r.reduced (0.5f);
        g.setColour (p.sunken);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (box.hasKeyboardFocus (true) || box.isMouseOver (true) ? p.blue.withAlpha (0.8f) : p.line);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        juce::Path arrow;
        const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (p.dim);
        g.fillPath (arrow);
    }

    juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox& b)   { return uiFont (juce::jmin (16.0f, b.getHeight() * 0.56f)); }

    void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    juce::Font LookAndFeel::getPopupMenuFont()   { return uiFont (16.0f, false); }

    void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
    {
        const auto& p = pal();
        g.fillAll (p.raised);
        g.setColour (p.line);
        g.drawRect (0, 0, w, h, 1);
    }

    void LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                         bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                         const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* textColour)
    {
        const auto& p = pal();
        if (isSeparator)
        {
            g.setColour (p.line);
            g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
            return;
        }
        auto r = area.reduced (3, 1);
        if (isHighlighted && isActive)
        {
            g.setColour (p.blue.withAlpha (0.18f));
            g.fillRoundedRectangle (r.toFloat(), 5.0f);
        }
        auto c = textColour != nullptr ? *textColour : p.text;
        if (! isActive) c = c.withAlpha (0.4f);
        g.setColour (c);
        g.setFont (getPopupMenuFont());
        auto textArea = r.reduced (10, 0);
        auto tickArea = textArea.removeFromLeft (16);
        if (isTicked)
        {
            g.setColour (p.orange);
            g.fillEllipse (tickArea.toFloat().withSizeKeepingCentre (7.0f, 7.0f));
            g.setColour (c);
        }
        if (icon != nullptr) icon->drawWithin (g, tickArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
        g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
        if (shortcut.isNotEmpty())
        {
            g.setColour (p.dim);
            g.setFont (uiFont (13.0f, false));
            g.drawText (shortcut, textArea, juce::Justification::centredRight);
        }
        if (hasSubMenu)
        {
            juce::Path arrow;
            const float x = (float) r.getRight() - 12.0f, y = (float) r.getCentreY();
            arrow.addTriangle (x - 3, y - 4, x - 3, y + 4, x + 2, y);
            g.setColour (p.dim);
            g.fillPath (arrow);
        }
    }

    void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
    {
        const auto& p = pal();
        if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearBar)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
            return;
        }
        const auto accent = s.getProperties().contains ("accent") ? juce::Colour::fromString (s.getProperties()["accent"].toString()) : p.blue;
        auto track = juce::Rectangle<float> ((float) x, (float) y + (float) h * 0.5f - 3.0f, (float) w, 6.0f);
        g.setColour (p.sunken);
        g.fillRoundedRectangle (track, 3.0f);
        g.setColour (p.line.withAlpha (0.8f));
        g.drawRoundedRectangle (track, 3.0f, 1.0f);
        auto fill = track.withRight (pos);
        juce::ColourGradient grad (accent.withAlpha (0.55f), fill.getX(), 0, accent, fill.getRight(), 0, false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 3.0f);
        // thumb
        const float tx = pos, ty = track.getCentreY();
        if (! p.light)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.fillEllipse (tx - 9, ty - 9, 18, 18);
        }
        g.setColour (p.light ? juce::Colours::white : juce::Colour (0xfff4f7ff));
        g.fillEllipse (tx - 6, ty - 6, 12, 12);
        g.setColour (accent);
        g.drawEllipse (tx - 6, ty - 6, 12, 12, 2.0f);
    }

    void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
    {
        const auto& p = pal();
        const auto accent = s.getProperties().contains ("accent") ? juce::Colour::fromString (s.getProperties()["accent"].toString()) : p.orange;
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float radius = size * 0.5f;
        const float angle = start + pos * (end - start);

        // the ring
        juce::Path bg;
        bg.addCentredArc (c.x, c.y, radius - 2, radius - 2, 0, start, end, true);
        g.setColour (p.sunken.brighter (p.light ? 0.0f : 0.1f));
        g.strokePath (bg, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // from the centre for bipolar knobs (tune, gain), from the start otherwise
        const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float from = bipolar ? start + (float) ((0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * (end - start) : start;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, radius - 2, radius - 2, 0, juce::jmin (from, angle), juce::jmax (from, angle), true);
        if (! p.light)
        {
            g.setColour (accent.withAlpha (0.25f));
            g.strokePath (arc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (accent);
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // the knob body
        auto body = r.reduced (radius * 0.28f);
        juce::ColourGradient grad (p.raised.brighter (p.light ? 0.0f : 0.25f), body.getX(), body.getY(),
                                   p.sunken, body.getRight(), body.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (body);
        g.setColour (p.line);
        g.drawEllipse (body, 1.0f);
        const float pr = body.getWidth() * 0.5f - 4;
        g.setColour (p.text);
        g.drawLine (c.x + std::sin (angle) * pr * 0.35f, c.y - std::cos (angle) * pr * 0.35f,
                    c.x + std::sin (angle) * pr, c.y - std::cos (angle) * pr, 2.2f);
    }

    juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& s)
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (uiFont (13.5f));
        l->setColour (juce::Label::textColourId, pal().text);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setJustificationType (juce::Justification::centred);
        return l;
    }

    void LookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& e)
    {
        g.setColour (e.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (juce::Rectangle<float> (0, 0, (float) w, (float) h), 8.0f);
    }

    void LookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
    {
        const auto& p = pal();
        const bool focus = e.hasKeyboardFocus (true) && ! e.isReadOnly();
        g.setColour (focus ? p.blue : p.line);
        g.drawRoundedRectangle (juce::Rectangle<float> (0.5f, 0.5f, (float) w - 1, (float) h - 1), 8.0f, focus ? 1.5f : 1.0f);
    }

    void LookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                     int thumbStart, int thumbSize, bool over, bool down)
    {
        const auto& p = pal();
        juce::Rectangle<float> thumb = vertical ? juce::Rectangle<float> ((float) x + 2, (float) thumbStart, (float) w - 4, (float) thumbSize)
                                                : juce::Rectangle<float> ((float) thumbStart, (float) y + 2, (float) thumbSize, (float) h - 4);
        g.setColour ((down ? p.blue : over ? p.dim : p.faint).withAlpha (0.7f));
        g.fillRoundedRectangle (thumb, 3.0f);
    }

    juce::Rectangle<int> LookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
    {
        juce::AttributedString s;
        s.append (text, uiFont (15.0f, false), pal().text);
        juce::TextLayout tl;
        tl.createLayout (s, 320.0f);
        const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 14;
        return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 18,
                                     pos.y > parent.getCentreY() ? pos.y - (h + 6) : pos.y + 20, w, h).constrainedWithin (parent);
    }

    void LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
    {
        const auto& p = pal();
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
        g.setColour (p.raised);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (p.blue.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        juce::AttributedString s;
        s.append (text, uiFont (15.0f, false), p.text);
        juce::TextLayout tl;
        tl.createLayout (s, (float) w - 22);
        tl.draw (g, r.reduced (11, 7));
    }

    void LookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
    {
        g.fillAll (l.findColour (juce::Label::backgroundColourId));
        if (! l.isBeingEdited())
        {
            g.setColour (l.findColour (juce::Label::textColourId).withMultipliedAlpha (l.isEnabled() ? 1.0f : 0.5f));
            g.setFont (l.getFont());
            g.drawFittedText (l.getText(), l.getBorderSize().subtractedFrom (l.getLocalBounds()), l.getJustificationType(),
                              juce::jmax (1, (int) ((float) l.getHeight() / l.getFont().getHeight())), l.getMinimumHorizontalScale());
        }
    }

    void LookAndFeel::drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&)
    {
        const auto& p = pal();
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillPath (path, juce::AffineTransform::translation (0, 3));
        g.setColour (p.raised);
        g.fillPath (path);
        g.setColour (p.blue.withAlpha (0.6f));
        g.strokePath (path, juce::PathStrokeType (1.2f));
    }

    void LookAndFeel::drawProgressBar (juce::Graphics& g, juce::ProgressBar&, int w, int h, double progress, const juce::String& text)
    {
        const auto& p = pal();
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
        g.setColour (p.sunken);
        g.fillRoundedRectangle (r, h * 0.5f);
        if (progress >= 0 && progress <= 1)
        {
            g.setGradientFill (brandGradient (r));
            g.fillRoundedRectangle (r.withWidth (juce::jmax ((float) h, (float) (w * progress))), h * 0.5f);
        }
        if (text.isNotEmpty())
        {
            g.setColour (p.text);
            g.setFont (uiFont (13.0f));
            g.drawText (text, r, juce::Justification::centred);
        }
    }
}
