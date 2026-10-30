// Groove Grabber - the sound library: every one-shot in the folders you point it at, analysed once
// and remembered (a small cache file in app data, shared by the app and every plug-in window).
//
// Scanning runs in the background at low priority on at most half the cores, so a DAW session keeps
// playing while a big library is analysed. Only new or changed files are analysed again.
#pragma once

#include "Settings.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

namespace gg
{
    using SoundList = std::vector<SoundInfo>;
    using SoundListPtr = std::shared_ptr<const SoundList>;

    class Library : public juce::ChangeBroadcaster,
                    private juce::Thread,
                    private juce::Timer
    {
    public:
        Library();
        ~Library() override;

        // ---- folders ----
        std::vector<SourceFolder> getSources() const;
        void addSource (const juce::File& folder, Source kind);
        void removeSource (int index);
        void setSourceKind (int index, Source kind);
        void setSourceEnabled (int index, bool enabled);

        // ---- scanning ----
        void startScan();                       // picks up new / changed / removed files
        void cancelScan();
        bool isScanning() const noexcept        { return scanning.load(); }
        float getProgress() const noexcept      { return progress.load(); }
        juce::String getStatus() const;
        void analyseFilesNow (const juce::Array<juce::File>& files, Source kind, const juce::File& root);   // blocking, any thread

        // ---- the sounds ----
        SoundListPtr getSounds() const;         // an immutable snapshot; cheap to copy
        int getVersion() const noexcept         { return version.load(); }
        std::optional<SoundInfo> find (const juce::String& path) const;
        std::array<int, numCats> countByCategory() const;

        // ---- your opinion of a sound ----
        void setCategory (const juce::String& path, Cat);
        void setRating (const juce::String& path, int rating);
        void setHidden (const juce::String& path, bool);

        juce::File getCacheFile() const;
        bool reloadIfChangedOnDisk();           // another Groove Grabber rescanned: pick it up

        Settings& getSettings() noexcept        { return *settings; }

        // Reads the files of one folder tree that Groove Grabber can analyse
        static bool isAudioFile (const juce::File&);

    private:
        void run() override;
        void timerCallback() override;
        void publish (SoundList&&);
        void editSound (const juce::String& path, const std::function<void (SoundInfo&)>&);
        bool loadCache();
        void saveCache (const SoundList&);

        juce::SharedResourcePointer<Settings> settings;
        mutable std::mutex soundsLock;
        std::recursive_mutex writeLock;      // one read-modify-publish-save at a time (edits, harvests, scans)
        SoundListPtr sounds;
        std::atomic<int> version { 0 };
        std::atomic<bool> scanning { false }, rescanRequested { false };
        std::atomic<float> progress { 0.0f };
        mutable juce::CriticalSection statusLock;
        juce::String status;
        juce::Time cacheTimeOnDisk;
        bool scannedOnce = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Library)
    };
}
