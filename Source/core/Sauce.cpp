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
