#include "SoundTypes.h"

namespace gg
{
    juce::String catName (Cat c)
    {
        switch (c)
        {
            case Cat::kick:      return "Kick";
            case Cat::bass808:   return "808";
            case Cat::snare:     return "Snare";
            case Cat::clap:      return "Clap";
            case Cat::rim:       return "Rim";
            case Cat::closedHat: return "Closed Hat";
            case Cat::openHat:   return "Open Hat";
            case Cat::tom:       return "Tom";
            case Cat::crash:     return "Crash";
            case Cat::ride:      return "Ride";
            case Cat::perc:      return "Perc";
            case Cat::fx:        return "FX";
            case Cat::loop:      return "Loop";
            case Cat::melodic:   return "Vocal / Melodic";
            case Cat::unknown:
            case Cat::count:     break;
        }
        return "Other";
    }

    juce::String catId (Cat c)
    {
        switch (c)
        {
            case Cat::kick:      return "kick";
            case Cat::bass808:   return "808";
            case Cat::snare:     return "snare";
            case Cat::clap:      return "clap";
            case Cat::rim:       return "rim";
            case Cat::closedHat: return "chat";
            case Cat::openHat:   return "ohat";
            case Cat::tom:       return "tom";
            case Cat::crash:     return "crash";
            case Cat::ride:      return "ride";
            case Cat::perc:      return "perc";
            case Cat::fx:        return "fx";
            case Cat::loop:      return "loop";
            case Cat::melodic:   return "melodic";
            case Cat::unknown:
            case Cat::count:     break;
        }
        return "other";
    }

    Cat catFromId (const juce::String& s)
    {
        for (int i = 0; i < numCats; ++i)
            if (catId ((Cat) i) == s)
                return (Cat) i;
        return Cat::unknown;
    }

    bool catIsKitSound (Cat c) noexcept    { return c != Cat::loop && c != Cat::unknown && c != Cat::melodic && c != Cat::count; }

    juce::String percKindName (PercKind k)
    {
        switch (k)
        {
            case PercKind::shaker:     return "Shaker";
            case PercKind::tambourine: return "Tambourine";
            case PercKind::conga:      return "Conga / Bongo";
            case PercKind::cowbell:    return "Cowbell";
            case PercKind::snap:       return "Snap";
            case PercKind::block:      return "Block / Clave";
            case PercKind::triangle:   return "Triangle";
            case PercKind::other:      return "Perc";
            case PercKind::none:       break;
        }
        return {};
    }

    //==============================================================================
    juce::String genreName (Genre g)
    {
        switch (g)
        {
            case Genre::trap:      return "Trap / Drill";
            case Genre::boomBap:   return "Hip-Hop";
            case Genre::rnb:       return "R&B / Soul";
            case Genre::pop:       return "Pop";
            case Genre::rock:      return "Rock";
            case Genre::country:   return "Country";
            case Genre::worship:   return "Worship / Gospel";
            case Genre::edm:       return "EDM / Dance";
            case Genre::lofi:      return "Lo-Fi";
            case Genre::cinematic: return "Cinematic";
            case Genre::count:     break;
        }
        return {};
    }

    juce::String genreId (Genre g)
    {
        switch (g)
        {
            case Genre::trap:      return "trap";
            case Genre::boomBap:   return "boombap";
            case Genre::rnb:       return "rnb";
            case Genre::pop:       return "pop";
            case Genre::rock:      return "rock";
            case Genre::country:   return "country";
            case Genre::worship:   return "worship";
            case Genre::edm:       return "edm";
            case Genre::lofi:      return "lofi";
            case Genre::cinematic: return "cinematic";
            case Genre::count:     break;
        }
        return {};
    }

    Genre genreFromId (const juce::String& s, Genre fallback)
    {
        for (int i = 0; i < numGenres; ++i)
            if (genreId ((Genre) i) == s)
                return (Genre) i;
        return fallback;
    }

