#include "Library.h"
#include "Analyzer.h"
#include <unordered_map>

namespace gg
{
    namespace
    {
        constexpr int cacheMagic = 0x47474c31;   // "GGL1"
        constexpr int cacheVersion = 2;

        juce::String folderPartsBetween (const juce::File& root, const juce::File& file)
        {
            auto rel = file.getParentDirectory().getRelativePathFrom (root).replaceCharacter ('\\', '/');
            if (rel == ".") rel = {};
            return rel;
        }

        juce::String packNameFor (const juce::File& root, const juce::File& file)
        {
            const auto rel = folderPartsBetween (root, file);
            if (rel.isEmpty()) return root.getFileName();
            return rel.upToFirstOccurrenceOf ("/", false, false);
        }

        int numAnalysisThreads()
        {
            // leave the DAW room: at most half the cores, and always two free
            const int cores = juce::SystemStats::getNumCpus();
            return juce::jlimit (1, 4, juce::jmin (cores / 2, cores - 2));
        }
    }

    bool Library::isAudioFile (const juce::File& f)
    {
        if (f.getFileName().startsWithChar ('.') || f.isHidden()) return false;
        const auto ext = f.getFileExtension().toLowerCase();
        return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3" || ext == ".ogg";
    }

    //==============================================================================
    Library::Library() : juce::Thread ("Groove Grabber scan")
    {
        sounds = std::make_shared<const SoundList>();
        loadCache();
        startTimer (4000);
    }

    Library::~Library()
    {
        stopTimer();
        cancelScan();
        stopThread (8000);
    }

    juce::File Library::getCacheFile() const   { return paths::appDataDir().getChildFile ("library.ggl"); }

    std::vector<SourceFolder> Library::getSources() const   { return settings->getSources(); }

    void Library::addSource (const juce::File& folder, Source kind)
    {
        auto s = settings->getSources();
        for (auto& existing : s)
            if (existing.folder == folder) { existing.kind = kind; existing.enabled = true; settings->setSources (s); startScan(); return; }
        s.push_back ({ folder, kind, true });
        settings->setSources (s);
        startScan();
    }

    void Library::removeSource (int index)
    {
        auto s = settings->getSources();
        if (juce::isPositiveAndBelow (index, (int) s.size()))
        {
            s.erase (s.begin() + index);
            settings->setSources (s);
            startScan();
        }
    }

    void Library::setSourceKind (int index, Source kind)
    {
        auto s = settings->getSources();
        if (juce::isPositiveAndBelow (index, (int) s.size()))
        {
            s[(size_t) index].kind = kind;
            settings->setSources (s);
            // re-tag without re-analysing
            const auto root = s[(size_t) index].folder;
            std::lock_guard<std::recursive_mutex> wl (writeLock);
            SoundList copy (*getSounds());
            for (auto& snd : copy)
                if (juce::File (snd.path).isAChildOf (root))
                    snd.source = kind;
            publish (std::move (copy));
            saveCache (*getSounds());
        }
    }

    void Library::setSourceEnabled (int index, bool enabled)
    {
        auto s = settings->getSources();
        if (juce::isPositiveAndBelow (index, (int) s.size()))
        {
            s[(size_t) index].enabled = enabled;
            settings->setSources (s);
            startScan();
        }
    }

    //==============================================================================
    SoundListPtr Library::getSounds() const
    {
        std::lock_guard<std::mutex> l (soundsLock);
        return sounds;
    }

    void Library::publish (SoundList&& list)
    {
        std::sort (list.begin(), list.end(), [] (const SoundInfo& a, const SoundInfo& b) { return a.path < b.path; });
        {
            std::lock_guard<std::mutex> l (soundsLock);
            sounds = std::make_shared<const SoundList> (std::move (list));
        }
        ++version;
        sendChangeMessage();
    }

    std::optional<SoundInfo> Library::find (const juce::String& path) const
    {
        auto list = getSounds();
        auto it = std::lower_bound (list->begin(), list->end(), path, [] (const SoundInfo& s, const juce::String& p) { return s.path < p; });
        if (it != list->end() && it->path == path) return *it;
        return std::nullopt;
    }

    std::array<int, numCats> Library::countByCategory() const
    {
        std::array<int, numCats> c {};
        for (auto& s : *getSounds())
            if (! s.hidden)
                ++c[(size_t) s.cat];
        return c;
    }

    juce::String Library::getStatus() const
    {
        const juce::ScopedLock sl (statusLock);
        return status;
    }

    void Library::editSound (const juce::String& path, const std::function<void (SoundInfo&)>& fn)
    {
        std::lock_guard<std::recursive_mutex> wl (writeLock);
        SoundList copy (*getSounds());
        for (auto& s : copy)
            if (s.path == path) { fn (s); break; }
        publish (std::move (copy));
        saveCache (*getSounds());
    }

