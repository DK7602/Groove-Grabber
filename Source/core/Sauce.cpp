#include "Sauce.h"
#include "GGBinaryData.h"
#include <map>

namespace gg
{
    namespace
    {
        // Resources/Sauce.md split at its "@@ key" lines, read once
        const std::map<juce::String, juce::String>& sections()
        {
            static const auto all = []
            {
                std::map<juce::String, juce::String> m;
                const auto text = juce::String::fromUTF8 (GGBinary::Sauce_md, GGBinary::Sauce_mdSize);
                juce::String key;
                juce::StringArray body;
                auto flush = [&]
                {
                    if (key.isNotEmpty()) m[key] = body.joinIntoString ("\n").trim();
                    body.clear();
                };
                for (const auto& line : juce::StringArray::fromLines (text))
                {
                    if (line.startsWith ("@@ "))
                    {
                        flush();
                        key = line.substring (3).trim();
                        continue;
                    }
                    if (key.isNotEmpty()) body.add (line);
                }
                flush();
                return m;
            }();
            return all;
        }
    }

    juce::String sauceSection (const juce::String& key)
    {
        const auto& m = sections();
        const auto it = m.find (key);
        return it != m.end() ? it->second : juce::String();
    }

    juce::StringArray sauceKeys()
    {
        juce::StringArray keys;
        for (const auto& [k, v] : sections()) keys.add (k);
        return keys;
    }

    juce::String sauceMarkdown (Genre genre, int subStyle, int era)
    {
        const auto& list = stylesOf (genre);
        const auto& style = styleFor (genre, subStyle);
        const auto genreKey = list.front().id;

        juce::String md = sauceSection (style.id);
        if (md.isEmpty()) md = sauceSection (genreKey);

        if (era >= 0 && era < numEras)
        {
            md << "\n\n## " << (era == numEras - 1 ? juce::String ("Today") : "In the " + eraName (era)) << "\n\n";
            if (const auto line = sauceSection ("era." + juce::String (era) + "." + genreKey); line.isNotEmpty())
                md << line << "\n\n";
            md << sauceSection ("era." + juce::String (era));
        }
        return md.trim();
    }

    std::vector<const Groove*> sauceGrooves (const Style& style)
    {
        std::vector<const Groove*> out;
        for (const auto& d : style.directions)
        {
            const auto* g = &grooveFor (d.groove);
            if (std::find (out.begin(), out.end(), g) == out.end()) out.push_back (g);
        }
        return out;
    }

    SauceTable sauceTable (const Style& style)
    {
        SauceTable t;
        const auto lines = juce::StringArray::fromLines (sauceSection (style.id));
        const int sep = lines.indexOf ("| --- | --- | --- | --- |");
        if (sep < 1 || sep + 1 >= lines.size()) return t;
        auto cells = juce::StringArray::fromTokens (lines[sep + 1], "|", "");
        cells.removeEmptyStrings (true);
        if (cells.size() < 4) return t;
        // the numbers in a cell, within a range ("85 to 98 BPM" -> 85, 98)
        const auto numbers = [] (const juce::String& cell, int lo, int hi)
        {
            std::vector<int> v;
            juce::String digits;
            for (auto c : cell + " ")
            {
                if (juce::CharacterFunctions::isDigit (c)) { digits << juce::String::charToString (c); continue; }
                if (digits.isNotEmpty()) { const int n = digits.getIntValue(); if (n >= lo && n <= hi) v.push_back (n); digits.clear(); }
            }
            return v;
        };
        const auto tempo = numbers (cells[0], 40, 250), swing = numbers (cells[1], 45, 80);
        if (! tempo.empty()) { t.tempoLo = *std::min_element (tempo.begin(), tempo.end()); t.tempoHi = *std::max_element (tempo.begin(), tempo.end()); }
        if (! swing.empty()) { t.swingLo = *std::min_element (swing.begin(), swing.end()); t.swingHi = *std::max_element (swing.begin(), swing.end()); }
        else if (cells[1].containsIgnoreCase ("straight")) { t.swingLo = t.swingHi = 50; }
        t.timeFeel = cells[2].trim();
        t.lowEnd = cells[3].trim();
        return t;
    }

    juce::String sauceLowEnd (const Style& style)
    {
        int with808 = 0;
        for (const auto& d : style.directions)
            if (d.use808 > 0 || (d.use808 < 0 && style.use808)) ++with808;
        if (with808 == 0) return "Kick";
        if (with808 == (int) style.directions.size()) return "Kick + 808";
        return "Kick, often an 808";
    }

    juce::String saucePercussion (const Style& style)
    {
        juce::StringArray names;
        for (auto k : style.percPrefs)
            if (k != PercKind::none && k != PercKind::other) names.addIfNotAlreadyThere (percKindName (k));
        if (names.isEmpty()) return "Sound effects and hits";
        return names.joinIntoString (", ");
    }

    juce::String sauceEras (const Style& style)
    {
        juce::StringArray names;
        for (int e : style.eras) names.add (eraName (e));
        return names.isEmpty() ? juce::String ("Every decade") : names.joinIntoString (", ");
    }
}