    //==============================================================================
    juce::String slotName (Slot s)
    {
        switch (s)
        {
            case Slot::kick:      return "Kick";
            case Slot::bass808:   return "808";
            case Slot::snare:     return "Snare";
            case Slot::clap:      return "Clap";
            case Slot::rim:       return "Rim";
            case Slot::closedHat: return "Closed Hat";
            case Slot::openHat:   return "Open Hat";
            case Slot::crash:     return "Crash";
            case Slot::ride:      return "Ride";
            case Slot::tomLow:    return "Tom Low";
            case Slot::tomMid:    return "Tom Mid";
            case Slot::tomHigh:   return "Tom High";
            case Slot::perc1:     return "Perc 1";
            case Slot::perc2:     return "Perc 2";
            case Slot::perc3:     return "Perc 3";
            case Slot::fx1:       return "FX 1";
            case Slot::fx2:       return "FX 2";
            case Slot::count:     break;
        }
        return {};
    }

    juce::String slotId (Slot s)
    {
        return slotName (s).toLowerCase().removeCharacters (" ");
    }

    Slot slotFromId (const juce::String& id)
    {
        for (int i = 0; i < numSlots; ++i)
            if (slotId ((Slot) i) == id)
                return (Slot) i;
        return Slot::count;
    }

    Cat slotCat (Slot s) noexcept
    {
        switch (s)
        {
            case Slot::kick:      return Cat::kick;
            case Slot::bass808:   return Cat::bass808;
            case Slot::snare:     return Cat::snare;
            case Slot::clap:      return Cat::clap;
            case Slot::rim:       return Cat::rim;
            case Slot::closedHat: return Cat::closedHat;
            case Slot::openHat:   return Cat::openHat;
            case Slot::crash:     return Cat::crash;
            case Slot::ride:      return Cat::ride;
            case Slot::tomLow:
            case Slot::tomMid:
            case Slot::tomHigh:   return Cat::tom;
            case Slot::perc1:
            case Slot::perc2:
            case Slot::perc3:     return Cat::perc;
            case Slot::fx1:
            case Slot::fx2:       return Cat::fx;
            case Slot::count:     break;
        }
        return Cat::unknown;
    }

    bool slotIsOptional (Slot s) noexcept
    {
        return s == Slot::bass808 || s == Slot::perc1 || s == Slot::perc2 || s == Slot::perc3
            || s == Slot::fx1 || s == Slot::fx2;
    }

    //==============================================================================
    const std::array<PadDef, numPads>& padLayout()
    {
        static const std::array<PadDef, numPads> pads {{
            { 36, Slot::kick,      0.0f,  0.0f, 1.0f,  "Kick",        0 },
            { 37, Slot::rim,       0.0f,  0.0f, 1.0f,  "Rim",         0 },
            { 38, Slot::snare,     0.0f,  0.0f, 1.0f,  "Snare",       0 },
            { 39, Slot::clap,      0.0f,  0.0f, 1.0f,  "Clap",        0 },
            { 40, Slot::snare,     0.0f, -1.5f, 1.0f,  "Snare 2",     0 },
            { 41, Slot::tomLow,   -3.0f,  0.0f, 1.0f,  "Floor Tom 2", 0 },
            { 42, Slot::closedHat, 0.0f,  0.0f, 1.0f,  "Closed Hat",  1 },
            { 43, Slot::tomLow,    0.0f,  0.0f, 1.0f,  "Floor Tom",   0 },
            { 44, Slot::closedHat, 0.0f, -5.0f, 0.45f, "Pedal Hat",   1 },
            { 45, Slot::tomMid,   -2.0f,  0.0f, 1.0f,  "Low Tom",     0 },
            { 46, Slot::openHat,   0.0f,  0.0f, 1.0f,  "Open Hat",    1 },
            { 47, Slot::tomMid,    0.0f,  0.0f, 1.0f,  "Mid Tom",     0 },
            { 48, Slot::tomHigh,  -2.0f,  0.0f, 1.0f,  "Hi-Mid Tom",  0 },
            { 49, Slot::crash,     0.0f,  0.0f, 1.0f,  "Crash",       0 },
            { 50, Slot::tomHigh,   0.0f,  0.0f, 1.0f,  "High Tom",    0 },
            { 51, Slot::ride,      0.0f,  0.0f, 1.0f,  "Ride",        0 },
            { 52, Slot::bass808,   0.0f,  0.0f, 1.0f,  "808",         0 },
            { 53, Slot::perc1,     0.0f,  0.0f, 1.0f,  "Perc 1",      0 },
            { 54, Slot::perc2,     0.0f,  0.0f, 1.0f,  "Perc 2",      0 },
            { 55, Slot::perc3,     0.0f,  0.0f, 1.0f,  "Perc 3",      0 },
            { 56, Slot::fx1,       0.0f,  0.0f, 1.0f,  "FX 1",        0 },
            { 57, Slot::fx2,       0.0f,  0.0f, 1.0f,  "FX 2",        0 },
        }};
        return pads;
    }

