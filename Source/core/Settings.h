// Groove Grabber - where things live, and the settings every Groove Grabber on this computer shares.
#pragma once

#include "SoundInfo.h"
#include <juce_data_structures/juce_data_structures.h>

namespace gg
{
    namespace paths
    {
        juce::File appDataDir();             // .../Groove Grabber (settings, library cache, history)
        juce::File documentsDir();           // Documents/Groove Grabber
        juce::File exportsDir();             // Documents/Groove Grabber/Exports
        juce::File kitsDir();                // Documents/Groove Grabber/Kits
        juce::File harvestedDir();           // Documents/Groove Grabber/Harvested
        juce::File recordingsDir();          // Documents/Groove Grabber/My Recordings
        juce::File sampleSnaggerLibrary();   // Documents/Sample Snagger/Samples (Sample Snagger's default)
        juce::File keyCompassSongs();        // Key Compass's "My Songs" folder
        juce::String safeFileName (const juce::String&, int maxLength = 60);
    }

    struct SourceFolder
    {
        juce::File folder;
        Source kind = Source::packs;
        bool enabled = true;
    };

    class Settings
    {
    public:
        Settings();
        ~Settings();

        std::vector<SourceFolder> getSources() const;
        void setSources (const std::vector<SourceFolder>&);

        bool isLightTheme() const;
        void setLightTheme (bool);

        juce::File getExportDir() const;
        void setExportDir (const juce::File&);

        juce::File getKeyCompassDir() const;
        void setKeyCompassDir (const juce::File&);

        int getExportBitDepth() const;        // 16 or 24
        void setExportBitDepth (int);
        double getExportSampleRate() const;   // 44100 or 48000
        void setExportSampleRate (double);

        juce::String get (const juce::String& key, const juce::String& fallback = {}) const;
        void set (const juce::String& key, const juce::String& value);

        void reload();

    private:
        std::unique_ptr<juce::PropertiesFile> props;
        mutable juce::CriticalSection lock;
        JUCE_DECLARE_NON_COPYABLE (Settings)
    };
}