    void Library::setCategory (const juce::String& path, Cat c)
    {
        editSound (path, [c] (SoundInfo& s)
        {
            s.cat = c;
            s.catByUser = true;
            s.catConfidence = 1.0f;
            analysis::NameGuess g;
            g = analysis::guessFromName (juce::File (s.path).getFileNameWithoutExtension(), {});
            analysis::finish (s, g);
        });
    }

    void Library::setRating (const juce::String& path, int r)   { editSound (path, [r] (SoundInfo& s) { s.rating = juce::jlimit (-1, 1, r); }); }
    void Library::setHidden (const juce::String& path, bool h)  { editSound (path, [h] (SoundInfo& s) { s.hidden = h; }); }

    //==============================================================================
    bool Library::loadCache()
    {
        auto f = getCacheFile();
        if (! f.existsAsFile()) return false;
        juce::FileInputStream in (f);
        if (! in.openedOk() || in.readInt() != cacheMagic) return false;
        const int ver = in.readInt();
        if (ver != cacheVersion) return false;
        const int count = in.readInt();
        if (count < 0 || count > 5000000) return false;       // a damaged file
        SoundList list;
        list.reserve ((size_t) juce::jmin (count, 100000));
        for (int i = 0; i < count && ! in.isExhausted(); ++i)
        {
            SoundInfo s;
            if (! s.read (in, ver)) break;
            list.push_back (std::move (s));
        }
        cacheTimeOnDisk = f.getLastModificationTime();
        publish (std::move (list));
        return true;
    }

    void Library::saveCache (const SoundList& list)
    {
        auto f = getCacheFile();
        auto tmp = f.getSiblingFile ("library.tmp");
        {
            juce::FileOutputStream out (tmp);
            if (! out.openedOk()) return;
            out.setPosition (0);
            out.truncate();
            out.writeInt (cacheMagic);
            out.writeInt (cacheVersion);
            out.writeInt ((int) list.size());
            for (auto& s : list) s.write (out);
            out.flush();
            if (out.getStatus().failed()) return;
        }
        tmp.moveFileTo (f);
        cacheTimeOnDisk = f.getLastModificationTime();
    }

    bool Library::reloadIfChangedOnDisk()
    {
        if (scanning) return false;
        auto f = getCacheFile();
        if (f.existsAsFile() && f.getLastModificationTime() > cacheTimeOnDisk + juce::RelativeTime::milliseconds (500))
            return loadCache();
        return false;
    }

    void Library::timerCallback()
    {
        // a few seconds after opening, pick up anything added to your folders since last time
        if (! scannedOnce)
        {
            scannedOnce = true;
            if (! settings->getSources().empty() && ! isScanning())
            {
                startScan();
                return;
            }
        }
        reloadIfChangedOnDisk();
    }

    //==============================================================================
    void Library::startScan()
    {
        scannedOnce = true;
        rescanRequested = true;
        if (! isThreadRunning())
        {
            scanning = true;
            startThread (juce::Thread::Priority::low);
        }
    }

    void Library::cancelScan()
    {
        signalThreadShouldExit();
    }

