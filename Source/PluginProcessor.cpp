#include "PluginProcessor.h"
#include "core/KitNames.h"
#include "core/MyGrooves.h"
#include "PluginEditor.h"

GrooveGrabberProcessor::GrooveGrabberProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    options.character = options.center();
    seed = (juce::uint32) juce::Time::currentTimeMillis();
    player.setGroove (&gg::grooveFor (grooveId));
    player.setTempo (options.style().bpm);
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
    if (text.trim().isEmpty()) genreByHand = false;
    understood = gg::parseDescription (text);
    // an artist or song is read inside the genre you picked (or the current one, if they have a take in it)
    gg::resolveDescription (understood, (int) options.genre, genreByHand, false);
    gg::applyDescription (understood, options);
    if (understood.bpm > 0) setBpm (understood.bpm);
    else if (understood.hasGenre) setBpm (options.style().bpm);
    if (understood.hasGenre)
        setGrooveId (firstGroove());
    notify();
}

juce::String GrooveGrabberProcessor::firstGroove() const
{
    return options.style().directions[(size_t) options.directionFor (0)].groove;
}

void GrooveGrabberProcessor::resetCharacter()
{
    options.character = options.center();
    for (int i = 0; i < gg::Traits::num; ++i)
        options.character[i] = juce::jlimit (0.0f, 1.0f, options.character[i] + understood.nudge[(size_t) i]);
}

void GrooveGrabberProcessor::setGenre (gg::Genre g)
{
    options.genre = g;
    genreByHand = true;
    // re-read the description in this genre: a style you typed for it ("g-funk") stays, an artist's
    // take for it is used ("Post Malone" in Country), else the era's style ("80s" + Pop -> Synth Pop / 80s)
    gg::resolveDescription (understood, (int) g, true, true);
    if (understood.subStyle >= 0 && ! understood.styleFromEra) options.subStyle = understood.subStyle;
    else options.subStyle = gg::styleForEra (g, options.era);
    options.leadDirection = options.subStyle == understood.subStyle ? understood.lead : -1;
    options.secondaryGenre = understood.secondaryGenre;
    resetCharacter();
    setGrooveId (firstGroove());
    notify();
}

void GrooveGrabberProcessor::setStyle (int sub)
{
    options.subStyle = juce::jlimit (0, (int) gg::stylesOf (options.genre).size() - 1, sub);
    // the artist's kit 1 only when it's their style
    options.leadDirection = understood.hasGenre && understood.genre == options.genre && understood.subStyle == options.subStyle
                              ? understood.lead : -1;
    resetCharacter();
    setGrooveId (firstGroove());
    if (understood.bpm <= 0) setBpm (options.style().bpm);
    notify();
}

void GrooveGrabberProcessor::setEra (int era)
{
    // move the sliders by the difference between the eras, keeping your own tweaks
    const auto before = gg::eraDelta (options.era);
    options.era = juce::jlimit (-1, gg::numEras - 1, era);
    const auto after = gg::eraDelta (options.era);
    for (int i = 0; i < gg::Traits::num; ++i)
        options.character[i] = juce::jlimit (0.0f, 1.0f, options.character[i] + after[(size_t) i] - before[(size_t) i]);
    notify();
}

