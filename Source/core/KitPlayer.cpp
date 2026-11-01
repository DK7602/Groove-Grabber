#include "KitPlayer.h"
#include <cmath>
#include <map>
#include <mutex>

namespace gg
{
    //==============================================================================
    namespace
    {
        struct RenderCache
        {
            std::mutex lock;
            std::map<juce::String, std::pair<std::shared_ptr<const juce::AudioBuffer<float>>, juce::int64>> items;
            juce::int64 counter = 0;

            std::shared_ptr<const juce::AudioBuffer<float>> get (const juce::String& key)
            {
                std::lock_guard<std::mutex> l (lock);
                auto it = items.find (key);
                if (it == items.end()) return nullptr;
                it->second.second = ++counter;
                return it->second.first;
            }

            void put (const juce::String& key, std::shared_ptr<const juce::AudioBuffer<float>> b)
            {
                std::lock_guard<std::mutex> l (lock);
                items[key] = { std::move (b), ++counter };
                while (items.size() > 600)
                {
                    auto oldest = items.begin();
                    for (auto it = items.begin(); it != items.end(); ++it)
                        if (it->second.second < oldest->second.second) oldest = it;
                    items.erase (oldest);
                }
            }
        };

        RenderCache& renderCache()
        {
            static RenderCache c;
            return c;
        }

        std::shared_ptr<const juce::AudioBuffer<float>> renderCached (const SlotState& st, Cat cat, bool keyTuning,
                                                                    float extraTune, float padGainDb, float lengthScale, double sr)
        {
            if (st.isEmpty()) return nullptr;
            RenderParams p;
            p.tune = st.tune (keyTuning) + extraTune;
            p.startMs = st.startMs;
            p.lengthPct = st.lengthPct;
            p.gainDb = st.gainDb;
            p.reverse = st.reverse;
            p.lengthScale = lengthScale;
            p.padGainDb = padGainDb;
            const auto key = st.path + "|" + juce::String (p.tune, 3) + "|" + juce::String (p.startMs, 1) + "|" + juce::String (p.lengthPct, 1)
                           + "|" + juce::String (p.gainDb, 2) + "|" + (p.reverse ? "r" : "f") + "|" + juce::String (lengthScale, 3)
                           + "|" + juce::String (padGainDb, 2) + "|" + juce::String ((int) sr) + "|" + catId (cat);
            if (auto hit = renderCache().get (key)) return hit;
            auto buf = std::make_shared<juce::AudioBuffer<float>> (render::sound (st.path, cat, p, sr));
            if (buf->getNumSamples() == 0) return nullptr;
            renderCache().put (key, buf);
            return buf;
        }
    }

    std::shared_ptr<const juce::AudioBuffer<float>> renderPad (const Kit& kit, const PadDef& pad, double sr)
    {
        return renderCached (kit[pad.slot], slotCat (pad.slot), kit.tuneToKey, pad.tuneOffset, pad.gainDb, pad.lengthScale, sr);
    }

    std::shared_ptr<const juce::AudioBuffer<float>> renderSlotPreview (const Kit& kit, Slot slot, double sr)
    {
        return renderCached (kit[slot], slotCat (slot), kit.tuneToKey, 0, 0, 1, sr);
    }

    PlayableKit::Ptr buildPlayable (const Kit& kit, double sr)
    {
        PlayableKit::Ptr pk (new PlayableKit());
        pk->kitId = kit.id;
        pk->sampleRate = sr;
        for (int i = 0; i < numPads; ++i)
            pk->pads[(size_t) i] = renderPad (kit, padLayout()[(size_t) i], sr);
        return pk;
    }

    void clearRenderCache()
    {
        std::lock_guard<std::mutex> l (renderCache().lock);
        renderCache().items.clear();
    }

    //==============================================================================
    KitPlayer::KitPlayer()
    {
        startTimer (1500);
    }

    KitPlayer::~KitPlayer()
    {
        stopTimer();
    }

    void KitPlayer::prepare (double sr, int)
    {
        sampleRate = sr;
        allNotesOff();
    }

    void KitPlayer::setKit (PlayableKit::Ptr k)
    {
        if (k != nullptr) keep (k.get());
        const juce::SpinLock::ScopedLockType sl (kitLock);
        pendingKit = k;
    }

    PlayableKit::Ptr KitPlayer::getKit() const
    {
        const juce::SpinLock::ScopedLockType sl (const_cast<juce::SpinLock&> (kitLock));
        return pendingKit;
    }

