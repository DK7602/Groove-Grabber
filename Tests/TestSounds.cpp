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

    //==============================================================================
    // effects
    namespace
    {
        // a sine whose frequency follows f (t in 0..1), with an envelope
        template <typename F, typename E>
        juce::AudioBuffer<float> tone (double sr, float length, F freq, E env, float harmonics = 0.0f)
        {
            auto b = make (sr, length);
            float phase = 0;
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float pos = (float) i / (float) b.getNumSamples();
                phase += twoPi * freq (pos) / (float) sr;
                float v = std::sin (phase);
                if (harmonics > 0) v += harmonics * (std::sin (2 * phase) / 2 + std::sin (3 * phase) / 3 + std::sin (4 * phase) / 4);
                b.setSample (0, i, v * env (pos) * 0.8f);
            }
            toStereo (b);
            return b;
        }
    }

    juce::AudioBuffer<float> Synth::reverseCymbal (float length)
    {
        auto c = crash (length / 2.2f);
        juce::AudioBuffer<float> b (2, c.getNumSamples());
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < c.getNumSamples(); ++i) b.setSample (ch, i, c.getSample (ch, c.getNumSamples() - 1 - i));
        return b;
    }

    juce::AudioBuffer<float> Synth::swell (float length)
    {
        auto c = snare (190, length / 3.0f, 0.8f);
        const int n = juce::jmin (c.getNumSamples(), (int) (sr * length));
        juce::AudioBuffer<float> b (2, n);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i) b.setSample (ch, i, c.getSample (ch, n - 1 - i));
        return b;
    }

    juce::AudioBuffer<float> Synth::impact (float length)
    {
        auto b = make (sr, length);
        LP lp (900, sr);
        float phase = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            phase += twoPi * (48.0f + 30.0f * std::exp (-t / 0.05f)) / (float) sr;
            const float boom = std::sin (phase) * std::exp (-t / (length * 0.35f));
            const float crack = lp (rng.nextFloat() * 2 - 1) * std::exp (-t / 0.12f) * 1.5f;
            b.setSample (0, i, std::tanh ((boom + crack) * 1.2f) * 0.85f * juce::jmin (1.0f, t / 0.002f));
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::subDrop (float length)
    {
        return tone (sr, length, [] (float p) { return 70.0f * std::pow (0.45f, p); },
                     [] (float p) { return juce::jmin (1.0f, p * 200.0f) * std::exp (-p * 2.5f); });
    }

    juce::AudioBuffer<float> Synth::downlifter (float length)
    {
        auto b = make (sr, length);
        float z = 0, phase = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float pos = (float) i / (float) b.getNumSamples();
            const float fc = 6000.0f * std::pow (0.04f, pos);
            const float a = std::exp (-twoPi * fc / (float) sr);
            z = (rng.nextFloat() * 2 - 1) * (1 - a) + z * a;
            phase += twoPi * (900.0f * std::pow (0.12f, pos)) / (float) sr;
            const float env = juce::jmin (1.0f, pos * 100.0f) * (1.0f - pos) * (1.0f - pos * 0.5f);
            b.setSample (0, i, (z * 1.6f + 0.35f * std::sin (phase)) * env * 0.8f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::whoosh (float length)
    {
        auto b = make (sr, length);
        float z = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float pos = (float) i / (float) b.getNumSamples();
            const float fc = 800.0f + 3000.0f * std::sin (juce::MathConstants<float>::pi * pos);
            const float a = std::exp (-twoPi * fc / (float) sr);
            z = (rng.nextFloat() * 2 - 1) * (1 - a) + z * a;
            const float env = std::pow (std::sin (juce::MathConstants<float>::pi * pos), 3.0f);
            b.setSample (0, i, z * env * 2.2f);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::tapeStop (float length)
    {
        return tone (sr, length, [] (float p) { return 330.0f * std::pow (1.0f - p, 1.6f) + 1.0f; },
                     [] (float p) { return juce::jmin (1.0f, p * 100.0f) * (p > 0.92f ? (1.0f - p) / 0.08f : 1.0f); }, 0.6f);
    }

    juce::AudioBuffer<float> Synth::crackle (float length)
    {
        auto b = make (sr, length);
        LP lp (3000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float v = lp (rng.nextFloat() * 2 - 1) * 0.04f;
            if (rng.nextFloat() < 12.0f / (float) sr) v += (rng.nextBool() ? 1.0f : -1.0f) * (0.4f + 0.5f * rng.nextFloat());
            b.setSample (0, i, v);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::noiseBed (float length)
    {
        auto b = make (sr, length);
        LP lp (4000, sr);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float pos = (float) i / (float) b.getNumSamples();
            const float env = juce::jmin (1.0f, pos * 20.0f, (1.0f - pos) * 20.0f);
            b.setSample (0, i, lp (rng.nextFloat() * 2 - 1) * 0.25f * env);
        }
        toStereo (b);
        return b;
    }

    juce::AudioBuffer<float> Synth::scratch (float length)
    {
        // a tone pushed back and forth: pitch swinging up and down six times a second
        return tone (sr, length, [length] (float p) { return 400.0f * std::pow (2.0f, 1.2f * std::sin (twoPi * 6.0f * p * length)); },
                     [] (float p) { return juce::jmin (1.0f, p * 60.0f, (1.0f - p) * 30.0f); }, 0.8f);
    }

    juce::AudioBuffer<float> Synth::zap (float length)
    {
        return tone (sr, length, [] (float p) { return 300.0f * std::pow (8.0f, p); },
                     [] (float p) { return juce::jmin (1.0f, p * 50.0f) * (1.0f - p); });
    }

    juce::AudioBuffer<float> Synth::laser (float length)
    {
        return tone (sr, length, [] (float p) { return 2400.0f * std::pow (0.08f, p); },
                     [] (float p) { return juce::jmin (1.0f, p * 80.0f) * (1.0f - p * 0.7f); });
    }

    juce::AudioBuffer<float> Synth::blip (float length)
    {
        return tone (sr, length, [] (float) { return 1046.5f; }, [] (float p) { return juce::jmin (1.0f, p * 30.0f) * (1.0f - p); });
    }

    juce::AudioBuffer<float> Synth::glitch (float length)
    {
        // a buzzy tone chopped on and off, faster and faster
        auto b = tone (sr, length, [] (float p) { return 220.0f + 440.0f * (float) ((int) (p * 9) % 3); }, [] (float) { return 1.0f; }, 1.0f);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float t = (float) (i / sr);
            const float rate = 8.0f + 10.0f * t;
            const bool on = std::fmod (t * rate, 1.0f) < 0.5f;
            for (int ch = 0; ch < 2; ++ch) b.setSample (ch, i, on ? b.getSample (ch, i) : 0.0f);
        }
        return b;
    }

    juce::AudioBuffer<float> Synth::horn (float length)
    {
        return tone (sr, length, [] (float p) { return 466.0f * (1.0f + 0.01f * std::sin (twoPi * 5.0f * p)); },
                     [] (float p) { return juce::jmin (1.0f, p * 40.0f, (1.0f - p) * 20.0f); }, 2.0f);
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
