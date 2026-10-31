#include "HelpPanel.h"
#include "GGBinaryData.h"
#include "../core/Settings.h"

namespace ggui
{
    namespace
    {
        bool isWholeBold (const juce::String& t)
        {
            return t.length() > 4 && t.startsWith ("**") && t.endsWith ("**") && t.substring (2, t.length() - 2).indexOf ("**") < 0;
        }

        juce::String plain (const juce::String& s)       { return s.replace ("**", "").replace ("`", ""); }

        // "text with **bold** words" -> runs in two fonts
        void appendRich (juce::AttributedString& as, const juce::String& text, const juce::Font& normal, const juce::Font& bold,
                         juce::Colour colour, juce::Colour boldColour)
        {
            const auto t = text.replace ("`", "");
            bool isBold = false;
            int pos = 0;
            for (;;)
            {
                const int next = t.indexOf (pos, "**");
                const auto piece = next < 0 ? t.substring (pos) : t.substring (pos, next);
                if (piece.isNotEmpty()) as.append (piece, isBold ? bold : normal, isBold ? boldColour : colour);
                if (next < 0) break;
                isBold = ! isBold;
                pos = next + 2;
            }
        }

        juce::TextLayout makeLayout (const juce::String& text, const juce::Font& normal, const juce::Font& bold,
                                     juce::Colour colour, juce::Colour boldColour, float width, float lineSpacing = 3.0f)
        {
            juce::AttributedString as;
            as.setLineSpacing (lineSpacing);
            appendRich (as, text, normal, bold, colour, boldColour);
            juce::TextLayout tl;
            tl.createLayout (as, juce::jmax (40.0f, width));
            return tl;
        }

        juce::Font monoFont (float size)
        {
            return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
        }
    }

    //==============================================================================
    void DocView::setMarkdown (const juce::String& md)
    {
        blocks.clear();
        matches.clear();
        laidOutWidth = -1;
        const auto lines = juce::StringArray::fromLines (md);
        juce::String para;
        auto flush = [this, &para]
        {
            const auto t = para.trim();
            para.clear();
            if (t.isEmpty()) return;
            Block b;
            if (isWholeBold (t)) { b.type = Block::h3; b.text = t.substring (2, t.length() - 2); }
            else                 { b.type = Block::para; b.text = t; }
            blocks.push_back (std::move (b));
        };
        auto add = [this, &flush] (Block::Type type, const juce::String& text, int level = 0, int ordinal = 0)
        {
            flush();
            Block b;
            b.type = type;
            b.text = text;
            b.level = level;
            b.ordinal = ordinal;
            blocks.push_back (std::move (b));
        };

        for (int i = 0; i < lines.size(); ++i)
        {
            const auto line = lines[i].trimEnd();
            const auto t = line.trimStart();
            const int indent = line.length() - t.length();
            if (t.startsWith ("```"))
            {
                juce::StringArray code;
                for (++i; i < lines.size() && ! lines[i].trimStart().startsWith ("```"); ++i) code.add (lines[i].trimEnd());
                add (Block::code, code.joinIntoString ("\n"));
                continue;
            }
            if (t.isEmpty())             { flush(); continue; }
            if (t.startsWith ("### "))   { add (Block::h3, t.substring (4).trim()); continue; }
            if (t.startsWith ("## "))    { add (Block::h2, t.substring (3).trim()); continue; }
            if (t.startsWith ("# "))     { add (Block::h1, t.substring (2).trim()); continue; }
            if (t.startsWithChar ('|'))
            {
                flush();
                Block b;
                b.type = Block::table;
                for (; i < lines.size() && lines[i].trimStart().startsWithChar ('|'); ++i)
                {
                    auto row = lines[i].trim();
                    if (row.containsOnly ("|-: ")) continue;          // the | --- | line
                    row = row.substring (1);
                    if (row.endsWithChar ('|')) row = row.dropLastCharacters (1);
                    std::vector<Cell> cells;
                    for (auto& c : juce::StringArray::fromTokens (row, "|", ""))
                    {
                        Cell cell;
                        cell.text = c.trim();
                        cells.push_back (std::move (cell));
                    }
                    b.rows.push_back (std::move (cells));
                }
                --i;
                if (! b.rows.empty()) blocks.push_back (std::move (b));
                continue;
            }
            if (t.startsWith ("- ") || t.startsWith ("* "))
            {
                add (Block::bullet, t.substring (2).trim(), indent >= 2 ? 1 : 0);
                continue;
            }
            int digits = 0;
            while (digits < t.length() && juce::CharacterFunctions::isDigit (t[digits])) ++digits;
            if (digits > 0 && digits < 4 && t.substring (digits).startsWith (". "))
            {
                add (Block::number, t.substring (digits + 2).trim(), indent >= 2 ? 1 : 0, t.substring (0, digits).getIntValue());
                continue;
            }
            para << (para.isEmpty() ? "" : " ") << t;
        }
        flush();
    }

