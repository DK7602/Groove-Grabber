#include "CoachPanel.h"

namespace ggui
{
    namespace
    {
        juce::TextLayout wrapped (const juce::String& text, const juce::Font& f, juce::Colour c, float width)
        {
            juce::AttributedString as;
            as.setWordWrap (juce::AttributedString::byWord);
            as.append (text, f, c);
            juce::TextLayout tl;
            tl.createLayout (as, juce::jmax (40.0f, width));
            return tl;
        }

        juce::Colour ratingColour (int score)
        {
            const auto& p = pal();
            if (score < 0) return p.faint;
            return score >= 85 ? p.green : score >= 65 ? p.blue : score >= 40 ? p.amber : p.red;
        }
    }

    //==============================================================================
    // One tip: a number (or a tick, for what's working), the bold direct line, why the style does it, and its buttons
    class CoachPanel::TipCard : public juce::Component
    {
    public:
        TipCard (const gg::coach::Tip& t, int n) : tip (t), number (n)
        {
            if (! tip.good)
            {
                if (tip.hasFix)
                {
                    tryIt.getProperties().set ("style", "accent");
                    tryIt.setTooltip ("Make this change (UNDO takes it back)");
                    tryIt.addMouseListener (this, false);
                    addAndMakeVisible (tryIt);
                }
                onPurpose.getProperties().set ("style", "ghost");
                onPurpose.setTooltip ("It's like that on purpose: leave this tip out from now on");
                onPurpose.addMouseListener (this, false);
                addAndMakeVisible (onPurpose);
            }
        }

        int heightFor (int width) const
        {
            const float w = (float) textWidth (width);
            const float h = 12.0f + wrapped (tip.title, titleFont(), juce::Colours::white, w).getHeight() + 4.0f
                            + wrapped (tip.why, whyFont(), juce::Colours::white, w).getHeight() + 12.0f;
            // (room for its buttons: TRY IT over ON PURPOSE)
            return juce::jmax (tip.good ? 44 : tip.hasFix ? 24 + 28 + 6 + 28 : 24 + 28, (int) std::ceil (h));
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = pal();
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (p.raised.withAlpha (p.light ? 1.0f : 0.9f));
            g.fillRoundedRectangle (r, 9.0f);
            g.setColour (hovered ? p.blue.withAlpha (0.7f) : p.line);
            g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, hovered ? 1.5f : 1.0f);

            // the number, or a tick for what's working
            auto badge = juce::Rectangle<float> (14.0f, 13.0f, 24.0f, 24.0f);
            if (tip.good)
            {
                g.setColour (p.green.withAlpha (0.2f));
                g.fillEllipse (badge);
                juce::Path tick;
                tick.startNewSubPath (badge.getX() + 7.0f, badge.getCentreY());
                tick.lineTo (badge.getX() + 10.5f, badge.getCentreY() + 4.0f);
                tick.lineTo (badge.getRight() - 6.5f, badge.getY() + 7.5f);
                g.setColour (p.green);
                g.strokePath (tick, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            else
            {
                g.setColour (p.orange.withAlpha (0.18f));
                g.fillEllipse (badge);
                g.setColour (p.orange);
                g.setFont (headFont (12.0f));
                g.drawText (juce::String (number), badge, juce::Justification::centred);
            }

            const float w = (float) textWidth (getWidth());
            auto title = wrapped (tip.title, titleFont(), p.text, w);
            title.draw (g, { (float) textLeft, 12.0f, w, title.getHeight() });
            auto why = wrapped (tip.why, whyFont(), p.dim, w);
            why.draw (g, { (float) textLeft, 12.0f + title.getHeight() + 4.0f, w, why.getHeight() });
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (12, 12).removeFromRight (buttonsWidth);
            if (tryIt.isVisible()) { tryIt.setBounds (r.removeFromTop (28)); r.removeFromTop (6); }
            onPurpose.setBounds (r.removeFromTop (28));
        }

        void mouseEnter (const juce::MouseEvent&) override   { setHovered (true); }
        void mouseExit (const juce::MouseEvent&) override    { setHovered (isMouseOver (true)); }

        std::function<void (bool)> onHoverChanged;
        juce::TextButton tryIt { "TRY IT" }, onPurpose { "ON PURPOSE" };
        const gg::coach::Tip tip;

    private:
        static constexpr int textLeft = 52, buttonsWidth = 116;
        int textWidth (int width) const   { return width - textLeft - (tip.good ? 16 : buttonsWidth + 28); }
        static juce::Font titleFont()     { return uiFont (17.0f, true); }
        static juce::Font whyFont()       { return bodyFont (15.5f); }
        void setHovered (bool h)
        {
            if (h == hovered) return;
            hovered = h;
            repaint();
            if (onHoverChanged) onHoverChanged (h);
        }
        const int number;
        bool hovered = false;
    };

