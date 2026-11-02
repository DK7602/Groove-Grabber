#include "MyGrooves.h"
#include "Settings.h"

namespace gg::mygrooves
{
    namespace
    {
        struct Store
        {
            juce::CriticalSection lock;
            bool loaded = false;
            std::vector<Entry> entries;
        };

        Store& store()
        {
            static Store s;
            return s;
        }

        juce::var toVar (const Entry& e)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("version", 1);
            o->setProperty ("id", e.groove->id);
            o->setProperty ("name", e.groove->name);
            o->setProperty ("genre", genreId (e.genre));
            o->setProperty ("style", e.styleId);
            o->setProperty ("from", e.from);
            o->setProperty ("bpm", e.groove->bpm);
            o->setProperty ("bars", e.groove->bars);
            o->setProperty ("swing", (double) e.groove->swing);
            o->setProperty ("swingEighths", e.groove->swingEighths);
            o->setProperty ("hits", hitsToVar (e.groove->hits));
            return juce::var (o);
        }

        bool fromVar (const juce::var& v, const juce::File& f, Entry& e)
        {
            if (! v.isObject()) return false;
            const auto id = v["id"].toString();
            if (! isMine (id)) return false;
            auto g = std::make_shared<Groove>();
            g->id = id;
            g->name = v["name"].toString().trim().substring (0, 60);
            if (g->name.isEmpty()) g->name = f.getFileNameWithoutExtension();
            const double bpm = v.getProperty ("bpm", 100.0);
            g->bpm = std::isfinite (bpm) ? juce::jlimit (40.0, 250.0, bpm) : 100.0;
            g->bars = juce::jlimit (1, 4, (int) v.getProperty ("bars", 2));
            const double swing = v.getProperty ("swing", 0.0);
            g->swing = std::isfinite (swing) ? (float) juce::jlimit (0.0, 0.7, swing) : 0.0f;
            g->swingEighths = (bool) v.getProperty ("swingEighths", false);
            g->hits = hitsFromVar (v["hits"], *g);
            e.groove = std::move (g);
            e.genre = genreFromId (v["genre"].toString());
            e.styleId = v["style"].toString();
            e.from = v["from"].toString();
            e.file = f;
            return true;
        }

        void sortByName (std::vector<Entry>& v)
        {
            std::sort (v.begin(), v.end(), [] (const Entry& a, const Entry& b)
                       { return a.groove->name.compareNatural (b.groove->name) < 0; });
        }

        void loadIfNeeded (Store& s)
        {
            if (s.loaded) return;
            s.loaded = true;
            s.entries.clear();
            for (const auto& it : juce::RangedDirectoryIterator (folder(), false, "*.gggroove", juce::File::findFiles))
            {
                Entry e;
                if (it.getFileSize() < 512 * 1024 && fromVar (juce::JSON::parse (it.getFile()), it.getFile(), e))
                {
                    bool dupe = false;
                    for (auto& x : s.entries) if (x.groove->id == e.groove->id) dupe = true;
                    if (! dupe) s.entries.push_back (std::move (e));
                }
            }
            sortByName (s.entries);
        }

        Entry* findEntry (Store& s, const juce::String& id)
        {
            loadIfNeeded (s);
            for (auto& e : s.entries)
                if (e.groove->id == id) return &e;
            return nullptr;
        }

        bool write (const Entry& e)
        {
            return e.file.replaceWithText (juce::JSON::toString (toVar (e)));
        }