    int DocView::layoutFor (int width)
    {
        if (width == laidOutWidth && ! blocks.empty()) return laidOutHeight;
        laidOutWidth = width;
        const auto& p = pal();
        const float left = 8.0f, W = (float) width - left - 14.0f;
        const auto body = bodyFont (17.5f), bold = uiFont (17.5f, true);
        float y = 8.0f;
        sections.clear();

        if (logo.isValid())
        {
            logoBox = { left, y, 112.0f, 112.0f };
            y += 128.0f;
        }
        else
            logoBox = {};

        auto isList = [] (Block::Type t) { return t == Block::bullet || t == Block::number; };
        Block::Type prev = Block::h1;
        bool first = true;
        for (auto& b : blocks)
        {
            if (! first)
            {
                if (b.type == Block::h2)                            y += 26.0f;
                else if (b.type == Block::h3)                       y += 10.0f;
                else if (isList (prev) && ! isList (b.type))        y += 8.0f;
            }
            switch (b.type)
            {
                case Block::h1:
                    b.layout = makeLayout (b.text, headFont (26.0f), headFont (26.0f), p.text, p.text, W);
                    b.box = { left, y, W, b.layout.getHeight() };
                    y += b.box.getHeight() + 16.0f;
                    break;
                case Block::h2:
                    sections.push_back ({ b.text, (int) y - 12 });
                    b.layout = makeLayout (b.text.toUpperCase(), headFont (17.0f), headFont (17.0f), p.orange, p.orange, W);
                    b.box = { left, y, W, b.layout.getHeight() + 10.0f };
                    y += b.box.getHeight() + 12.0f;
                    break;
                case Block::h3:
                    b.layout = makeLayout (b.text, uiFont (19.0f, true), uiFont (19.0f, true), p.text, p.text, W);
                    b.box = { left, y, W, b.layout.getHeight() };
                    y += b.box.getHeight() + 6.0f;
                    break;
                case Block::para:
                    b.layout = makeLayout (b.text, body, bold, p.text, p.text, W);
                    b.box = { left, y, W, b.layout.getHeight() };
                    y += b.box.getHeight() + 12.0f;
                    break;
                case Block::bullet:
                case Block::number:
                {
                    const float indent = (b.type == Block::number ? 30.0f : 24.0f) + (float) b.level * 24.0f;
                    b.layout = makeLayout (b.text, body, bold, p.text, p.text, W - indent);
                    b.box = { left + indent, y, W - indent, b.layout.getHeight() };
                    y += b.box.getHeight() + 7.0f;
                    break;
                }
                case Block::code:
                {
                    juce::AttributedString as;
                    as.setLineSpacing (2.0f);
                    as.append (b.text, monoFont (14.0f), p.text);
                    b.layout.createLayout (as, W - 28.0f);
                    b.box = { left, y, W, b.layout.getHeight() + 26.0f };
                    y += b.box.getHeight() + 16.0f;
                    break;
                }
                case Block::table:
                {
                    const float pad = 9.0f;
                    size_t cols = 0;
                    for (auto& r : b.rows) cols = juce::jmax (cols, r.size());
                    std::vector<float> natural (cols, 60.0f);
                    for (size_t ri = 0; ri < b.rows.size(); ++ri)
                        for (size_t c = 0; c < b.rows[ri].size(); ++c)
                        {
                            const auto& f = ri == 0 ? bold : body;
                            const float w = juce::GlyphArrangement::getStringWidth (f, plain (b.rows[ri][c].text)) + 2.0f * pad + 4.0f;
                            natural[c] = juce::jlimit (natural[c], 520.0f, w);
                        }
                    float total = 0.0f;
                    for (auto w : natural) total += w;
                    std::vector<float> widths (cols);
                    for (size_t c = 0; c < cols; ++c) widths[c] = W * natural[c] / juce::jmax (1.0f, total);

                    float ty = y;
                    b.rowTops.clear();
                    for (size_t ri = 0; ri < b.rows.size(); ++ri)
                    {
                        b.rowTops.push_back (ty);
                        float x = left, rowH = 0.0f;
                        for (size_t c = 0; c < b.rows[ri].size(); ++c)
                        {
                            auto& cell = b.rows[ri][c];
                            const auto& f = ri == 0 ? bold : body;
                            cell.layout = makeLayout (cell.text, f, bold, ri == 0 ? p.text : p.text, p.text, widths[c] - 2.0f * pad, 2.0f);
                            cell.box = { x, ty, widths[c], 0.0f };
                            rowH = juce::jmax (rowH, cell.layout.getHeight() + 2.0f * pad);
                            x += widths[c];
                        }
                        for (auto& cell : b.rows[ri]) cell.box.setHeight (rowH);
                        ty += rowH;
                    }
                    b.box = { left, y, W, ty - y };
                    y = ty + 16.0f;
                    break;
                }
                default: break;
            }
            prev = b.type;
            first = false;
        }
        laidOutHeight = (int) std::ceil (y + 24.0f);
        return laidOutHeight;
    }

