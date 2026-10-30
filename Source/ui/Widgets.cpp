#include "Widgets.h"
#include "core/SampleRender.h"

namespace ggui
{
    juce::Path iconPath (Icon i)
    {
        juce::Path p;
        switch (i)
        {
            case Icon::play:
                p.addTriangle (0.28f, 0.18f, 0.28f, 0.82f, 0.84f, 0.5f);
                break;
            case Icon::stop:
                p.addRoundedRectangle (0.24f, 0.24f, 0.52f, 0.52f, 0.06f);
                break;
            case Icon::lock:
            case Icon::unlock:
            {
                p.addRoundedRectangle (0.2f, 0.45f, 0.6f, 0.45f, 0.07f);
                juce::Path shackle;
                if (i == Icon::lock) shackle.addCentredArc (0.5f, 0.45f, 0.19f, 0.22f, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
                else                 shackle.addCentredArc (0.5f, 0.38f, 0.19f, 0.2f, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi * 0.6f, true);
                juce::PathStrokeType (0.09f).createStrokedPath (shackle, shackle);
                p.addPath (shackle);
                if (i == Icon::unlock) p.applyTransform (juce::AffineTransform());
                break;
            }
            case Icon::heart:
            case Icon::heartFilled:
            {
                juce::Path h;
                h.startNewSubPath (0.5f, 0.85f);
                h.cubicTo (0.05f, 0.55f, 0.08f, 0.15f, 0.32f, 0.17f);
                h.cubicTo (0.42f, 0.18f, 0.48f, 0.25f, 0.5f, 0.32f);
                h.cubicTo (0.52f, 0.25f, 0.58f, 0.18f, 0.68f, 0.17f);
                h.cubicTo (0.92f, 0.15f, 0.95f, 0.55f, 0.5f, 0.85f);
                h.closeSubPath();
                if (i == Icon::heart) juce::PathStrokeType (0.08f).createStrokedPath (p, h);
                else p = h;
                break;
            }
            case Icon::dice:
            {
                juce::Path box;
                box.addRoundedRectangle (0.14f, 0.14f, 0.72f, 0.72f, 0.14f);
                juce::PathStrokeType (0.07f).createStrokedPath (p, box);
                for (auto pt : { juce::Point<float> (0.33f, 0.33f), { 0.67f, 0.67f }, { 0.5f, 0.5f }, { 0.67f, 0.33f }, { 0.33f, 0.67f } })
                    p.addEllipse (pt.x - 0.06f, pt.y - 0.06f, 0.12f, 0.12f);
                break;
            }
            case Icon::refresh:
            {
                juce::Path arc;
                arc.addCentredArc (0.5f, 0.5f, 0.3f, 0.3f, 0, 0.6f, juce::MathConstants<float>::twoPi - 0.2f, true);
                juce::PathStrokeType (0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, arc);
                p.addTriangle (0.62f, 0.08f, 0.88f, 0.24f, 0.6f, 0.36f);
                break;
            }
            case Icon::gear:
            {
                for (int k = 0; k < 8; ++k)
                {
                    juce::Path tooth;
                    tooth.addRoundedRectangle (0.44f, 0.06f, 0.12f, 0.2f, 0.03f);
                    tooth.applyTransform (juce::AffineTransform::rotation ((float) k * juce::MathConstants<float>::twoPi / 8.0f, 0.5f, 0.5f));
                    p.addPath (tooth);
                }
                juce::Path ring;
                ring.addEllipse (0.2f, 0.2f, 0.6f, 0.6f);
                ring.addEllipse (0.36f, 0.36f, 0.28f, 0.28f);
                ring.setUsingNonZeroWinding (false);
                p.addPath (ring);
                p.setUsingNonZeroWinding (false);
                break;
            }
            case Icon::sun:
            {
                p.addEllipse (0.32f, 0.32f, 0.36f, 0.36f);
                for (int k = 0; k < 8; ++k)
                {
                    juce::Path ray;
                    ray.addRoundedRectangle (0.465f, 0.04f, 0.07f, 0.16f, 0.035f);
                    ray.applyTransform (juce::AffineTransform::rotation ((float) k * juce::MathConstants<float>::twoPi / 8.0f, 0.5f, 0.5f));
                    p.addPath (ray);
                }
                break;
            }
            case Icon::moon:
            {
                // a crescent: the part of one circle that a second, offset circle doesn't cover
                const juce::Point<float> c1 (0.48f, 0.54f), c2 (0.68f, 0.36f);
                const float r1 = 0.38f, r2 = 0.33f;
                const float pi = juce::MathConstants<float>::pi;
                auto onCircle = [] (juce::Point<float> c, float r, float a) { return juce::Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };
                auto angleOf = [] (juce::Point<float> v) { return std::atan2 (v.x, -v.y); };
                std::vector<juce::Point<float>> outer, inner;
                const float away = angleOf (c1 - c2);          // the side of circle 1 facing away from circle 2
                for (int k = 0; k <= 96; ++k)
                {
                    const auto pt = onCircle (c1, r1, away - pi + 2 * pi * (float) k / 96.0f);
                    if (pt.getDistanceFrom (c2) >= r2) outer.push_back (pt);
                }
                const float toward = angleOf (c1 - c2);
                for (int k = 0; k <= 96; ++k)
                {
                    const auto pt = onCircle (c2, r2, toward - pi + 2 * pi * (float) k / 96.0f);
                    if (pt.getDistanceFrom (c1) <= r1) inner.push_back (pt);
                }
                if (! outer.empty() && ! inner.empty())
                {
                    if (outer.back().getDistanceFrom (inner.front()) > outer.back().getDistanceFrom (inner.back()))
                        std::reverse (inner.begin(), inner.end());
                    p.startNewSubPath (outer.front());
                    for (auto& pt : outer) p.lineTo (pt);
                    for (auto& pt : inner) p.lineTo (pt);
                    p.closeSubPath();
                }
                break;
            }
            case Icon::folder:
                p.startNewSubPath (0.1f, 0.25f);
                p.lineTo (0.4f, 0.25f); p.lineTo (0.48f, 0.34f); p.lineTo (0.9f, 0.34f); p.lineTo (0.9f, 0.8f); p.lineTo (0.1f, 0.8f);
                p.closeSubPath();
                break;
            case Icon::exportIcon:
            {
                juce::Path s;
                s.startNewSubPath (0.5f, 0.62f); s.lineTo (0.5f, 0.12f);
                s.startNewSubPath (0.3f, 0.3f); s.lineTo (0.5f, 0.1f); s.lineTo (0.7f, 0.3f);
                s.startNewSubPath (0.2f, 0.55f); s.lineTo (0.2f, 0.88f); s.lineTo (0.8f, 0.88f); s.lineTo (0.8f, 0.55f);
                juce::PathStrokeType (0.08f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, s);
                break;
            }
            case Icon::compass:
            {
                juce::Path ring;
                ring.addEllipse (0.1f, 0.1f, 0.8f, 0.8f);
                juce::PathStrokeType (0.07f).createStrokedPath (p, ring);
                p.addTriangle (0.5f, 0.2f, 0.6f, 0.5f, 0.4f, 0.5f);
                juce::Path s;
                s.addTriangle (0.5f, 0.8f, 0.6f, 0.5f, 0.4f, 0.5f);
                p.addPath (s);
                break;
            }
            case Icon::search:
            {
                juce::Path s;
                s.addEllipse (0.15f, 0.15f, 0.5f, 0.5f);
                s.startNewSubPath (0.58f, 0.58f); s.lineTo (0.86f, 0.86f);
                juce::PathStrokeType (0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, s);
                break;
            }
            case Icon::close:
            {
                juce::Path s;
                s.startNewSubPath (0.25f, 0.25f); s.lineTo (0.75f, 0.75f);
                s.startNewSubPath (0.75f, 0.25f); s.lineTo (0.25f, 0.75f);
                juce::PathStrokeType (0.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, s);
                break;
            }
            case Icon::star:
                p.addStar ({ 0.5f, 0.52f }, 5, 0.18f, 0.42f, 0);
                break;
            case Icon::plus:
            case Icon::minus:
            {
                juce::Path s;
                s.startNewSubPath (0.2f, 0.5f); s.lineTo (0.8f, 0.5f);
                if (i == Icon::plus) { s.startNewSubPath (0.5f, 0.2f); s.lineTo (0.5f, 0.8f); }
                juce::PathStrokeType (0.11f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, s);
                break;
            }
            case Icon::waveform:
            {
                const float h[] = { 0.2f, 0.45f, 0.7f, 0.35f, 0.9f, 0.55f, 0.3f, 0.6f, 0.25f };
                for (int k = 0; k < 9; ++k)
                    p.addRoundedRectangle (0.08f + (float) k * 0.1f, 0.5f - h[k] * 0.42f, 0.055f, h[k] * 0.84f, 0.025f);
                break;
            }
            case Icon::drag:
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 2; ++c)
                        p.addEllipse (0.33f + (float) c * 0.26f - 0.07f, 0.22f + (float) r * 0.28f - 0.07f, 0.14f, 0.14f);
                break;
            case Icon::check:
            {
                juce::Path s;
                s.startNewSubPath (0.18f, 0.52f); s.lineTo (0.42f, 0.75f); s.lineTo (0.84f, 0.26f);
                juce::PathStrokeType (0.11f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (p, s);
                break;
            }
            case Icon::music:
                p.addEllipse (0.14f, 0.62f, 0.26f, 0.22f);
                p.addEllipse (0.56f, 0.54f, 0.26f, 0.22f);
                p.addRectangle (0.34f, 0.18f, 0.07f, 0.56f);
                p.addRectangle (0.76f, 0.1f, 0.07f, 0.56f);
                p.addQuadrilateral (0.34f, 0.18f, 0.83f, 0.1f, 0.83f, 0.22f, 0.34f, 0.3f);
                break;
            case Icon::trash:
                p.addRoundedRectangle (0.25f, 0.3f, 0.5f, 0.58f, 0.05f);
                p.addRectangle (0.18f, 0.18f, 0.64f, 0.07f);
                p.addRectangle (0.4f, 0.1f, 0.2f, 0.08f);
                break;
            case Icon::eye:
            {
                juce::Path s;
                s.startNewSubPath (0.06f, 0.5f);
                s.quadraticTo (0.5f, 0.05f, 0.94f, 0.5f);
                s.quadraticTo (0.5f, 0.95f, 0.06f, 0.5f);
                juce::PathStrokeType (0.07f).createStrokedPath (p, s);
                p.addEllipse (0.38f, 0.38f, 0.24f, 0.24f);
                break;
            }
            case Icon::library:
                p.addRoundedRectangle (0.12f, 0.15f, 0.14f, 0.7f, 0.03f);
                p.addRoundedRectangle (0.32f, 0.15f, 0.14f, 0.7f, 0.03f);
                p.addQuadrilateral (0.54f, 0.2f, 0.66f, 0.15f, 0.9f, 0.8f, 0.78f, 0.85f);
                break;
            case Icon::grid:
                for (int r = 0; r < 2; ++r)
                    for (int c = 0; c < 2; ++c)
                        p.addRoundedRectangle (0.14f + (float) c * 0.38f, 0.14f + (float) r * 0.38f, 0.32f, 0.32f, 0.06f);
                break;
        }
        return p;
    }

    void drawIcon (juce::Graphics& g, Icon i, juce::Rectangle<float> r, juce::Colour c)
    {
        auto p = iconPath (i);
        const float s = juce::jmin (r.getWidth(), r.getHeight());
        p.applyTransform (juce::AffineTransform::scale (s).translated (r.getCentreX() - s * 0.5f, r.getCentreY() - s * 0.5f));
        g.setColour (c);
        g.fillPath (p);
    }

    //==============================================================================
    IconButton::IconButton (const juce::String& name, Icon i, Icon on, bool hasOn)
        : juce::Button (name), icon (i), iconOn (on), hasOnIcon (hasOn)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
    {
        const auto& p = pal();
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const auto a = accent.isTransparent() ? p.blue : accent;
        const bool on = getToggleState();
        if (filled || on)
        {
            g.setColour (a.withAlpha (filled ? (down ? 0.45f : over ? 0.35f : 0.25f) : 0.2f));
            g.fillRoundedRectangle (r, 7.0f);
            g.setColour (a.withAlpha (0.8f));
            g.drawRoundedRectangle (r, 7.0f, 1.0f);
        }
        else if (over)
        {
            g.setColour (p.text.withAlpha (down ? 0.14f : 0.07f));
            g.fillRoundedRectangle (r, 7.0f);
        }
        auto iconArea = r;
        if (label.isNotEmpty())
            iconArea = r.removeFromLeft (r.getHeight());
        const auto colour = ! isEnabled() ? p.faint : (on || filled ? a : (over ? p.text : p.dim));
        const float side = juce::jmin (iconArea.getWidth(), iconArea.getHeight());
        drawIcon (g, hasOnIcon && on ? iconOn : icon, iconArea.withSizeKeepingCentre (side, side).reduced (side * 0.2f), colour);
        if (label.isNotEmpty())
        {
            g.setColour (on || filled ? a : (over ? p.text : p.dim));
            g.setFont (uiFont (juce::jmin (15.0f, r.getHeight() * 0.55f)));
            g.drawText (label, r.withTrimmedRight (6), juce::Justification::centredLeft, true);
        }
    }

    //==============================================================================
    Segmented::Segmented (const juce::StringArray& labels)
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            auto* b = buttons.add (new juce::TextButton (labels[i]));
            b->getProperties().set ("style", "segment");
            b->setClickingTogglesState (false);
            b->onClick = [this, i] { setSelected (i, true); };
            addAndMakeVisible (b);
        }
        setSelected (0);
    }

    void Segmented::setTooltips (const juce::StringArray& t)
    {
        for (int i = 0; i < juce::jmin (t.size(), buttons.size()); ++i) buttons[i]->setTooltip (t[i]);
    }

    void Segmented::setSelected (int i, bool notify)
    {
        selected = juce::jlimit (0, buttons.size() - 1, i);
        for (int k = 0; k < buttons.size(); ++k) buttons[k]->setToggleState (k == selected, juce::dontSendNotification);
        if (notify && onChange) onChange (selected);
    }

    void Segmented::resized()
    {
        auto r = getLocalBounds().reduced (2);
        const int w = r.getWidth() / juce::jmax (1, buttons.size());
        for (int i = 0; i < buttons.size(); ++i)
            buttons[i]->setBounds (i == buttons.size() - 1 ? r : r.removeFromLeft (w));
    }

    void Segmented::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().toFloat();
        g.setColour (p.sunken);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (p.line);
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    }

    //==============================================================================
    void WaveView::setAudio (std::shared_ptr<const juce::AudioBuffer<float>> a)
    {
        if (a == audio) return;
        audio = std::move (a);
        rebuild();
    }

    void WaveView::rebuild()
    {
        if (audio == nullptr || audio->getNumSamples() == 0 || getWidth() <= 0) { peaks.clear(); repaint(); return; }
        peaks = gg::render::peaks (*audio, juce::jmax (8, getWidth() / 2));
        repaint();
    }

    void WaveView::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        auto r = getLocalBounds().toFloat();
        if (peaks.empty())
        {
            if (emptyText.isNotEmpty())
            {
                g.setColour (p.faint);
                g.setFont (uiFont (13.0f, false));
                g.drawText (emptyText, r, juce::Justification::centred);
            }
            return;
        }
        const auto c = colour.isTransparent() ? p.blue : colour;
        float mx = 0.001f;
        for (auto& pk : peaks) mx = juce::jmax (mx, -pk.first, pk.second);
        const float mid = r.getCentreY(), half = r.getHeight() * 0.48f / mx;
        juce::Path path;
        const float step = r.getWidth() / (float) peaks.size();
        path.startNewSubPath (r.getX(), mid);
        for (size_t i = 0; i < peaks.size(); ++i) path.lineTo (r.getX() + (float) i * step, mid - peaks[i].second * half);
        for (size_t i = peaks.size(); i-- > 0;) path.lineTo (r.getX() + (float) i * step, mid - peaks[i].first * half);
        path.closeSubPath();
        juce::ColourGradient grad (c.brighter (0.3f), r.getX(), r.getY(), c.withAlpha (0.65f), r.getX(), r.getBottom(), false);
        if (! p.light)
        {
            g.setColour (c.withAlpha (0.18f));
            g.strokePath (path, juce::PathStrokeType (3.0f));
        }
        g.setGradientFill (grad);
        g.fillPath (path);

        if (startMark >= 0)
        {
            g.setColour (p.bg.withAlpha (0.55f));
            g.fillRect (r.withWidth (r.getWidth() * startMark));
        }
        if (endMark >= 0 && endMark < 1)
        {
            g.setColour (p.bg.withAlpha (0.55f));
            g.fillRect (r.withTrimmedLeft (r.getWidth() * endMark));
        }
        if (playhead >= 0)
        {
            g.setColour (p.orange);
            g.fillRect (r.getX() + r.getWidth() * playhead - 1, r.getY(), 2.0f, r.getHeight());
        }
    }

    //==============================================================================
    LabelledKnob::LabelledKnob (const juce::String& n, double min, double max, double step, double def, const juce::String& suffix)
        : name (n)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setRange (min, max, step);
        slider.setValue (def, juce::dontSendNotification);
        slider.setDoubleClickReturnValue (true, def);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        slider.setTextValueSuffix (suffix);
        slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        addAndMakeVisible (slider);
    }

    void LabelledKnob::setAccent (juce::Colour c)
    {
        slider.getProperties().set ("accent", c.toString());
        slider.setColour (juce::Slider::textBoxTextColourId, pal().text);     // (re)colours the value box for the theme
        slider.repaint();
    }

    void LabelledKnob::resized()
    {
        auto r = getLocalBounds();
        r.removeFromTop (16);
        const int size = juce::jmin (r.getWidth(), r.getHeight() - 20, 74);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, juce::jmax (60, r.getWidth()), 18);
        slider.setBounds (r.withSizeKeepingCentre (r.getWidth(), size + 20).withY (r.getY()));
    }

    void LabelledKnob::paint (juce::Graphics& g)
    {
        g.setColour (pal().dim);
        g.setFont (headFont (9.5f).withExtraKerningFactor (0.12f));
        g.drawText (name.toUpperCase(), getLocalBounds().removeFromTop (14), juce::Justification::centred);
    }

    //==============================================================================
    void Toast::show (const juce::String& t, juce::Colour a)
    {
        text = t;
        accent = a;
        alpha = 1.0f;
        ticks = 0;
        startTimerHz (20);
        repaint();
    }

    void Toast::timerCallback()
    {
        if (++ticks > 140) alpha -= 0.05f;
        if (alpha <= 0) { alpha = 0; stopTimer(); }
        repaint();
    }

    void Toast::paint (juce::Graphics& g)
    {
        if (alpha <= 0 || text.isEmpty()) return;
        const auto& p = pal();
        auto r = getLocalBounds().toFloat();
        g.setColour ((accent.isTransparent() ? p.blue : accent).withAlpha (alpha));
        g.fillEllipse (r.removeFromLeft (14).withSizeKeepingCentre (7, 7));
        g.setColour (p.text.withAlpha (alpha * 0.9f));
        g.setFont (uiFont (15.0f, false));
        g.drawText (text, r.withTrimmedLeft (4), juce::Justification::centredLeft, true);
    }
}
