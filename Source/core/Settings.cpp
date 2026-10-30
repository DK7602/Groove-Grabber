#include "Settings.h"

namespace gg
{
    namespace paths
    {
        juce::File appDataDir()
        {
            auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
           #if JUCE_MAC
            auto dir = base.getChildFile ("Application Support").getChildFile ("Groove Grabber");
           #else
            auto dir = base.getChildFile ("Groove Grabber");
           #endif
           #if GG_TEST_RUNNER
            if (auto env = juce::SystemStats::getEnvironmentVariable ("GG_TEST_HOME", {}); env.isNotEmpty())
                dir = juce::File (env).getChildFile ("AppData");
           #endif
            dir.createDirectory();
            return dir;
        }

        juce::File documentsDir()
        {
            auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Groove Grabber");
           #if GG_TEST_RUNNER
            if (auto env = juce::SystemStats::getEnvironmentVariable ("GG_TEST_HOME", {}); env.isNotEmpty())
                dir = juce::File (env).getChildFile ("Documents");
           #endif
            return dir;
        }

        static juce::File docSub (const char* name)
        {
            auto d = documentsDir().getChildFile (name);
            d.createDirectory();
            return d;
        }

        juce::File exportsDir()       { return docSub ("Exports"); }
        juce::File kitsDir()          { return docSub ("Kits"); }
        juce::File harvestedDir()     { return docSub ("Harvested"); }
        juce::File recordingsDir()    { return docSub ("My Recordings"); }

        juce::File sampleSnaggerLibrary()
        {
            return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                     .getChildFile ("Sample Snagger").getChildFile ("Samples");
        }

        juce::File keyCompassSongs()
        {
            return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                     .getChildFile ("KeyCompass").getChildFile ("Songs");
        }

        juce::String safeFileName (const juce::String& name, int maxLen)
        {
            auto s = juce::File::createLegalFileName (name.trim())
                       .retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 _-().,&'!#+=[]")
                       .trim();
            if (s.isEmpty()) s = "Kit";
            return s.substring (0, maxLen).trim();
        }
    }

    //==============================================================================
    Settings::Settings()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "settings";
        o.filenameSuffix = ".xml";
        o.folderName = {};
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat = juce::PropertiesFile::storeAsXML;
        o.millisecondsBeforeSaving = 500;
        o.processLock = nullptr;
        props = std::make_unique<juce::PropertiesFile> (paths::appDataDir().getChildFile ("settings.xml"), o);

        // first run: suggest the folders Groove Grabber knows about
        if (! props->containsKey ("sources"))
        {
            std::vector<SourceFolder> s;
            s.push_back ({ paths::recordingsDir(), Source::recordings, true });
            s.push_back ({ paths::harvestedDir(), Source::harvested, true });
            if (paths::sampleSnaggerLibrary().isDirectory())
                s.push_back ({ paths::sampleSnaggerLibrary(), Source::sampleSnagger, true });
            setSources (s);
        }
    }

    Settings::~Settings()
    {
        props->saveIfNeeded();
    }

    std::vector<SourceFolder> Settings::getSources() const
    {
        const juce::ScopedLock sl (lock);
        std::vector<SourceFolder> out;
        auto v = juce::JSON::parse (props->getValue ("sources"));
        if (auto* a = v.getArray())
            for (auto& item : *a)
            {
                SourceFolder s;
                s.folder = juce::File (item["path"].toString());
                s.kind = (Source) juce::jlimit (0, (int) Source::count - 1, (int) item["kind"]);
                s.enabled = (bool) item.getProperty ("enabled", true);
                if (juce::File::isAbsolutePath (s.folder.getFullPathName()))
                    out.push_back (s);
            }
        return out;
    }

    void Settings::setSources (const std::vector<SourceFolder>& sources)
    {
        const juce::ScopedLock sl (lock);
        juce::Array<juce::var> a;
        for (auto& s : sources)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("path", s.folder.getFullPathName());
            o->setProperty ("kind", (int) s.kind);
            o->setProperty ("enabled", s.enabled);
            a.add (juce::var (o));
        }
        props->setValue ("sources", juce::JSON::toString (a, true));
        props->saveIfNeeded();
    }

    bool Settings::isLightTheme() const              { const juce::ScopedLock sl (lock); return props->getBoolValue ("light", false); }
    void Settings::setLightTheme (bool b)            { const juce::ScopedLock sl (lock); props->setValue ("light", b); }

    juce::File Settings::getExportDir() const
    {
        const juce::ScopedLock sl (lock);
        auto p = props->getValue ("exportDir");
        return p.isNotEmpty() && juce::File::isAbsolutePath (p) ? juce::File (p) : paths::exportsDir();
    }
    void Settings::setExportDir (const juce::File& f) { const juce::ScopedLock sl (lock); props->setValue ("exportDir", f.getFullPathName()); }

    juce::File Settings::getKeyCompassDir() const
    {
        const juce::ScopedLock sl (lock);
        auto p = props->getValue ("keyCompassDir");
        return p.isNotEmpty() && juce::File::isAbsolutePath (p) ? juce::File (p) : paths::keyCompassSongs();
    }
    void Settings::setKeyCompassDir (const juce::File& f) { const juce::ScopedLock sl (lock); props->setValue ("keyCompassDir", f.getFullPathName()); }

    int Settings::getExportBitDepth() const          { const juce::ScopedLock sl (lock); return props->getIntValue ("bits", 24) == 16 ? 16 : 24; }
    void Settings::setExportBitDepth (int b)         { const juce::ScopedLock sl (lock); props->setValue ("bits", b); }
    double Settings::getExportSampleRate() const     { const juce::ScopedLock sl (lock); return props->getDoubleValue ("rate", 44100.0) > 46000 ? 48000.0 : 44100.0; }
    void Settings::setExportSampleRate (double r)    { const juce::ScopedLock sl (lock); props->setValue ("rate", r); }

    juce::String Settings::get (const juce::String& k, const juce::String& fb) const { const juce::ScopedLock sl (lock); return props->getValue (k, fb); }
    void Settings::set (const juce::String& k, const juce::String& v)                 { const juce::ScopedLock sl (lock); props->setValue (k, v); }

    void Settings::reload()                          { const juce::ScopedLock sl (lock); props->reload(); }
}
