#include "SongContext.h"
#include <optional>

namespace gg
{
    namespace
    {
        juce::String normalise (const juce::String& s)
        {
            juce::String out;
            const auto lower = s.toLowerCase();
            for (auto p = lower.getCharPointer(); ! p.isEmpty(); ++p)
            {
                const auto c = *p;
                out << (juce::CharacterFunctions::isLetterOrDigit (c) || c == '&' || c == '#' ? c : (juce::juce_wchar) ' ');
            }
            return " " + out.replace ("  ", " ") + " ";
        }

        bool has (const juce::String& text, const char* phrase)
        {
            return text.contains (" " + juce::String (phrase) + " ");
        }

        int findKeyInText (const juce::String& original)
        {
            // explicit keys only: "A minor", "Am", "F# major", "key of Eb", "in C" (never the word "a")
            const auto words = juce::StringArray::fromTokens (original.replaceCharacters (",.;:!()", "       "), " \t\n", "");
            for (int i = 0; i < words.size(); ++i)
            {
                const auto w = words[i];
                if (w.isEmpty()) continue;
                const auto first = w[0];
                if (first < 'A' || first > 'G')
                {
                    if (w.equalsIgnoreCase ("key") && i + 2 < words.size() && words[i + 1].equalsIgnoreCase ("of"))
                    {
                        const int k = parseKey (words[i + 2] + " " + words[juce::jmin (i + 3, words.size() - 1)]);
                        if (k >= 0) return k;
                    }
                    continue;
                }
                const auto next = i + 1 < words.size() ? words[i + 1].toLowerCase() : juce::String();
                if (next == "minor" || next == "min" || next == "major" || next == "maj")
                    return parseKey (w + " " + next);
                // "Am", "F#m", "Bbm", "C#" ... as a whole word
                if (w.length() <= 4 && juce::String ("#bm").containsChar (w.getLastCharacter()) && w.length() >= 2)
                {
                    const int k = parseKey (w);
                    if (k >= 0) return k;
                }
                // "in C"
                if (w.length() == 1 && i > 0 && words[i - 1].equalsIgnoreCase ("in"))
                    return parseKey (w);
            }
            return -1;
        }

        double findBpm (const juce::String& text)
        {
            const auto lower = text.toLowerCase();
            int at = lower.indexOf ("bpm");
            while (at >= 0)
            {
                int end = at;
                while (end > 0 && lower[end - 1] == ' ') --end;
                int start = end;
                while (start > 0 && juce::CharacterFunctions::isDigit (lower[start - 1])) --start;
                if (start < end)
                {
                    const double v = lower.substring (start, end).getDoubleValue();
                    if (v >= 40 && v <= 250) return v;
                }
                // "bpm 90"
                int s2 = at + 3;
                while (s2 < lower.length() && lower[s2] == ' ') ++s2;
                int e2 = s2;
                while (e2 < lower.length() && juce::CharacterFunctions::isDigit (lower[e2])) ++e2;
                if (e2 > s2)
                {
                    const double v = lower.substring (s2, e2).getDoubleValue();
                    if (v >= 40 && v <= 250) return v;
                }
                at = lower.indexOf (at + 3, "bpm");
            }
            return 0;
        }
    }

