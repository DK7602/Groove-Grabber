#include "SongAnalysis.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <numeric>

namespace gg
{
    double detectTempo (const float* x, int n, double sr)
    {
        // onset strength (spectral flux) at ~86 frames a second, then the best-repeating lag
        constexpr int order = 10, size = 1 << order;
        const int hop = juce::jmax (1, (int) (sr / 86.0));
        if (n < size * 4) return 0;
        juce::dsp::FFT fft (order);
        std::vector<float> win (size), buf (2 * size), prev (size / 2 + 1, 0.0f);
        for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * i / (size - 1));
        const int frames = (n - size) / hop;
        std::vector<float> flux ((size_t) frames);
        for (int f = 0; f < frames; ++f)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            for (int i = 0; i < size; ++i) buf[(size_t) i] = x[f * hop + i] * win[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (buf.data(), true);
            float s = 0;
            for (int k = 1; k <= size / 2; ++k)
            {
                const float m = std::log1p (buf[(size_t) k] * 10.0f);
                s += juce::jmax (0.0f, m - prev[(size_t) k]);
                prev[(size_t) k] = m;
            }
            flux[(size_t) f] = s;
        }
        // remove the slow trend
        const float mean = std::accumulate (flux.begin(), flux.end(), 0.0f) / (float) frames;
        for (auto& v : flux) v = juce::jmax (0.0f, v - mean);

        const double fps = sr / hop;
        double bestBpm = 0, bestScore = 0;
        for (double bpm = 60; bpm <= 190; bpm += 0.5)
        {
            const double lag = fps * 60.0 / bpm;
            double s = 0;
            for (int mult = 1; mult <= 4; ++mult)
            {
                const double l = lag * mult;
                const int li = (int) l;
                const double frac = l - li;
                double acc = 0;
                for (int f = 0; f + li + 1 < frames; ++f)
                    acc += flux[(size_t) f] * ((1 - frac) * flux[(size_t) (f + li)] + frac * flux[(size_t) (f + li + 1)]);
                s += acc / mult;
            }
            // gentle preference for the usual 80..140 range
            s *= std::exp (-0.5 * std::pow (std::log2 (bpm / 110.0) / 0.9, 2.0));
            if (s > bestScore) { bestScore = s; bestBpm = bpm; }
        }
        return std::round (bestBpm);
    }

    int detectKey (const float* x, int n, double sr, float& confidence)
    {
        constexpr int order = 13, size = 1 << order;
        confidence = 0;
        if (n < size) return -1;
        juce::dsp::FFT fft (order);
        std::vector<float> win (size), buf (2 * size);
        for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * i / (size - 1));
        std::array<double, 12> chroma {};
        for (int start = 0; start + size <= n; start += size / 2)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            for (int i = 0; i < size; ++i) buf[(size_t) i] = x[start + i] * win[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (buf.data(), true);
            for (int k = 1; k <= size / 2; ++k)
            {
                const double f = k * sr / size;
                if (f < 55 || f > 4200) continue;
                const double midi = 69 + 12 * std::log2 (f / 440.0);
                const int pc = ((int) std::round (midi) % 12 + 12) % 12;
                chroma[(size_t) pc] += std::sqrt ((double) buf[(size_t) k]);
            }
        }
        static const double major[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
        static const double minor[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
        auto corr = [&] (const double* prof, int tonic)
        {
            double mx = 0, my = 0;
            for (int i = 0; i < 12; ++i) { mx += chroma[(size_t) ((i + tonic) % 12)]; my += prof[i]; }
            mx /= 12; my /= 12;
            double num = 0, dx = 0, dy = 0;
            for (int i = 0; i < 12; ++i)
            {
                const double a = chroma[(size_t) ((i + tonic) % 12)] - mx, b = prof[i] - my;
                num += a * b; dx += a * a; dy += b * b;
            }
            return dx > 0 && dy > 0 ? num / std::sqrt (dx * dy) : 0.0;
        };
        int best = -1;
        double bestR = -2, second = -2;
        for (int k = 0; k < 24; ++k)
        {
            const double r = corr (k < 12 ? major : minor, k % 12);
            if (r > bestR) { second = bestR; bestR = r; best = k; }
            else if (r > second) second = r;
        }
        confidence = (float) juce::jlimit (0.0, 1.0, (bestR - second) * 6.0 + bestR * 0.3);
        return best;
    }

    SongFacts analyseSong (const juce::File& file, const std::function<bool()>& shouldStop)
    {
        SongFacts s;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr) { s.error = "Couldn't read " + file.getFileName(); return s; }
        // up to 90 seconds from the middle-ish of the song, at its own rate
        const double sr = reader->sampleRate;
        const juce::int64 total = reader->lengthInSamples;
        const juce::int64 want = (juce::int64) (sr * 90.0);
        const juce::int64 start = total > want ? juce::jmin ((juce::int64) (sr * 20.0), total - want) : 0;
        const int n = (int) juce::jmin (want, total - start);
        if (n < (int) (sr * 4.0)) { s.error = "That song is too short to measure."; return s; }
        juce::AudioBuffer<float> b ((int) juce::jmax (1u, reader->numChannels), n);
        reader->read (&b, 0, n, start, true, true);
        std::vector<float> mono ((size_t) n, 0.0f);
        for (int c = 0; c < b.getNumChannels(); ++c)
            for (int i = 0; i < n; ++i) mono[(size_t) i] += b.getSample (c, i) / (float) b.getNumChannels();
        if (shouldStop && shouldStop()) return s;
        s.bpm = detectTempo (mono.data(), n, sr);
        if (shouldStop && shouldStop()) return s;
        s.key = detectKey (mono.data(), n, sr, s.keyConfidence);
        s.ok = s.bpm > 0 || s.key >= 0;
        return s;
    }
}
