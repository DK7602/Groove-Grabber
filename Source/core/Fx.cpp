#include "Fx.h"
#include "Analyzer.h"
#include "Settings.h"
#include "GGBinaryData.h"
#include <cmath>

namespace gg::fx
{
    //==============================================================================
    namespace
    {
        struct TypeDef { Type type; const char* id; const char* name; Group group; Anchor anchor; };
        const TypeDef defs[] = {
            { Type::riser,         "riser",         "Riser",              Group::buildUp, Anchor::end },
            { Type::reverseCymbal, "reverseCymbal", "Reverse cymbal",     Group::buildUp, Anchor::end },
            { Type::swell,         "swell",         "Swell",              Group::buildUp, Anchor::end },
            { Type::impact,        "impact",        "Impact",             Group::hit,     Anchor::start },
            { Type::subDrop,       "subDrop",       "Sub drop / boom",    Group::hit,     Anchor::start },
            { Type::downlifter,    "downlifter",    "Downlifter",         Group::move,    Anchor::start },
            { Type::sweep,         "sweep",         "Sweep",              Group::move,    Anchor::end },
            { Type::whoosh,        "whoosh",        "Whoosh",             Group::move,    Anchor::peak },
            { Type::tapeStop,      "tapeStop",      "Tape stop",          Group::move,    Anchor::end },
            { Type::rewind,        "rewind",        "Rewind",             Group::move,    Anchor::end },
            { Type::crackle,       "crackle",       "Vinyl crackle",      Group::texture, Anchor::start },
            { Type::noise,         "noise",         "Noise bed",          Group::texture, Anchor::start },
            { Type::scratch,       "scratch",       "Scratch",            Group::accent,  Anchor::start },
            { Type::zap,           "zap",           "Zap",                Group::accent,  Anchor::start },
            { Type::laser,         "laser",         "Laser",              Group::accent,  Anchor::start },
            { Type::blip,          "blip",          "Blip",               Group::accent,  Anchor::start },
            { Type::glitch,        "glitch",        "Glitch",             Group::accent,  Anchor::start },
            { Type::horn,          "horn",          "Air horn / siren",   Group::accent,  Anchor::start },
            { Type::other,         "other",         "Other FX",           Group::other,   Anchor::start },
        };
        static_assert (sizeof (defs) / sizeof (defs[0]) == (size_t) numTypes, "one definition per type");

        const TypeDef& def (Type t)
        {
            return defs[(size_t) juce::jlimit (0, numTypes - 1, (int) t)];
        }
    }

    juce::String typeName (Type t)     { return def (t).name; }
    juce::String typeId (Type t)       { return def (t).id; }
    Group groupOf (Type t)             { return def (t).group; }
    Anchor anchorOf (Type t)           { return def (t).anchor; }

    Type typeFromId (const juce::String& id)
    {
        for (const auto& d : defs) if (id == d.id) return d.type;
        return Type::count;
    }

    juce::String groupName (Group g)
    {
        switch (g)
        {
            case Group::buildUp: return "Build-ups";
            case Group::hit:     return "Hits";
            case Group::move:    return "Falls and moves";
            case Group::texture: return "Textures";
            case Group::accent:  return "Accents";
            default:             return "Other";
        }
    }

    //==============================================================================
    // the name
    namespace
    {
        struct Words
        {
            juce::StringArray tokens;
            juce::String joined, compact;
            bool has (std::initializer_list<const char*> ws) const
            {
                for (auto* w : ws) if (tokens.contains (w)) return true;
                return false;
            }
            bool phrase (std::initializer_list<const char*> ps) const
            {
                for (auto* p : ps) if (joined.contains (juce::String (" ") + p + " ") || compact.contains (juce::String (p).removeCharacters (" "))) return true;
                return false;
            }
        };

        Words words (const juce::String& text)
        {
            Words w;
            juce::String clean;
            const auto lower = text.toLowerCase();
            for (int i = 0; i < lower.length(); ++i)
            {
                const auto c = lower[i];
                clean << (juce::CharacterFunctions::isLetterOrDigit (c) ? juce::String::charToString (c) : juce::String (" "));
            }
            w.tokens = juce::StringArray::fromTokens (clean, " ", "");
            w.tokens.removeEmptyStrings();
            // plurals: "risers" is a riser
            const int n = w.tokens.size();
            for (int i = 0; i < n; ++i)
                if (w.tokens[i].length() > 3 && w.tokens[i].endsWithChar ('s')) w.tokens.add (w.tokens[i].dropLastCharacters (1));
            w.joined = " " + w.tokens.joinIntoString (" ") + " ";
            w.compact = clean.removeCharacters (" ");
            return w;
        }

