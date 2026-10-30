#include "TestSounds.h"
#include <cmath>

namespace ggtest
{
    namespace
    {
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        juce::AudioBuffer<float> make (double sr, float seconds)
        {
            juce::AudioBuffer<float> b (2, juce::jmax (1, (int) (sr * seconds)));
            b.clear();
            return b;
        }

        void toStereo (juce::AudioBuffer<float>& b)
        {
            b.copyFrom (1, 0, b, 0, 0, b.getNumSamples());
        }

        // simple one-pole filters
        struct LP { float a, z = 0; LP (float fc, double sr) : a (std::exp (-twoPi * fc / (float) sr)) {} float operator() (float x) { z = x * (1 - a) + z * a; return z; } };
        struct HP { LP lp; HP (float fc, double sr) : lp (fc, sr) {} float operator() (float x) { return x - lp (x); } };

        // 2nd-order filters
        struct Biquad
        {
            juce::IIRFilter f;
            Biquad (const juce::IIRCoefficients& c) { f.setCoefficients (c); }
            float operator() (float x) { return f.processSingleSampleRaw (x); }
        };
    }

    juce::AudioBuffer<float> Synth::kick (float f0, float decay, float click)
    {
        auto b = make (sr, decay * 3.0f);
        float phase = 0;
        HP hp (3000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float f = f0 + (f0 * 2.6f) * std::exp (-t / 0.03f);
            phase += twoPi * f / (float) sr;
            float s = std::sin (phase) * std::exp (-t / decay);
            if (t < 0.004f) s += click * hp (rng.nextFloat() * 2 - 1) * (1 - t / 0.004f);
            b.setSample (0, i, std::tanh (s * 1.4f) * 0.9f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::bass808 (float f0, float length)
    {
        auto b = make (sr, length);
        float phase = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float f = f0 * (1.0f + 0.35f * std::exp (-t / 0.02f));
            phase += twoPi * f / (float) sr;
            const float env = std::exp (-t / (length * 0.45f)) * juce::jmin (1.0f, t / 0.002f);
            b.setSample (0, i, std::tanh (std::sin (phase) * env * 1.6f) * 0.85f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::snare (float tone, float decay, float noiseAmt)
    {
        auto b = make (sr, decay * 3.5f);
        float phase = 0;
        Biquad hp (juce::IIRCoefficients::makeHighPass (sr, 1400.0)), lp (juce::IIRCoefficients::makeLowPass (sr, 7000.0));
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            phase += twoPi * tone / (float) sr;
            const float body = (std::sin (phase) + 0.4f * std::sin (phase * 2.3f)) * std::exp (-t / 0.05f) * 0.55f;
            const float noise = lp (hp (rng.nextFloat() * 2 - 1)) * std::exp (-t / decay) * noiseAmt * 1.6f;
            b.setSample (0, i, (body + noise) * 0.8f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::clap (float decay)
    {
        auto b = make (sr, 0.5f);
        Biquad bp (juce::IIRCoefficients::makeBandPass (sr, 1500.0, 0.9)), lp (juce::IIRCoefficients::makeLowPass (sr, 5000.0));
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            float env = 0;
            for (int k = 0; k < 3; ++k)
            {
                const float tk = t - (float) k * 0.011f;
                if (tk >= 0 && tk < 0.011f) env = juce::jmax (env, std::exp (-tk / 0.0025f));
            }
            const float tail = t >= 0.033f ? std::exp (-(t - 0.033f) / decay) : 0.0f;
            env = juce::jmax (env, tail);
            b.setSample (0, i, lp (bp (rng.nextFloat() * 2 - 1)) * env * 3.0f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::rim()
    {
        auto b = make (sr, 0.12f);
        HP hp (2000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float s = (0.6f * std::sin (twoPi * 1700 * t) + 0.5f * std::sin (twoPi * 520 * t)) * std::exp (-t / 0.012f)
                          + 0.15f * hp (rng.nextFloat() * 2 - 1) * std::exp (-t / 0.004f);
            b.setSample (0, i, s * 0.9f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::hat (float decay)
    {
        auto b = make (sr, juce::jmax (0.08f, decay * 3.5f));
        HP hp1 (7000, sr), hp2 (7000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            float metal = 0;
            for (float f : { 4120.0f, 5310.0f, 6870.0f, 8140.0f, 9530.0f })
                metal += std::sin (twoPi * f * t) > 0 ? 0.12f : -0.12f;
            const float s = hp2 (hp1 (rng.nextFloat() * 2 - 1 + metal * 0.5f)) * std::exp (-t / decay);
            b.setSample (0, i, s * 1.4f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::tom (float f0, float decay)
    {
        auto b = make (sr, decay * 3.0f);
        float phase = 0;
        Biquad stick (juce::IIRCoefficients::makeBandPass (sr, 2200.0, 0.8));
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float f = f0 * (1.0f + 0.25f * std::exp (-t / 0.05f));
            phase += twoPi * f / (float) sr;
            float s = std::sin (phase) * std::exp (-t / decay) * 0.7f
                    + 0.3f * std::sin (phase * 1.59f) * std::exp (-t / (decay * 0.5f))
                    + 0.18f * std::sin (phase * 2.14f) * std::exp (-t / (decay * 0.35f));
            if (t < 0.02f) s += 0.5f * stick (rng.nextFloat() * 2 - 1) * (1 - t / 0.02f);
            b.setSample (0, i, s);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::crash (float decay)
    {
        auto b = make (sr, decay * 2.2f);
        HP hp (3500, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            b.setSample (0, i, hp (rng.nextFloat() * 2 - 1) * std::exp (-t / decay) * 0.9f * juce::jmin (1.0f, t / 0.003f));
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::ride (float decay)
    {
        auto b = make (sr, decay * 2.2f);
        HP hp (5000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            float bell = 0;
            for (float f : { 2950.0f, 3720.0f, 4410.0f, 5230.0f })
                bell += std::sin (twoPi * f * t) * 0.2f;
            const float s = (bell * std::exp (-t / (decay * 0.6f)) + 0.12f * hp (rng.nextFloat() * 2 - 1) * std::exp (-t / decay));
            b.setSample (0, i, s * 0.8f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::shaker()
    {
        auto b = make (sr, 0.25f);
        HP hp (5000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float env = t < 0.03f ? t / 0.03f : std::exp (-(t - 0.03f) / 0.04f);
            b.setSample (0, i, hp (rng.nextFloat() * 2 - 1) * env * 0.8f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::cowbell()
    {
        auto b = make (sr, 0.5f);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float s = (std::sin (twoPi * 562 * t) + 0.6f * std::sin (twoPi * 845 * t)) * std::exp (-t / 0.09f) * 0.6f;
            b.setSample (0, i, s);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::riser (float length)
    {
        auto b = make (sr, length);
        LP lp (2000, sr);
        float phase = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float pos = t / length;
            phase += twoPi * (200.0f + 1800.0f * pos * pos) / (float) sr;
            const float s = (0.5f * std::sin (phase) + 0.3f * lp (rng.nextFloat() * 2 - 1)) * pos * pos;
            b.setSample (0, i, s * 0.9f);
        }
        toStereo (b);
        return b;
    }

    void Synth::vintage (juce::AudioBuffer<float>& b, float cutoff, float hissDb)
    {
        const float hiss = juce::Decibels::decibelsToGain (hissDb);
        for (int c = 0; c < b.getNumChannels(); ++c)
        {
            LP lp1 (cutoff, sr), lp2 (cutoff, sr);
            for (int i = 0; i < b.getNumSamples(); ++i)
                b.setSample (c, i, lp2 (lp1 (b.getSample (c, i))) + hiss * (rng.nextFloat() * 2 - 1));
        }
        // a little extra tail of hiss after the sound
        const int extra = (int) (sr * 0.25);
        juce::AudioBuffer<float> longer (b.getNumChannels(), b.getNumSamples() + extra);
        longer.clear();
        for (int c = 0; c < b.getNumChannels(); ++c)
        {
            longer.copyFrom (c, 0, b, c, 0, b.getNumSamples());
            for (int i = b.getNumSamples(); i < longer.getNumSamples(); ++i)
                longer.setSample (c, i, hiss * (rng.nextFloat() * 2 - 1));
        }
        b = std::move (longer);
    }

    bool Synth::write (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
    {
        f.getParentDirectory().createDirectory();
        f.deleteFile();
        juce::WavAudioFormat wav;
        auto os = f.createOutputStream();
        if (os == nullptr) return false;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0));
        if (w == nullptr) return false;
        os.release();
        return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}