    void KitPlayer::playPreview (std::shared_ptr<const juce::AudioBuffer<float>> audio)
    {
        if (audio == nullptr) return;
        PreviewSound::Ptr p (new PreviewSound());
        p->audio = std::move (audio);
        keep (p.get());
        {
            const juce::SpinLock::ScopedLockType sl (kitLock);
            pendingPreview = p;
        }
        previewFlag = true;
    }

    void KitPlayer::setGroovePlaying (bool b)
    {
        if (b && ! groovePlaying) resetGroove = true;
        groovePlaying = b;
    }

    void KitPlayer::timerCallback()
    {
        // free kits / previews nobody uses any more - here, never on the audio thread. Something is only
        // freed once nothing else has held it for two ticks in a row (3 s), long after any fade-out ended.
        for (auto& k : keepAlive)
            k.idleTicks = k.object->getReferenceCount() <= 1 ? k.idleTicks + 1 : 0;
        keepAlive.erase (std::remove_if (keepAlive.begin(), keepAlive.end(), [] (const Kept& k) { return k.idleTicks >= 2; }),
                         keepAlive.end());
    }

    void KitPlayer::keep (juce::ReferenceCountedObject* o)
    {
        for (auto& k : keepAlive)
            if (k.object.get() == o) { k.idleTicks = 0; return; }
        keepAlive.push_back ({ o, 0 });
    }

    void KitPlayer::trigger (int note, float velocity)
    {
        const juce::SpinLock::ScopedLockType sl (fifoWriteLock);
        const auto scope = fifo.write (1);
        if (scope.blockSize1 > 0) trigs[(size_t) scope.startIndex1] = { note, velocity };
    }

    std::uint32_t KitPlayer::getPadFlash (int padIndex) const noexcept
    {
        return juce::isPositiveAndBelow (padIndex, numPads) ? padFlash[(size_t) padIndex].load() : 0u;
    }

    void KitPlayer::allNotesOff()
    {
        for (auto& v : voices) v.active = false;
    }

    //==============================================================================
    void KitPlayer::startVoice (int note, float velocity, int offset)
    {
        const auto* pad = padForNote (note);
        if (pad == nullptr || currentKit == nullptr) return;
        const int padIndex = note - firstPadNote;
        auto& buf = currentKit->pads[(size_t) padIndex];
        if (buf == nullptr || buf->getNumSamples() == 0) return;

        // same pad again, or a hat cutting another hat: fade the old one out fast
        for (auto& v : voices)
            if (v.active && (v.note == note || (pad->chokeGroup != 0 && v.choke == pad->chokeGroup)))
                v.fadeStep = 1.0f / (float) juce::jmax (1.0, sampleRate.load() * 0.004);

        Voice* slot = nullptr;
        for (auto& v : voices)
            if (! v.active) { slot = &v; break; }
        if (slot == nullptr)
        {
            slot = &voices[0];
            for (auto& v : voices)
                if (v.age < slot->age) slot = &v;
        }

        Voice& v = *slot;
        v.l = buf->getReadPointer (0);
        v.r = buf->getReadPointer (juce::jmin (1, buf->getNumChannels() - 1));
        v.length = buf->getNumSamples();
        v.pos = 0;
        v.startOffset = offset;
        v.note = note;
        v.choke = pad->chokeGroup;
        const float vel = juce::jlimit (0.0f, 1.0f, velocity);
        v.gain = 0.2f + 0.8f * std::pow (vel, 1.4f);
        v.fade = 1;
        v.fadeStep = 0;
        v.active = true;
        v.age = ++voiceCounter;
        padFlash[(size_t) padIndex].fetch_add (1);
    }

    void KitPlayer::startPreviewVoice()
    {
        if (currentPreview == nullptr || currentPreview->audio == nullptr) return;
        auto& buf = *currentPreview->audio;
        if (buf.getNumSamples() == 0) return;
        for (auto& v : voices)
            if (v.active && v.note == 1000) v.fadeStep = 1.0f / (float) juce::jmax (1.0, sampleRate.load() * 0.004);
        Voice* slot = &voices[0];
        for (auto& v : voices)
            if (! v.active) { slot = &v; break; }
        auto& v = *slot;
        v.l = buf.getReadPointer (0);
        v.r = buf.getReadPointer (juce::jmin (1, buf.getNumChannels() - 1));
        v.length = buf.getNumSamples();
        v.pos = 0;
        v.startOffset = 0;
        v.note = 1000;
        v.choke = 0;
        v.gain = 0.9f;
        v.fade = 1;
        v.fadeStep = 0;
        v.active = true;
        v.age = ++voiceCounter;
    }