        Type fromWords (const Words& w)
        {
            if (w.phrase ({ "tape stop", "vinyl stop", "record stop", "turntable stop", "power down", "slow down", "tapestop" })
                 || (w.has ({ "stop", "stops" }) && w.has ({ "tape", "vinyl", "record", "turntable" })) || w.has ({ "powerdown", "brake" }))
                return Type::tapeStop;
            if (w.has ({ "rewind", "rewinds", "spinback", "backspin", "rewinder", "pullup" }) || w.phrase ({ "spin back", "pull up" }))
                return Type::rewind;
            const bool reversed = w.has ({ "reverse", "reversed", "rev", "rvs", "reverso", "backwards" });
            if ((reversed && w.has ({ "cymbal", "cym", "cymb", "crash", "ride", "splash", "china" }))
                 || w.has ({ "revcym", "reversecymbal", "revcrash", "reversecrash" }))
                return Type::reverseCymbal;
            if (w.has ({ "swell", "swells" }) || (reversed && w.has ({ "snare", "clap", "hit", "kick", "tom", "piano", "vox", "vocal", "perc" })))
                return Type::swell;
            if (w.has ({ "subdrop", "boom", "booms", "bassdrop", "808drop" }) || w.phrase ({ "sub drop", "bass drop", "sub boom", "808 drop" }))
                return Type::subDrop;
            if (w.has ({ "downlifter", "downlift", "downsweep", "faller", "falldown" }) || w.phrase ({ "down lifter", "down sweep", "sweep down" }))
                return Type::downlifter;
            if (w.has ({ "riser", "rise", "rising", "uplifter", "uplift", "buildup", "build", "upsweep" }) || w.phrase ({ "build up", "up lifter", "sweep up" }))
                return Type::riser;
            if (w.has ({ "sweep", "sweeper" }))
                return Type::sweep;
            if (w.has ({ "whoosh", "woosh", "swoosh", "swish", "swoop", "flyby", "passby", "whoosh" }) || w.phrase ({ "fly by", "pass by" }))
                return Type::whoosh;
            if (w.has ({ "impact", "slam", "smash", "thud", "hitfx", "stinger" }) || w.phrase ({ "cinematic hit", "fx hit" }))
                return Type::impact;
            if (w.has ({ "crackle", "crackling", "vinyl", "dust", "pops" }) || w.phrase ({ "record noise" }))
                return Type::crackle;
            if (w.has ({ "scratch", "scratche", "scratching", "scr", "chirp", "transformer", "scribble" }))
                return Type::scratch;
            if (w.has ({ "laser", "pew", "pewpew" }))
                return Type::laser;
            if (w.has ({ "zap", "zapper", "spark", "electric", "shock" }))
                return Type::zap;
            if (w.has ({ "blip", "bleep", "bloop", "beep", "ping", "ui", "notification" }))
                return Type::blip;
            if (w.has ({ "glitch", "glitche", "glitchy", "stutter", "granular", "bitcrush", "bitcrushed", "buzz", "error" }))
                return Type::glitch;
            if (w.has ({ "airhorn", "siren", "alarm", "foghorn", "horn" }) || w.phrase ({ "air horn", "dub siren" }))
                return Type::horn;
            if (w.has ({ "noise", "hiss", "static", "texture", "atmos", "atmosphere", "ambience", "ambient", "drone", "roomtone",
                         "wind", "rain", "foley", "bed" }) || w.phrase ({ "room tone", "white noise" }))
                return Type::noise;
            if (w.has ({ "impact", "hit", "hits" }))
                return Type::impact;
            return Type::count;
        }
    }

    Type fromName (const juce::String& fileName, const juce::String& folders)
    {
        // the file's own name first, then its folders ("Risers/Long 01.wav")
        const auto t = fromWords (words (fileName));
        if (t != Type::count) return t;
        auto f = fromWords (words (folders));
        return f == Type::impact && words (folders).has ({ "hit", "hits" }) ? Type::count : f;   // ("One Shot Hits" says nothing)
    }