    void Library::run()
    {
        while (rescanRequested.exchange (false) && ! threadShouldExit())
        {
            scanning = true;
            progress = 0.0f;
            { const juce::ScopedLock sl (statusLock); status = "Looking for sounds..."; }

            // ---- what's on disk ----
            struct Item { juce::File file, root; Source kind; };
            std::vector<Item> items;
            for (auto& src : settings->getSources())
            {
                if (! src.enabled || ! src.folder.isDirectory()) continue;
                for (const auto& entry : juce::RangedDirectoryIterator (src.folder, true, "*", juce::File::findFiles))
                {
                    if (threadShouldExit()) break;
                    const auto f = entry.getFile();
                    if (! isAudioFile (f) || f.getFullPathName().contains ("__MACOSX")) continue;
                    items.push_back ({ f, src.folder, src.kind });
                }
            }

            // ---- reuse what hasn't changed ----
            auto old = getSounds();
            std::unordered_map<std::string, const SoundInfo*> byPath;
            for (auto& s : *old) byPath[s.path.toStdString()] = &s;

            SoundList result;
            result.reserve (items.size());
            std::vector<size_t> todo;
            std::unordered_map<std::string, bool> seen;
            for (size_t i = 0; i < items.size(); ++i)
            {
                const auto key = items[i].file.getFullPathName().toStdString();
                if (seen.count (key)) continue;          // the same folder added twice
                seen[key] = true;
                auto it = byPath.find (key);
                if (it != byPath.end() && it->second->fileSize == items[i].file.getSize()
                     && it->second->modTime == items[i].file.getLastModificationTime().toMilliseconds())
                {
                    auto s = *it->second;
                    s.source = items[i].kind;
                    s.pack = packNameFor (items[i].root, items[i].file);
                    result.push_back (std::move (s));
                }
                else
                    todo.push_back (i);
            }

            // ---- analyse the new ones ----
            const int total = (int) todo.size();
            std::atomic<int> done { 0 };
            std::mutex resultLock;
            if (total > 0)
            {
                juce::ThreadPool pool (juce::ThreadPoolOptions{}.withThreadName ("GG analysis")
                                                                 .withNumberOfThreads (numAnalysisThreads())
                                                                 .withDesiredThreadPriority (juce::Thread::Priority::low));
                const int chunk = 32;
                for (int start = 0; start < total; start += chunk)
                {
                    pool.addJob ([&, start]
                    {
                        juce::AudioFormatManager fm;
                        fm.registerBasicFormats();
                        for (int k = start; k < juce::jmin (total, start + chunk); ++k)
                        {
                            if (threadShouldExit()) return;
                            const auto& it = items[todo[(size_t) k]];
                            SoundInfo s;
                            // keep your own category / rating / hidden through a re-analysis
                            if (auto prev = byPath.find (it.file.getFullPathName().toStdString()); prev != byPath.end())
                            {
                                s.catByUser = prev->second->catByUser;
                                if (s.catByUser) s.cat = prev->second->cat;
                                s.rating = prev->second->rating;
                                s.hidden = prev->second->hidden;
                            }
                            s.source = it.kind;
                            s.pack = packNameFor (it.root, it.file);
                            if (analysis::analyzeFile (it.file, folderPartsBetween (it.root, it.file), fm, s))
                            {
                                std::lock_guard<std::mutex> l (resultLock);
                                result.push_back (std::move (s));
                            }
                            const int d = ++done;
                            progress = (float) d / (float) total;
                        }
                    });
                }

                while (pool.getNumJobs() > 0 && ! threadShouldExit())
                {
                    {
                        const juce::ScopedLock sl (statusLock);
                        status = "Analysing " + juce::String (done.load()) + " of " + juce::String (total) + " new sounds...";
                    }
                    sendChangeMessage();
                    wait (250);
                }
                if (threadShouldExit())
                    pool.removeAllJobs (true, 4000);
            }

            if (threadShouldExit())
                break;

            // merge with what changed while scanning: your edits (favourite, never use, category) and
            // sounds harvested in the meantime win over the scan's older copy
            std::lock_guard<std::recursive_mutex> wl (writeLock);
            {
                auto now = getSounds();
                std::unordered_map<std::string, const SoundInfo*> current;
                for (auto& s : *now) current[s.path.toStdString()] = &s;
                std::unordered_map<std::string, bool> inResult;
                for (auto& s : result)
                {
                    inResult[s.path.toStdString()] = true;
                    if (auto it = current.find (s.path.toStdString()); it != current.end())
                    {
                        const auto& c = *it->second;
                        s.rating = c.rating;
                        s.hidden = c.hidden;
                        if (c.catByUser && ! (s.catByUser && s.cat == c.cat))
                        {
                            s.catByUser = true;
                            s.cat = c.cat;
                            s.catConfidence = 1.0f;
                            s.percKind = c.percKind;
                            s.traits = c.traits;
                        }
                    }
                }
                for (auto& s : *now)
                    if (! inResult.count (s.path.toStdString()) && ! byPath.count (s.path.toStdString())
                         && juce::File (s.path).existsAsFile())
                        result.push_back (s);       // added while the scan ran
            }
            const int count = (int) result.size();
            publish (std::move (result));
            saveCache (*getSounds());
            {
                const juce::ScopedLock sl (statusLock);
                status = juce::String (count) + " sounds" + (total > 0 ? " (" + juce::String (total) + " new)" : juce::String());
            }
            progress = 1.0f;
        }
        scanning = false;
        sendChangeMessage();
    }

    void Library::analyseFilesNow (const juce::Array<juce::File>& files, Source kind, const juce::File& root)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        SoundList fresh;
        for (auto& f : files)
        {
            SoundInfo s;
            s.source = kind;
            s.pack = packNameFor (root, f);
            if (analysis::analyzeFile (f, folderPartsBetween (root, f), fm, s)) fresh.push_back (std::move (s));
        }
        std::lock_guard<std::recursive_mutex> wl (writeLock);
        SoundList copy (*getSounds());
        for (auto& s : fresh)
        {
            bool replaced = false;
            for (auto& existing : copy)
                if (existing.path == s.path) { existing = s; replaced = true; break; }
            if (! replaced) copy.push_back (std::move (s));
        }
        publish (std::move (copy));
        saveCache (*getSounds());
    }
}