    ParsedDescription parseDescription (const juce::String& raw)
    {
        ParsedDescription p;
        const auto text = normalise (raw);

        // ---- genre: the first one mentioned wins; one mentioned with "influence / touch / hint" is a flavour ----
        struct GW { const char* w; Genre g; };
        static const GW genreWords[] = {
            { "boom bap", Genre::boomBap }, { "boombap", Genre::boomBap }, { "hip hop", Genre::boomBap }, { "hiphop", Genre::boomBap },
            { "rap", Genre::boomBap }, { "golden era", Genre::boomBap }, { "old school", Genre::boomBap },
            { "trap", Genre::trap }, { "drill", Genre::trap }, { "rage", Genre::trap }, { "plugg", Genre::trap },
            { "r&b", Genre::rnb }, { "r b", Genre::rnb }, { "rnb", Genre::rnb }, { "soul", Genre::rnb }, { "neo soul", Genre::rnb }, { "funk", Genre::rnb },
            { "pop", Genre::pop }, { "synthpop", Genre::pop }, { "indie pop", Genre::pop },
            { "rock", Genre::rock }, { "punk", Genre::rock }, { "metal", Genre::rock }, { "grunge", Genre::rock }, { "alt", Genre::rock },
            { "alternative", Genre::rock }, { "indie rock", Genre::rock }, { "garage", Genre::rock },
            { "country", Genre::country }, { "americana", Genre::country }, { "bluegrass", Genre::country }, { "folk", Genre::country },
            { "nashville", Genre::country }, { "southern", Genre::country },
            { "worship", Genre::worship }, { "gospel", Genre::worship }, { "church", Genre::worship }, { "praise", Genre::worship },
            { "edm", Genre::edm }, { "house", Genre::edm }, { "techno", Genre::edm }, { "dance", Genre::edm }, { "dubstep", Genre::edm },
            { "future bass", Genre::edm }, { "club", Genre::edm }, { "trance", Genre::edm },
            { "lofi", Genre::lofi }, { "lo fi", Genre::lofi }, { "chillhop", Genre::lofi }, { "chill hop", Genre::lofi }, { "study", Genre::lofi },
            { "cinematic", Genre::cinematic }, { "trailer", Genre::cinematic }, { "epic", Genre::cinematic }, { "film", Genre::cinematic },
            { "score", Genre::cinematic }, { "soundtrack", Genre::cinematic },
        };

        struct Match { int at; Genre g; bool flavour; };
        std::vector<Match> matches;
        for (auto& gw : genreWords)
        {
            int at = text.indexOf (" " + juce::String (gw.w) + " ");
            while (at >= 0)
            {
                // a flavour? ("with a hip-hop influence", "a touch of trap")
                const auto after = text.substring (at, at + 40);
                const auto before = text.substring (juce::jmax (0, at - 24), at);
                const bool flavour = after.contains ("influence") || after.contains ("flavor") || after.contains ("flavour")
                                  || before.contains ("touch of") || before.contains ("hint of") || before.contains ("little")
                                  || before.contains ("bit of") || after.contains ("elements");
                matches.push_back ({ at, gw.g, flavour });
                at = text.indexOf (at + 1, " " + juce::String (gw.w) + " ");
            }
        }
        std::sort (matches.begin(), matches.end(), [] (const Match& a, const Match& b) { return a.at < b.at; });
        for (auto& m : matches)
            if (! m.flavour) { p.genre = m.g; p.hasGenre = true; break; }
        if (! p.hasGenre && ! matches.empty()) { p.genre = matches.front().g; p.hasGenre = true; }
        for (auto& m : matches)                                   // a flavour first, then any other genre named
            if (m.flavour && m.g != p.genre) { p.secondaryGenre = (int) m.g; break; }
        if (p.secondaryGenre < 0)
            for (auto& m : matches)
                if (m.g != p.genre) { p.secondaryGenre = (int) m.g; break; }
        if (p.secondaryGenre >= 0) p.understood.add ("+ a little " + genreName ((Genre) p.secondaryGenre));
        if (p.hasGenre) p.understood.insert (0, genreName (p.genre));
        if (p.secondaryGenre == (int) p.genre) p.secondaryGenre = -1;

        // ---- character words ----
        enum { PUNCH, WARM, BRIGHT, ROOM, VINT, AGGR, ORG };
        struct CW { const char* w; int trait; float amount; };
        static const CW charWords[] = {
            { "punchy", PUNCH, 0.2f }, { "punch", PUNCH, 0.18f }, { "knock", PUNCH, 0.2f }, { "knocking", PUNCH, 0.22f }, { "slap", PUNCH, 0.18f },
            { "slapping", PUNCH, 0.2f }, { "hard hitting", PUNCH, 0.22f }, { "snappy", PUNCH, 0.15f }, { "tight", PUNCH, 0.12f },
            { "soft", PUNCH, -0.2f }, { "gentle", PUNCH, -0.18f }, { "laid back", PUNCH, -0.15f }, { "mellow", PUNCH, -0.15f },
            { "warm", WARM, 0.2f }, { "fat", WARM, 0.15f }, { "thick", WARM, 0.15f }, { "round", WARM, 0.12f }, { "cold", WARM, -0.18f }, { "thin", WARM, -0.18f },
            { "dark", BRIGHT, -0.22f }, { "muffled", BRIGHT, -0.25f }, { "moody", BRIGHT, -0.12f }, { "bright", BRIGHT, 0.22f },
            { "crisp", BRIGHT, 0.18f }, { "shiny", BRIGHT, 0.18f }, { "airy", BRIGHT, 0.15f }, { "sparkly", BRIGHT, 0.18f },
            { "big", ROOM, 0.15f }, { "huge", ROOM, 0.22f }, { "massive", ROOM, 0.22f }, { "arena", ROOM, 0.28f }, { "stadium", ROOM, 0.28f },
            { "roomy", ROOM, 0.25f }, { "ambient", ROOM, 0.2f }, { "spacious", ROOM, 0.2f }, { "reverb", ROOM, 0.15f }, { "anthemic", ROOM, 0.2f },
            { "dry", ROOM, -0.22f }, { "intimate", ROOM, -0.15f }, { "close", ROOM, -0.12f },
            { "vintage", VINT, 0.25f }, { "vinyl", VINT, 0.28f }, { "dusty", VINT, 0.3f }, { "lofi", VINT, 0.25f }, { "lo fi", VINT, 0.25f },
            { "tape", VINT, 0.2f }, { "retro", VINT, 0.22f }, { "old school", VINT, 0.25f }, { "classic", VINT, 0.15f }, { "gritty", VINT, 0.18f },
            { "grimy", VINT, 0.2f }, { "crunchy", VINT, 0.18f }, { "70s", VINT, 0.3f }, { "80s", VINT, 0.22f }, { "90s", VINT, 0.25f },
            { "modern", VINT, -0.25f }, { "clean", VINT, -0.22f }, { "polished", VINT, -0.2f }, { "pristine", VINT, -0.25f }, { "glossy", VINT, -0.2f },
            { "aggressive", AGGR, 0.25f }, { "hard", AGGR, 0.18f }, { "heavy", AGGR, 0.22f }, { "angry", AGGR, 0.25f }, { "dirty", AGGR, 0.18f },
            { "distorted", AGGR, 0.25f }, { "loud", AGGR, 0.15f }, { "driving", AGGR, 0.12f }, { "energetic", AGGR, 0.15f },
            { "chill", AGGR, -0.2f }, { "smooth", AGGR, -0.2f }, { "relaxed", AGGR, -0.2f }, { "calm", AGGR, -0.2f }, { "dreamy", AGGR, -0.18f },
            { "live", ORG, 0.25f }, { "acoustic", ORG, 0.3f }, { "organic", ORG, 0.28f }, { "real drums", ORG, 0.3f }, { "natural", ORG, 0.2f },
            { "jazzy", ORG, 0.2f }, { "electronic", ORG, -0.28f }, { "programmed", ORG, -0.25f }, { "synthetic", ORG, -0.25f }, { "drum machine", ORG, -0.3f },
        };
        for (auto& cw : charWords)
            if (has (text, cw.w))
            {
                p.nudge[(size_t) cw.trait] = juce::jlimit (-0.45f, 0.45f, p.nudge[(size_t) cw.trait] + cw.amount);
                p.understood.addIfNotAlreadyThere (cw.w);
            }

        // "punchy but warm" etc. are already covered; "not too bright" flips the next word
        for (auto* neg : { "not too ", "not so ", "less ", "no " })
        {
            int at = text.indexOf (neg);
            while (at >= 0)
            {
                const auto word = text.substring (at + juce::String (neg).length()).upToFirstOccurrenceOf (" ", false, false);
                for (auto& cw : charWords)
                    if (word == cw.w)
                        p.nudge[(size_t) cw.trait] -= 2.0f * cw.amount;
                at = text.indexOf (at + 1, neg);
            }
        }
        for (auto& n : p.nudge) n = juce::jlimit (-0.45f, 0.45f, n);

        // ---- feel ----
        if (has (text, "hybrid"))                                                 p.feel = 2;
        else if (has (text, "live") || has (text, "acoustic") || has (text, "real drums") || has (text, "band"))  p.feel = 1;
        else if (has (text, "programmed") || has (text, "drum machine") || has (text, "electronic"))              p.feel = 3;

        // ---- options ----
        if (has (text, "no 808") || has (text, "without 808") || has (text, "no 808s") || has (text, "kick only"))   p.lowEnd = 0;
        else if (has (text, "808") || has (text, "808s") || has (text, "sub") || has (text, "slides"))                p.lowEnd = 1;
        if (has (text, "fx") || has (text, "effects") || has (text, "risers") || has (text, "riser") || has (text, "sound effects")
             || has (text, "impacts") || has (text, "transitions"))
            p.fx = 1;
        if (has (text, "no fx") || has (text, "no effects")) p.fx = 0;
        if (has (text, "lots of percussion") || has (text, "percussive") || has (text, "latin") || has (text, "afro")) p.percCount = 3;
        if (has (text, "no percussion") || has (text, "no perc") || has (text, "minimal"))                            p.percCount = 0;

        p.bpm = findBpm (raw);
        p.key = findKeyInText (raw);
        if (p.bpm > 0) p.understood.add (juce::String (p.bpm, 0) + " BPM");
        if (p.key >= 0) p.understood.add (keyName (p.key));
        return p;
    }