    //==============================================================================
    // the sound
    Shape measureShape (const float* x, int n, double sr)
    {
        Shape s;
        if (x == nullptr || n < 64 || sr <= 0) return s;
        const int hop = juce::jmax (16, (int) (sr * 0.02));
        const int frames = n / hop;
        if (frames < 2) return s;

        std::vector<float> db ((size_t) frames), bright ((size_t) frames), crest ((size_t) frames);
        float peakDb = -200;
        int peakFrame = 0;
        for (int f = 0; f < frames; ++f)
        {
            const float* p = x + f * hop;
            double sum = 0; float pk = 0; int zc = 0;
            for (int i = 0; i < hop; ++i)
            {
                sum += (double) p[i] * p[i];
                pk = juce::jmax (pk, std::abs (p[i]));
                if (i > 0 && ((p[i - 1] < 0) != (p[i] < 0))) ++zc;
            }
            const float rms = (float) std::sqrt (sum / hop);
            db[(size_t) f] = juce::Decibels::gainToDecibels (rms, -200.0f);
            crest[(size_t) f] = rms > 0 ? juce::Decibels::gainToDecibels (pk / rms) : 0.0f;
            bright[(size_t) f] = (float) zc * (float) sr / (2.0f * (float) hop);
            if (db[(size_t) f] > peakDb) { peakDb = db[(size_t) f]; peakFrame = f; }
        }
        if (peakDb < -80) return s;

        // where it sounds: from the first frame within 30 dB of the peak to the last
        int first = 0, last = frames - 1;
        while (first < peakFrame && db[(size_t) first] < peakDb - 30) ++first;
        while (last > peakFrame && db[(size_t) last] < peakDb - 30) --last;
        const int len = juce::jmax (1, last - first + 1);
        const float secs = (float) len * hop / (float) sr;
        s.duration = secs;
        s.peakPos = len > 1 ? (float) (peakFrame - first) / (float) (len - 1) : 0.0f;

        auto meanDb = [&] (int a, int b)
        {
            double t = 0; int c = 0;
            for (int f = a; f <= b && f < frames; ++f) { t += db[(size_t) f]; ++c; }
            return c > 0 ? (float) (t / c) : peakDb;
        };
        const int sixth = juce::jmax (1, len / 6);
        s.startRelDb = meanDb (first, first + sixth - 1) - peakDb;
        s.endRelDb = meanDb (last - sixth + 1, last) - peakDb;

        // after the peak: how long it holds within 12 dB
        int hold = peakFrame;
        while (hold < last && db[(size_t) hold] > peakDb - 12) ++hold;
        s.holdPos = (float) (hold - peakFrame) / (float) len;

        // brightness: its trend over the loud part (within 25 dB of the peak)
        std::vector<int> loud;
        for (int f = first; f <= last; ++f) if (db[(size_t) f] > peakDb - 25) loud.push_back (f);
        if (loud.size() >= 3)
        {
            const size_t third = juce::jmax ((size_t) 1, loud.size() / 3);
            double a = 0, b = 0, all = 0;
            for (size_t i = 0; i < third; ++i) a += std::log2 (juce::jmax (20.0f, bright[(size_t) loud[i]]));
            for (size_t i = loud.size() - third; i < loud.size(); ++i) b += std::log2 (juce::jmax (20.0f, bright[(size_t) loud[i]]));
            for (int f : loud) all += std::log2 (juce::jmax (20.0f, bright[(size_t) f]));
            s.brightTrend = (float) ((b - a) / (double) third);
            s.brightHz = (float) std::pow (2.0, all / (double) loud.size());
        }

        // pitch, frame by frame (40 ms windows)
        const int win = juce::jmin (n, (int) (sr * 0.04));
        std::vector<float> pitch;
        std::vector<float> track;          // what wobbles: the pitch when it has one, else the brightness
        int pitched = 0, tried = 0;
        const int stride = juce::jmax (1, (int) loud.size() / 240);
        for (size_t i = 0; i < loud.size(); i += (size_t) stride)
        {
            const int start = juce::jmin (loud[i] * hop, n - win);
            if (start < 0) break;
            float conf = 0;
            const float hz = analysis::detectPitch (x + start, win, sr, 50.0f, 2500.0f, conf);
            ++tried;
            if (hz > 0 && conf >= 0.6f) { ++pitched; pitch.push_back (12.0f * std::log2 (hz / 440.0f) + 69.0f); }
        }
        s.pitchShare = tried > 0 ? (float) pitched / (float) tried : 0.0f;
        if (pitch.size() >= 3)
        {
            auto sorted = pitch;
            std::sort (sorted.begin(), sorted.end());
            s.pitchHz = 440.0f * std::pow (2.0f, (sorted[sorted.size() / 2] - 69.0f) / 12.0f);
            const size_t k = juce::jmax ((size_t) 1, pitch.size() / 5);
            float a = 0, b = 0;
            for (size_t i = 0; i < k; ++i) { a += pitch[i]; b += pitch[pitch.size() - 1 - i]; }
            s.pitchTrend = (b - a) / (float) k;
        }
        // (noise's brightness jitters from frame to frame: smoothed, and only big swings count)
        const bool byPitch = s.pitchShare > 0.45f;
        if (byPitch) track = pitch;
        else for (int f : loud) track.push_back (12.0f * std::log2 (juce::jmax (20.0f, bright[(size_t) f]) / 440.0f));
        if (track.size() >= 5)
        {
            auto raw = track;
            for (size_t i = 0; i < track.size(); ++i)
            {
                float t = 0; int c = 0;
                for (size_t k = (i >= 2 ? i - 2 : 0); k <= juce::jmin (track.size() - 1, i + 2); ++k) { t += raw[k]; ++c; }
                track[i] = t / (float) c;
            }
        }

        // wobble: the track turning back and forth
        if (track.size() >= 4 && secs > 0.1f)
        {
            const float swing = byPitch ? 2.5f : 6.0f;
            int turns = 0, dir = 0;
            float anchor = track[0];
            for (size_t i = 1; i < track.size(); ++i)
            {
                const float d = track[i] - anchor;
                if (std::abs (d) < swing) continue;
                const int nd = d > 0 ? 1 : -1;
                if (dir != 0 && nd != dir) ++turns;
                dir = nd;
                anchor = track[i];
            }
            s.wobble = (float) turns / secs;
        }

        // clicks (a sharp spike in a frame) and gaps (a drop-out, then back)
        int clicks = 0, gaps = 0;
        for (int f = first; f <= last; ++f)
        {
            if (crest[(size_t) f] > 16.0f && db[(size_t) f] > peakDb - 35) ++clicks;
            if (f > first && f < last && db[(size_t) f] < db[(size_t) f - 1] - 15 && db[(size_t) (f + 1)] > db[(size_t) f] + 12) ++gaps;
        }
        s.clicks = (float) clicks / secs;
        s.gaps = (float) gaps / secs;

        // how steady the level is
        if (! loud.empty())
        {
            double m = 0, v = 0;
            for (int f : loud) m += db[(size_t) f];
            m /= (double) loud.size();
            for (int f : loud) v += (db[(size_t) f] - m) * (db[(size_t) f] - m);
            s.steadyDb = (float) std::sqrt (v / (double) loud.size());
        }

        // low end: a one-pole low-pass at 150 Hz against everything
        {
            const float a = std::exp (-juce::MathConstants<float>::twoPi * 150.0f / (float) sr);
            float z = 0;
            double lo = 0, tot = 0;
            for (int i = first * hop; i < juce::jmin (n, (last + 1) * hop); ++i)
            {
                z = x[i] * (1 - a) + z * a;
                lo += (double) z * z;
                tot += (double) x[i] * x[i];
            }
            s.lowShare = tot > 0 ? (float) (lo / tot) : 0.0f;
        }
        s.valid = true;
        return s;
    }