    const PadDef* padForNote (int note) noexcept
    {
        const int i = note - firstPadNote;
        return i >= 0 && i < numPads ? &padLayout()[(size_t) i] : nullptr;
    }

    int mainNoteForSlot (Slot s) noexcept
    {
        // the "real" pad (no extra tuning / gain), e.g. 43 for the low tom rather than 41
        int best = -1;
        for (const auto& p : padLayout())
            if (p.slot == s)
            {
                if (best < 0) best = p.note;
                if (p.tuneOffset == 0.0f && p.gainDb == 0.0f && p.lengthScale == 1.0f)
                    return p.note;
            }
        return best;
    }

    //==============================================================================
    juce::String pitchClassName (int pc)
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return names[((pc % 12) + 12) % 12];
    }

    juce::String keyName (int key)
    {
        if (key < 0 || key > 23) return "No key";
        return pitchClassName (key % 12) + (key >= 12 ? " Minor" : " Major");
    }

    juce::String keyShortName (int key)
    {
        if (key < 0 || key > 23) return "-";
        return pitchClassName (key % 12) + (key >= 12 ? "m" : "");
    }

    juce::String noteName (int midiNote)
    {
        // Groove Agent / Cubase convention: note 36 = C1
        return pitchClassName (midiNote % 12) + juce::String (midiNote / 12 - 2);
    }

    int parseKey (const juce::String& text)
    {
        auto s = text.trim();
        if (s.isEmpty()) return -1;

        const juce::juce_wchar letter = juce::CharacterFunctions::toUpperCase (s[0]);
        static const int letterPc[7] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
        if (letter < 'A' || letter > 'G') return -1;
        int pc = letterPc[letter - 'A'];
        int i = 1;

        auto rest = s.substring (i);
        if (rest.startsWith ("#") || rest.startsWith (juce::CharPointer_UTF8 ("\xe2\x99\xaf")))   { pc += 1; rest = rest.substring (1); }
        else if (rest.startsWithIgnoreCase ("sharp"))                                        { pc += 1; rest = rest.substring (5); }
        else if (rest.startsWith ("b") && ! rest.startsWithIgnoreCase ("bm"))                    { pc -= 1; rest = rest.substring (1); }
        else if (rest.startsWith (juce::CharPointer_UTF8 ("\xe2\x99\xad")))                    { pc -= 1; rest = rest.substring (1); }
        else if (rest.startsWithIgnoreCase ("flat"))                                         { pc -= 1; rest = rest.substring (4); }
        else if (rest.startsWith ("bm"))                                                     { pc -= 1; rest = rest.substring (1); }

        pc = (pc + 12) % 12;
        rest = rest.trim().toLowerCase();

        bool minor = false;
        if (rest.startsWith ("maj") || rest.startsWith ("major")) minor = false;
        else if (rest.startsWith ("m")) minor = true;           // m, min, minor
        else if (rest.isNotEmpty() && juce::CharacterFunctions::isLetter (rest[0])) return -1;

        return pc + (minor ? 12 : 0);
    }
}
