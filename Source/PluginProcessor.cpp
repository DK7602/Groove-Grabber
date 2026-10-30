#include "PluginProcessor.h"
#include "PluginEditor.h"

GrooveGrabberProcessor::GrooveGrabberProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    options.character = gg::genreCenter (options.genre);
    seed = (juce::uint32) juce::Time::currentTimeMillis();
    player.setGroove (&gg::grooveFor (grooveId));
    player.setTempo (gg::styleFor (options.genre).bpm);
    library->addChangeListener (this);
    addChangeListener (this);       // keeps the state snapshot up to date
}

GrooveGrabberProcessor::~GrooveGrabberProcessor()
{
    alive->store (false);
    library->removeChangeListener (this);
    removeChangeListener (this);
    worker.removeAllJobs (true, 5000);
}

bool GrooveGrabberProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void GrooveGrabberProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const bool rateChanged = std::abs (player.getSampleRate() - sampleRate) > 1.0;
    player.prepare (sampleRate, samplesPerBlock);
    monoScratch.setSize (2, juce::jmax (1, samplesPerBlock), false, false, true);
    if (rateChanged && ! kits.empty())
        juce::MessageManager::callAsync ([this, a = alive] { if (a->load()) renderKits(); });
}

void GrooveGrabberProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    gg::KitPlayer::HostInfo host;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            host.valid = ! isStandalone();
            host.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) host.ppq = *ppq;
            if (auto bpm = pos->getBpm()) host.bpm = *bpm;
        }

    if (buffer.getNumChannels() >= 2)
        player.process (buffer, midi, host);
    else
    {
        // no allocation here: the scratch buffer was sized in prepareToPlay (and only grows if a host
        // sends a bigger block than it promised)
        monoScratch.setSize (2, buffer.getNumSamples(), false, false, true);
        auto& stereo = monoScratch;
        player.process (stereo, midi, host);
        buffer.clear();
        buffer.addFrom (0, 0, stereo, 0, 0, buffer.getNumSamples(), 0.5f);
        buffer.addFrom (0, 0, stereo, 1, 0, buffer.getNumSamples(), 0.5f);
    }
    midi.clear();
}

juce::AudioProcessorEditor* GrooveGrabberProcessor::createEditor()
{
    return new GrooveGrabberEditor (*this);
}

//==============================================================================
void GrooveGrabberProcessor::notify (const juce::String& message)
{
    if (message.isNotEmpty()) statusMessage = message;
    sendChangeMessage();
}

void GrooveGrabberProcessor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &library.get())
    {
        sendChangeMessage();     // the library changed: pages refresh
        return;
    }
    const auto s = makeState();
    const juce::ScopedLock sl (snapshotLock);
    stateSnapshot = s;
}

void GrooveGrabberProcessor::runInBackground (std::function<void()> job)
{
    worker.addJob (std::move (job));
}

//==============================================================================
void GrooveGrabberProcessor::applyDescription (const juce::String& text)
{
    description = text;
    understood = gg::parseDescription (text);
    gg::applyDescription (understood, options);
    if (understood.bpm > 0) setBpm (understood.bpm);
    else if (understood.hasGenre) setBpm (gg::styleFor (options.genre).bpm);
    if (understood.hasGenre)
    {
        const auto& dirs = gg::styleFor (options.genre).directions;
        setGrooveId (dirs.front().groove);
    }
    notify();
}

void GrooveGrabberProcessor::setGenre (gg::Genre g)
{
    options.genre = g;
    options.character = gg::genreCenter (g);
    for (int i = 0; i < gg::Traits::num; ++i)
        options.character[i] = juce::jlimit (0.0f, 1.0f, options.character[i] + understood.nudge[(size_t) i]);
    setGrooveId (gg::styleFor (g).directions.front().groove);
    notify();
}

void GrooveGrabberProcessor::setKey (int key)
{
    options.key = juce::jlimit (-1, 23, key);
    // retune what's there now (each kit keeps its sounds)
    auto gen = makeGenerator();
    for (auto& k : kits)
    {
        if (options.key >= 0) gen.tuneToKey (k, options.key);
        else { k.key = -1; for (auto& s : k.slots) s.autoTune = 0; }
    }
    renderKits();
    notify();
}

void GrooveGrabberProcessor::setBpm (double bpm)
{
    options.bpm = juce::jlimit (40.0, 250.0, bpm);
    player.setTempo (options.bpm);
    for (auto& k : kits) k.bpm = options.bpm;
    notify();
}

void GrooveGrabberProcessor::setGrooveId (const juce::String& id)
{
    grooveId = id;
    const auto& g = gg::grooveFor (id);
    player.setGroove (&g);
    player.setSwing (g.swing);
    notify();
}

