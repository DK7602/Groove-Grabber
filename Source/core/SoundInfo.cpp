#include "SoundInfo.h"
#include <cmath>

namespace gg
{
    float& Traits::operator[] (int i) noexcept
    {
        switch (i)
        {
            case 0: return punch;   case 1: return warmth;  case 2: return bright;
            case 3: return room;    case 4: return vintage; case 5: return aggr;
            default: return organic;
        }
    }

    float Traits::operator[] (int i) const noexcept   { return const_cast<Traits&> (*this)[i]; }

    juce::String Traits::name (int i)
    {
        static const char* n[] = { "Punch", "Warmth", "Brightness", "Room", "Vintage", "Aggression", "Live Feel" };
        return n[juce::jlimit (0, num - 1, i)];
    }

    juce::var Traits::toVar() const
    {
        juce::Array<juce::var> a;
        for (int i = 0; i < num; ++i) a.add ((double) (*this)[i]);
        return a;
    }

    Traits Traits::fromVar (const juce::var& v)
    {
        Traits t;
        if (auto* a = v.getArray())
            for (int i = 0; i < juce::jmin (num, a->size()); ++i)
                t[i] = juce::jlimit (0.0f, 1.0f, (float) (double) a->getReference (i));
        return t;
    }

    juce::String sourceName (Source s)
    {
        switch (s)
        {
            case Source::packs:         return "Drum Packs";
            case Source::oneShots:      return "One Shots";
            case Source::recordings:    return "My Recordings";
            case Source::sampleSnagger: return "Sample Snagger";
            case Source::harvested:     return "Harvested";
            case Source::count:         break;
        }
        return {};
    }

    float SoundInfo::midiPitch() const noexcept
    {
        return pitchHz > 0 ? 69.0f + 12.0f * std::log2 (pitchHz / 440.0f) : 0.0f;
    }

    //==============================================================================
    void SoundInfo::write (juce::OutputStream& o) const
    {
        o.writeString (path);
        o.writeString (pack);
        o.writeByte ((char) source);
        o.writeInt64 (fileSize);
        o.writeInt64 (modTime);
        o.writeByte ((char) cat);
        o.writeFloat (catConfidence);
        o.writeBool (catByUser);
        o.writeByte ((char) percKind);
        o.writeInt ((int) genreTags);
        for (float f : { duration, (float) sampleRate, peakDb, loudDb, leadSilenceMs, attackMs, decayMs,
                         centroidHz, rolloffHz, flatness, subShare, lowShare, midShare, highShare,
                         pitchHz, pitchConf, crestDb, tailFloorDb })
            o.writeFloat (f);
        o.writeShort ((short) channels);
        o.writeShort ((short) onsets);
        o.writeShort ((short) burstCount);
        o.writeBool (clipped);
        for (int i = 0; i < Traits::num; ++i)
            o.writeFloat (traits[i]);
        o.writeByte ((char) rating);
        o.writeBool (hidden);
        // version 4
        o.writeFloat (airFlatness);
        o.writeFloat (ringMs);
        o.writeFloat (pitchGlide);
        o.writeByte ((char) measureVersion);
    }

    bool SoundInfo::read (juce::InputStream& in, int version)
    {
        path = in.readString();
        pack = in.readString();
        source = (Source) juce::jlimit (0, (int) Source::count - 1, (int) in.readByte());
        fileSize = in.readInt64();
        modTime = in.readInt64();
        cat = (Cat) juce::jlimit (0, numCats - 1, (int) in.readByte());
        catConfidence = in.readFloat();
        catByUser = in.readBool();
        percKind = (PercKind) juce::jlimit (0, (int) PercKind::other, (int) in.readByte());
        genreTags = (juce::uint32) in.readInt();
        float* fs[] = { &duration, nullptr, &peakDb, &loudDb, &leadSilenceMs, &attackMs, &decayMs,
                        &centroidHz, &rolloffHz, &flatness, &subShare, &lowShare, &midShare, &highShare,
                        &pitchHz, &pitchConf, &crestDb, &tailFloorDb };
        for (auto* f : fs)
        {
            const float v = in.readFloat();
            if (f != nullptr) *f = v; else sampleRate = v;
        }
        channels = in.readShort();
        onsets = in.readShort();
        burstCount = in.readShort();
        clipped = in.readBool();
        for (int i = 0; i < Traits::num; ++i)
            traits[i] = in.readFloat();
        rating = (signed char) in.readByte();
        hidden = in.readBool();
        if (version >= 4)
        {
            airFlatness = in.readFloat();
            ringMs = in.readFloat();
            pitchGlide = in.readFloat();
            measureVersion = (int) in.readByte();
        }
        else
        {
            // measured by an older version: usable for now, measured again on the next scan
            ringMs = decayMs * 1.8f;
            measureVersion = 1;
        }
        return path.isNotEmpty();
    }

    //==============================================================================
    juce::uint32 genreTagsForPath (const juce::String& p)
    {
        struct Word { const char* w; Genre g; };
        static const Word words[] = {
            { "trap", Genre::trap }, { "drill", Genre::trap }, { "rage", Genre::trap }, { "plugg", Genre::trap },
            { "boom bap", Genre::boomBap }, { "boombap", Genre::boomBap }, { "boom-bap", Genre::boomBap },
            { "hip hop", Genre::boomBap }, { "hiphop", Genre::boomBap }, { "hip-hop", Genre::boomBap },
            { "90s", Genre::boomBap }, { "golden era", Genre::boomBap }, { "sp1200", Genre::boomBap }, { "sp-1200", Genre::boomBap },
            { "r&b", Genre::rnb }, { "rnb", Genre::rnb }, { "soul", Genre::rnb }, { "neo soul", Genre::rnb }, { "motown", Genre::rnb },
            { "pop", Genre::pop }, { "radio", Genre::pop },
            { "rock", Genre::rock }, { "metal", Genre::rock }, { "punk", Genre::rock }, { "grunge", Genre::rock }, { "indie", Genre::rock },
            { "country", Genre::country }, { "nashville", Genre::country }, { "americana", Genre::country },
            { "bluegrass", Genre::country }, { "folk", Genre::country },
            { "worship", Genre::worship }, { "gospel", Genre::worship }, { "church", Genre::worship },
            { "edm", Genre::edm }, { "house", Genre::edm }, { "techno", Genre::edm }, { "dance", Genre::edm },
            { "dubstep", Genre::edm }, { "future bass", Genre::edm }, { "trance", Genre::edm }, { "909", Genre::edm },
            { "lofi", Genre::lofi }, { "lo-fi", Genre::lofi }, { "lo fi", Genre::lofi }, { "chillhop", Genre::lofi }, { "vinyl", Genre::lofi },
            { "cinematic", Genre::cinematic }, { "trailer", Genre::cinematic }, { "epic", Genre::cinematic }, { "score", Genre::cinematic },
        };

        juce::uint32 tags = 0;
        for (const auto& w : words)
        {
            const juce::String word (w.w);
            int from = 0;
            for (;;)
            {
                const int at = p.indexOf (from, word);
                if (at < 0) break;
                const bool startOk = at == 0 || ! juce::CharacterFunctions::isLetter (p[at - 1]);
                const int end = at + word.length();
                const bool endOk = end >= p.length() || ! juce::CharacterFunctions::isLetter (p[end]);
                if (startOk && endOk) { tags |= 1u << (int) w.g; break; }
                from = at + 1;
            }
        }
        return tags;
    }
}