    Shape measureShape (const juce::File& file, juce::AudioFormatManager& fm)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr || reader->sampleRate <= 0 || reader->lengthInSamples <= 0) return {};
        // at most 30 s, at about 22 kHz (plenty for the shape)
        const int n = (int) juce::jmin ((juce::int64) (reader->sampleRate * 30.0), reader->lengthInSamples);
        juce::AudioBuffer<float> buf ((int) juce::jmax (1u, juce::jmin (2u, reader->numChannels)), n);
        if (! reader->read (&buf, 0, n, 0, true, buf.getNumChannels() > 1)) return {};
        const int factor = reader->sampleRate > 30000 ? 2 : 1;
        std::vector<float> mono ((size_t) (n / factor));
        for (size_t i = 0; i < mono.size(); ++i)
        {
            float v = 0;
            for (int c = 0; c < buf.getNumChannels(); ++c)
                for (int k = 0; k < factor; ++k) v += buf.getSample (c, (int) i * factor + k);
            mono[i] = v / (float) (factor * buf.getNumChannels());
        }
        return measureShape (mono.data(), (int) mono.size(), reader->sampleRate / factor);
    }

    Type fromShape (const Shape& s, const SoundInfo& info)
    {
        if (! s.valid) return Type::other;
        const float d = s.duration;
        const bool tonal = s.pitchShare > 0.45f;
        const bool low = s.lowShare > 0.45f || (tonal && s.pitchHz > 0 && s.pitchHz < 130.0f);
        const bool build = s.peakPos > 0.7f && s.startRelDb < -9.0f;
        const bool hitShape = s.peakPos < 0.15f;
        (void) info;

        if (d < 0.35f)
        {
            if (tonal && std::abs (s.pitchTrend) >= 4.0f) return Type::zap;
            if (tonal) return Type::blip;
            return s.gaps > 0 || s.clicks > 3.0f ? Type::glitch : Type::other;
        }
        if (tonal && d < 0.9f && s.pitchTrend <= -10.0f && s.pitchHz > 250.0f && s.wobble < 2.0f && ! build) return Type::laser;
        if (tonal && d < 0.9f && s.pitchTrend >= 5.0f && s.wobble < 2.0f && ! build) return Type::zap;
        if (low && hitShape) return s.pitchTrend <= -3.0f ? Type::subDrop : Type::impact;
        if (s.gaps > 2.0f && ! build) return Type::glitch;
        if (s.wobble > 2.5f && d < 6.0f && ! build) return Type::scratch;
        if (tonal && s.pitchTrend <= -10.0f && d >= 0.5f && ! build) return Type::tapeStop;
        if (build)
        {
            if (s.brightTrend > 0.6f || s.pitchTrend > 4.0f) return Type::riser;
            if (d < 1.2f) return Type::swell;
            if (s.brightHz > 3500.0f && d < 6.0f) return Type::reverseCymbal;
            return d < 2.2f ? Type::swell : Type::riser;
        }
        if (hitShape && d > 0.6f)
        {
            if ((s.brightTrend < -0.8f || s.pitchTrend < -5.0f) && s.holdPos > 0.3f) return Type::downlifter;
            return Type::impact;
        }
        if (d > 1.5f && s.steadyDb < 4.5f) return s.clicks > 3.0f ? Type::crackle : Type::noise;
        if (s.gaps > 2.0f || s.clicks > 6.0f) return Type::glitch;
        if (std::abs (s.brightTrend) > 1.0f) return s.brightTrend < 0 && s.peakPos < 0.4f ? Type::downlifter : Type::sweep;
        if (! tonal && s.peakPos > 0.2f && s.peakPos < 0.8f && s.startRelDb < -8.0f && s.endRelDb < -8.0f) return Type::whoosh;
        if (tonal && d > 0.6f && s.pitchHz > 200.0f && s.pitchHz < 2000.0f) return Type::horn;
        return Type::other;
    }

    //==============================================================================
    // fit: the genres that use it most, the ones it fits, and its decades
    namespace
    {
        using G = Genre;
        struct FitDef { Type type; std::vector<Genre> great, fits; std::vector<int> goodEras, offEras; };
        // eras: 0 60s, 1 70s, 2 80s, 3 90s, 4 2000s, 5 2010s, 6 today
        const std::vector<FitDef>& fitDefs()
        {
            static const std::vector<FitDef> all {
                { Type::riser,         { G::edm, G::pop, G::trap, G::cinematic, G::worship }, { G::rnb, G::rock },              { 4, 5, 6 }, { 0, 1 } },
                { Type::reverseCymbal, { G::pop, G::rock, G::worship, G::country, G::rnb, G::cinematic }, { G::edm, G::trap, G::boomBap, G::lofi }, { 2, 3, 4, 5, 6 }, {} },
                { Type::swell,         { G::pop, G::edm, G::trap, G::rnb }, { G::rock, G::worship, G::cinematic, G::boomBap },   { 4, 5, 6 }, { 0 } },
                { Type::impact,        { G::cinematic, G::edm, G::pop, G::trap }, { G::worship, G::rock, G::rnb },             { 4, 5, 6 }, { 0, 1 } },
                { Type::subDrop,       { G::trap, G::edm, G::pop, G::rnb }, { G::boomBap, G::cinematic },                       { 5, 6 }, { 0, 1, 2 } },
                { Type::downlifter,    { G::edm, G::pop, G::cinematic, G::trap }, { G::rnb, G::worship, G::rock },             { 4, 5, 6 }, { 0, 1 } },
                { Type::sweep,         { G::edm, G::pop, G::trap }, { G::rnb, G::cinematic, G::worship, G::rock },             { 3, 4, 5, 6 }, { 0, 1 } },
                { Type::whoosh,        { G::cinematic, G::edm, G::pop, G::trap }, { G::rnb, G::rock, G::worship },             { 4, 5, 6 }, { 0, 1 } },
                { Type::tapeStop,      { G::trap, G::edm, G::pop, G::rnb }, { G::boomBap, G::lofi, G::rock },                  { 5, 6 }, { 0, 1 } },
                { Type::rewind,        { G::boomBap, G::trap, G::edm }, { G::pop, G::rnb, G::lofi },                            { 3, 4, 5 }, { 0, 1 } },
                { Type::crackle,       { G::lofi, G::boomBap }, { G::rnb, G::pop, G::country, G::cinematic, G::trap },          { 3, 6 }, {} },
                { Type::noise,         { G::lofi, G::cinematic, G::edm }, { G::pop, G::rnb, G::worship, G::boomBap, G::trap },  { 4, 5, 6 }, {} },
                { Type::scratch,       { G::boomBap }, { G::trap, G::rock, G::pop, G::rnb, G::lofi },                           { 2, 3, 4 }, { 0, 1 } },
                { Type::zap,           { G::trap, G::edm, G::pop }, { G::boomBap, G::rnb },                                     { 2, 5, 6 }, { 0, 1 } },
                { Type::laser,         { G::edm, G::pop, G::trap }, { G::boomBap, G::rnb },                                     { 2, 5, 6 }, { 0, 1 } },
                { Type::blip,          { G::pop, G::edm, G::lofi, G::trap }, { G::rnb, G::boomBap },                            { 4, 5, 6 }, { 0, 1 } },
                { Type::glitch,        { G::edm, G::trap, G::pop }, { G::rnb, G::cinematic },                                   { 4, 5, 6 }, { 0, 1, 2 } },
                { Type::horn,          { G::trap, G::edm }, { G::boomBap, G::pop },                                              { 3, 4, 5 }, { 0, 1 } },
                { Type::other,         {}, { G::trap, G::boomBap, G::rnb, G::pop, G::rock, G::country, G::worship, G::edm, G::lofi, G::cinematic }, {}, {} },
            };
            return all;
        }

        const FitDef& fitDef (Type t)
        {
            for (const auto& f : fitDefs()) if (f.type == t) return f;
            return fitDefs().back();
        }

        bool in (const std::vector<Genre>& v, Genre g)   { return std::find (v.begin(), v.end(), g) != v.end(); }
        bool in (const std::vector<int>& v, int e)       { return std::find (v.begin(), v.end(), e) != v.end(); }

        juce::String eraWords (int era)
        {
            if (era == numEras - 1) return "today";
            return "in the " + eraName (era);
        }
    }

    Fit fitFor (Type t, Genre genre, int era)
    {
        const auto& d = fitDef (t);
        int level = in (d.great, genre) ? 2 : in (d.fits, genre) ? 1 : 0;
        const bool eraKnown = juce::isPositiveAndBelow (era, numEras);
        if (eraKnown && in (d.offEras, era)) level = juce::jmax (0, level - 1);
        if (eraKnown && level == 1 && in (d.goodEras, era) && in (d.great, genre)) level = 2;
        Fit f;
        f.level = level;
        f.word = level == 2 ? "Great fit" : level == 1 ? "Fits" : "Unusual";
        const auto where = genreName (genre) + (eraKnown ? " " + eraWords (era) : juce::String());
        f.why = level == 2 ? "Great fit for " + where : level == 1 ? "Fits " + where : "Unusual for " + where + " (use it if you like it)";
        return f;
    }

    juce::String bestFor (Type t)
    {
        const auto& d = fitDef (t);
        if (d.great.empty()) return "Fits any genre, used with taste";
        juce::StringArray g;
        for (auto x : d.great) g.add (genreName (x));
        juce::String s = "Best in: " + g.joinIntoString (", ");
        if (! d.goodEras.empty())
        {
            const int a = d.goodEras.front(), b = d.goodEras.back();
            const bool run = b - a + 1 == (int) d.goodEras.size();
            if (run && b == numEras - 1) s << "  -  " << eraName (a) << (a == b ? juce::String() : " to today");
            else
            {
                juce::StringArray e;
                for (int x : d.goodEras) e.add (eraName (x));
                s << "  -  " << e.joinIntoString (", ");
            }
        }
        return s;
    }

    namespace
    {
        const std::map<juce::String, juce::String>& tips()
        {
            static const auto all = []
            {
                std::map<juce::String, juce::String> m;
                const auto text = juce::String::fromUTF8 (GGBinary::Fx_md, GGBinary::Fx_mdSize);
                juce::String key;
                juce::StringArray body;
                auto flush = [&] { if (key.isNotEmpty()) m[key] = body.joinIntoString ("\n").trim(); body.clear(); };
                for (const auto& line : juce::StringArray::fromLines (text))
                {
                    if (line.startsWith ("@@ ")) { flush(); key = line.substring (3).trim(); continue; }
                    if (key.isNotEmpty()) body.add (line);
                }
                flush();
                return m;
            }();
            return all;
        }
    }

    juce::String tip (Type t)
    {
        const auto it = tips().find (typeId (t));
        return it != tips().end() ? it->second : juce::String();
    }

    juce::StringArray tipKeys()
    {
        juce::StringArray k;
        for (const auto& [key, v] : tips()) k.add (key);
        return k;
    }

    double barsAt (double seconds, double bpm)
    {
        return bpm > 0 ? seconds * bpm / 240.0 : 0.0;
    }

    Placement place (Type t, double lengthSec, double peakSec, double bpm)
    {
        Placement p;
        bpm = juce::jlimit (40.0, 250.0, bpm);
        const double len = lengthSec * bpm / 60.0, peak = juce::jlimit (0.0, len, peakSec * bpm / 60.0);
        auto barsFor = [] (double beats) { return juce::jmax (1.0, std::ceil (beats / 4.0 - 1.0e-6)); };
        switch (anchorOf (t))
        {
            case Anchor::end:
                p.downbeat = 4.0 * (1.0 + barsFor (len));
                p.startBeat = p.downbeat - len;
                p.totalBeats = p.downbeat + 8.0;
                break;
            case Anchor::peak:
                p.downbeat = 4.0 * (1.0 + barsFor (peak));
                p.startBeat = p.downbeat - peak;
                p.totalBeats = p.downbeat + juce::jmax (8.0, 4.0 * barsFor (len - peak));
                break;
            default:
                p.startBeat = p.downbeat = 4.0;
                p.totalBeats = 4.0 + juce::jmax (8.0, 4.0 * barsFor (len) + 4.0);
                break;
        }
        p.totalBeats = juce::jmin (p.totalBeats, 4.0 * 14.0);
        return p;
    }

    //==============================================================================
    // the catalog
    namespace
    {
        juce::var shapeToVar (const Shape& s)
        {
            juce::Array<juce::var> a { s.duration, s.peakPos, s.startRelDb, s.endRelDb, s.holdPos, s.brightTrend, s.brightHz, s.pitchShare,
                                       s.pitchHz, s.pitchTrend, s.wobble, s.clicks, s.gaps, s.steadyDb, s.lowShare };
            return a;
        }

        Shape shapeFromVar (const juce::var& v)
        {
            Shape s;
            auto* a = v.getArray();
            if (a == nullptr || a->size() < 15) return s;
            float* f[] = { &s.duration, &s.peakPos, &s.startRelDb, &s.endRelDb, &s.holdPos, &s.brightTrend, &s.brightHz, &s.pitchShare,
                           &s.pitchHz, &s.pitchTrend, &s.wobble, &s.clicks, &s.gaps, &s.steadyDb, &s.lowShare };
            for (int i = 0; i < 15; ++i)
            {
                const double d = (*a)[i];
                *f[i] = std::isfinite (d) ? (float) d : 0.0f;
            }
            s.valid = true;
            return s;
        }

        juce::String foldersOf (const juce::String& path)
        {
            juce::StringArray parts;
            for (auto dir = juce::File (path).getParentDirectory(); parts.size() < 3 && dir.getParentDirectory() != dir; dir = dir.getParentDirectory())
                parts.insert (0, dir.getFileName());
            return parts.joinIntoString ("/");
        }
    }

    Catalog::Catalog() : juce::Thread ("GG FX")
    {
        file = paths::appDataDir().getChildFile ("fx-types.json");
        load();
    }

    Catalog::~Catalog()
    {
        stopThread (10000);
    }

    void Catalog::load()
    {
        const juce::ScopedLock sl (lock);
        items.clear();
        if (! file.existsAsFile() || file.getSize() > 64 * 1024 * 1024) return;
        const auto parsed = juce::JSON::parse (file);       // (kept while it's read)
        if (auto* o = parsed.getDynamicObject())
            for (const auto& p : o->getProperties())
            {
                Stored st;
                const auto t = typeFromId (p.value["t"].toString());
                if (t == Type::count) continue;
                st.item.type = t;
                st.item.byUser = (bool) p.value.getProperty ("u", false);
                st.item.byName = (bool) p.value.getProperty ("n", false);
                st.item.shape = shapeFromVar (p.value["s"]);
                st.item.analysed = st.item.shape.valid || st.item.byUser;
                st.modTime = p.value["m"].toString().getLargeIntValue();
                st.size = p.value["z"].toString().getLargeIntValue();
                items[p.name.toString()] = st;
            }
    }

    void Catalog::save()
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        {
            const juce::ScopedLock sl (lock);
            for (const auto& [path, st] : items)
            {
                auto* e = new juce::DynamicObject();
                e->setProperty ("t", typeId (st.item.type));
                if (st.item.byUser) e->setProperty ("u", true);
                if (st.item.byName) e->setProperty ("n", true);
                if (st.item.shape.valid) e->setProperty ("s", shapeToVar (st.item.shape));
                e->setProperty ("m", juce::String (st.modTime));
                e->setProperty ("z", juce::String (st.size));
                o->setProperty (path, juce::var (e));
            }
        }
        file.replaceWithText (juce::JSON::toString (juce::var (o.get()), true));
    }

    void Catalog::update (const SoundListPtr& sounds)
    {
        if (sounds == nullptr) return;
        bool any = false;
        {
            const juce::ScopedLock sl (lock);
            for (const auto& s : *sounds)
            {
                if (s.cat != Cat::fx) continue;
                const auto it = items.find (s.path);
                if (it != items.end() && (it->second.item.byUser || (it->second.item.analysed && it->second.modTime == s.modTime && it->second.size == s.fileSize)))
                    continue;
                bool queued = false;
                for (const auto& p : pending) if (p.path == s.path) queued = true;
                if (! queued) { pending.push_back (s); any = true; }
            }
        }
        if (any)
        {
            busy = true;
            if (! isThreadRunning()) startThread (juce::Thread::Priority::low);
            else notify();
        }
    }

    Catalog::Item Catalog::get (const SoundInfo& s) const
    {
        {
            const juce::ScopedLock sl (lock);
            const auto it = items.find (s.path);
            if (it != items.end()) return it->second.item;
        }
        Item i;
        const auto t = fromName (juce::File (s.path).getFileNameWithoutExtension(), foldersOf (s.path));
        i.type = t == Type::count ? Type::other : t;
        i.byName = t != Type::count;
        return i;
    }

    void Catalog::setType (const juce::String& path, Type t)
    {
        {
            const juce::ScopedLock sl (lock);
            auto& st = items[path];
            st.item.type = t;
            st.item.byUser = true;
            st.item.analysed = true;
        }
        save();
        sendChangeMessage();
    }

    void Catalog::waitUntilDone (int timeoutMs)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
        while (busy.load() && juce::Time::getMillisecondCounter() < end)
            juce::Thread::sleep (20);          // (the sorting needs nothing from the caller's thread)
    }

    void Catalog::run()
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        while (! threadShouldExit())
        {
            std::vector<SoundInfo> todo;
            {
                const juce::ScopedLock sl (lock);
                todo.swap (pending);
            }
            if (todo.empty())
            {
                busy = false;
                wait (-1);
                continue;
            }
            int done = 0;
            for (const auto& s : todo)
            {
                if (threadShouldExit()) return;
                Stored st;
                st.modTime = s.modTime;
                st.size = s.fileSize;
                const auto byName = fromName (juce::File (s.path).getFileNameWithoutExtension(), foldersOf (s.path));
                st.item.shape = measureShape (juce::File (s.path), fm);
                st.item.byName = byName != Type::count;
                st.item.type = st.item.byName ? byName : fromShape (st.item.shape, s);
                st.item.analysed = true;
                {
                    const juce::ScopedLock sl (lock);
                    const auto it = items.find (s.path);
                    if (it != items.end() && it->second.item.byUser) continue;     // set by hand meanwhile
                    items[s.path] = st;
                }
                if (++done % 25 == 0) sendChangeMessage();
            }
            save();
            {
                const juce::ScopedLock sl (lock);
                if (pending.empty()) busy = false;
            }
            sendChangeMessage();
        }
    }
}