    int DocView::setSearch (const juce::String& text)
    {
        const auto q = text.trim().toLowerCase();
        matches.clear();
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            auto& b = blocks[i];
            bool hit = false;
            if (q.isNotEmpty())
            {
                if (b.type == Block::table)
                {
                    for (auto& r : b.rows)
                        for (auto& c : r)
                            hit = hit || plain (c.text).toLowerCase().contains (q);
                }
                else
                    hit = plain (b.text).toLowerCase().contains (q);
            }
            b.match = hit;
            if (hit) matches.push_back ((int) i);
        }
        repaint();
        return (int) matches.size();
    }

    int DocView::getMatchY (int index) const
    {
        if (! juce::isPositiveAndBelow (index, (int) matches.size())) return -1;
        return (int) blocks[(size_t) matches[(size_t) index]].box.getY();
    }

    void DocView::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        const auto clip = g.getClipBounds().toFloat().expanded (0.0f, 40.0f);
        if (! logoBox.isEmpty() && clip.intersects (logoBox))
        {
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (logo, logoBox, juce::RectanglePlacement::centred);
        }

        for (const auto& b : blocks)
        {
            if (! clip.intersects (b.box.expanded (40.0f, 10.0f))) continue;
            if (b.match)
            {
                g.setColour (p.amber.withAlpha (p.light ? 0.22f : 0.16f));
                g.fillRoundedRectangle (b.box.expanded (b.type == Block::bullet || b.type == Block::number ? 30.0f : 6.0f, 4.0f)
                                               .withTrimmedRight (b.type == Block::bullet || b.type == Block::number ? 24.0f : 0.0f), 6.0f);
            }
            switch (b.type)
            {
                case Block::h2:
                    b.layout.draw (g, b.box.withHeight (b.layout.getHeight()));
                    g.setColour (p.line);
                    g.fillRect (b.box.getX(), b.box.getBottom() - 2.0f, b.box.getWidth(), 1.0f);
                    g.setColour (p.orange);
                    g.fillRect (b.box.getX(), b.box.getBottom() - 2.5f, 44.0f, 2.0f);
                    break;
                case Block::bullet:
                {
                    const float cx = b.box.getX() - 13.0f, cy = b.box.getY() + 12.0f;
                    g.setColour (b.level == 0 ? p.blue : p.dim);
                    if (b.level == 0) g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
                    else              g.drawEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f, 1.2f);
                    b.layout.draw (g, b.box);
                    break;
                }
                case Block::number:
                    g.setColour (p.blue);
                    g.setFont (uiFont (17.5f, true));
                    g.drawText (juce::String (b.ordinal) + ".", juce::Rectangle<float> (b.box.getX() - 30.0f, b.box.getY() - 2.0f, 24.0f, 22.0f),
                                juce::Justification::centredRight, false);
                    b.layout.draw (g, b.box);
                    break;
                case Block::code:
                    g.setColour (p.sunken);
                    g.fillRoundedRectangle (b.box, 8.0f);
                    g.setColour (p.line);
                    g.drawRoundedRectangle (b.box.reduced (0.5f), 8.0f, 1.0f);
                    b.layout.draw (g, b.box.reduced (14.0f, 13.0f));
                    break;
                case Block::table:
                {
                    g.setColour (p.sunken);
                    if (! b.rows.empty() && ! b.rows.front().empty())
                        g.fillRoundedRectangle (b.box.withHeight (b.rows.front().front().box.getHeight()), 8.0f);
                    g.setColour (p.line);
                    g.drawRoundedRectangle (b.box.reduced (0.5f), 8.0f, 1.0f);
                    for (size_t ri = 1; ri < b.rowTops.size(); ++ri)
                        g.fillRect (b.box.getX() + 1.0f, b.rowTops[ri], b.box.getWidth() - 2.0f, 1.0f);
                    for (auto& r : b.rows)
                        for (size_t c = 0; c < r.size(); ++c)
                        {
                            if (c > 0)
                            {
                                g.setColour (p.line.withAlpha (0.6f));
                                g.fillRect (r[c].box.getX(), r[c].box.getY() + 6.0f, 1.0f, r[c].box.getHeight() - 12.0f);
                            }
                            r[c].layout.draw (g, r[c].box.reduced (9.0f));
                        }
                    break;
                }
                default:
                    b.layout.draw (g, b.box);
                    break;
            }
        }
    }

    //==============================================================================
    class HelpPanel::Scroller : public juce::Viewport
    {
    public:
        std::function<void()> onScroll;
        void visibleAreaChanged (const juce::Rectangle<int>&) override   { if (onScroll) onScroll(); }
    };

    class HelpPanel::Contents : public juce::Component
    {
    public:
        std::function<void (int)> onPick;
        void setTitles (juce::StringArray t)     { titles = std::move (t); repaint(); }
        void setCurrent (int c)                  { if (c != current) { current = c; repaint(); } }

        void paint (juce::Graphics& g) override
        {
            const auto& p = pal();
            g.setColour (p.sunken);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
            g.setColour (p.dim);
            g.setFont (headFont (11.0f).withExtraKerningFactor (0.12f));
            g.drawText ("CONTENTS", getLocalBounds().reduced (14, 0).removeFromTop (34), juce::Justification::centredLeft);
            for (int i = 0; i < titles.size(); ++i)
            {
                auto r = rowBounds (i);
                if (i == current || i == hover)
                {
                    g.setColour ((i == current ? p.blue : p.text).withAlpha (i == current ? 0.16f : 0.06f));
                    g.fillRoundedRectangle (r.toFloat().reduced (6.0f, 1.0f), 6.0f);
                }
                if (i == current)
                {
                    g.setColour (p.orange);
                    g.fillRoundedRectangle (juce::Rectangle<float> (7.0f, (float) r.getY() + 7.0f, 3.0f, (float) r.getHeight() - 14.0f), 1.5f);
                }
                g.setColour (i == current ? p.text : p.dim);
                g.setFont (uiFont (15.5f, i == current));
                g.drawFittedText (titles[i], r.reduced (18, 0), juce::Justification::centredLeft, 1, 0.8f);
            }
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const int h = rowAt (e.y);
            setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
            if (h != hover) { hover = h; repaint(); }
        }
        void mouseExit (const juce::MouseEvent&) override   { hover = -1; repaint(); }
        void mouseDown (const juce::MouseEvent& e) override
        {
            const int i = rowAt (e.y);
            if (i >= 0 && onPick) onPick (i);
        }

    private:
        int rowH() const                          { return juce::jlimit (24, 34, (getHeight() - 40) / juce::jmax (1, titles.size())); }
        juce::Rectangle<int> rowBounds (int i) const { return { 0, 38 + i * rowH(), getWidth(), rowH() }; }
        int rowAt (int y) const
        {
            const int i = (y - 38) / juce::jmax (1, rowH());
            return y >= 38 && i < titles.size() ? i : -1;
        }
        juce::StringArray titles;
        int current = 0, hover = -1;
    };

    //==============================================================================
    HelpPanel::HelpPanel (GrooveGrabberProcessor& p) : proc (p)
    {
        juce::ignoreUnused (proc);
        setWantsKeyboardFocus (true);

        manual.setMarkdown (juce::String::fromUTF8 (GGBinary::Manual_md, GGBinary::Manual_mdSize).replace ("{VERSION}", GG_VERSION));

        juce::String a;
        a << "# Groove Grabber\n\n"
          << "**Version " << GG_VERSION << "**\n\n"
          << "Your sounds. Your style. Your groove.\n\n"
          << "Groove Grabber builds drum kits from your own library for the song you're making, and exports them ready for Groove Agent. "
          << "It never ships any drum sounds: every kit is made from sounds you already own.\n\n"
          << "## Credits and licences\n\n"
          << juce::String::fromUTF8 (GGBinary::THIRD_PARTY_NOTICES_txt, GGBinary::THIRD_PARTY_NOTICES_txtSize) << "\n\n"
          << "## Your files\n\n"
          << "- **Exports, saved kits, harvested hits:** Documents\\Groove Grabber\n"
          << "- **Settings and the library memory:** " << gg::paths::appDataDir().getFullPathName() << "\n";
        about.setMarkdown (a);
        about.setLogo (badgeImage());

        viewport = std::make_unique<Scroller>();
        viewport->setViewedComponent (&manual, false);
        viewport->setScrollBarsShown (true, false);
        viewport->setSingleStepSizes (0, 48);
        viewport->onScroll = [this] { updateCurrentSection(); };
        addAndMakeVisible (*viewport);

        contents = std::make_unique<Contents>();
        contents->onPick = [this] (int i) { scrollToSection (i); };
        addAndMakeVisible (*contents);

        tabs.onChange = [this] (int i) { showPage (i); };
        addAndMakeVisible (tabs);

        search.setTextToShowWhenEmpty ("Search the manual...", pal().faint);
        search.setFont (uiFont (16.0f, false));
        search.setBorder (juce::BorderSize<int> (4, 10, 4, 10));
        search.setTooltip ("Highlights every part of the manual that mentions it. Enter jumps to the next one.");
        search.onTextChange = [this]
        {
            matchCount = manual.setSearch (search.getText());
            matchIndex = -1;
            nextMatch();
            repaint (matchArea);
        };
        search.onReturnKey = [this] { nextMatch(); };
        search.onEscapeKey = [this]
        {
            if (search.getText().isNotEmpty()) search.clear();
            else setVisible (false);
        };
        addAndMakeVisible (search);

        close.getProperties().set ("style", "ghost");
        close.onClick = [this] { setVisible (false); };
        addAndMakeVisible (close);
        lookAndFeelChanged();
    }

    HelpPanel::~HelpPanel()
    {
        if (viewport != nullptr) viewport->setViewedComponent (nullptr, false);
    }

    void HelpPanel::show (int p)
    {
        lookAndFeelChanged();          // the theme may have changed since it was built
        setVisible (true);
        toFront (true);
        showPage (p);
        grabKeyboardFocus();
    }

    void HelpPanel::showPage (int p)
    {
        page = juce::jlimit (0, 1, p);
        tabs.setSelected (page);
        viewport->setViewedComponent (page == 0 ? static_cast<juce::Component*> (&manual) : &about, false);
        contents->setVisible (page == 0);
        search.setVisible (page == 0);
        resized();
        viewport->setViewPosition (0, 0);
        repaint();
    }

    void HelpPanel::layoutDoc()
    {
        if (viewport->getWidth() <= 0) return;
        auto& doc = page == 0 ? manual : about;
        const int w = viewport->getWidth() - viewport->getScrollBarThickness() - 4;
        const int h = doc.layoutFor (w);
        doc.setSize (w, juce::jmax (h, viewport->getHeight()));
        if (page == 0)
        {
            juce::StringArray titles;
            for (auto& s : manual.getSections()) titles.add (s.title);
            contents->setTitles (titles);
            updateCurrentSection();
        }
        doc.repaint();
    }

    void HelpPanel::scrollToSection (int index)
    {
        const auto& s = manual.getSections();
        if (! juce::isPositiveAndBelow (index, (int) s.size())) return;
        viewport->setViewPosition (0, juce::jmax (0, s[(size_t) index].y));
        contents->setCurrent (index);
    }

    void HelpPanel::nextMatch()
    {
        if (matchCount <= 0) { repaint (matchArea); return; }
        matchIndex = (matchIndex + 1) % matchCount;
        const int y = manual.getMatchY (matchIndex);
        if (y >= 0) viewport->setViewPosition (0, juce::jmax (0, y - 60));
        repaint (matchArea);
    }

    void HelpPanel::updateCurrentSection()
    {
        if (page != 0) return;
        const int y = viewport->getViewPositionY() + 40;
        const auto& s = manual.getSections();
        int current = 0;
        for (int i = 0; i < (int) s.size(); ++i)
            if (s[(size_t) i].y <= y) current = i;
        // at the very bottom, the last section is the one you're reading
        if (viewport->getViewPositionY() + viewport->getHeight() >= manual.getHeight() - 4) current = (int) s.size() - 1;
        contents->setCurrent (current);
    }

    void HelpPanel::lookAndFeelChanged()
    {
        search.setColour (juce::TextEditor::backgroundColourId, pal().sunken);
        search.setColour (juce::TextEditor::outlineColourId, pal().line);
        search.applyColourToAllText (pal().text);
        search.setTextToShowWhenEmpty ("Search the manual...", pal().faint);
        layoutDoc();
        repaint();
    }

    void HelpPanel::resized()
    {
        card = getLocalBounds().withSizeKeepingCentre (juce::jmin (1220, getWidth() - 48), getHeight() - 48);
        auto r = card.reduced (22, 18);
        auto top = r.removeFromTop (40);
        titleArea = top.removeFromLeft (190);
        close.setBounds (top.removeFromRight (100).reduced (0, 3));
        top.removeFromRight (14);
        tabs.setBounds (top.removeFromLeft (230).reduced (0, 3));
        top.removeFromLeft (16);
        matchArea = top.removeFromRight (96);
        search.setBounds (top.removeFromRight (juce::jmin (360, top.getWidth())).reduced (0, 3));
        r.removeFromTop (14);
        if (page == 0)
        {
            contents->setBounds (r.removeFromLeft (250));
            r.removeFromLeft (18);
        }
        viewport->setBounds (r);
        layoutDoc();
    }

    void HelpPanel::paint (juce::Graphics& g)
    {
        const auto& p = pal();
        g.fillAll (juce::Colours::black.withAlpha (p.light ? 0.35f : 0.6f));
        drawPanel (g, card.toFloat(), 16.0f, true);
        drawSectionTitle (g, titleArea.withSizeKeepingCentre (titleArea.getWidth(), 20), page == 0 ? "Manual" : "About", p.orange);
        if (page == 0 && search.getText().trim().isNotEmpty())
        {
            g.setColour (matchCount > 0 ? p.dim : p.red);
            g.setFont (uiFont (14.5f, false));
            g.drawText (matchCount > 0 ? juce::String (matchIndex + 1) + " of " + juce::String (matchCount) : juce::String ("No matches"),
                        matchArea, juce::Justification::centred);
        }
    }

    void HelpPanel::mouseDown (const juce::MouseEvent& e)
    {
        if (! card.contains (e.getPosition())) setVisible (false);
    }

    bool HelpPanel::keyPressed (const juce::KeyPress& k)
    {
        if (k == juce::KeyPress::escapeKey) { setVisible (false); return true; }
        if (k == juce::KeyPress::pageDownKey || k == juce::KeyPress::pageUpKey)
        {
            const int dir = k == juce::KeyPress::pageDownKey ? 1 : -1;
            viewport->setViewPosition (0, juce::jmax (0, viewport->getViewPositionY() + dir * (viewport->getHeight() - 60)));
            return true;
        }
        if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'F') { search.grabKeyboardFocus(); return true; }
        return false;
    }
}