    void KitPlayer::scheduleGroove (double beatStart, double beatEnd, int blockSamples, double samplesPerBeat)
    {
        const auto* g = groove.load();
        if (g == nullptr || g->hits.empty()) return;
        const double len = g->lengthBeats();
        const float sw = swing.load();

        // walk every loop repetition the block touches
        const double firstLoop = std::floor (beatStart / len);
        for (double loop = firstLoop; loop * len < beatEnd; loop += 1.0)
            for (auto& h : g->hits)
            {
                const double b = loop * len + swungBeat (h.beat, sw, g->swingEighths);
                if (b >= beatStart && b < beatEnd)
                {
                    const int offset = juce::jlimit (0, blockSamples - 1, (int) ((b - beatStart) * samplesPerBeat));
                    const int note = mainNoteForSlot (h.slot);
                    if (note >= 0) startVoice (note, h.velocity, offset);
                }
            }
    }

    void KitPlayer::process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const HostInfo& host)
    {
        out.clear();
        const int n = out.getNumSamples();
        if (n <= 0) return;

        {
            const juce::SpinLock::ScopedTryLockType sl (kitLock);
            if (sl.isLocked())
            {
                if (currentKit != pendingKit)
                {
                    // the new kit replaces the old: fade out what the old one is still playing (its audio stays
                    // alive on the message thread for a while, so a 4 ms fade is safe)
                    for (auto& v : voices)
                        if (v.active && v.note != 1000) v.fadeStep = 1.0f / (float) juce::jmax (1.0, sampleRate.load() * 0.004);
                    currentKit = pendingKit;
                }
                if (previewFlag.exchange (false))
                {
                    currentPreview = pendingPreview;
                    startPreviewVoice();
                }
            }
        }

        // on-screen pads
        {
            const auto scope = fifo.read (fifo.getNumReady());
            for (int i = 0; i < scope.blockSize1; ++i) startVoice (trigs[(size_t) (scope.startIndex1 + i)].note, trigs[(size_t) (scope.startIndex1 + i)].vel, 0);
            for (int i = 0; i < scope.blockSize2; ++i) startVoice (trigs[(size_t) (scope.startIndex2 + i)].note, trigs[(size_t) (scope.startIndex2 + i)].vel, 0);
        }

        // MIDI in
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn()) startVoice (m.getNoteNumber(), m.getFloatVelocity(), juce::jlimit (0, n - 1, meta.samplePosition));
            else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
        }

        // the preview groove
        const double sr = sampleRate.load();
        const bool hostPlay = host.valid && host.playing && followHost.load();
        hostDriving = hostPlay;
        if (groovePlaying.load())
        {
            if (hostPlay)
            {
                const double bpm = host.bpm > 0 ? host.bpm : tempo.load();
                const double spb = sr * 60.0 / bpm;
                const double start = host.ppq;
                const double end = start + n / spb;
                scheduleGroove (start, end, n, spb);
                displayBeat = start;
            }
            else
            {
                if (resetGroove.exchange (false) || (wasHostPlaying && ! hostPlay)) internalBeat = 0;
                const double spb = sr * 60.0 / tempo.load();
                const double end = internalBeat + n / spb;
                scheduleGroove (internalBeat, end, n, spb);
                internalBeat = end;
                if (const auto* g = groove.load())
                    if (internalBeat > g->lengthBeats() * 1000.0) internalBeat = std::fmod (internalBeat, g->lengthBeats());
                displayBeat = internalBeat;
            }
        }
        wasHostPlaying = hostPlay;

        render (out, 0, n);
        out.applyGain (volume.load());
    }

    void KitPlayer::render (juce::AudioBuffer<float>& out, int start, int num)
    {
        float* L = out.getWritePointer (0, start);
        float* R = out.getNumChannels() > 1 ? out.getWritePointer (1, start) : nullptr;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            int i = v.startOffset;
            v.startOffset = 0;
            for (; i < num; ++i)
            {
                if (v.pos >= v.length) { v.active = false; break; }
                if (v.fadeStep > 0)
                {
                    v.fade -= v.fadeStep;
                    if (v.fade <= 0) { v.active = false; break; }
                }
                const float g = v.gain * v.fade;
                L[i] += v.l[v.pos] * g;
                if (R != nullptr) R[i] += v.r[v.pos] * g;
                ++v.pos;
            }
        }
        if (R == nullptr) return;
        // gentle safety clip: drums stacked at full velocity can pass 0 dBFS
        for (int i = 0; i < num; ++i)
        {
            L[i] = std::tanh (L[i] * 0.9f) / 0.9f;
            R[i] = std::tanh (R[i] * 0.9f) / 0.9f;
        }
    }
}
