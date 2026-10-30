#include "Harvester.h"
#include "Analyzer.h"
#include "Settings.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <numeric>

namespace gg
{
    std::vector<int> findOnsets (const float* x, int n, double sr)
    {
        // energy in 5 ms windows every 2.5 ms; a hit = the level jumping 9 dB over the quietest point of the
        // previous 30 ms. Works for soft hits as well as loud ones (it's relative), and a ringing drum's
        // tail or pitch sweep never counts as a new hit (its level only falls).
        std::vector<int> onsets;
        const int hop = juce::jmax (1, (int) (sr * 0.0025));
        const int win = hop * 2;
        const int frames = n / hop;
        if (frames < 4) return onsets;
        std::vector<float> e ((size_t) frames);
        float maxDb = -200;
        for (int f = 0; f < frames; ++f)
        {
            double sum = 0;
            const int a0 = f * hop, a1 = juce::jmin (n, a0 + win);
            for (int i = a0; i < a1; ++i) sum += (double) x[i] * x[i];
            const float db = 10.0f * std::log10 ((float) (sum / juce::jmax (1, a1 - a0)) + 1.0e-12f);
            e[(size_t) f] = db;
            maxDb = juce::jmax (maxDb, db);
        }
        const float floorDb = maxDb - 55.0f;
        const int back = juce::jmax (2, (int) (0.030 * sr / hop));
        const int refractory = juce::jmax (2, (int) (0.040 * sr / hop));
        int last = -refractory;
        for (int f = 0; f < frames; ++f)
        {
            if (e[(size_t) f] < floorDb || f - last < refractory) continue;
            float prevMin = -120.0f;
            if (f > 0)
            {
                prevMin = 1000.0f;
                for (int k = juce::jmax (0, f - back); k < f; ++k) prevMin = juce::jmin (prevMin, e[(size_t) k]);
            }
            if (e[(size_t) f] - juce::jmax (prevMin, floorDb - 10.0f) < 9.0f) continue;

            // start = where the level first climbed 3 dB above that quiet point
            int startF = f;
            while (startF > 0 && startF > f - back && e[(size_t) startF - 1] > prevMin + 3.0f) --startF;
            int pos = startF * hop;
            // first sample that is really part of the hit
            float localPeak = 0;
            for (int k = pos; k < juce::jmin (n, pos + (int) (sr * 0.02)); ++k) localPeak = juce::jmax (localPeak, std::abs (x[k]));
            while (pos < n - 1 && std::abs (x[pos]) < localPeak * 0.03f && pos < startF * hop + (int) (sr * 0.01)) ++pos;
            onsets.push_back (juce::jmax (0, pos - (int) (sr * 0.001)));
            last = f;
        }
        return onsets;
    }

    namespace
    {
        struct Hit
        {
            int start = 0, length = 0;
            SoundInfo info;
            float overlap = 0;      // 0 clean .. 1 two drums at once
            float loud = -100;
            int cluster = -1;
        };

        std::array<float, 6> features (const SoundInfo& s)
        {
            return { std::log2 (juce::jmax (40.0f, s.centroidHz)) / 14.0f,
                     std::log2 (juce::jmax (5.0f, s.decayMs)) / 12.0f,
                     s.hasPitch() ? s.midiPitch() / 100.0f : 0.0f,
                     juce::jlimit (0.0f, 1.0f, s.flatness * 3.0f),
                     s.subShare + s.lowShare,
                     s.highShare };
        }

        float distance (const std::array<float, 6>& a, const std::array<float, 6>& b)
        {
            float d = 0;
            for (size_t i = 0; i < a.size(); ++i) d += (a[i] - b[i]) * (a[i] - b[i]);
            return std::sqrt (d);
        }
    }

    HarvestResult harvestHits (const juce::File& file, const juce::File& destRoot, const std::function<bool (float)>& progress)
    {
        HarvestResult r;
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr) { r.error = "Couldn't read " + file.getFileName(); return r; }

        const double sr = reader->sampleRate;
        const int n = (int) juce::jmin ((juce::int64) (sr * 600.0), reader->lengthInSamples);
        if (n < (int) (sr * 0.05)) { r.error = "That file is too short."; return r; }
        juce::AudioBuffer<float> audio (2, n);
        juce::AudioBuffer<float> tmp ((int) juce::jmax (1u, reader->numChannels), n);
        reader->read (&tmp, 0, n, 0, true, true);
        audio.copyFrom (0, 0, tmp, 0, 0, n);
        audio.copyFrom (1, 0, tmp, juce::jmin (1, tmp.getNumChannels() - 1), 0, n);
        std::vector<float> mono ((size_t) n);
        for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (audio.getSample (0, i) + audio.getSample (1, i));

        if (progress && ! progress (0.1f)) return r;
        const auto onsets = findOnsets (mono.data(), n, sr);
        r.hitsFound = (int) onsets.size();
        if (onsets.empty()) { r.error = "No drum hits found in " + file.getFileName() + "."; return r; }