void GrooveGrabberProcessor::setKey (int key)
{
    options.key = juce::jlimit (-1, 23, key);
    // retune what's there now (each kit keeps its sounds)
    auto gen = makeGenerator();
    for (auto& k : kits)
        gen.tuneToKey (k, options.key);     // (no key: just the kick to the 808)
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

void GrooveGrabberProcessor::useSongSavantSong (const gg::SongSavantSong& s)
{
    songName = s.name;
    genreByHand = false;
    applyDescription (s.description);               // the words: an artist, the feel, the character
    if (s.era >= 0 && s.era != options.era) setEra (s.era);
    if (s.genre >= 0) setGenre ((gg::Genre) s.genre);   // the genre and style Song Savant used win
    gg::Genre g = options.genre;
    int sub = 0;
    if (s.styleId.isNotEmpty() && gg::findStyle (s.styleId, g, sub) && g == options.genre && sub != options.subStyle)
        setStyle (sub);
    if (s.bpm > 0) setBpm (s.bpm);
    options.key = s.key;                            // the kick, 808 and toms are tuned to it as the kits are built
    generate (false);
    const auto built = statusMessage;
    juce::String msg = "From Song Savant: " + s.name;
    if (const auto sum = s.summary(); sum.isNotEmpty()) msg << " (" << sum << ")";
    notify (msg + ". " + built);
}

void GrooveGrabberProcessor::setGrooveId (const juce::String& wanted)
{
    // one of my grooves that's gone (deleted, or a song from another computer): the style's own groove
    const auto id = gg::mygrooves::isMine (wanted) && gg::mygrooves::find (wanted) == nullptr ? firstGroove() : wanted;
    grooveId = id;
    const auto g = getGroove (id);
    player.setGroove (g.get());
    player.setSwing (g->swing);
    notify();
}

//==============================================================================
std::shared_ptr<const gg::Groove> GrooveGrabberProcessor::getGroove (const juce::String& id) const
{
    if (gg::mygrooves::isMine (id))
    {
        if (auto mine = gg::mygrooves::find (id)) return mine;
        return getGroove (firstGroove());
    }
    if (const auto it = grooveEdits.find (id); it != grooveEdits.end()) return it->second;
    return std::shared_ptr<const gg::Groove> (std::shared_ptr<const gg::Groove>(), &gg::grooveFor (id));   // built in: not owned
}

bool GrooveGrabberProcessor::isGrooveEdited (const juce::String& id) const
{
    return grooveEdits.find (id) != grooveEdits.end();
}

void GrooveGrabberProcessor::editGroove (const juce::String& id, std::vector<gg::GrooveHit> hits)
{
    if (gg::mygrooves::isMine (id))
    {
        // your own groove: the change is saved to it straight away
        if (auto old = gg::mygrooves::update (id, std::move (hits)))
        {
            retireGroove (std::move (old));
            repointPlayer (id);
            notify();
        }
        return;
    }
    const auto& original = gg::grooveFor (id);
    if (original.id != id) return;                       // not one of ours
    gg::sortHits (hits);
    if (gg::sameHits (hits, original.hits)) { resetGroove (id); return; }
    auto g = std::make_shared<gg::Groove> (original);
    g->hits = std::move (hits);
    swapGroove (id, std::move (g));
}

void GrooveGrabberProcessor::resetGroove (const juce::String& id)
{
    if (! isGrooveEdited (id)) return;
    swapGroove (id, nullptr);
}

void GrooveGrabberProcessor::swapGroove (const juce::String& id, std::shared_ptr<const gg::Groove> edit)
{
    if (const auto it = grooveEdits.find (id); it != grooveEdits.end())
    {
        retireGroove (it->second);
        grooveEdits.erase (it);
    }
    if (edit != nullptr) grooveEdits[id] = std::move (edit);
    repointPlayer (id);
    notify();
}

void GrooveGrabberProcessor::retireGroove (std::shared_ptr<const gg::Groove> g)
{
    const auto now = juce::Time::getMillisecondCounter();
    if (g != nullptr) retiredGrooves.push_back ({ std::move (g), now });
    retiredGrooves.erase (std::remove_if (retiredGrooves.begin(), retiredGrooves.end(), [this, now] (const auto& r)
    {
        return now - r.second > 10000 && r.first.get() != player.getGroove();
    }), retiredGrooves.end());
}

void GrooveGrabberProcessor::repointPlayer (const juce::String& id)
{
    // whatever is playing this groove plays its current version (BUILD's preview, or a pattern on SAUCE)
    if (const auto* playing = player.getGroove(); playing != nullptr && playing->id == id)
        player.setGroove (getGroove (id).get());
}

juce::String GrooveGrabberProcessor::saveMyGroove (const juce::String& fromId, const juce::String& name, gg::Genre genre, const juce::String& styleId)
{
    const auto id = gg::mygrooves::save (*getGroove (fromId), name, genre, styleId);
    notify (id.isNotEmpty() ? "Saved \"" + name.trim() + "\" to MY GROOVES" : "Couldn't save the groove in " + gg::mygrooves::folder().getFullPathName());
    return id;
}

void GrooveGrabberProcessor::renameMyGroove (const juce::String& id, const juce::String& name)
{
    if (auto old = gg::mygrooves::rename (id, name))
    {
        retireGroove (std::move (old));
        repointPlayer (id);
        notify();
    }
}

void GrooveGrabberProcessor::deleteMyGroove (const juce::String& id)
{
    auto old = gg::mygrooves::remove (id);
    if (old == nullptr) return;
    retireGroove (std::move (old));
    if (grooveId == id) setGrooveId (firstGroove());       // BUILD's preview goes back to the style's groove
    repointPlayer (id);                                     // (SAUCE playing it: the fallback groove)
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
    gg::nameKits (kits, description, seed);     // fresh names every build
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
    const auto& dirs = options.style().directions;
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
    if (options.key < 0 && (slot == gg::Slot::kick || slot == gg::Slot::bass808))
        makeGenerator().tuneToKey (kits[(size_t) kitIndex], -1);     // keep the kick on the 808's note
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
    if (copy.empty()) return;          // nothing to render (a fresh instance, e.g. while a DAW scans plug-ins)
    const int active = juce::jlimit (0, (int) copy.size() - 1, activeKit);
    // the kit you're looking at first, then the others
    std::vector<int> order { active };
    for (int i = 0; i < (int) copy.size(); ++i) if (i != active) order.push_back (i);
    for (int i : order)
    {
        if (i >= (int) playables.size() || i >= (int) copy.size()) continue;
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
    o->setProperty ("genreByHand", genreByHand);
    o->setProperty ("song", songName);
    o->setProperty ("options", options.toVar());
    juce::Array<juce::var> k;
    for (auto& kit : kits) k.add (kit.toVar());
    o->setProperty ("kits", k);
    o->setProperty ("active", activeKit);
    o->setProperty ("slot", gg::slotId (selectedSlot));
    o->setProperty ("groove", grooveId);
    auto* edits = new juce::DynamicObject();
    for (const auto& [id, g] : grooveEdits) edits->setProperty (id, gg::hitsToVar (g->hits));
    o->setProperty ("grooveEdits", juce::var (edits));
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
    genreByHand = (bool) v.getProperty ("genreByHand", false);
    songName = v["song"].toString();
    options = gg::KitOptions::fromVar (v["options"]);
    understood = gg::parseDescription (description);
    gg::resolveDescription (understood, (int) options.genre, genreByHand, true);
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
    // your edited grooves (a project without any gets the originals back)
    {
        std::map<juce::String, std::shared_ptr<const gg::Groove>> restored;
        if (auto* edits = v["grooveEdits"].getDynamicObject())
            for (const auto& prop : edits->getProperties())
            {
                const auto id = prop.name.toString();
                const auto& original = gg::grooveFor (id);
                if (original.id != id || restored.size() >= 64) continue;
                auto hits = gg::hitsFromVar (prop.value, original);
                if (gg::sameHits (hits, original.hits)) continue;
                auto g = std::make_shared<gg::Groove> (original);
                g->hits = std::move (hits);
                restored[id] = std::move (g);
            }
        const auto now = juce::Time::getMillisecondCounter();
        for (auto& [id, g] : grooveEdits) retiredGrooves.push_back ({ g, now });
        grooveEdits = std::move (restored);
    }
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