    void applyDescription (const ParsedDescription& p, KitOptions& o)
    {
        if (p.hasGenre) o.genre = p.genre;
        o.secondaryGenre = p.secondaryGenre;
        const auto center = genreCenter (o.genre);
        for (int i = 0; i < Traits::num; ++i)
            o.character[i] = juce::jlimit (0.0f, 1.0f, center[i] + p.nudge[(size_t) i]);
        o.feel = p.feel;
        o.lowEnd = p.lowEnd;
        if (p.fx >= 0) o.fx = p.fx == 1;
        o.percCount = p.percCount;
        if (p.bpm > 0) o.bpm = p.bpm;
        if (p.key >= 0) o.key = p.key;
    }

    //==============================================================================
    int keyFromChords (const juce::StringArray& chords)
    {
        struct C { int root; bool minor; };
        std::vector<C> list;
        for (auto c : chords)
        {
            c = c.trim();
            if (c.isEmpty()) continue;
            const auto letter = juce::CharacterFunctions::toUpperCase (c[0]);
            if (letter < 'A' || letter > 'G') continue;
            static const int pcs[7] = { 9, 11, 0, 2, 4, 5, 7 };
            int pc = pcs[letter - 'A'];
            int i = 1;
            if (c.length() > 1 && (c[1] == '#' || c[1] == 0x266F)) { ++pc; ++i; }
            else if (c.length() > 1 && (c[1] == 'b' || c[1] == 0x266D)) { --pc; ++i; }
            const auto rest = c.substring (i);
            const bool minor = (rest.startsWith ("m") && ! rest.startsWith ("maj")) || rest.startsWith ("dim");
            list.push_back ({ (pc + 12) % 12, minor });
        }
        if (list.empty()) return -1;

        int bestKey = -1;
        float bestScore = -1;
        for (int key = 0; key < 24; ++key)
        {
            const int tonic = key % 12;
            const bool minorKey = key >= 12;
            // diatonic triads: (interval from tonic, minor?)
            static const int majI[6] = { 0, 2, 4, 5, 7, 9 };      static const bool majM[6] = { false, true, true, false, false, true };
            static const int minI[7] = { 0, 3, 5, 7, 7, 8, 10 };  static const bool minM[7] = { true, false, true, true, false, false, false };
            float score = 0;
            for (size_t n = 0; n < list.size(); ++n)
            {
                const int iv = (list[n].root - tonic + 12) % 12;
                bool fits = false;
                if (! minorKey) { for (int k = 0; k < 6; ++k) if (majI[k] == iv && majM[k] == list[n].minor) fits = true; }
                else            { for (int k = 0; k < 7; ++k) if (minI[k] == iv && minM[k] == list[n].minor) fits = true; }
                if (fits) score += 2.0f;
                const bool isTonic = iv == 0 && list[n].minor == minorKey;
                if (isTonic && n == 0) score += 1.6f;
                if (isTonic && n + 1 == list.size()) score += 0.8f;
                if (isTonic) score += 0.4f;
            }
            if (score > bestScore) { bestScore = score; bestKey = key; }
        }
        return bestKey;
    }