    //==============================================================================
    // The scrolling list: what to work on, what's working, and the tips left out ON PURPOSE
    class CoachPanel::Content : public juce::Component
    {
    public:
        explicit Content (CoachPanel& o) : owner (o)
        {
            bringBack.getProperties().set ("style", "ghost");
            bringBack.setTooltip ("Show the tips you marked ON PURPOSE again");
            bringBack.onClick = [this] { if (owner.onBringBack) owner.onBringBack(); };
            addChildComponent (bringBack);
        }

        void build (const gg::coach::Report& r)
        {
            cards.clear();
            int n = 1;
            for (const auto& t : r.fixes) add (t, n++);
            fixCount = (int) r.fixes.size();
            for (const auto& t : r.working) add (t, 0);
            workingCount = (int) r.working.size();
            onPurposeCount = r.onPurpose;
            styleName = r.coachedAs.upToFirstOccurrenceOf ("  -  ", false, false);
            bringBack.setVisible (onPurposeCount > 0);
        }

        int layout (int width)
        {
            int y = 4;
            const auto heading = [&] (juce::Rectangle<int>& area) { area = { 0, y, width, 26 }; y += 30; };
            if (fixCount > 0) heading (fixHeading);
            else
            {
                fixHeading = {};
                emptyArea = { 0, y, width, 54 };
                y += 62;
            }
            for (int i = 0; i < cards.size(); ++i)
            {
                if (i == fixCount && workingCount > 0) { y += 6; heading (workingHeading); }
                const int h = cards[i]->heightFor (width);
                cards[i]->setBounds (0, y, width, h);
                y += h + 8;
            }
            if (workingCount == 0) workingHeading = {};
            if (onPurposeCount > 0)
            {
                footer = { 0, y + 4, width, 32 };
                bringBack.setBounds (footer.removeFromRight (170).reduced (0, 2));
                y += 44;
            }
            setSize (width, y + 4);
            return y + 4;
        }

        void paint (juce::Graphics& g) override
        {
            const auto& p = pal();
            if (! fixHeading.isEmpty())     drawSectionTitle (g, fixHeading.withHeight (20), "What to work on", p.orange);
            if (! workingHeading.isEmpty()) drawSectionTitle (g, workingHeading.withHeight (20), "What's working", p.green);
            if (fixCount == 0 && ! emptyArea.isEmpty())
            {
                g.setColour (p.text);
                g.setFont (uiFont (18.0f, true));
                g.drawFittedText ("Nothing to fix next to " + styleName + ".", emptyArea.removeFromTop (26), juce::Justification::centredLeft, 1, 0.8f);
                g.setColour (p.dim);
                g.setFont (bodyFont (15.5f));
                g.drawFittedText ("It's your beat now: trust your ears, and drag it into your song.", emptyArea, juce::Justification::centredLeft, 2, 0.8f);
            }
            if (onPurposeCount > 0)
            {
                g.setColour (p.dim);
                g.setFont (bodyFont (15.0f));
                g.drawFittedText (juce::String (onPurposeCount) + (onPurposeCount == 1 ? " tip" : " tips") + " left out: you marked "
                                  + (onPurposeCount == 1 ? "it" : "them") + " ON PURPOSE.",
                                  footer.withTrimmedRight (8), juce::Justification::centredLeft, 1, 0.8f);
            }
        }

        juce::OwnedArray<TipCard> cards;