        // A copy of the groove as it is now, before it's saved over, renamed or deleted
        void backUp (const Entry& e, const juce::String& why)
        {
            auto v = toVar (e);
            if (auto* o = v.getDynamicObject())
            {
                o->setProperty ("backupReason", why);
                o->setProperty ("backupTime", juce::Time::currentTimeMillis());
            }
            const auto dir = backupFolder();
            const auto stamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M-%S");
            dir.getNonexistentChildFile (paths::safeFileName (e.groove->name, 40) + " " + stamp, ".gggroove", false)
               .replaceWithText (juce::JSON::toString (v));

            // the newest 300 are plenty
            auto files = dir.findChildFiles (juce::File::findFiles, false, "*.gggroove");
            if (files.size() > 300)
            {
                std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
                           { return a.getLastModificationTime() > b.getLastModificationTime(); });
                for (int i = 300; i < files.size(); ++i) files.getReference (i).deleteFile();
            }
        }
    }

    juce::File backupFolder()
    {
        auto d = folder().getChildFile ("Backups");
        d.createDirectory();
        return d;
    }

    bool isMine (const juce::String& id)   { return id.startsWith ("my."); }

    juce::File folder()
    {
        auto d = paths::documentsDir().getChildFile ("My Grooves");
        d.createDirectory();
        return d;
    }

    std::vector<Entry> list()
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        loadIfNeeded (s);
        return s.entries;
    }

    std::shared_ptr<const Groove> find (const juce::String& id)
    {
        if (! isMine (id)) return nullptr;
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        if (auto* e = findEntry (s, id)) return e->groove;
        return nullptr;
    }

    const Entry* entry (const juce::String& id)
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        return findEntry (s, id);
    }

    juce::String save (const Groove& g, const juce::String& name, Genre genre, const juce::String& styleId)
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        loadIfNeeded (s);
        Entry e;
        auto mine = std::make_shared<Groove> (g);
        mine->id = "my." + juce::Uuid().toString();
        mine->name = name.trim().isNotEmpty() ? name.trim().substring (0, 60) : g.name;
        e.from = isMine (g.id) ? juce::String() : g.id;
        e.groove = std::move (mine);
        e.genre = genre;
        e.styleId = styleId;
        e.file = folder().getNonexistentChildFile (paths::safeFileName (e.groove->name), ".gggroove", false);
        if (! write (e)) return {};
        const auto id = e.groove->id;
        s.entries.push_back (std::move (e));
        sortByName (s.entries);
        return id;
    }

    std::shared_ptr<const Groove> update (const juce::String& id, std::vector<GrooveHit> hits)
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        auto* e = findEntry (s, id);
        if (e == nullptr) return nullptr;
        auto old = e->groove;
        backUp (*e, "before saving changes");
        auto g = std::make_shared<Groove> (*old);
        sortHits (hits);
        g->hits = std::move (hits);
        e->groove = std::move (g);
        write (*e);
        return old;
    }

    std::shared_ptr<const Groove> rename (const juce::String& id, const juce::String& name)
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        auto* e = findEntry (s, id);
        const auto clean = name.trim().substring (0, 60);
        if (e == nullptr || clean.isEmpty()) return nullptr;
        auto old = e->groove;
        backUp (*e, "before renaming to " + clean);
        auto g = std::make_shared<Groove> (*old);
        g->name = clean;
        e->groove = std::move (g);
        const auto oldFile = e->file;
        e->file = folder().getNonexistentChildFile (paths::safeFileName (clean), ".gggroove", false);
        if (write (*e) && oldFile != e->file) oldFile.deleteFile();
        sortByName (s.entries);
        return old;
    }

    std::shared_ptr<const Groove> remove (const juce::String& id)
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        loadIfNeeded (s);
        for (auto it = s.entries.begin(); it != s.entries.end(); ++it)
            if (it->groove->id == id)
            {
                auto g = it->groove;
                backUp (*it, "deleted");
                it->file.deleteFile();
                s.entries.erase (it);
                return g;
            }
        return nullptr;
    }

    std::vector<Backup> backups()
    {
        std::vector<Backup> out;
        for (const auto& f : backupFolder().findChildFiles (juce::File::findFiles, false, "*.gggroove"))
        {
            if (f.getSize() > 512 * 1024) continue;
            const auto v = juce::JSON::parse (f);
            if (! v.isObject()) continue;
            Backup b;
            b.file = f;
            b.name = v["name"].toString();
            b.why = v["backupReason"].toString();
            const juce::int64 ms = v.getProperty ("backupTime", (juce::int64) 0);
            b.when = ms > 0 ? juce::Time (ms) : f.getLastModificationTime();
            out.push_back (std::move (b));
        }
        std::sort (out.begin(), out.end(), [] (const Backup& a, const Backup& b) { return a.when > b.when; });
        return out;
    }

    juce::String restore (const juce::File& backupFile)
    {
        Entry e;
        if (! fromVar (juce::JSON::parse (backupFile), backupFile, e)) return {};
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        loadIfNeeded (s);
        // a groove of its own (a new id), so it never clashes with the one it was copied from
        auto g = std::make_shared<Groove> (*e.groove);
        g->id = "my." + juce::Uuid().toString();
        bool taken = false;
        for (const auto& x : s.entries) if (x.groove->name.equalsIgnoreCase (g->name)) taken = true;
        if (taken) g->name = (g->name + " (restored)").substring (0, 60);
        e.groove = std::move (g);
        e.file = folder().getNonexistentChildFile (paths::safeFileName (e.groove->name), ".gggroove", false);
        if (! write (e)) return {};
        const auto id = e.groove->id;
        s.entries.push_back (std::move (e));
        sortByName (s.entries);
        return id;
    }

    void reloadForTests()
    {
        auto& s = store();
        const juce::ScopedLock sl (s.lock);
        s.loaded = false;
        s.entries.clear();
    }
}
