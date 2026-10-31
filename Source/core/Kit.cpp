#include "Kit.h"
#include "Settings.h"
#include <cmath>

namespace gg
{
    bool SlotState::sameSound (const SlotState& o) const noexcept
    {
        return path == o.path && autoTune == o.autoTune && userTune == o.userTune && startMs == o.startMs
            && lengthPct == o.lengthPct && gainDb == o.gainDb && reverse == o.reverse;
    }

    juce::var SlotState::toVar() const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("path", path);
        if (autoTune != 0)    o->setProperty ("autoTune", autoTune);
        if (userTune != 0)    o->setProperty ("tune", userTune);
        if (startMs != 0)     o->setProperty ("start", startMs);
        if (lengthPct != 100) o->setProperty ("length", lengthPct);
        if (gainDb != 0)      o->setProperty ("gain", gainDb);
        if (reverse)          o->setProperty ("reverse", true);
        if (locked)           o->setProperty ("locked", true);
        return juce::var (o);
    }

    SlotState SlotState::fromVar (const juce::var& v)
    {
        SlotState s;
        s.path = v["path"].toString();
        // anything read from a file is kept inside the ranges the controls allow
        auto num = [&v] (const char* key, double fallback, double lo, double hi)
        {
            const double x = (double) v.getProperty (key, fallback);
            return (float) (std::isfinite (x) ? juce::jlimit (lo, hi, x) : fallback);
        };
        s.autoTune = num ("autoTune", 0.0, -12.0, 12.0);
        s.userTune = num ("tune", 0.0, -12.0, 12.0);
        s.startMs = num ("start", 0.0, 0.0, 250.0);
        s.lengthPct = num ("length", 100.0, 5.0, 100.0);
        s.gainDb = num ("gain", 0.0, -24.0, 12.0);
        s.reverse = (bool) v.getProperty ("reverse", false);
        s.locked = (bool) v.getProperty ("locked", false);
        return s;
    }

    int Kit::numFilled() const noexcept
    {
        int n = 0;
        for (auto& s : slots) n += s.isEmpty() ? 0 : 1;
        return n;
    }

    juce::String Kit::newId()   { return juce::Uuid().toString().substring (0, 12); }

    juce::var Kit::toVar() const
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("format", "groove-grabber-kit");
        o->setProperty ("version", 1);
        o->setProperty ("id", id);
        o->setProperty ("name", name);
        o->setProperty ("blurb", blurb);
        o->setProperty ("direction", directionId);
        o->setProperty ("genre", genreId (genre));
        o->setProperty ("style", styleId);
        o->setProperty ("era", era);
        o->setProperty ("target", target.toVar());
        o->setProperty ("key", key);
        o->setProperty ("bpm", bpm);
        o->setProperty ("tuneToKey", tuneToKey);
        o->setProperty ("favorite", favorite);
        o->setProperty ("description", description);
        o->setProperty ("created", juce::String (created));
        auto* sl = new juce::DynamicObject();
        for (int i = 0; i < numSlots; ++i)
            if (! slots[(size_t) i].isEmpty() || slots[(size_t) i].locked)
                sl->setProperty (slotId ((Slot) i), slots[(size_t) i].toVar());
        o->setProperty ("slots", juce::var (sl));
        return juce::var (o);
    }

    Kit Kit::fromVar (const juce::var& v)
    {
        Kit k;
        if (! v.isObject()) return k;
        k.id = v["id"].toString();
        if (k.id.isEmpty()) k.id = newId();
        k.name = v["name"].toString();
        k.blurb = v["blurb"].toString();
        k.directionId = v["direction"].toString();
        k.genre = genreFromId (v["genre"].toString());
        k.styleId = v["style"].toString();
        k.era = juce::jlimit (-1, numEras - 1, (int) v.getProperty ("era", -1));
        k.target = Traits::fromVar (v["target"]);
        k.key = juce::jlimit (-1, 23, (int) v.getProperty ("key", -1));
        k.bpm = (double) v.getProperty ("bpm", 0.0);
        if (! std::isfinite (k.bpm) || k.bpm < 0 || k.bpm > 300) k.bpm = 0;
        k.tuneToKey = (bool) v.getProperty ("tuneToKey", true);
        k.favorite = (bool) v.getProperty ("favorite", false);
        k.description = v["description"].toString();
        k.created = v["created"].toString().getLargeIntValue();
        if (auto* sl = v["slots"].getDynamicObject())
            for (auto& p : sl->getProperties())
            {
                const auto s = slotFromId (p.name.toString());
                if (s != Slot::count)
                    k[s] = SlotState::fromVar (p.value);
            }
        return k;
    }

    //==============================================================================
    namespace kitstore
    {
        juce::File folderFor (Genre g)
        {
            return paths::kitsDir().getChildFile (genreFolderName (g));
        }

        static void removeFolderIfEmpty (const juce::File& dir)
        {
            const auto root = paths::kitsDir();
            if (dir != root && dir.isAChildOf (root) && dir.isDirectory()
                 && dir.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 0)
                dir.deleteFile();
        }

        // kits saved before genre folders sit loose in Kits/: file each one in its genre's folder
        static void fileLooseKits()
        {
            juce::Array<juce::File> loose;
            for (const auto& e : juce::RangedDirectoryIterator (paths::kitsDir(), false, "*.ggkit", juce::File::findFiles))
                loose.add (e.getFile());
            for (auto& f : loose)
            {
                const auto k = Kit::fromVar (juce::JSON::parse (f));
                if (! k.isValid()) continue;
                auto dir = folderFor (k.genre);
                if (dir.createDirectory())
                    f.moveFileTo (dir.getNonexistentChildFile (f.getFileNameWithoutExtension(), ".ggkit", false));
            }
        }

        juce::File save (const Kit& k)
        {
            // one file per kit id, named after the kit, in its genre's folder
            for (auto& [file, kit] : listSaved())
                if (kit.id == k.id)
                {
                    file.deleteFile();
                    if (kit.genre != k.genre) removeFolderIfEmpty (file.getParentDirectory());
                }
            auto dir = folderFor (k.genre);
            if (! dir.createDirectory()) dir = paths::kitsDir();
            auto f = dir.getNonexistentChildFile (paths::safeFileName (k.name.isNotEmpty() ? k.name : "Kit"), ".ggkit", false);
            f.replaceWithText (juce::JSON::toString (k.toVar()));
            return f;
        }

        std::vector<std::pair<juce::File, Kit>> listSaved()
        {
            fileLooseKits();
            std::vector<std::pair<juce::File, Kit>> out;
            for (const auto& e : juce::RangedDirectoryIterator (paths::kitsDir(), true, "*.ggkit", juce::File::findFiles))
            {
                auto k = Kit::fromVar (juce::JSON::parse (e.getFile()));
                if (k.isValid()) out.emplace_back (e.getFile(), k);
            }
            std::sort (out.begin(), out.end(), [] (auto& a, auto& b) { return a.second.created > b.second.created; });
            return out;
        }

        bool remove (const juce::File& f)
        {
            const bool ok = f.deleteFile();
            removeFolderIfEmpty (f.getParentDirectory());
            return ok;
        }

        static juce::File historyFile()     { return paths::appDataDir().getChildFile ("history.json"); }

        void addToHistory (const std::vector<Kit>& kits)
        {
            auto list = history();
            for (auto it = kits.rbegin(); it != kits.rend(); ++it)
            {
                list.erase (std::remove_if (list.begin(), list.end(), [&] (const Kit& k) { return k.id == it->id; }), list.end());
                list.insert (list.begin(), *it);
            }
            if (list.size() > 48) list.resize (48);
            juce::Array<juce::var> a;
            for (auto& k : list) a.add (k.toVar());
            historyFile().replaceWithText (juce::JSON::toString (a, true));
        }

        std::vector<Kit> history()
        {
            std::vector<Kit> out;
            auto v = juce::JSON::parse (historyFile());
            if (auto* a = v.getArray())
                for (auto& item : *a)
                {
                    auto k = Kit::fromVar (item);
                    if (k.isValid()) out.push_back (k);
                }
            return out;
        }
    }
}