    private:
        void add (const gg::coach::Tip& t, int n)
        {
            auto* c = cards.add (new TipCard (t, n));
            const auto* tip = &c->tip;
            c->tryIt.onClick = [this, tip] { if (owner.onTry) owner.onTry (*tip); };
            c->onPurpose.onClick = [this, tip] { if (owner.onOnPurpose) owner.onOnPurpose (*tip); };
            c->onHoverChanged = [this, tip] (bool on) { if (owner.onHover) owner.onHover (on ? tip : nullptr); };
            addAndMakeVisible (c);
        }

        CoachPanel& owner;
        juce::TextButton bringBack { "BRING THEM BACK" };
        juce::Rectangle<int> fixHeading, workingHeading, emptyArea, footer;
        int fixCount = 0, workingCount = 0, onPurposeCount = 0;
        juce::String styleName;
    };

    //==============================================================================
    CoachPanel::CoachPanel()
    {
        content = std::make_unique<Content> (*this);
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        view.setSingleStepSizes (0, 40);
        addAndMakeVisible (view);
    }

    CoachPanel::~CoachPanel()
    {
        view.setViewedComponent (nullptr, false);
    }

    void CoachPanel::setReport (const gg::coach::Report& r)
    {
        report = r;
        stale = false;
        rebuild();
    }

    void CoachPanel::setStale (bool s)
    {
        if (s == stale) return;
        stale = s;
        resized();
        repaint();
    }

    void CoachPanel::rebuild()
    {
        if (onHover) onHover (nullptr);
        content->build (report);
        resized();
        view.setViewPosition (0, 0);
        repaint();
    }

    void CoachPanel::tryTip (int i)
    {
        if (juce::isPositiveAndBelow (i, (int) report.fixes.size()) && report.fixes[(size_t) i].hasFix && onTry)
        {
            const auto tip = report.fixes[(size_t) i];
            onTry (tip);
        }
    }

    void CoachPanel::markOnPurpose (int i)
    {
        if (juce::isPositiveAndBelow (i, (int) report.fixes.size()) && onOnPurpose)
        {
            const auto tip = report.fixes[(size_t) i];
            onOnPurpose (tip);
        }
    }

    void CoachPanel::lookAndFeelChanged()
    {
        repaint();
        content->repaint();
    }

    void CoachPanel::resized()
    {
        auto r = getLocalBounds();
        ratingsArea = r.removeFromTop (54);
        staleArea = stale ? r.removeFromTop (34) : juce::Rectangle<int>();
        r.removeFromTop (8);
        view.setBounds (r);
        content->layout (juce::jmax (200, r.getWidth() - view.getScrollBarThickness() - 6));
    }

    void CoachPanel::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        // the four ratings
        const int n = (int) report.ratings.size();
        if (n > 0)
        {
            const int w = ratingsArea.getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                const auto& rt = report.ratings[(size_t) i];
                auto cell = ratingsArea.withX (ratingsArea.getX() + i * w).withWidth (w).reduced (i == 0 ? 0 : 8, 4).withTrimmedRight (i == n - 1 ? 0 : 8);
                const auto c = ratingColour (rt.score);
                g.setColour (p.dim);
                g.setFont (headFont (10.5f).withExtraKerningFactor (0.12f));
                g.drawText (rt.name.toUpperCase(), cell.removeFromTop (16), juce::Justification::centredLeft);
                auto bar = cell.removeFromTop (12).toFloat().withSizeKeepingCentre ((float) cell.getWidth(), 6.0f);
                g.setColour (p.line);
                g.fillRoundedRectangle (bar, 3.0f);
                if (rt.score > 0)
                {
                    g.setColour (c);
                    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * (float) rt.score / 100.0f), 3.0f);
                }
                g.setColour (rt.score < 0 ? p.faint : p.text);
                g.setFont (uiFont (15.0f, true));
                g.drawText (rt.word, cell, juce::Justification::centredLeft);
            }
        }
        if (stale)
        {
            auto s = staleArea.reduced (0, 3).toFloat();
            g.setColour (p.amber.withAlpha (0.14f));
            g.fillRoundedRectangle (s, 7.0f);
            g.setColour (p.light ? p.amber.darker (0.4f) : p.amber);
            g.setFont (uiFont (15.0f, true));
            g.drawFittedText ("You've changed the beat or the style since: press COACH AGAIN for fresh coaching.", s.toNearestInt().reduced (12, 0),
                              juce::Justification::centredLeft, 1, 0.8f);
        }
    }
}
