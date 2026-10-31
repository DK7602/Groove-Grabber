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

        WordHit catFromTokens (const juce::StringArray& tokens, NameGuess& g)
        {
            // words run together ("realclaps", "trapsnare", "snarehard"): the unmistakable drum words inside
            // a longer token count as words of their own
            juce::StringArray t (tokens);
            for (auto& tok : tokens)
                if (tok.length() >= 6)
                    for (auto* w : { "kick", "snare", "clap", "hihat", "cymbal", "shaker", "cowbell", "rimshot", "conga", "bongo", "tambourine", "crash" })
                        if (tok.contains (w) && tok != w && tok != juce::String (w) + "s")
                            t.addIfNotAlreadyThere (w);
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
            if (hasAny (t, { "snare", "snares", "snr", "sd", "sn", "snre", "snaredrum" })) set (Cat::snare, 0.9f);
            if (hasAny (t, { "clap", "claps", "clp", "handclap", "handclaps" })) set (Cat::clap, 0.92f);
            if (t.contains ("cp"))                                          set (Cat::clap, 0.85f);    // drum-machine shorthand
            if (hasAny (t, { "rim", "rims", "rimshot", "sidestick", "crossstick" }) || phrase (joined, "side stick") || phrase (joined, "cross stick"))
                set (Cat::rim, 0.92f);
            if (t.contains ("rs"))                                          set (Cat::rim, 0.85f);
            if (t.contains ("stick") || t.contains ("sticks"))              set (Cat::rim, 0.55f);
            if (hatWord)
            {
                if (g.saysOpen && ! t.contains ("closed"))                  set (Cat::openHat, 0.93f);
                else                                                        set (Cat::closedHat, g.saysClosed ? 0.93f : 0.75f);
            }
            if (hasAny (t, { "tom", "toms", "ftom", "floortom", "racktom", "rtom", "lowtom", "midtom", "hitom", "hightom" }) || phrase (joined, "floor tom"))
                set (Cat::tom, 0.9f);
            if (hasAny (t, { "lt", "mt", "ht", "ft" }))                    set (Cat::tom, 0.84f);     // low / mid / high / floor tom
            if (hasAny (t, { "crash", "crashes", "splash", "china", "cymbal", "cymbals", "cym", "cymb" })) set (Cat::crash, 0.88f);
            if (t.contains ("cy"))                                          set (Cat::crash, 0.84f);
            if (hasAny (t, { "ride", "rides" }))                            set (Cat::ride, 0.93f);
            if (t.contains ("rd"))                                          set (Cat::ride, 0.84f);
            if (hasAny (t, { "sub", "subs", "subbass", "bass", "basses", "808bass" }) && ! phrase (joined, "bass drum"))
            {
                set (Cat::bass808, 0.8f);
                g.saysBass = true;
            }

            // percussion, with its kind
            if (hasAny (t, { "shaker", "shakers", "shk", "shake", "maraca", "maracas", "marc", "mrc", "cabasa", "egg" })) set (Cat::perc, 0.9f, PercKind::shaker);
            if (hasAny (t, { "tamb", "tambo", "tambourine", "tambourines", "tamborine", "jingle" }))     set (Cat::perc, 0.9f, PercKind::tambourine);
            if (hasAny (t, { "conga", "congas", "cng", "bongo", "bongos", "djembe", "tabla", "timbale", "timbales", "cajon", "darbuka", "lc", "lcg", "mcg", "hcg" }))
                set (Cat::perc, 0.9f, PercKind::conga);
            if (hasAny (t, { "cowbell", "cowbells", "cowb", "cow", "cb", "agogo" }))                          set (Cat::perc, 0.9f, PercKind::cowbell);
            if (hasAny (t, { "snap", "snaps", "fingersnap", "fingersnaps", "finger" }))                set (Cat::perc, 0.9f, PercKind::snap);
            if (hasAny (t, { "block", "woodblock", "clave", "claves", "clv", "knock", "wood" }))              set (Cat::perc, 0.85f, PercKind::block);
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

            // "808" on its own is the bass; "808 snare", "808 clap", "808 LT" or a "TR-808 Kit" folder is the drum machine
            if (g.says808 && h.cat == Cat::unknown
                 && ! hasAny (t, { "kit", "kits", "drum", "drums", "machine", "machines", "tr", "rhythm", "perc", "percussion", "oneshots" }))
                set (Cat::bass808, 0.88f);
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
            g.saysBass = g.saysBass || (fg.saysBass && hit.cat == Cat::bass808);
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

            // a drum's overtones aren't harmonic (a tom's ring at 1.59x and 2.14x its note), so the deepest dip
            // can land on a "period" two notes long - an octave too low. An earlier dip that is nearly as deep
            // is the real note.
            if (cmnd[(size_t) best] < 0.35f)
                for (int tau = juce::jmax (minLag + 1, best / 4); tau < best * 3 / 4; ++tau)
                    if (cmnd[(size_t) tau] < cmnd[(size_t) tau - 1] && cmnd[(size_t) tau] <= cmnd[(size_t) tau + 1]
                         && cmnd[(size_t) tau] < cmnd[(size_t) best] + 0.06f)
                    {
                        best = tau;
                        break;
                    }
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
        info.measureVersion = SoundInfo::currentMeasure;
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

        // how long it rings: to -40 dB, or to just above the hiss / vinyl floor if that's louder
        {
            std::vector<float> after (env.begin() + maxF, env.end());
            float floorLevel = 0;
            if (after.size() > 20)
            {
                std::nth_element (after.begin(), after.begin() + (long) after.size() / 10, after.end());
                floorLevel = after[after.size() / 10];
            }
            const float thresh = juce::jmin (envMax * 0.1f, juce::jmax (envMax * 0.01f, floorLevel * 2.0f));
            int ringF = frames - 1;
            while (ringF > maxF && env[(size_t) ringF] < thresh) --ringF;
            info.ringMs = (float) (ringF - maxF);
        }

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
        double total = 0, weighted = 0, sub = 0, low = 0, mid = 0, high = 0, maxP = 0;
        for (int k = 1; k <= Spectrum::size / 2; ++k)
        {
            const double f = k * binHz;
            if (f < 20 || f > 20000) continue;
            const double p = sp.power[(size_t) k];
            total += p;
            weighted += f * p;
            maxP = juce::jmax (maxP, p);
            if (f < 70) sub += p;
            else if (f < 250) low += p;
            else if (f < 2500) mid += p;
            else if (f >= 5000) high += p;
        }

        // flatness (1 = noise, 0 = pure tones), only over the band the sound actually covers: an old 8-bit
        // or band-limited sample has empty top octaves, which would otherwise read as "tonal"
        double edgeHz = juce::jmin (16000.0, sr * 0.45);
        if (total > 0)
        {
            double cum = 0;
            for (int k = 1; k <= Spectrum::size / 2; ++k)
            {
                cum += sp.power[(size_t) k];
                if (cum >= 0.995 * total) { edgeHz = juce::jmin (edgeHz, juce::jmax (1000.0, k * binHz)); break; }
            }
        }
        auto flatnessBetween = [&] (double loHz, double hiHz)
        {
            const double fl = maxP * 1.0e-7 + 1.0e-14;
            double logSum = 0, linSum = 0;
            int bins = 0;
            for (int k = (int) std::ceil (loHz / binHz); k <= juce::jmin (Spectrum::size / 2, (int) (hiHz / binHz)); ++k)
            {
                const double p = sp.power[(size_t) k] + fl;
                logSum += std::log (p);
                linSum += p;
                ++bins;
            }
            return bins > 4 ? (float) (std::exp (logSum / bins) / (linSum / bins)) : 0.0f;
        };
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
        info.flatness = flatnessBetween (100.0, edgeHz);
        info.airFlatness = edgeHz > 3500.0 ? flatnessBetween (2000.0, edgeHz) : 0.0f;

        // separate bursts in the first 60 ms (a clap's 3 or 4 hands). A burst only counts after a real gap:
        // the level has to fall to half (-6 dB) and climb back to twice that. The flutter of plain noise
        // (a snare's wires, a shaker) never does that.
        if (info.centroidHz > 700.0f && info.subShare + info.lowShare < 0.4f)
        {
            const float fastPeak = *std::max_element (fastEnv.begin(), fastEnv.end());
            int bursts = 0;
            bool inBurst = false;
            float burstPeak = 0, dip = 0;
            const int from = juce::jmax (0, onsetF - 5), end = juce::jmin (frames, onsetF + 60);
            for (int f = from; f < end; ++f)
            {
                const float e = fastEnv[(size_t) f];
                if (! inBurst)
                {
                    if (e >= fastPeak * 0.3f && (bursts == 0 || e >= dip * 2.0f)) { inBurst = true; ++bursts; burstPeak = e; }
                    else dip = juce::jmin (dip, e);
                }
                else
                {
                    burstPeak = juce::jmax (burstPeak, e);
                    if (e < burstPeak * 0.5f) { inBurst = false; dip = e; }
                }
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

            // the note it settles on once any pitch drop or slide is over - what an 808 has to be tuned by
            // (an 808 that starts an octave up and falls for 50 ms reads several notes sharp over the whole
            // hit). Pitch in overlapping windows along the body; the steady stretch at the end wins.
            if (info.pitchConf > 0.4f)
            {
                const int win = (int) (sr * 0.12), step = (int) (sr * 0.04);
                std::vector<std::pair<float, float>> notes;     // (midi, confidence)
                for (int start = onsetSample + (int) (sr * 0.02); start + win <= soundEnd && start < onsetSample + (int) (sr * 0.9); start += step)
                {
                    const int fr = juce::jlimit (0, frames - 1, (start + win / 2) / hop);
                    if (env[(size_t) fr] < envMax * 0.1f) break;        // faded 20 dB: the tail can fool it
                    float c = 0;
                    const float p = detectPitch (x + start, win, sr, 28.0f, 1000.0f, c);
                    if (p > 0 && c > 0.6f) notes.emplace_back (69.0f + 12.0f * std::log2 (p / 440.0f), c);
                }
                if (notes.size() >= 2)
                {
                    // walk back from the last window while the note holds (within half a semitone)
                    const float last = notes.back().first;
                    std::vector<float> steady;
                    float confSum = 0;
                    for (auto it = notes.rbegin(); it != notes.rend() && std::abs (it->first - last) < 0.5f; ++it)
                    {
                        steady.push_back (it->first);
                        confSum += it->second;
                    }
                    if (steady.size() >= 2 || notes.size() == 2)
                    {
                        std::nth_element (steady.begin(), steady.begin() + (long) steady.size() / 2, steady.end());
                        const float note = steady[steady.size() / 2];
                        // a drop or a slide only ever lands lower, and not by more than an octave and a bit;
                        // anything else is the window being fooled
                        const float overall = 69.0f + 12.0f * std::log2 (info.pitchHz / 440.0f);
                        if (overall - note > -1.0f && overall - note < 8.5f)
                        {
                            info.pitchHz = 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
                            info.pitchConf = juce::jmax (info.pitchConf, confSum / (float) steady.size());
                        }
                    }
                }
            }

            // the pitch right at the start: a kick sweeps down from an octave or more above, a tom barely moves
            info.pitchGlide = 0;
            const int eFrom = onsetSample + (int) (sr * 0.002), eLen = juce::jmin ((int) (sr * 0.045), soundEnd - eFrom);
            if (info.pitchConf > 0.45f && eLen > (int) (sr * 0.03))
            {
                float econf = 0;
                const float early = detectPitch (x + eFrom, eLen, sr, 40.0f, 1200.0f, econf);
                if (early > 0 && econf > 0.45f)
                    info.pitchGlide = juce::jlimit (-12.0f, 36.0f, 12.0f * std::log2 (early / info.pitchHz));
            }
        }
    }

    //==============================================================================
    Cat classifyAudio (const SoundInfo& s, float& confidence)
    {
        // Rules checked against ~1,900 labelled one-shots from 90 drum machines and kits (with their names
        // hidden). Each sound gets a 0..1 score per category; the best one wins.
        const float sub = s.subShare, low = s.lowShare, mid = s.midShare, high = s.highShare;
        const float lowAll = sub + low;
        const float mh = mid + high;
        const float air = juce::jmax (0.0f, 1.0f - lowAll - mid);          // everything above 2.5 kHz
        const float cen = s.centroidHz, decay = s.decayMs, ring = s.ringMs, flat = s.flatness, af = s.airFlatness;
        const float ph = s.pitchHz, pc = s.pitchConf, att = s.attackMs, dur = s.duration;
        const bool pitched = pc > 0.4f, clearPitch = pc > 0.55f;
        const int bursts = s.burstCount;
        std::array<float, numCats> sc {};

        // ---- low drums ----
        // a kick: a low fundamental (they sit around 45-75 Hz) or real sub, and next to nothing up top
        const float kickLow = clearPitch ? juce::jmax (fall (ph, 68, 92), rise (sub, 0.2f, 0.4f)) : 1.0f;
        sc[(size_t) Cat::kick] = rise (lowAll, 0.35f, 0.7f) * fall (cen, 400, 1800) * fall (decay, 600, 1300) * kickLow
                               * fall (mh, 0.2f, 0.45f) * fall (high, 0.06f, 0.15f);

        // an 808: low, clean and long
        sc[(size_t) Cat::bass808] = rise (lowAll, 0.5f, 0.85f) * fall (cen, 250, 800) * juce::jmax (rise (decay, 400, 800), rise (ring, 800, 1400))
                                  * (0.4f + 0.6f * rise (pc, 0.4f, 0.75f)) * (pitched ? fall (ph, 75, 110) : 1.0f) * fall (mh, 0.06f, 0.2f);

        // a tom: a clear note above a kick's, ringing on
        sc[(size_t) Cat::tom] = rise (pc, 0.45f, 0.75f) * rise (ph, 62, 85) * fall (ph, 350, 500) * fall (cen, 1500, 3500)
                              * rise (decay, 60, 140) * fall (decay, 2500, 4000) * fall (sub, 0.3f, 0.6f)
                              * (0.8f + 0.2f * rise (mh, 0.02f, 0.08f)) * fall (mh, 0.15f, 0.3f) * (0.7f + 0.3f * rise (ring, 200, 450));

        // ---- cymbals: all air, no body. Closed < open < crash by how long they ring ----
        const float metal = rise (air, 0.3f, 0.55f) * fall (lowAll, 0.04f, 0.12f) * rise (cen, 2500, 4000);
        const float hat = metal * rise (cen, 4000, 6000) * rise (high, 0.3f, 0.5f) * rise (flat, 0.05f, 0.15f);
        const float cymbal = juce::jmax (rise (ring, 800, 1400), rise (dur, 0.9f, 1.4f) * rise (ring, 400, 650));
        sc[(size_t) Cat::closedHat] = hat * fall (ring, 200, 400) * fall (decay, 160, 350) * fall (att, 20, 35);
        sc[(size_t) Cat::openHat] = hat * juce::jmax (rise (ring, 220, 380), rise (decay, 200, 350)) * (1.0f - 0.85f * cymbal);
        sc[(size_t) Cat::crash] = metal * cymbal * rise (flat, 0.05f, 0.12f);
        sc[(size_t) Cat::ride] = metal * cymbal * fall (flat, 0.1f, 0.2f) * 0.9f;

        // ---- snare: a body plus a noise (the wires). Without a body it has to be short ----
        const float noisy = juce::jmax (rise (air, 0.03f, 0.12f) * juce::jmax (rise (flat, 0.04f, 0.12f), rise (af, 0.25f, 0.4f)),
                                        rise (mid, 0.25f, 0.5f) * rise (flat, 0.12f, 0.25f));
        const float bodyOrBright = juce::jmax (rise (lowAll, 0.1f, 0.3f), rise (cen, 1500, 2500) * rise (flat, 0.1f, 0.2f) * fall (ring, 400, 800));
        sc[(size_t) Cat::snare] = noisy * bodyOrBright * fall (high, 0.6f, 0.8f) * fall (cen, 7000, 10500) * fall (lowAll, 0.9f, 0.98f)
                                * rise (decay, 25, 55) * fall (decay, 1200, 2000) * (bursts >= 3 ? 0.5f : 1.0f);

        // ---- clap: mid-band noise in separate bursts, almost no low end, not much air ----
        sc[(size_t) Cat::clap] = rise (cen, 800, 1500) * fall (cen, 3500, 5500) * rise (flat, 0.05f, 0.12f) * fall (lowAll, 0.12f, 0.3f)
                               * fall (high, 0.25f, 0.45f) * (bursts >= 3 ? 1.1f : (bursts == 2 ? 0.7f : 0.3f)) * fall (decay, 400, 900);

        // ---- rim: a short, mid-band click ----
        // (a tonal click is a rim only around an 808 rim's note: higher is a clave or a block, lower a conga)
        const float rimTone = flat >= 0.1f ? 1.0f : (pc > 0.6f ? rise (ph, 300, 380) * fall (ph, 650, 800) : fall (cen, 1200, 1800));
        sc[(size_t) Cat::rim] = fall (decay, 45, 90) * fall (ring, 120, 220) * rise (cen, 220, 600) * fall (cen, 6000, 9000)
                              * fall (flat, 0.25f, 0.45f) * fall (lowAll, 0.5f, 0.9f) * rimTone;

        // ---- percussion: shakers (noise with a soft attack), bells / claves / blocks / congas (clear tones) ----
        const float shaker = rise (air, 0.5f, 0.8f) * rise (att, 15, 25) * fall (ring, 300, 600);
        const float tone = juce::jmax ((pc > 0.6f ? 1.0f : 0.0f) * rise (ph, 300, 420), fall (flat, 0.02f, 0.05f) * rise (cen, 800, 1500))
                         * fall (lowAll, 0.1f, 0.3f) * fall (decay, 400, 900);
        sc[(size_t) Cat::perc] = 0.22f + juce::jmax (0.4f * shaker, 0.45f * tone);

        // ---- effects: long and evolving, or a slow build (risers, reversed hits) ----
        sc[(size_t) Cat::fx] = juce::jmax (rise (dur, 1.5f, 3.0f) * rise (decay, 1500, 3000) * fall (cen, 2500, 5000) * fall (flat, 0.15f, 0.3f),
                                           rise (att, 80, 350));

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

            // hats: how long it rings decides open / closed when the name doesn't say
            if ((c == Cat::closedHat || c == Cat::openHat) && ! g.saysOpen && ! g.saysClosed)
                c = s.ringMs > 300 ? Cat::openHat : Cat::closedHat;

            // "808" alone could be the drum machine ("808 Cow", "808 MC"): with no low end it isn't the bass
            if (c == Cat::bass808 && g.cat == Cat::bass808 && ! g.saysBass && audio != Cat::bass808 && audio != Cat::kick
                 && (s.subShare + s.lowShare < 0.4f || s.centroidHz > 700.0f))
            {
                c = audio;
                conf = audioConf * 0.9f;
            }

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