    std::optional<KeyCompassSong> readKeyCompassSong (const juce::File& f)
    {
        auto xml = juce::XmlDocument::parse (f);
        if (xml == nullptr || ! xml->hasTagName ("KeyCompassSong") || xml->getBoolAttribute ("deleted", false))
            return std::nullopt;
        auto* map = xml->getChildByName ("SongMap");
        if (map == nullptr) return std::nullopt;

        KeyCompassSong s;
        s.file = f;
        s.name = xml->getStringAttribute ("name", map->getStringAttribute ("name", f.getFileNameWithoutExtension()));
        s.modified = juce::Time (xml->getStringAttribute ("modified", "0").getLargeIntValue());
        if (s.modified.toMilliseconds() == 0) s.modified = f.getLastModificationTime();
        s.bpm = map->getDoubleAttribute ("bpm", 0.0);
        s.key = map->getIntAttribute ("keyOverride", -1);
        if (s.key < 0) s.key = map->getIntAttribute ("lastSetKey", -1);

        juce::StringArray chords;
        for (auto* sec : map->getChildWithTagNameIterator ("Section"))
        {
            s.sections.add (sec->getStringAttribute ("name"));
            chords.addTokens (sec->getStringAttribute ("chords"), " ", "");
        }
        if (s.key < 0 || s.key > 23)
            s.key = keyFromChords (chords);
        return s;
    }

    std::vector<KeyCompassSong> listKeyCompassSongs (const juce::File& folder, int maxSongs)
    {
        std::vector<KeyCompassSong> out;
        if (! folder.isDirectory()) return out;
        for (const auto& entry : juce::RangedDirectoryIterator (folder, false, "*.kcsong", juce::File::findFiles))
            if (auto s = readKeyCompassSong (entry.getFile()))
                out.push_back (*s);
        std::sort (out.begin(), out.end(), [] (auto& a, auto& b) { return a.modified > b.modified; });
        if ((int) out.size() > maxSongs) out.resize ((size_t) maxSongs);
        return out;
    }
}
