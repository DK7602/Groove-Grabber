#include "Analyzer.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace gg::analysis
{
    namespace
    {
        inline float clamp01 (float x) noexcept               { return juce::jlimit (0.0f, 1.0f, x); }
        inline float rise (float x, float a, float b) noexcept { return clamp01 ((x - a) / (b - a)); }
        inline float fall (float x, float a, float b) noexcept { return 1.0f - rise (x, a, b); }
        inline float toDb (float g) noexcept                    { return g > 1.0e-9f ? 20.0f * std::log10 (g) : -180.0f; }

        // log-scaled position of x between lo and hi, 0..1
        inline float logPos (float x, float lo, float hi) noexcept
        {
            if (x <= 0) return 0;
            return clamp01 (std::log (x / lo) / std::log (hi / lo));
        }

        //==============================================================================
        juce::StringArray tokenize (const juce::String& s)
        {
            juce::String spaced;
            juce::juce_wchar prev = 0;
            for (auto p = s.getCharPointer(); ! p.isEmpty(); ++p)
            {
                const auto c = *p;
                const bool letter = juce::CharacterFunctions::isLetter (c);
                const bool digit = juce::CharacterFunctions::isDigit (c);
                if (! letter && ! digit) { spaced << ' '; prev = 0; continue; }
                // split camelCase and letter/digit boundaries ("BDDusty03" -> "BD Dusty 03"),
                // but keep "808", "909", "sp1200" style tokens together
                if (prev != 0)
                {
                    const bool prevLetter = juce::CharacterFunctions::isLetter (prev);
                    const bool prevDigit = juce::CharacterFunctions::isDigit (prev);
                    if ((prevLetter && juce::CharacterFunctions::isLowerCase (prev) && juce::CharacterFunctions::isUpperCase (c))
                         || (prevLetter && digit) || (prevDigit && letter))
                        spaced << ' ';
                }
                spaced << juce::CharacterFunctions::toLowerCase (c);
                prev = c;
            }
            auto t = juce::StringArray::fromTokens (spaced, " ", "");
            t.removeEmptyStrings();
            return t;
        }

        bool hasAny (const juce::StringArray& t, std::initializer_list<const char*> words)
        {
            for (auto* w : words)
                if (t.contains (w))
                    return true;
            return false;
        }

        bool phrase (const juce::String& joined, const char* p)
        {
            return (" " + joined + " ").contains (" " + juce::String (p) + " ");
        }

        struct WordHit { Cat cat = Cat::unknown; float strength = 0; PercKind perc = PercKind::none; };

        WordHit catFromTokens (const juce::StringArray& t, NameGuess& g)
        {
            const auto joined = t.joinIntoString (" ");
            WordHit h;
            auto set = [&h] (Cat c, float s, PercKind p = PercKind::none)
            {
                if (s > h.strength) { h.cat = c; h.strength = s; h.perc = p; }
            };

            g.says808 = g.says808 || t.contains ("808") || t.contains ("808s");
            const bool kickWord = hasAny (t, { "kick", "kicks", "kik", "kck", "bd", "bassdrum", "kickdrum" }) || phrase (joined, "bass drum");
            g.saysKick = g.saysKick || kickWord;
            const bool hatWord = hasAny (t, { "hat", "hats", "hh", "hihat", "hihats", "hhat", "chh", "ohh", "hht", "oh", "ch" }) || phrase (joined, "hi hat");
            g.saysHat = g.saysHat || hatWord;
            g.saysOpen = g.saysOpen || hasAny (t, { "open", "opn", "ohh", "oh", "opened" });
            g.saysClosed = g.saysClosed || hasAny (t, { "closed", "close", "chh", "ch", "cl", "tight", "pedal", "clsd" });

            if (kickWord)                                                   set (Cat::kick, 0.9f);
            if (hasAny (t, { "snare", "snares", "snr", "sd", "sn", "snre" })) set (Cat::snare, 0.9f);
            if (hasAny (t, { "clap", "claps", "clp", "handclap", "handclaps" })) set (Cat::clap, 0.92f);
            if (hasAny (t, { "rim", "rims", "rimshot", "sidestick", "crossstick" }) || phrase (joined, "side stick") || phrase (joined, "cross stick"))
                set (Cat::rim, 0.92f);
            if (t.contains ("stick") || t.contains ("sticks"))              set (Cat::rim, 0.55f);
            if (hatWord)
            {
                if (g.saysOpen && ! t.contains ("closed"))                  set (Cat::openHat, 0.93f);
                else                                                        set (Cat::closedHat, g.saysClosed ? 0.93f : 0.75f);
            }
            if (hasAny (t, { "tom", "toms", "ftom", "floortom", "racktom", "rtom" }) || phrase (joined, "floor tom"))
                set (Cat::tom, 0.9f);
            if (hasAny (t, { "crash", "crashes", "splash", "china", "cymbal", "cymbals", "cym", "cymb" })) set (Cat::crash, 0.88f);
            if (hasAny (t, { "ride", "rides" }))                            set (Cat::ride, 0.93f);
            if (hasAny (t, { "sub", "subs", "subbass", "bass", "basses", "808bass" }) && ! phrase (joined, "bass drum"))
                set (Cat::bass808, 0.8f);

            // percussion, with its kind
            if (hasAny (t, { "shaker", "shakers", "shk", "shake", "maraca", "maracas", "cabasa", "egg" })) set (Cat::perc, 0.9f, PercKind::shaker);
            if (hasAny (t, { "tamb", "tambo", "tambourine", "tambourines", "tamborine", "jingle" }))     set (Cat::perc, 0.9f, PercKind::tambourine);
            if (hasAny (t, { "conga", "congas", "bongo", "bongos", "djembe", "tabla", "timbale", "timbales", "cajon", "darbuka" }))
                set (Cat::perc, 0.9f, PercKind::conga);
            if (hasAny (t, { "cowbell", "cowbells", "cowb", "cb", "agogo" }))                          set (Cat::perc, 0.9f, PercKind::cowbell);
            if (hasAny (t, { "snap", "snaps", "fingersnap", "fingersnaps", "finger" }))                set (Cat::perc, 0.9f, PercKind::snap);
            if (hasAny (t, { "block", "woodblock", "clave", "claves", "knock", "wood" }))              set (Cat::perc, 0.85f, PercKind::block);
            if (hasAny (t, { "triangle", "tri", "chime", "chimes", "bell", "bells" }))                 set (Cat::perc, 0.6f, PercKind::triangle);
            if (hasAny (t, { "perc", "percs", "percussion", "guiro", "taiko", "click", "tick" }))     set (Cat::perc, 0.82f, PercKind::other);

            if (hasAny (t, { "fx", "sfx", "riser", "risers", "rise", "sweep", "sweeps", "impact", "impacts", "uplifter",
                             "downlifter", "whoosh", "swoosh", "noise", "reverse", "rev", "reversed", "laser", "zap", "scratch",
                             "scratches", "stab", "stabs", "transition", "glitch", "foley",
                             "texture", "atmos", "drone", "siren", "gunshot", "airhorn", "tag" }))
                set (Cat::fx, 0.85f);

            // vocals and instruments: never drums, unless a drum word says otherwise ("Vocal Snare", "Synth Kick")
            {
                const bool drumContext = hasAny (t, { "drum", "drums", "kit", "kits", "808", "909", "perc", "percussion", "beatbox", "oneshot" })
                                      || phrase (joined, "drum kit");
                const bool vocal = hasAny (t, { "vocal", "vocals", "vox", "voice", "voices", "chant", "chants", "adlib", "adlibs", "acapella",
                                                "acappella", "accapella", "acapellas", "sung", "singing", "singer", "choir", "hook", "hooks",
                                                "phrase", "phrases", "shout", "shouts", "verse", "spoken", "speech", "lyric", "lyrics",
                                                "harmony", "harmonies", "ooh", "aah", "humming" });
                const bool instrument = hasAny (t, { "synth", "synths", "piano", "pianos", "rhodes", "wurli", "wurlitzer", "organ", "keys",
                                                     "guitar", "guitars", "gtr", "pads", "lead", "leads", "pluck", "plucks", "chord", "chords",
                                                     "melody", "melodies", "melodic", "arp", "arps", "arpeggio", "strings", "violin", "cello",
                                                     "brass", "flute", "sax", "saxophone", "trumpet", "marimba", "kalimba", "harp", "instrument",
                                                     "instruments", "bassline", "basslines" })
                                     || ((t.contains ("bass") || t.contains ("basses")) && (t.contains ("loop") || t.contains ("loops")));
                if ((vocal || (instrument && ! drumContext)) && ! (drumContext && ! vocal && h.strength >= 0.8f))
                {
                    g.saysMelodic = true;
                    set (Cat::melodic, 0.86f);
                }
            }

            // "808" on its own is the bass; "808 snare", "808 clap" etc. is the drum machine's sound
            if (g.says808 && h.strength < 0.85f)                           set (Cat::bass808, 0.88f);
            if (g.says808 && kickWord && h.cat == Cat::kick)                h.strength = 0.7f;   // "808 kick": the audio decides kick vs 808

            if (hasAny (t, { "loop", "loops", "lp", "break", "breaks", "breakbeat", "groove", "grooves", "fill", "fills" }) || t.contains ("bpm"))
                g.loopWord = true;
            return h;
        }

        //==============================================================================
        // Small FFT helper (one per thread)
        struct Spectrum
        {
            static constexpr int order = 12, size = 1 << order;
            juce::dsp::FFT fft { order };
            std::vector<float> buf = std::vector<float> (2 * size), power = std::vector<float> (size / 2 + 1), window = std::vector<float> (size);

            Spectrum()
            {
                for (int i = 0; i < size; ++i)
                    window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (size - 1));
            }

            void clear()   { std::fill (power.begin(), power.end(), 0.0f); }

            void add (const float* x, int n)
            {
                std::fill (buf.begin(), buf.end(), 0.0f);
                for (int i = 0; i < juce::jmin (n, size); ++i)
                    buf[(size_t) i] = x[i] * window[(size_t) i];
                fft.performFrequencyOnlyForwardTransform (buf.data(), true);
                for (int k = 0; k <= size / 2; ++k)
                    power[(size_t) k] += buf[(size_t) k] * buf[(size_t) k];
            }
        };

        Spectrum& spectrum()
        {
            static thread_local std::unique_ptr<Spectrum> s;
            if (s == nullptr) s = std::make_unique<Spectrum>();
            return *s;
        }
    }

    //==============================================================================
    NameGuess guessFromName (const juce::String& fileName, const juce::String& folders)
    {
        NameGuess g;
        const auto nameTokens = tokenize (fileName);
        auto hit = catFromTokens (nameTokens, g);

        // folders count for less, the nearest folder most ("Drums/Kicks/Dusty 3.wav")
        auto folderList = juce::StringArray::fromTokens (folders.replaceCharacter ('\\', '/'), "/", "");
        folderList.removeEmptyStrings();
        for (int i = folderList.size(); --i >= 0;)
        {
            NameGuess fg;
            const auto ft = tokenize (folderList[i]);
            auto fh = catFromTokens (ft, fg);
            const float weight = i == folderList.size() - 1 ? 0.78f : 0.62f;
            if (hit.cat == Cat::unknown && fh.cat != Cat::unknown)
            {
                hit = fh;
                hit.strength *= weight;
            }
            if (hit.perc == PercKind::none && fh.perc != PercKind::none && hit.cat == Cat::perc)
                hit.perc = fh.perc;
            g.loopWord = g.loopWord || (fg.loopWord && i == folderList.size() - 1);
            g.saysMelodic = g.saysMelodic || (fg.saysMelodic && hit.cat == Cat::melodic);
            g.says808 = g.says808 || (fg.says808 && hit.cat == Cat::bass808);
            if (hit.cat == Cat::closedHat || hit.cat == Cat::openHat)
            {
                g.saysOpen = g.saysOpen || fg.saysOpen;
                g.saysClosed = g.saysClosed || fg.saysClosed;
            }
        }

        g.cat = hit.cat;
        g.strength = hit.strength;
        g.perc = hit.perc;

        // character words, from the whole path
        auto all = tokenize (folders + " " + fileName);
        const auto joined = all.joinIntoString (" ");
        float vintage = 0, organic = 0;
        for (auto* w : { "vintage", "vinyl", "dusty", "dust", "lofi", "tape", "sp1200", "sp12", "mpc", "mpc60", "mpc3000",
                         "old", "oldschool", "70s", "80s", "90s", "retro", "crunchy", "crunch", "bitcrushed", "bitcrush", "gritty",
                         "grit", "sampled", "chopped", "classic", "soul", "funk", "jazz", "worn", "lo" })
            if (all.contains (w)) vintage += 0.35f;
        for (auto* w : { "clean", "modern", "crisp", "polished", "tight", "pristine", "hd", "2024", "2025", "2026", "future" })
            if (all.contains (w)) vintage -= 0.35f;
        for (auto* w : { "acoustic", "live", "real", "studio", "room", "ludwig", "gretsch", "pearl", "dw", "tama", "sonor",
                         "zildjian", "sabian", "paiste", "brush", "brushes", "mallet", "mallets", "natural", "jazz", "rock",
                         "country", "funk", "overhead", "overheads", "mic", "close", "kit", "drummer", "played", "organic" })
            if (all.contains (w)) organic += 0.3f;
        for (auto* w : { "808", "909", "707", "606", "linn", "linndrum", "machine", "synth", "synthetic", "analog", "analogue",
                         "electro", "electronic", "edm", "trap", "digital", "fm", "glitch", "drill", "techno", "house", "modular" })
            if (all.contains (w)) organic -= 0.3f;
        if (phrase (joined, "drum machine")) organic -= 0.3f;
        g.vintage = juce::jlimit (-1.0f, 1.0f, vintage);
        g.organic = juce::jlimit (-1.0f, 1.0f, organic);
        return g;
    }

    //==============================================================================
    float detectPitch (const float* x, int n, double sr, float minHz, float maxHz, float& conf)
    {
        conf = 0;
        if (n < 64 || sr <= 0) return 0;

        // decimate to ~11 kHz (box filter is plenty for fundamentals under 1 kHz)
        const int factor = juce::jmax (1, (int) std::floor (sr / 11025.0));
        const double dsr = sr / factor;
        std::vector<float> d ((size_t) (n / factor));
        for (size_t i = 0; i < d.size(); ++i)
        {
            float s = 0;
            for (int k = 0; k < factor; ++k) s += x[(int) i * factor + k];
            d[i] = s / (float) factor;
        }

        const int maxLag = juce::jmin ((int) (dsr / minHz), (int) d.size() / 2);
        const int minLag = juce::jmax (2, (int) (dsr / maxHz));
        if (maxLag <= minLag + 2) return 0;
        const int w = (int) d.size() - maxLag;
        if (w < 32) return 0;

        // energy check: nothing to find in silence
        double energy = 0;
        for (int i = 0; i < w; ++i) energy += (double) d[(size_t) i] * d[(size_t) i];
        if (energy < 1.0e-8) return 0;

        std::vector<float> cmnd ((size_t) maxLag + 1, 1.0f);
        double running = 0;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double diff = 0;
            for (int i = 0; i < w; ++i)
            {
                const double v = (double) d[(size_t) i] - d[(size_t) (i + tau)];
                diff += v * v;
            }
            running += diff;
            cmnd[(size_t) tau] = running > 0 ? (float) (diff * tau / running) : 1.0f;
        }

        int best = -1;
        for (int tau = minLag; tau < maxLag; ++tau)
            if (cmnd[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < maxLag && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        if (best < 0)
        {
            best = minLag;
            for (int tau = minLag; tau < maxLag; ++tau)
                if (cmnd[(size_t) tau] < cmnd[(size_t) best]) best = tau;
        }

        // parabolic interpolation
        double tauF = best;
        if (best > minLag && best < maxLag - 1)
        {
            const double a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
            const double den = a - 2 * b + c;
            if (std::abs (den) > 1.0e-9) tauF = best + 0.5 * (a - c) / den;
        }
        conf = clamp01 (1.0f - cmnd[(size_t) best]);
        return (float) (dsr / tauF);
    }

    float shortTermLoudnessDb (const float* const* ch, int numCh, int n, double sr)
    {
        const int win = juce::jmax (1, (int) (sr * 0.05));
        const int hop = juce::jmax (1, win / 4);
        double best = 0;
        for (int start = 0; start < juce::jmax (1, n - win + 1); start += hop)
        {
            double sum = 0;
            const int end = juce::jmin (n, start + win);
            for (int c = 0; c < numCh; ++c)
                for (int i = start; i < end; ++i)
                    sum += (double) ch[c][i] * ch[c][i];
            best = juce::jmax (best, sum / (double) (win * juce::jmax (1, numCh)));
        }
        return toDb ((float) std::sqrt (best));
    }

    //==============================================================================
    void measure (const float* x, int n, double sr, SoundInfo& info)
    {
        info.sampleRate = sr;
        if (n <= 0 || sr <= 0) return;

        // ---- peak, clipping ----
        float peak = 0;
        int run = 0;
        bool clipped = false;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (x[i]);
            peak = juce::jmax (peak, a);
            if (a >= 0.999f) { if (++run >= 3) clipped = true; }
            else run = 0;
        }
        info.peakDb = toDb (peak);
        info.clipped = clipped;
        if (peak < 1.0e-6f) { info.cat = Cat::unknown; return; }

        // first sample above -40 dB of the peak = where the hit starts
        int firstLoud = 0;
        while (firstLoud < n && std::abs (x[firstLoud]) < peak * 0.01f) ++firstLoud;
        info.leadSilenceMs = (float) (1000.0 * firstLoud / sr);

        // ---- envelopes, one value per millisecond ----
        // slow: 12 ms RMS (smooth even for 45 Hz subs) for length, loudness and counting hits
        // fast: 2 ms RMS for the attack and for the separate bursts of a clap
        const int hop = juce::jmax (1, (int) std::round (sr / 1000.0));
        const int frames = juce::jmax (1, n / hop);
        std::vector<float> env ((size_t) frames), fastEnv ((size_t) frames);
        {
            std::vector<double> cum ((size_t) n + 1, 0.0);
            for (int i = 0; i < n; ++i) cum[(size_t) i + 1] = cum[(size_t) i] + (double) x[i] * x[i];
            auto rms = [&] (int centre, int halfWidth)
            {
                const int a = juce::jmax (0, centre - halfWidth), b = juce::jmin (n, centre + halfWidth);
                return b > a ? (float) std::sqrt ((cum[(size_t) b] - cum[(size_t) a]) / (b - a)) : 0.0f;
            };
            for (int f = 0; f < frames; ++f)
            {
                env[(size_t) f] = rms (f * hop, 6 * hop);
                fastEnv[(size_t) f] = rms (f * hop + hop / 2, hop);
            }
        }
        const int maxF = (int) (std::max_element (env.begin(), env.end()) - env.begin());
        const float envMax = env[(size_t) maxF];

        // attack: from where the fast envelope wakes up to its peak
        {
            const int fastMax = (int) (std::max_element (fastEnv.begin(), fastEnv.end()) - fastEnv.begin());
            const float fm = fastEnv[(size_t) fastMax];
            int fo = 0;
            while (fo < fastMax && fastEnv[(size_t) fo] < fm * 0.1f) ++fo;
            info.attackMs = (float) (fastMax - fo);
        }
        int onsetF = 0;
        while (onsetF < maxF && env[(size_t) onsetF] < envMax * 0.1f) ++onsetF;

        // length: peak to -20 dB
        int endF = frames - 1;
        while (endF > maxF && env[(size_t) endF] < envMax * 0.1f) --endF;
        info.decayMs = (float) (endF - maxF);
        // (the quiet tail, to -40 dB, still counts as "the sound" for the spectrum and noise floor)
        int tailF = frames - 1;
        while (tailF > maxF && env[(size_t) tailF] < envMax * 0.01f) --tailF;

        // loudest 50 ms
        {
            double best = 0, sum = 0;
            const int w = 50;
            for (int f = 0; f < frames; ++f)
            {
                sum += (double) env[(size_t) f] * env[(size_t) f];
                if (f >= w) sum -= (double) env[(size_t) (f - w)] * env[(size_t) (f - w)];
                best = juce::jmax (best, sum / juce::jmin (f + 1, w));
            }
            info.loudDb = toDb ((float) std::sqrt (best));
        }

        // crest factor over the sounding part
        {
            double s = 0;
            int cnt = 0;
            for (int f = onsetF; f <= juce::jmax (onsetF, endF); ++f) { s += (double) fastEnv[(size_t) f] * fastEnv[(size_t) f]; ++cnt; }
            info.crestDb = info.peakDb - toDb ((float) std::sqrt (s / juce::jmax (1, cnt)));
        }

        // what's left once it has died away: hiss / vinyl crackle
        {
            const int tailStart = juce::jmin (frames - 1, tailF + 20);
            if (frames - tailStart >= 60)
            {
                std::vector<float> tail (env.begin() + tailStart, env.end());
                std::nth_element (tail.begin(), tail.begin() + (long) tail.size() / 2, tail.end());
                info.tailFloorDb = toDb (tail[tail.size() / 2] / envMax);
            }
            else
            {
                // rings all the way to the end: use the quietest part of its last fifth as a hint
                const int from = maxF + (frames - maxF) * 4 / 5;
                float mn = envMax;
                for (int f = from; f < frames; ++f) mn = juce::jmin (mn, env[(size_t) f]);
                info.tailFloorDb = juce::jmin (-30.0f, toDb (mn / envMax));
            }
        }

        // hits in the file (one-shots have one; loops many). 60 ms refractory so a clap counts once
        {
            int count = 0, lastHit = -1000;
            float floorLevel = envMax;
            for (int f = 0; f < frames; ++f)
            {
                const float e = env[(size_t) f];
                floorLevel = juce::jmin (floorLevel * 1.02f + 1.0e-9f, e);   // slowly-rising follower of the dips
                if (e > envMax * 0.18f && e > floorLevel * 4.0f && f - lastHit > 60)
                {
                    ++count;
                    lastHit = f;
                    floorLevel = e;
                }
                if (f - lastHit < 60) floorLevel = juce::jmin (floorLevel, e);
            }
            info.onsets = juce::jmax (1, count);
        }

        // ---- spectrum of the hit (up to three frames from the onset) ----
        auto& sp = spectrum();
        sp.clear();
        const int onsetSample = onsetF * hop;
        const int soundEnd = juce::jmin (n, (tailF + 1) * hop);
        int framesAdded = 0;
        for (int start = onsetSample; start < soundEnd && framesAdded < 3; start += Spectrum::size / 2)
        {
            sp.add (x + start, juce::jmin (Spectrum::size, n - start));
            ++framesAdded;
        }
        const double binHz = sr / Spectrum::size;
        double total = 0, weighted = 0, sub = 0, low = 0, mid = 0, high = 0, logSum = 0, linSum = 0;
        int flatBins = 0;
        for (int k = 1; k <= Spectrum::size / 2; ++k)
        {
            const double f = k * binHz;
            if (f < 20 || f > 20000) continue;
            const double p = sp.power[(size_t) k];
            total += p;
            weighted += f * p;
            if (f < 70) sub += p;
            else if (f < 250) low += p;
            else if (f < 2500) mid += p;
            else if (f >= 5000) high += p;
            if (f >= 100 && f <= juce::jmin (16000.0, sr * 0.45))
            {
                logSum += std::log (p + 1.0e-12);
                linSum += p + 1.0e-12;
                ++flatBins;
            }
        }
        if (total > 0)
        {
            info.centroidHz = (float) (weighted / total);
            info.subShare = (float) (sub / total);
            info.lowShare = (float) (low / total);
            info.midShare = (float) (mid / total);
            info.highShare = (float) (high / total);
            double cum = 0;
            info.rolloffHz = (float) (sr / 2);
            for (int k = 1; k <= Spectrum::size / 2; ++k)
            {
                cum += sp.power[(size_t) k];
                if (cum >= 0.85 * total) { info.rolloffHz = (float) (k * binHz); break; }
            }
        }
        if (flatBins > 0)
            info.flatness = (float) (std::exp (logSum / flatBins) / (linSum / flatBins));

        // separate bursts in the first 50 ms (claps have 3 or 4)
        if (info.centroidHz > 700.0f && info.subShare + info.lowShare < 0.4f)
        {
            const float fastPeak = *std::max_element (fastEnv.begin(), fastEnv.end());
            int bursts = 0;
            const int end = juce::jmin (frames - 1, onsetF + 50);
            float lastMin = 0;
            bool rising = true;
            for (int f = onsetF + 1; f < end; ++f)
            {
                const float a = fastEnv[(size_t) f - 1], b = fastEnv[(size_t) f];
                if (rising && b < a)
                {
                    if (a > fastPeak * 0.25f && (bursts == 0 || a > lastMin * 1.4f)) ++bursts;
                    rising = false;
                    lastMin = b;
                }
                else if (! rising && b > a) rising = true;
                else if (! rising) lastMin = juce::jmin (lastMin, b);
            }
            info.burstCount = juce::jmax (1, bursts);
        }
        else
            info.burstCount = 1;

        // ---- pitch, for anything with real low / low-mid body ----
        info.pitchHz = 0;
        info.pitchConf = 0;
        if (info.subShare + info.lowShare > 0.15f || info.centroidHz < 2500)
        {
            const int from = onsetSample + (int) (sr * 0.012);
            const int len = juce::jmin ((int) (sr * 0.16), soundEnd - from);
            if (len > (int) (sr * 0.03))
            {
                float conf = 0;
                const float f0 = detectPitch (x + from, len, sr, 28.0f, 1000.0f, conf);
                if (f0 > 0) { info.pitchHz = f0; info.pitchConf = conf; }
            }
        }
    }

    //==============================================================================
    Cat classifyAudio (const SoundInfo& s, float& confidence)
    {
        const float lowAll = s.subShare + s.lowShare;
        const float cen = s.centroidHz, decay = s.decayMs, flat = s.flatness, high = s.highShare;
        const bool tonal = s.pitchConf > 0.6f;
        std::array<float, numCats> sc {};

        const float body = lowAll + s.midShare;                  // anything below 2.5 kHz
        const bool pitched = s.pitchConf > 0.4f;

        auto& kick = sc[(size_t) Cat::kick];
        kick = rise (lowAll, 0.35f, 0.7f) * fall (cen, 400, 1800) * fall (decay, 600, 1300);

        auto& b808 = sc[(size_t) Cat::bass808];
        b808 = rise (lowAll, 0.5f, 0.85f) * fall (cen, 250, 800) * rise (decay, 400, 800) * (0.4f + 0.6f * rise (s.pitchConf, 0.4f, 0.75f))
             * (pitched ? fall (s.pitchHz, 95, 150) : 1.0f) * fall (s.midShare + s.highShare, 0.06f, 0.2f);

        auto& snare = sc[(size_t) Cat::snare];
        snare = rise (cen, 700, 1600) * fall (cen, 7000, 10500) * rise (flat, 0.04f, 0.15f) * fall (lowAll, 0.45f, 0.75f)
              * rise (body, 0.08f, 0.2f) * rise (decay, 50, 110) * fall (decay, 1200, 2000) * (s.burstCount >= 3 ? 0.55f : 1.0f);

        auto& clap = sc[(size_t) Cat::clap];
        clap = rise (cen, 800, 1500) * fall (cen, 6000, 9000) * rise (flat, 0.01f, 0.05f) * fall (lowAll, 0.35f, 0.6f)
             * (s.burstCount >= 3 ? 1.05f : (s.burstCount == 2 ? 0.55f : 0.25f)) * fall (decay, 1000, 1700);

        auto& rim = sc[(size_t) Cat::rim];
        rim = fall (decay, 80, 180) * rise (cen, 700, 1400) * fall (cen, 6000, 9000) * fall (flat, 0.12f, 0.35f) * fall (lowAll, 0.3f, 0.6f);

        const float hatShape = rise (cen, 4000, 7000) * rise (high, 0.3f, 0.55f) * rise (flat, 0.06f, 0.2f)
                             * fall (body, 0.12f, 0.3f) * (s.burstCount >= 3 ? 0.5f : 1.0f);
        sc[(size_t) Cat::closedHat] = hatShape * fall (decay, 160, 350);
        sc[(size_t) Cat::openHat] = hatShape * rise (decay, 200, 400) * fall (decay, 2000, 3500);

        auto& crash = sc[(size_t) Cat::crash];
        crash = rise (cen, 2800, 5000) * rise (decay, 1300, 2300) * rise (flat, 0.1f, 0.28f);

        auto& ride = sc[(size_t) Cat::ride];
        ride = rise (cen, 2500, 4500) * rise (decay, 1000, 2000) * fall (flat, 0.1f, 0.3f) * 0.95f;

        auto& tom = sc[(size_t) Cat::tom];
        tom = rise (s.pitchConf, 0.45f, 0.75f) * rise (s.pitchHz, 55, 80) * fall (s.pitchHz, 350, 500) * fall (cen, 1500, 3500)
            * rise (decay, 150, 300) * fall (s.subShare, 0.35f, 0.65f) * fall (decay, 2500, 4000)
            * (0.55f + 0.45f * rise (s.midShare + s.highShare, 0.02f, 0.08f));

        auto& perc = sc[(size_t) Cat::perc];
        perc = 0.22f + 0.4f * (tonal && s.pitchHz > 400 ? 1.0f : 0.0f) * fall (decay, 400, 900)
             + 0.25f * (flat > 0.3f && cen > 3500 && s.attackMs > 4 ? 1.0f : 0.0f) * fall (decay, 400, 800);

        auto& fx = sc[(size_t) Cat::fx];
        fx = juce::jmax (rise (s.duration, 1.5f, 3.0f) * rise (decay, 1500, 3000) * fall (cen, 2500, 5000) * fall (flat, 0.15f, 0.3f),
                         rise (s.attackMs, 80, 350));   // long build-ups (risers, reverse hits)

        int best = 0, second = 1;
        for (int i = 0; i < numCats; ++i)
            if (sc[(size_t) i] > sc[(size_t) best]) best = i;
        if (best == second) second = 0;
        for (int i = 0; i < numCats; ++i)
            if (i != best && sc[(size_t) i] > sc[(size_t) second]) second = i;

        const float top = sc[(size_t) best];
        confidence = clamp01 (0.35f + 0.5f * top + 0.6f * (top - sc[(size_t) second]));
        if (top < 0.08f) { confidence = 0.2f; return Cat::perc; }
        return (Cat) best;
    }

    //==============================================================================
    void computeTraits (SoundInfo& s, const NameGuess& g)
    {
        struct Ref { float briLo, briHi, roomLo, roomHi, rollLo, rollHi; };
        auto ref = [] (Cat c) -> Ref
        {
            switch (c)
            {
                case Cat::kick:      return { 80, 1200, 80, 500, 1500, 7000 };
                case Cat::bass808:   return { 50, 400, 300, 1600, 200, 2500 };
                case Cat::snare:     return { 900, 5500, 80, 520, 4000, 11000 };
                case Cat::clap:      return { 900, 5000, 70, 450, 4000, 11000 };
                case Cat::rim:       return { 1000, 5000, 30, 200, 4000, 11000 };
                case Cat::closedHat: return { 4000, 11000, 30, 180, 8000, 16000 };
                case Cat::openHat:   return { 3500, 10000, 180, 900, 8000, 16000 };
                case Cat::tom:       return { 150, 1500, 140, 850, 1500, 7000 };
                case Cat::crash:     return { 2500, 8000, 700, 2800, 7000, 15000 };
                case Cat::ride:      return { 2500, 8000, 600, 2500, 7000, 15000 };
                case Cat::perc:      return { 800, 7000, 40, 420, 4000, 12000 };
                case Cat::fx:        return { 300, 6000, 350, 2800, 3000, 12000 };
                default:             return { 200, 6000, 70, 1000, 3000, 12000 };
            }
        };
        const auto r = ref (s.cat);
        auto& t = s.traits;

        t.bright = logPos (s.centroidHz, r.briLo, r.briHi);
        t.room = logPos (juce::jmax (1.0f, s.decayMs), r.roomLo, r.roomHi);

        const float fastAttack = fall (s.attackMs, 2.0f, 25.0f);
        const float crestPunch = rise (s.crestDb, 6.0f, 18.0f);
        t.punch = clamp01 (0.55f * fastAttack + 0.45f * crestPunch - 0.15f * t.room + 0.1f);

        const float body = s.lowShare + 0.6f * s.midShare - s.highShare;
        t.warmth = clamp01 (0.55f * (1.0f - t.bright) + 0.45f * clamp01 (body * 1.1f + 0.2f));

        float vintage = 0.0f;
        if (s.cat != Cat::bass808)
            vintage += 0.42f * fall (s.rolloffHz, r.rollLo, r.rollHi);
        vintage += 0.3f * rise (s.tailFloorDb, -66.0f, -42.0f);
        if (s.sampleRate > 0 && s.sampleRate < 32000) vintage += 0.2f;
        vintage += 0.3f * g.vintage;
        t.vintage = clamp01 (vintage + 0.12f);
        t.warmth = clamp01 (t.warmth + 0.15f * (t.vintage - 0.5f));

        t.aggr = clamp01 (0.35f * fall (s.crestDb, 8.0f, 20.0f) + 0.3f * t.bright + 0.2f * t.punch + (s.clipped ? 0.15f : 0.0f) + 0.05f);

        float organic = 0.5f + 0.3f * g.organic;
        if (s.cat == Cat::kick || s.cat == Cat::tom)
        {
            if (s.pitchConf > 0.85f && s.flatness < 0.02f && s.highShare < 0.03f) organic -= 0.15f;   // pure synthetic thump
            if (s.highShare > 0.08f && s.flatness > 0.03f) organic += 0.08f;                         // beater + shell
        }
        if (s.cat == Cat::bass808) organic = juce::jmin (organic, 0.12f);
        if (s.genreTags & ((1u << (int) Genre::trap) | (1u << (int) Genre::edm))) organic -= 0.15f;
        if (s.genreTags & ((1u << (int) Genre::rock) | (1u << (int) Genre::country) | (1u << (int) Genre::worship))) organic += 0.15f;
        t.organic = clamp01 (organic);
    }

    //==============================================================================
    void finish (SoundInfo& s, const NameGuess& g)
    {
        if (! s.catByUser)
        {
            float audioConf = 0;
            const Cat audio = classifyAudio (s, audioConf);

            Cat c = audio;
            float conf = audioConf;

            if (g.cat != Cat::unknown && g.strength >= 0.5f)
            {
                c = g.cat;
                conf = juce::jmax (g.strength, audio == g.cat ? audioConf : 0.0f);

                // a vague or folder-only word loses to a confident, very different sound
                if (g.strength < 0.7f && audio != g.cat && audioConf > 0.85f)
                {
                    c = audio;
                    conf = audioConf * 0.85f;
                }
            }

            // hats: the length decides open / closed when the name doesn't say
            if ((c == Cat::closedHat || c == Cat::openHat) && ! g.saysOpen && ! g.saysClosed)
                c = s.decayMs > 260 ? Cat::openHat : Cat::closedHat;

            // "808 kick" and kicks that are really 808s: long, tonal, low
            if (c == Cat::kick && g.says808 && s.decayMs > 380 && s.pitchConf > 0.5f)
                c = Cat::bass808;
            if (c == Cat::bass808 && g.saysKick && ! (s.decayMs > 380 && s.pitchConf > 0.5f))
                c = Cat::kick;

            // a long, steady, clearly pitched sound with no drum name is an instrument note, not a drum
            if (g.cat == Cat::unknown && s.duration > 1.5f && s.decayMs > 900.0f && s.pitchConf > 0.75f
                 && s.pitchHz > 150.0f && s.pitchHz < 2000.0f && s.flatness < 0.05f)
            {
                c = Cat::melodic;
                conf = 0.7f;
            }

            // loops and whole songs never go in a kit (vocal / instrument loops stay "Vocal / Melodic")
            if (c != Cat::melodic)
            if (s.duration > 12.0f || (s.onsets >= 4 && s.duration > 1.0f) || (g.loopWord && (s.onsets >= 2 || s.duration > 1.5f) && s.duration > 0.8f))
                if (! (c == Cat::fx && g.cat == Cat::fx))
                {
                    c = Cat::loop;
                    conf = 0.8f;
                }

            s.cat = c;
            s.catConfidence = conf;
        }

        // the kind of percussion
        if (s.cat == Cat::perc)
        {
            s.percKind = g.perc;
            if (s.percKind == PercKind::none)
            {
                if (s.pitchConf > 0.6f && s.pitchHz > 480 && s.decayMs < 500)       s.percKind = PercKind::cowbell;
                else if (s.pitchConf > 0.5f && s.pitchHz > 120 && s.pitchHz <= 480) s.percKind = PercKind::conga;
                else if (s.flatness > 0.25f && s.centroidHz > 4000)                 s.percKind = s.decayMs > 250 ? PercKind::tambourine : PercKind::shaker;
                else if (s.decayMs < 90 && s.centroidHz > 1500)                     s.percKind = PercKind::snap;
                else                                                               s.percKind = PercKind::other;
            }
        }
        else
            s.percKind = PercKind::none;

        computeTraits (s, g);
    }

    //==============================================================================
    bool analyzeFile (const juce::File& file, const juce::String& folders, juce::AudioFormatManager& fm, SoundInfo& s)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr || reader->sampleRate <= 0 || reader->lengthInSamples <= 0)
            return false;

        s.path = file.getFullPathName();
        s.fileSize = file.getSize();
        s.modTime = file.getLastModificationTime().toMilliseconds();
        s.sampleRate = reader->sampleRate;
        s.channels = (int) reader->numChannels;
        s.duration = (float) (reader->lengthInSamples / reader->sampleRate);
        s.genreTags = genreTagsForPath ((folders + "/" + file.getFileNameWithoutExtension()).toLowerCase());

        const auto guess = guessFromName (file.getFileNameWithoutExtension(), folders);

        if (s.duration > 30.0f)
        {
            // a song or a long loop: not worth reading
            if (! s.catByUser) { s.cat = Cat::loop; s.catConfidence = 0.9f; }
            return true;
        }

        const int n = (int) juce::jmin ((juce::int64) (reader->sampleRate * 8.0), reader->lengthInSamples);
        juce::AudioBuffer<float> buf ((int) juce::jmax (1u, reader->numChannels), n);
        if (! reader->read (&buf, 0, n, 0, true, true))
            return false;

        std::vector<float> mono ((size_t) n, 0.0f);
        const float scale = 1.0f / (float) buf.getNumChannels();
        for (int c = 0; c < buf.getNumChannels(); ++c)
        {
            const float* d = buf.getReadPointer (c);
            for (int i = 0; i < n; ++i) mono[(size_t) i] += d[i] * scale;
        }
        // a stereo file with opposite channels would cancel: fall back to the left channel
        float mPeak = 0, lPeak = 0;
        for (int i = 0; i < n; ++i) { mPeak = juce::jmax (mPeak, std::abs (mono[(size_t) i])); lPeak = juce::jmax (lPeak, std::abs (buf.getSample (0, i))); }
        if (lPeak > 0 && mPeak < lPeak * 0.25f)
            std::copy (buf.getReadPointer (0), buf.getReadPointer (0) + n, mono.begin());

        measure (mono.data(), n, reader->sampleRate, s);
        finish (s, guess);
        if (reader->bitsPerSample > 0 && reader->bitsPerSample <= 12)    // 8 / 12-bit: old sampler character
            s.traits.vintage = clamp01 (s.traits.vintage + 0.2f);
        return true;
    }
}