//==============================================================================
gg::KitGenerator GrooveGrabberProcessor::makeGenerator() const
{
    return gg::KitGenerator (library->getSounds(), options);
}

void GrooveGrabberProcessor::generate (bool surprise)
{
    auto gen = makeGenerator();
    if (gen.totalUsable() == 0)
    {
        notify ("Your library is empty: add your drum folders on the LIBRARY page first.");
        return;
    }
    ++seed;
    auto previous = kits;
    for (auto& k : previous) k.description = description;
    kits = gen.generateAll (seed, surprise ? 0.14f : 0.035f, previous.size() == 4 ? &previous : nullptr);
    for (auto& k : kits) k.description = description;
    activeKit = juce::jlimit (0, 3, activeKit);
    gg::kitstore::addToHistory (kits);

    juce::String missing;
    for (auto c : { gg::Cat::kick, gg::Cat::snare, gg::Cat::closedHat })
        if (gen.usableCount (c) == 0) missing << (missing.isEmpty() ? "" : ", ") << gg::catName (c).toLowerCase() << "s";
    renderKits();
    notify (missing.isNotEmpty() ? "No " + missing + " in your library yet - add a pack that has them."
                                 : "4 kits from " + juce::String (gen.totalUsable()) + " sounds. Click a kit to hear it.");
}

void GrooveGrabberProcessor::regenerateKit (int index, bool surprise)
{
    if (! juce::isPositiveAndBelow (index, (int) kits.size())) { generate (surprise); return; }
    auto gen = makeGenerator();
    ++seed;
    std::vector<gg::Kit> others;
    for (int i = 0; i < (int) kits.size(); ++i) if (i != index) others.push_back (kits[(size_t) i]);
    const auto& dirs = gg::styleFor (options.genre).directions;
    int dir = 0;
    for (int d = 0; d < (int) dirs.size(); ++d) if (dirs[(size_t) d].id == kits[(size_t) index].directionId) dir = d;
    auto fresh = gen.generate (dir, seed, surprise ? 0.14f : 0.06f, &kits[(size_t) index], others);
    fresh.description = description;
    kits[(size_t) index] = fresh;
    renderKits();
    notify ("New sounds for " + fresh.name + " (locked pads kept).");
}

void GrooveGrabberProcessor::setActiveKit (int index)
{
    if (! juce::isPositiveAndBelow (index, juce::jmin ((int) kits.size(), (int) playables.size()))) return;
    activeKit = index;
    if (playables[(size_t) index] != nullptr) player.setKit (playables[(size_t) index]);
    notify();
}

gg::Kit* GrooveGrabberProcessor::getActiveKit()
{
    return juce::isPositiveAndBelow (activeKit, (int) kits.size()) ? &kits[(size_t) activeKit] : nullptr;
}

gg::PlayableKit::Ptr GrooveGrabberProcessor::getPlayable (int i) const
{
    return juce::isPositiveAndBelow (i, 4) ? playables[(size_t) i] : nullptr;
}

void GrooveGrabberProcessor::kitChanged (int)
{
    renderKits();
    notify();
}

void GrooveGrabberProcessor::setSlotSound (int kitIndex, gg::Slot slot, const juce::String& path)
{
    if (! juce::isPositiveAndBelow (kitIndex, (int) kits.size())) return;
    auto& st = kits[(size_t) kitIndex][slot];
    const bool locked = st.locked;
    st = {};
    st.path = path;
    st.locked = locked;
    if (auto info = library->find (path); info.has_value() && options.key >= 0 && kits[(size_t) kitIndex].tuneToKey)
        st.autoTune = gg::KitGenerator::autoTuneFor (*info, slot, options.key);
    kitChanged (kitIndex);
}

void GrooveGrabberProcessor::setAllLocked (int kitIndex, bool locked)
{
    if (! juce::isPositiveAndBelow (kitIndex, (int) kits.size())) return;
    for (auto& s : kits[(size_t) kitIndex].slots) s.locked = locked && ! s.isEmpty();
    notify();
}

juce::StringArray GrooveGrabberProcessor::alternativesFor (int kitIndex, gg::Slot slot, int count)
{
    if (! juce::isPositiveAndBelow (kitIndex, (int) kits.size())) return {};
    auto opts = options;
    opts.genre = kits[(size_t) kitIndex].genre;
    gg::KitGenerator gen (library->getSounds(), opts);
    return gen.alternatives (kits[(size_t) kitIndex], slot, count);
}

void GrooveGrabberProcessor::loadKit (const gg::Kit& k, int into)
{
    while (kits.size() < 4)
    {
        gg::Kit empty;
        empty.id = gg::Kit::newId();
        empty.name = "Empty";
        kits.push_back (empty);
    }
    into = juce::jlimit (0, 3, into);
    kits[(size_t) into] = k;
    if (k.key >= 0) options.key = k.key;
    if (k.bpm > 0) setBpm (k.bpm);
    activeKit = into;
    renderKits();
    notify ("Loaded " + k.name + ".");
}