        // ---- cut and analyse every hit ----
        std::vector<Hit> hits;
        for (size_t i = 0; i < onsets.size(); ++i)
        {
            if (progress && ! progress (0.1f + 0.6f * (float) i / (float) onsets.size())) return r;
            const int start = onsets[i];
            const int next = i + 1 < onsets.size() ? onsets[i + 1] : n;
            int len = juce::jmin (next - start - (int) (sr * 0.001), (int) (sr * 3.0));
            if (len < (int) (sr * 0.025)) continue;

            Hit h;
            h.start = start;
            h.length = len;
            analysis::measure (mono.data() + start, len, sr, h.info);
            h.info.duration = (float) (len / sr);
            analysis::finish (h.info, {});
            if (! catIsKitSound (h.info.cat)) continue;
            h.loud = h.info.loudDb;
            // low end and lots of high end at once = probably two drums hit together
            h.overlap = juce::jlimit (0.0f, 1.0f, (juce::jmin (h.info.subShare + h.info.lowShare, h.info.highShare) - 0.08f) * 5.0f);
            // a hit cut short by the next one (still ringing) is less useful
            if (next - start < (int) (sr * 0.12) && h.info.decayMs > (float) ((next - start) * 1000.0 / sr) * 0.8f) h.overlap += 0.2f;
            hits.push_back (h);
        }
        if (hits.empty()) { r.error = "Found hits, but none that look like drums."; return r; }

        // ---- group the same drum together (per category) ----
        int clusters = 0;
        std::vector<std::array<float, 6>> centres;
        std::vector<Cat> clusterCat;
        for (auto& h : hits)
        {
            const auto f = features (h.info);
            int best = -1;
            float bestD = 0.12f;
            for (int c = 0; c < clusters; ++c)
                if (clusterCat[(size_t) c] == h.info.cat)
                {
                    const float d = distance (f, centres[(size_t) c]);
                    if (d < bestD) { bestD = d; best = c; }
                }
            if (best < 0)
            {
                best = clusters++;
                centres.push_back (f);
                clusterCat.push_back (h.info.cat);
            }
            h.cluster = best;
        }

        // ---- keep up to 4 drums per category, up to 3 loudness versions each ----
        r.folder = destRoot.getChildFile (paths::safeFileName (file.getFileNameWithoutExtension(), 48));
        r.folder.createDirectory();
        std::array<int, numCats> perCat {};
        std::vector<int> clusterOrder ((size_t) clusters);
        std::iota (clusterOrder.begin(), clusterOrder.end(), 0);
        auto clusterSize = [&] (int c) { int k = 0; for (auto& h : hits) k += h.cluster == c ? 1 : 0; return k; };
        std::sort (clusterOrder.begin(), clusterOrder.end(), [&] (int a, int b) { return clusterSize (a) > clusterSize (b); });

        juce::WavAudioFormat wav;
        int written = 0;
        for (int c : clusterOrder)
        {
            const Cat cat = clusterCat[(size_t) c];
            if (perCat[(size_t) cat] >= 4) continue;
            std::vector<Hit*> members;
            for (auto& h : hits) if (h.cluster == c && h.overlap < 0.7f) members.push_back (&h);
            if (members.empty()) continue;
            const int number = ++perCat[(size_t) cat];
            std::sort (members.begin(), members.end(), [] (Hit* a, Hit* b) { return a->loud > b->loud; });

            std::vector<std::pair<Hit*, juce::String>> picks;
            const float range = members.front()->loud - members.back()->loud;
            auto cleanest = [] (std::vector<Hit*> v) { return *std::min_element (v.begin(), v.end(), [] (Hit* a, Hit* b) { return a->overlap < b->overlap; }); };
            if (range > 6.0f && members.size() >= 3)
            {
                const size_t third = members.size() / 3;
                picks.push_back ({ cleanest ({ members.begin(), members.begin() + (long) juce::jmax ((size_t) 1, third) }), "hard" });
                picks.push_back ({ members[members.size() / 2], "medium" });
                picks.push_back ({ cleanest ({ members.end() - (long) juce::jmax ((size_t) 1, third), members.end() }), "soft" });
            }
            else
                picks.push_back ({ cleanest (members), {} });

            for (auto& [h, label] : picks)
            {
                auto name = catName (cat) + " " + juce::String (number) + (label.isNotEmpty() ? " (" + label + ")" : juce::String());
                auto out = r.folder.getChildFile (name + ".wav");
                juce::AudioBuffer<float> slice (2, h->length);
                for (int ch = 0; ch < 2; ++ch) slice.copyFrom (ch, 0, audio, ch, h->start, h->length);
                // trim the tail at -60 dB and fade
                int end = h->length - 1;
                const float pk = juce::jmax (slice.getMagnitude (0, 0, h->length), slice.getMagnitude (1, 0, h->length));
                while (end > 32 && std::abs (slice.getSample (0, end)) < pk * 0.001f && std::abs (slice.getSample (1, end)) < pk * 0.001f) --end;
                const int keep = juce::jmin (h->length, end + (int) (sr * 0.01));
                const int fade = juce::jmin ((int) (sr * 0.006), keep / 4);
                for (int i = 0; i < fade; ++i)
                    for (int ch = 0; ch < 2; ++ch)
                        slice.setSample (ch, keep - fade + i, slice.getSample (ch, keep - fade + i) * (1.0f - (float) (i + 1) / (float) fade));
                out.deleteFile();
                if (auto os = out.createOutputStream())
                {
                    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
                    if (w != nullptr)
                    {
                        os.release();
                        w->writeFromAudioSampleBuffer (slice, 0, keep);
                        r.files.add (out);
                        ++written;
                    }
                }
            }
        }
        if (progress) progress (1.0f);
        r.ok = written > 0;
        if (! r.ok) r.error = "No clean hits to keep in " + file.getFileName() + ".";
        return r;
    }
}
