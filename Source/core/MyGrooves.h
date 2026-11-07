// Groove Grabber - My grooves: patterns you saved as your own (SAVE AS MY GROOVE on SAUCE), for every song.
// One file each in Documents/Groove Grabber/My Grooves (<name>.gggroove, JSON), read once per run and kept in
// memory. Their ids start with "my." so they never clash with the built-in grooves.
//
// Message thread only. A groove is replaced, never changed in place: whoever plays the old version (the
// player holds a raw pointer) keeps it alive through the shared_ptr returned by update() and remove().
//
// Each keeps the kit it was saved with (the one playing on SAUCE then), so SAUCE can play it with "its original
// kit" later without touching BUILD's kits. Grooves saved before 1.0.29 have none until they're saved again.
//
// Nothing is ever lost: before a groove is saved over, renamed or deleted, a copy goes to My Grooves/Backups
// (the newest 300 are kept), and restore() brings any of them back as a groove of its own.
#pragma once

#include "Grooves.h"
#include "Kit.h"

namespace gg::mygrooves
{
    struct Entry
    {
        std::shared_ptr<const Groove> groove;   // id "my.<uuid>", name = what you called it
        Genre genre = Genre::boomBap;           // the genre and style it was saved from
        juce::String styleId, from;             // ... and the groove it started as ("boombap1")
        std::shared_ptr<const Kit> kit;         // the kit it was saved with (null: none kept, e.g. saved before 1.0.29)
        juce::File file;
    };

    bool isMine (const juce::String& grooveId);
    juce::File folder();                                          // Documents/Groove Grabber/My Grooves
    std::vector<Entry> list();                                    // by name
    std::shared_ptr<const Groove> find (const juce::String& id);  // null if there's no such groove
    const Entry* entry (const juce::String& id);                  // (valid until the next change)

    // A copy of the groove under your name, with the kit it's played with (if any); returns its id (empty if it
    // couldn't be written)
    juce::String save (const Groove&, const juce::String& name, Genre, const juce::String& styleId, const Kit* kit = nullptr);
    // its changes saved (returns the old version); a groove with no kit kept yet keeps `kitIfNone`, its own stays
    std::shared_ptr<const Groove> update (const juce::String& id, std::vector<GrooveHit> hits, float swing,
                                          const Kit* kitIfNone = nullptr);
    std::shared_ptr<const Groove> rename (const juce::String& id, const juce::String& name);       // returns the old version
    std::shared_ptr<const Groove> remove (const juce::String& id);                                // returns what was removed

    // Backups: the copies kept before each save, rename and delete, newest first
    struct Backup { juce::File file; juce::String name, why; juce::Time when; };
    juce::File backupFolder();                                    // My Grooves/Backups
    std::vector<Backup> backups();
    juce::String restore (const juce::File& backup);              // a new groove from a backup; returns its id

    void reloadForTests();                                        // forget what's in memory and read the folder again
}