//==============================================================================
void GrooveGrabberProcessor::renderKits()
{
    const int generation = ++renderGeneration;
    const double sr = player.getSampleRate() > 0 ? player.getSampleRate() : 44100.0;
    auto copy = kits;
    const int active = activeKit;
    // the kit you're looking at first, then the others
    std::vector<int> order { active };
    for (int i = 0; i < (int) copy.size(); ++i) if (i != active) order.push_back (i);
    for (int i : order)
    {
        if (i >= (int) playables.size()) break;
        runInBackground ([this, a = alive, generation, kit = copy[(size_t) i], i, sr]
        {
            if (! a->load() || renderGeneration.load() != generation) return;
            auto pk = gg::buildPlayable (kit, sr);
            juce::MessageManager::callAsync ([this, a, generation, pk, i]
            {
                if (! a->load() || renderGeneration.load() != generation) return;
                playables[(size_t) i] = pk;
                if (i == activeKit) player.setKit (pk);
                sendChangeMessage();
            });
        });
    }
}

void GrooveGrabberProcessor::auditionSlot (gg::Slot slot)
{
    selectedSlot = slot;
    const int note = gg::mainNoteForSlot (slot);
    if (note >= 0) player.trigger (note, 0.9f);
}

void GrooveGrabberProcessor::previewFile (const juce::String& path, gg::Cat cat)
{
    const double sr = player.getSampleRate() > 0 ? player.getSampleRate() : 44100.0;
    runInBackground ([this, a = alive, path, cat, sr]
    {
        auto buf = std::make_shared<juce::AudioBuffer<float>> (gg::render::sound (path, cat, {}, sr));
        juce::MessageManager::callAsync ([this, a, buf] { if (a->load()) player.playPreview (buf); });
    });
}

//==============================================================================
void GrooveGrabberProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::String text;
    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        text = makeState();
        const juce::ScopedLock sl (snapshotLock);
        stateSnapshot = text;
    }
    else
    {
        // some hosts save from another thread: hand over the last state the message thread made
        const juce::ScopedLock sl (snapshotLock);
        text = stateSnapshot.isNotEmpty() ? stateSnapshot : juce::String ("{}");
    }
    dest.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

juce::String GrooveGrabberProcessor::makeState()
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("version", 1);
    o->setProperty ("description", description);
    o->setProperty ("song", songName);
    o->setProperty ("options", options.toVar());
    juce::Array<juce::var> k;
    for (auto& kit : kits) k.add (kit.toVar());
    o->setProperty ("kits", k);
    o->setProperty ("active", activeKit);
    o->setProperty ("slot", gg::slotId (selectedSlot));
    o->setProperty ("groove", grooveId);
    o->setProperty ("swing", player.getSwing());
    o->setProperty ("volume", player.getVolume());
    o->setProperty ("follow", player.isFollowingHost());
    return juce::JSON::toString (juce::var (o), true);
}

void GrooveGrabberProcessor::setStateInformation (const void* data, int size)
{
    if (data == nullptr || size <= 0) return;
    const auto text = juce::String::fromUTF8 (static_cast<const char*> (data), size);
    if (juce::MessageManager::existsAndIsCurrentThread()) applyState (text);
    else juce::MessageManager::callAsync ([this, a = alive, text] { if (a->load()) applyState (text); });
}

void GrooveGrabberProcessor::applyState (const juce::String& text)
{
    const auto v = juce::JSON::parse (text);
    if (! v.isObject()) return;
    description = v["description"].toString();
    understood = gg::parseDescription (description);
    songName = v["song"].toString();
    options = gg::KitOptions::fromVar (v["options"]);
    kits.clear();
    if (auto* a = v["kits"].getArray())
        for (auto& item : *a)
        {
            if (kits.size() >= playables.size()) break;      // four kits, never more
            kits.push_back (gg::Kit::fromVar (item));
        }
    activeKit = juce::jlimit (0, juce::jmax (0, (int) kits.size() - 1), (int) v["active"]);
    const auto s = gg::slotFromId (v["slot"].toString());
    selectedSlot = s == gg::Slot::count ? gg::Slot::kick : s;
    setGrooveId (v.getProperty ("groove", "boombap1").toString());
    player.setSwing ((float) (double) v.getProperty ("swing", (double) player.getSwing()));
    player.setVolume ((float) (double) v.getProperty ("volume", 0.9));
    player.setFollowHost ((bool) v.getProperty ("follow", true));
    if (options.bpm > 0) player.setTempo (options.bpm);
    renderKits();
    notify();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrooveGrabberProcessor();
}
