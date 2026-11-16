#include "SampleRender.h"
#include "signalsmith-stretch/signalsmith-stretch.h"   // (ThirdParty: MIT)
#include "Analyzer.h"
#include <cmath>

namespace gg
{
    std::shared_ptr<const AudioCache::Entry> AudioCache::get (const juce::String& path)
    {
        std::lock_guard<std::mutex> l (lock);
        ++counter;
        for (auto& it : items)
            if (it.path == path) { it.lastUse = counter; return it.entry; }

        if (! formatsReady) { formats.registerBasicFormats(); formatsReady = true; }
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (path)));
        if (reader == nullptr || reader->sampleRate <= 0) return nullptr;

        auto e = std::make_shared<Entry>();
        e->sampleRate = reader->sampleRate;
        const int n = (int) juce::jmin ((juce::int64) (reader->sampleRate * 20.0), reader->lengthInSamples);
        if (n <= 0) return nullptr;
        e->audio.setSize (2, n);
        e->audio.clear();
        juce::AudioBuffer<float> tmp ((int) juce::jmax (1u, reader->numChannels), n);
        reader->read (&tmp, 0, n, 0, true, true);
        e->audio.copyFrom (0, 0, tmp, 0, 0, n);
        e->audio.copyFrom (1, 0, tmp, juce::jmin (1, tmp.getNumChannels() - 1), 0, n);

        items.push_back ({ path, e, counter });
        totalSamples += (size_t) n * 2;
        constexpr size_t maxSamples = 40u * 1000u * 1000u;     // ~160 MB of floats
        while (totalSamples > maxSamples && items.size() > 1)
        {
            auto oldest = std::min_element (items.begin(), items.end(), [] (auto& a, auto& b) { return a.lastUse < b.lastUse; });
            totalSamples -= (size_t) oldest->entry->audio.getNumSamples() * 2;
            items.erase (oldest);
        }
        return e;
    }

    void AudioCache::clear()
    {
        std::lock_guard<std::mutex> l (lock);
        items.clear();
        totalSamples = 0;
    }

    namespace render
    {
        float targetLoudnessDb (Cat c) noexcept
        {
            switch (c)
            {
                case Cat::kick:      return -10.0f;
                case Cat::bass808:   return -11.0f;
                case Cat::snare:     return -11.0f;
                case Cat::clap:      return -12.0f;
                case Cat::rim:       return -15.0f;
                case Cat::closedHat: return -17.0f;
                case Cat::openHat:   return -16.5f;
                case Cat::tom:       return -12.0f;
                case Cat::crash:     return -16.0f;
                case Cat::ride:      return -17.5f;
                case Cat::perc:      return -15.0f;
                case Cat::fx:        return -15.0f;
                default:             return -14.0f;
            }
        }

        float leadingSilenceMs (const juce::AudioBuffer<float>& b, double sr, float threshold)
        {
            float peak = 0;
            for (int c = 0; c < b.getNumChannels(); ++c) peak = juce::jmax (peak, b.getMagnitude (c, 0, b.getNumSamples()));
            if (peak <= 0) return 0;
            int first = b.getNumSamples();
            for (int c = 0; c < b.getNumChannels(); ++c)
            {
                const float* d = b.getReadPointer (c);
                for (int i = 0; i < first; ++i)
                    if (std::abs (d[i]) >= peak * threshold) { first = i; break; }
            }
            // keep half a millisecond so the attack is never clipped
            return (float) (juce::jmax (0.0, first - sr * 0.0005) * 1000.0 / sr);
        }

        juce::AudioBuffer<float> sound (const juce::String& path, Cat cat, const RenderParams& p, double outSr)
        {
            static juce::SharedResourcePointer<AudioCache> cache;   // one per process
            auto src = cache->get (path);
            if (src == nullptr || src->audio.getNumSamples() == 0) return {};

            const double sr = src->sampleRate;
            const auto& a = src->audio;
            const int total = a.getNumSamples();

            // ---- trim the silence in front, then your START ----
            int start = (int) (leadingSilenceMs (a, sr, cat == Cat::fx ? 0.001f : 0.01f) * sr / 1000.0);
            start += (int) (juce::jlimit (0.0f, 2000.0f, std::isfinite (p.startMs) ? p.startMs : 0.0f) * sr / 1000.0);
            start = juce::jlimit (0, juce::jmax (0, total - 1), start);
            int len = total - start;
            if (len <= 4) return {};

            // ---- copy (reversed if asked) ----
            juce::AudioBuffer<float> seg (2, len + 512);
            seg.clear();
            for (int c = 0; c < 2; ++c)
            {
                const float* in = a.getReadPointer (c, start);
                float* out = seg.getWritePointer (c);
                if (p.reverse) for (int i = 0; i < len; ++i) out[i] = in[len - 1 - i];
                else           std::copy (in, in + len, out);
            }

            // ---- tune + sample-rate conversion in one resampling pass ----
            const double tune = std::isfinite (p.tune) ? juce::jlimit (-24.0, 24.0, (double) p.tune) : 0.0;
            const double ratio = juce::jlimit (1.0 / 16.0, 16.0, std::pow (2.0, tune / 12.0) * sr / outSr);
            const int outLen = (int) std::floor (len / ratio);
            if (outLen < 8) return {};
            juce::AudioBuffer<float> out (2, outLen);
            if (std::abs (ratio - 1.0) < 1.0e-6)
            {
                for (int c = 0; c < 2; ++c) out.copyFrom (c, 0, seg, c, 0, outLen);
            }
            else
            {
                // the sinc filter lags its input by 100 samples: make that many extra and drop them
                const int skip = (int) std::round (juce::WindowedSincInterpolator::getBaseLatency() / ratio);
                std::vector<float> tmp ((size_t) (outLen + skip));
                for (int c = 0; c < 2; ++c)
                {
                    juce::WindowedSincInterpolator interp;
                    interp.process (ratio, seg.getReadPointer (c), tmp.data(), outLen + skip);
                    std::copy (tmp.begin() + skip, tmp.end(), out.getWritePointer (c));
                }
            }

            // ---- LENGTH: keep part of it, fade the end ----
            const float keepFrac = juce::jlimit (0.02f, 1.0f, p.lengthPct / 100.0f * p.lengthScale);
            int keep = juce::jmax (8, (int) (outLen * keepFrac));
            keep = juce::jmin (keep, outLen);
            // and stop at the true end of the sound (drop trailing silence)
            {
                float peak = 0;
                for (int c = 0; c < 2; ++c) peak = juce::jmax (peak, out.getMagnitude (c, 0, keep));
                int last = keep - 1;
                const float floorLevel = peak * 0.0003f;   // -70 dB
                while (last > 16 && std::abs (out.getSample (0, last)) < floorLevel && std::abs (out.getSample (1, last)) < floorLevel) --last;
                keep = juce::jmin (keep, last + (int) (outSr * 0.005) + 1);
                keep = juce::jlimit (8, outLen, keep);
            }
            juce::AudioBuffer<float> res (2, keep);
            for (int c = 0; c < 2; ++c) res.copyFrom (c, 0, out, c, 0, keep);

            const bool cut = keepFrac < 0.999f || p.reverse;
            const int fadeOut = cut ? juce::jmin ((int) (outSr * 0.025), keep * 3 / 10) : juce::jmin ((int) (outSr * 0.004), keep / 10);
            for (int i = 0; i < fadeOut; ++i)
            {
                const float g = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) (i + 1) / (float) fadeOut);
                for (int c = 0; c < 2; ++c) res.setSample (c, keep - fadeOut + i, res.getSample (c, keep - fadeOut + i) * g);
            }
            if (p.startMs > 0 || p.reverse)
            {
                const int fadeIn = juce::jmin ((int) (outSr * 0.001), keep / 4);
                for (int i = 0; i < fadeIn; ++i)
                    for (int c = 0; c < 2; ++c) res.setSample (c, i, res.getSample (c, i) * (float) i / (float) fadeIn);
            }

            // ---- FIT: so many bars at the song's tempo ----
            if (p.fitBeats > 0 && p.fitBpm > 0 && std::isfinite (p.fitBeats) && std::isfinite (p.fitBpm))
            {
                fitTo (res, outSr, p.fitBeats * 60.0 / juce::jlimit (40.0, 250.0, p.fitBpm), p.fitAnchor, p.fitStretch);
                keep = res.getNumSamples();
            }

            // ---- level: matched for its kind, then your GAIN; never over -0.5 dBFS ----
            float gainDb = juce::jlimit (-60.0f, 24.0f, std::isfinite (p.gainDb + p.padGainDb) ? p.gainDb + p.padGainDb : 0.0f);
            if (p.normalise)
            {
                const float* ch[2] = { res.getReadPointer (0), res.getReadPointer (1) };
                const float loud = analysis::shortTermLoudnessDb (ch, 2, keep, outSr);
                if (loud > -90.0f)
                    gainDb += juce::jlimit (-30.0f, 30.0f, targetLoudnessDb (cat) - loud);
            }
            float g = juce::Decibels::decibelsToGain (gainDb);
            float peak = juce::jmax (res.getMagnitude (0, 0, keep), res.getMagnitude (1, 0, keep)) * g;
            if (peak > 0.944f) g *= 0.944f / peak;
            if (! std::isfinite (g)) g = 0.0f;
            res.applyGain (g);
            return res;
        }

        int anchorSample (const juce::AudioBuffer<float>& b, int anchor)
        {
            const int n = b.getNumSamples();
            if (n == 0) return 0;
            float peak = 0;
            int peakAt = 0;
            for (int i = 0; i < n; ++i)
            {
                float v = 0;
                for (int c = 0; c < juce::jmin (2, b.getNumChannels()); ++c) v = juce::jmax (v, std::abs (b.getSample (c, i)));
                if (v > peak) { peak = v; peakAt = i; }
            }
            if (anchor == 1) return peakAt;
            if (anchor == 2) return 0;
            int end = n;
            while (end > peakAt + 1)
            {
                float v = 0;
                for (int c = 0; c < juce::jmin (2, b.getNumChannels()); ++c) v = juce::jmax (v, std::abs (b.getSample (c, end - 1)));
                if (v >= peak * 0.01f) break;
                --end;
            }
            return end;
        }

        namespace
        {
            // a time-stretch of the whole sound, its pitch kept (Signalsmith Stretch)
            juce::AudioBuffer<float> stretched (const juce::AudioBuffer<float>& in, double sampleRate, int outSamples)
            {
                const int ch = juce::jlimit (1, 2, in.getNumChannels());
                juce::AudioBuffer<float> out (ch, juce::jmax (1, outSamples));
                out.clear();
                signalsmith::stretch::SignalsmithStretch<float> st;
                st.presetDefault (ch, (float) sampleRate);
                std::vector<const float*> ins;
                std::vector<float*> outs;
                for (int c = 0; c < ch; ++c) { ins.push_back (in.getReadPointer (c)); outs.push_back (out.getWritePointer (c)); }
                if (! st.exact (ins.data(), in.getNumSamples(), outs.data(), outSamples))
                {
                    // too short for a stretch: as it is, silence after
                    for (int c = 0; c < ch; ++c) out.copyFrom (c, 0, in, c, 0, juce::jmin (outSamples, in.getNumSamples()));
                }
                return out;
            }

            void fade (juce::AudioBuffer<float>& b, int start, int length, bool in)
            {
                length = juce::jmin (length, b.getNumSamples() - start);
                for (int i = 0; i < length; ++i)
                {
                    const float x = (float) (i + 1) / (float) (length + 1);
                    const float g = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (in ? x : 1.0f - x));
                    for (int c = 0; c < b.getNumChannels(); ++c) b.setSample (c, start + i, b.getSample (c, start + i) * g);
                }
            }

            juce::AudioBuffer<float> slice (const juce::AudioBuffer<float>& b, int from, int length, int silenceBefore = 0)
            {
                juce::AudioBuffer<float> out (b.getNumChannels(), juce::jmax (1, silenceBefore + length));
                out.clear();
                for (int c = 0; c < b.getNumChannels(); ++c)
                    out.copyFrom (c, silenceBefore, b, c, from, juce::jmin (length, b.getNumSamples() - from));
                return out;
            }
        }

        double fitTo (juce::AudioBuffer<float>& b, double sr, double seconds, int anchor, bool stretch)
        {
            const int target = (int) std::round (seconds * sr);
            if (target < 64 || b.getNumSamples() < 64) return 1.0;
            double factor = 1.0;
            if (anchor == 2)
            {
                // its start on the bar line: the sound lasts the bars
                const int body = anchorSample (b, 0);
                if (body > target)
                {
                    b = slice (b, 0, target);
                    fade (b, target - juce::jmin (target / 4, (int) (sr * 0.12)), juce::jmin (target / 4, (int) (sr * 0.12)), false);
                }
                else if (body < target && stretch)
                {
                    factor = juce::jmin (4.0, (double) target / (double) juce::jmax (1, body));
                    b = stretched (slice (b, 0, body), sr, (int) std::round (body * factor));
                }
                return factor;
            }
            // its end (or peak) on the bar line, `target` samples in
            int at = anchorSample (b, anchor);
            if (at < 1) return 1.0;
            if (at < target && stretch)
            {
                factor = juce::jmin (4.0, (double) target / (double) at);
                b = stretched (b, sr, (int) std::round (b.getNumSamples() * factor));
                at = anchorSample (b, anchor);
            }
            if (at > target)
            {
                // too long: from the front, so the end stays (a short fade in where it's cut)
                b = slice (b, at - target, b.getNumSamples() - (at - target));
                fade (b, 0, juce::jmin (target / 8, (int) (sr * 0.03)), true);
            }
            else if (at < target)
            {
                // short of it still: silence in front
                b = slice (b, 0, b.getNumSamples(), target - at);
            }
            return factor;
        }

        std::vector<std::pair<float, float>> peaks (const juce::AudioBuffer<float>& b, int numPoints)
        {
            std::vector<std::pair<float, float>> out ((size_t) juce::jmax (1, numPoints), { 0.0f, 0.0f });
            const int n = b.getNumSamples();
            if (n == 0 || b.getNumChannels() == 0) return out;
            for (int i = 0; i < numPoints; ++i)
            {
                const int a = (int) ((juce::int64) i * n / numPoints);
                const int e = juce::jmax (a + 1, (int) ((juce::int64) (i + 1) * n / numPoints));
                float mn = 0, mx = 0;
                for (int c = 0; c < juce::jmin (2, b.getNumChannels()); ++c)
                {
                    auto r = b.findMinMax (c, a, juce::jmin (e, n) - a);
                    mn = juce::jmin (mn, r.getStart());
                    mx = juce::jmax (mx, r.getEnd());
                }
                out[(size_t) i] = { mn, mx };
            }
            return out;
        }
    }
}
