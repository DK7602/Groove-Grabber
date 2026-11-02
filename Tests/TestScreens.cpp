// Draws the whole window (dark and light, every page) to PNGs, so the look can be checked without a DAW.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "core/Settings.h"
#include "core/MyGrooves.h"
#include <iostream>

namespace
{
    void save (juce::Component& c, const juce::File& f)
    {
        auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
        f.deleteFile();
        juce::FileOutputStream out (f);
        juce::PNGImageFormat png;
        png.writeImageToStream (img, out);
        std::cout << "  wrote " << f.getFullPathName() << std::endl;
    }

    void pump (int ms)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) ms;
        while (juce::Time::getMillisecondCounter() < end)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }
}

int runScreenshots (const juce::File& outDir)
{
    std::cout << "\n== Screenshots" << std::endl;
    outDir.createDirectory();
    int failures = 0;

    GrooveGrabberProcessor proc;
    proc.prepareToPlay (44100.0, 512);
    // the library the engine tests built lives in the test home
    for (int i = 0; i < 400 && proc.library->isScanning(); ++i) pump (25);
    if (proc.library->getSounds()->empty())
    {
        std::cout << "  (no sounds in the test library - screenshots will be empty)" << std::endl;
        ++failures;
    }
    proc.applyDescription ("Dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor");
    proc.options.lowEnd = 1;
    proc.options.percCount = 2;
    proc.generate (false);
    proc.selectedSlot = gg::Slot::bass808;
    for (int i = 0; i < 3 && i < (int) proc.kits.size(); ++i) { auto k = proc.kits[(size_t) i]; k.favorite = i == 0; gg::kitstore::save (k); }
    if (! proc.kits.empty())
    {
        // a few more genres, so MY KITS shows its genre chips
        struct G { gg::Genre genre; const char* style; const char* name; };
        for (const auto& x : { G { gg::Genre::rock, "rock.hard", "Rock Hard Hitter" }, G { gg::Genre::country, "country.classic", "Honky Tonk Classic" },
                               G { gg::Genre::trap, "trap.atlanta", "ATL Trap House" }, G { gg::Genre::rock, "rock.hard", "80s Arena Hard Rock" } })
        {
            auto k = proc.kits.front();
            k.id = juce::Uuid().toString();
            k.genre = x.genre;
            k.styleId = x.style;
            k.name = x.name;
            k.favorite = false;
            gg::kitstore::save (k);
        }
    }
    pump (1500);

    for (bool light : { false, true })
    {
        proc.getSettings().setLightTheme (light);
        std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
        ed->setSize (1440, 900);
        ed->showTab (0);
        ed->refreshAll();
        pump (900);
        ed->refreshAll();
        pump (300);
        const juce::String suffix = light ? "-light" : "";
        save (*ed, outDir.getChildFile ("build" + suffix + ".png"));
        if (! light)
        {
            // a song Song Savant just sent: BUILD offers it under the description
            auto f = gg::paths::songSavantSongs().getChildFile ("Back Roads.sssong");
            f.getParentDirectory().createDirectory();
            f.replaceWithText (R"({"app": "Song Savant", "name": "Back Roads", "genre": "country", "style": "country.modern", "key": 21, "bpm": 92, "sent": ")"
                               + juce::Time::getCurrentTime().toISO8601 (true) + "\"}");
            pump (2300);
            save (*ed, outDir.getChildFile ("build-songsavant.png"));
            f.deleteFile();
            pump (2300);
        }
        ed->showTab (1);
        pump (300);
        save (*ed, outDir.getChildFile ("library" + suffix + ".png"));
        ed->showTab (2);
        pump (200);
        save (*ed, outDir.getChildFile ("kits" + suffix + ".png"));
        ed->showTab (3);
        pump (300);
        save (*ed, outDir.getChildFile ("sauce" + suffix + ".png"));
        if (! ed->getSauce().isFollowingBuild()) { std::cout << "  FAIL: SAUCE didn't open on BUILD's style" << std::endl; ++failures; }
        if (! light)
        {
            // edit the pattern on the grid: BUILD's preview groove changes with it
            auto& sauce = ed->getSauce();
            const auto id = sauce.getGrooveId();
            const auto before = proc.getGroove (id)->hits.size();
            sauce.getGrid().clickStep (0, 2, {});                                          // a kick on the & of 1
            sauce.getGrid().clickStep (0, 2, juce::ModifierKeys (juce::ModifierKeys::rightButtonModifier));   // ...medium
            sauce.getGrid().clickStep (2, 15, juce::ModifierKeys (juce::ModifierKeys::ctrlModifier));        // a roll at the end of bar 1
            pump (200);
            save (*ed, outDir.getChildFile ("sauce-edited.png"));
            const bool edited = proc.isGrooveEdited (id) && proc.getGroove (id)->hits.size() == before + 3
                                && (id != proc.grooveId || proc.player.getGroove() == proc.getGroove (id).get());
            std::cout << (edited ? "  ok: " : "  FAIL: ") << "pattern edited on SAUCE (" << id << "), and the preview plays the edit" << std::endl;
            if (! edited) ++failures;

            // SAVE AS MY GROOVE: the name box, then the groove itself. The pattern it started from goes back to
            // its original; BUILD keeps playing whatever it played.
            const auto editedHits = proc.getGroove (id)->hits;
            const auto buildGroove = proc.grooveId;
            sauce.startNaming();
            pump (150);
            save (*ed, outDir.getChildFile ("sauce-naming.png"));
            sauce.finishNaming (true, "Late Kick Bounce");
            pump (200);
            const auto mine = sauce.getGrooveId();
            save (*ed, outDir.getChildFile ("sauce-mine.png"));
            const auto mg = gg::mygrooves::find (mine);
            const bool saved = gg::mygrooves::isMine (mine) && mg != nullptr && mg->name == "Late Kick Bounce" && gg::sameHits (mg->hits, editedHits)
                               && ! proc.isGrooveEdited (id) && gg::sameHits (proc.getGroove (id)->hits, gg::grooveFor (id).hits)
                               && proc.grooveId == buildGroove;
            std::cout << (saved ? "  ok: " : "  FAIL: ") << "saved under its new name; the original is back to the original, BUILD unchanged" << std::endl;
            if (! saved) ++failures;
            // changing your groove: nothing is saved into it until SAVE
            sauce.getGrid().clickStep (0, 5, {});
            const bool waits = proc.isGrooveEdited (mine) && gg::mygrooves::find (mine)->hits.size() == mg->hits.size()
                               && proc.getGroove (mine)->hits.size() == mg->hits.size() + 1;
            save (*ed, outDir.getChildFile ("sauce-mine-changed.png"));
            // SAVE AS COPY: the copy gets the changes, your groove stays exactly as it was saved
            sauce.startNaming();
            sauce.finishNaming (true, "Late Kick Copy");
            pump (100);
            const auto copy = sauce.getGrooveId();
            const bool copied = gg::mygrooves::isMine (copy) && copy != mine && gg::mygrooves::find (copy)->hits.size() == mg->hits.size() + 1
                                && ! proc.isGrooveEdited (mine) && gg::sameHits (gg::mygrooves::find (mine)->hits, mg->hits);
            // SAVE: the changes go into it, and the version before is kept in Backups
            const auto backupsBefore = gg::mygrooves::backups().size();
            sauce.pickMine (mine);
            sauce.getGrid().clickStep (0, 6, {});
            const bool savedChanges = proc.saveGrooveChanges (mine) && ! proc.isGrooveEdited (mine)
                                      && gg::mygrooves::find (mine)->hits.size() == mg->hits.size() + 1
                                      && gg::mygrooves::backups().size() == backupsBefore + 1;
            proc.deleteMyGroove (copy);
            const bool savesAsYouGo = waits && copied && savedChanges;
            std::cout << (waits ? "  ok: " : "  FAIL: ") << "changing your groove waits for SAVE" << std::endl;
            std::cout << (copied ? "  ok: " : "  FAIL: ") << "SAVE AS COPY: a new groove with the changes, yours stays as saved" << std::endl;
            std::cout << (savedChanges ? "  ok: " : "  FAIL: ") << "SAVE: the changes go into your groove, the version before into Backups" << std::endl;
            // BUILD can play it; deleting it sends BUILD back to the style's groove
            proc.setGrooveId (mine);
            const bool plays = proc.grooveId == mine && proc.player.getGroove() == gg::mygrooves::find (mine).get();
            proc.deleteMyGroove (mine);
            pump (100);
            const bool fellBack = proc.grooveId == proc.firstGroove() && proc.player.getGroove() != nullptr
                                  && ! gg::mygrooves::isMine (proc.player.getGroove()->id) && ! gg::mygrooves::isMine (sauce.getGrooveId());
            proc.setGrooveId ("my.gone-for-good");
            const bool missing = proc.grooveId == proc.firstGroove();
            proc.setGrooveId (buildGroove);
            // deleting kept a copy in Backups; restoring it brings it back as a groove of its own
            const auto backs = gg::mygrooves::backups();
            const auto restored = backs.empty() ? juce::String() : proc.restoreMyGroove (backs.front().file);
            const bool restoredOk = gg::mygrooves::isMine (restored) && restored != mine && gg::mygrooves::find (restored) != nullptr
                                    && backs.front().why == "deleted";
            if (restoredOk) proc.deleteMyGroove (restored);
            std::cout << (restoredOk ? "  ok: " : "  FAIL: ") << "a deleted groove is kept in Backups and comes back with RESTORE" << std::endl;
            // EDIT IN SAUCE from BUILD: whatever BUILD plays, even a groove from another style
            sauce.openGroove ("disco1");
            const bool opened = sauce.getGrooveId() == "disco1";
            std::cout << (opened ? "  ok: " : "  FAIL: ") << "EDIT IN SAUCE opens BUILD's groove, even from another style" << std::endl;
            const bool ok = savesAsYouGo && plays && fellBack && missing && restoredOk && opened;
            std::cout << (ok ? "  ok: " : "  FAIL: ") << "my groove plays on BUILD, and deleting it falls back" << std::endl;
            if (! ok) ++failures;

            // SAVE & REPLACE ON BUILD: BUILD plays yours in place of the original (which stays the original)
            ed->getSauce().choose (proc.options.genre, proc.options.subStyle, proc.options.era);
            pump (100);
            const auto from = sauce.getGrooveId();
            sauce.getGrid().clickStep (1, 7, {});                                         // a snare on the & of 2
            const auto replacedHits = proc.getGroove (from)->hits;
            sauce.startNaming();
            sauce.finishNaming (true, "Bounce Two", true);
            pump (200);
            const auto two = sauce.getGrooveId();
            const bool replaced = gg::mygrooves::isMine (two) && proc.grooveId == two && proc.player.getGroove() == gg::mygrooves::find (two).get()
                                  && gg::sameHits (gg::mygrooves::find (two)->hits, replacedHits) && ! proc.isGrooveEdited (from);
            std::cout << (replaced ? "  ok: " : "  FAIL: ") << "SAVE & REPLACE ON BUILD: BUILD plays yours, the original stays the original" << std::endl;
            if (! replaced) ++failures;
        }
        if (! light)
        {
            // another style, read up on from SAUCE: BUILD stays where it was
            ed->getSauce().choose (gg::Genre::trap, gg::styleIndex (gg::Genre::trap, "trap.ukdrill"), 5);
            pump (200);
            save (*ed, outDir.getChildFile ("sauce-drill.png"));
            if (ed->getSauce().isFollowingBuild() || proc.options.genre != gg::Genre::boomBap)
            { std::cout << "  FAIL: picking a style on SAUCE changed BUILD" << std::endl; ++failures; }
            ed->setSize (1180, 760);
            pump (200);
            save (*ed, outDir.getChildFile ("sauce-small.png"));
            ed->setSize (1440, 900);
            ed->getSauce().choose (gg::Genre::trap, gg::styleIndex (gg::Genre::trap, "trap.atlanta"), 6);
            pump (200);
            save (*ed, outDir.getChildFile ("sauce-trap.png"));
            if (ed->getSauce().getGrid().numLanes() < 5) { std::cout << "  FAIL: trap pattern grid" << std::endl; ++failures; }
            ed->setSize (1440, 900);

            // the export panel, written for Cubase: before and after exporting
            proc.getSettings().set ("daw", "cubase");
            ed->showTab (0);
            if (auto* k = proc.getActiveKit()) ed->getExporter().show (*k);
            pump (200);
            save (*ed, outDir.getChildFile ("export.png"));
            for (auto* c : ed->getExporter().getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == "EXPORT KIT") b->triggerClick();
            for (int i = 0; i < 200 && ! ed->getExporter().getChildren().isEmpty(); ++i)
            {
                pump (50);
                bool shown = false;
                for (auto* c : ed->getExporter().getChildren())
                    if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == "SHOW FOLDER" && b->isVisible()) shown = true;
                if (shown) break;
            }
            pump (200);
            save (*ed, outDir.getChildFile ("export-done.png"));
            {
                // the export's Groove .mid is the edited pattern
                juce::File mid;
                if (auto* k = proc.getActiveKit())
                    for (const auto& f : gg::paths::exportsDir().findChildFiles (juce::File::findFiles, true, "*Groove.mid"))
                        if (f.getFileName().startsWith (gg::paths::safeFileName (k->name, 48)) && (mid == juce::File() || f.getLastModificationTime() > mid.getLastModificationTime())) mid = f;
                juce::FileInputStream in (mid);
                juce::MidiFile mf;
                int notes = 0;
                if (in.openedOk() && mf.readFrom (in))
                    for (auto* ev : *mf.getTrack (0)) if (ev->message.isNoteOn()) ++notes;
                const int want = 2 * (int) proc.getGroove (proc.grooveId)->hits.size();
                const bool ok = gg::mygrooves::isMine (proc.grooveId) && notes == want;
                std::cout << (ok ? "  ok: " : "  FAIL: ") << "exported Groove .mid is the groove BUILD plays: yours (" << notes << " notes, want " << want << ")" << std::endl;
                if (! ok) ++failures;
            }
            {
                // MY KITS: your grooves, under MY GROOVES
                ed->getExporter().setVisible (false);
                ed->showTab (2);
                ed->getKitsPage().setView (2);
                pump (200);
                save (*ed, outDir.getChildFile ("kits-grooves.png"));
                const bool listed = ed->getKitsPage().numRows() == (int) gg::mygrooves::list().size() && ed->getKitsPage().numRows() >= 1;
                std::cout << (listed ? "  ok: " : "  FAIL: ") << "MY KITS lists my grooves (" << ed->getKitsPage().numRows() << ")" << std::endl;
                if (! listed) ++failures;
                ed->getKitsPage().setView (0);
                const auto two = proc.grooveId;
                proc.setGrooveId (proc.firstGroove());
                proc.deleteMyGroove (two);
                ed->showTab (0);
            }
            proc.getSettings().set ("daw", "fl");
            if (auto* k = proc.getActiveKit()) ed->getExporter().show (*k);
            pump (200);
            save (*ed, outDir.getChildFile ("export-fl.png"));
            proc.getSettings().set ("daw", {});
            ed->getExporter().setVisible (false);
            ed->setSize (1180, 760);
            ed->showTab (0);
            ed->refreshAll();
            pump (300);
            save (*ed, outDir.getChildFile ("build-small.png"));
        }
        ed.reset();
        pump (100);
    }
    proc.getSettings().setLightTheme (false);

    // many folders: the list must scroll, not squash the last one
    {
        auto sources = proc.getSettings().getSources();
        for (int i = 0; i < 12; ++i)
            sources.push_back ({ juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("GG Pack " + juce::String (i + 1)), gg::Source::packs, true });
        proc.getSettings().setSources (sources);
        std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
        ed->setSize (1440, 900);
        ed->showTab (1);
        pump (300);
        save (*ed, outDir.getChildFile ("library-many-folders.png"));
    }

    // the manual and About, inside the window
    for (bool light : { false, true })
    {
        proc.getSettings().setLightTheme (light);
        std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (proc.createEditor()));
        ed->setSize (1440, 900);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        ed->showHelp (0);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        pump (300);
        auto& help = ed->getHelp();
        const auto sections = (int) help.getManual().getSections().size();
        std::cout << "  manual: " << sections << " sections, " << help.getManual().numBlocks() << " blocks, opened in " << (int) ms << " ms" << std::endl;
        if (sections < 12 || ! help.isVisible() || ms > 2000) { std::cout << "  FAIL: manual didn't open properly" << std::endl; ++failures; }
        const juce::String suffix = light ? "-light" : "";
        save (*ed, outDir.getChildFile ("manual" + suffix + ".png"));
        help.scrollToSection (8);
        pump (200);
        save (*ed, outDir.getChildFile ("manual-export" + suffix + ".png"));
        if (help.getManual().setSearch ("groove agent") < 5) { std::cout << "  FAIL: manual search" << std::endl; ++failures; }
        help.getManual().setSearch ({});
        ed->showHelp (1);
        pump (200);
        save (*ed, outDir.getChildFile ("about" + suffix + ".png"));
        if (help.getPage() != 1) { std::cout << "  FAIL: About page" << std::endl; ++failures; }
    }
    proc.getSettings().setLightTheme (false);

    // saved state: round trip, and a damaged / oversized one doesn't crash
    {
        proc.editGroove ("trap1", gg::withStep (*proc.getGroove ("trap1"), gg::Slot::kick, 3, 2, gg::mediumHit));
        pump (100);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        GrooveGrabberProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool same = other.kits.size() == proc.kits.size() && other.description == proc.description;
        if (! same) { std::cout << "  FAIL: state round trip" << std::endl; ++failures; }
        const bool keptEdit = other.isGrooveEdited ("trap1") && gg::sameHits (other.getGroove ("trap1")->hits, proc.getGroove ("trap1")->hits)
                              && ! other.isGrooveEdited ("boombap1");
        std::cout << (keptEdit ? "  ok: " : "  FAIL: ") << "an edited pattern is saved with the song" << std::endl;
        if (! keptEdit) ++failures;
        proc.editGroove ("trap1", gg::grooveFor ("trap1").hits);
        if (proc.isGrooveEdited ("trap1")) { std::cout << "  FAIL: the original's hits should reset the edit" << std::endl; ++failures; }
        auto v = juce::JSON::parse (mb.toString());
        if (auto* a = v["kits"].getArray()) { auto copy = *a; for (auto& k : copy) a->add (k); }   // 8 kits
        const auto big = juce::JSON::toString (v);
        other.setStateInformation (big.toRawUTF8(), (int) big.getNumBytesAsUTF8());
        pump (300);
        if (other.kits.size() > 4) { std::cout << "  FAIL: more than 4 kits loaded" << std::endl; ++failures; }
        const char junk[] = "{\"kits\": [ {\"slots\": {\"kick\": {\"path\": 5, \"tune\": \"abc\"}}} ], \"active\": 99, "
                            "\"grooveEdits\": {\"boombap1\": \"junk\", \"nope\": [[0, \"kick\", 1]], \"trap1\": [[0.0, \"kick\", 1], [99, \"snare\", 1]], \"rnb1\": 7}}";
        other.setStateInformation (junk, (int) sizeof (junk) - 1);
        pump (300);
        std::cout << "  state round trip and damaged state ok" << std::endl;
    }

    // a brand-new instance's state restored into itself (what a DAW does when it scans or loads a default preset)
    {
        GrooveGrabberProcessor fresh;
        juce::MemoryBlock mb;
        fresh.getStateInformation (mb);
        fresh.setStateInformation (mb.getData(), (int) mb.getSize());
        pump (200);
        std::cout << "  a new instance's own state restores without kits (no crash)" << std::endl;
    }

    // a song sent from Song Savant: everything set, four kits built, the build page offers it once
    {
        auto expect = [&failures] (bool ok, const juce::String& what)
        {
            std::cout << (ok ? "  ok: " : "  FAIL: ") << what << std::endl;
            if (! ok) ++failures;
        };
        GrooveGrabberProcessor p;
        for (int i = 0; i < 400 && p.library->isScanning(); ++i) pump (25);
        const auto folder = gg::paths::songSavantSongs();
        folder.createDirectory();
        auto f = folder.getChildFile ("Back Roads.sssong");
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("app", "Song Savant");
        o->setProperty ("name", "Back Roads");
        o->setProperty ("description", "modern country like Morgan Wallen, warm");
        o->setProperty ("genre", "country");
        o->setProperty ("style", "country.modern");
        o->setProperty ("key", 21);
        o->setProperty ("bpm", 92);
        o->setProperty ("sent", juce::Time::getCurrentTime().toISO8601 (true));
        f.replaceWithText (juce::JSON::toString (juce::var (o.get())));

        auto song = gg::newestSongSavantSong (folder);
        expect (song.has_value() && song->name == "Back Roads", "the newest Song Savant song is read");
        if (song)
        {
            ggui::SongPanel panel (p);
            panel.setSize (420, 900);
            expect (panel.isOfferingSongSavant(), "the build page offers a song Song Savant just sent");
            p.useSongSavantSong (*song);
            expect (p.options.genre == gg::Genre::country && p.options.style().id == "country.modern", "Song Savant's genre and style: " + p.options.style().id);
            expect (p.options.key == 21 && std::abs (p.options.bpm - 92) < 0.1, "Song Savant's key and tempo");
            expect (p.songName == "Back Roads" && p.description.contains ("Morgan Wallen"), "the song's name and description");
            expect (p.kits.size() == 4, "four kits built for it");
            expect (! p.kits.empty() && p.kits.front().key == 21, "the kits are made in the song's key");
            expect (p.getStatusMessage().startsWith ("From Song Savant: Back Roads"), "says where the song came from: " + p.getStatusMessage());
            p.getSettings().set ("songSavantSeen", song->sent.toISO8601 (true));
            panel.checkSongSavant();
            expect (! panel.isOfferingSongSavant(), "once used, it isn't offered again");
            p.getSettings().set ("songSavantSeen", {});

            // a style the words don't name ("modern trap" sent as NY Drill): it stays, through the window refreshing
            gg::SongSavantSong drill;
            drill.name = "Modern Trap";
            drill.description = "modern trap";
            drill.genre = (int) gg::Genre::trap;
            drill.styleId = "trap.nydrill";
            drill.key = 21;
            drill.bpm = 142;
            std::unique_ptr<GrooveGrabberEditor> ed (dynamic_cast<GrooveGrabberEditor*> (p.createEditor()));
            ed->setSize (1440, 900);
            pump (300);
            p.useSongSavantSong (drill);
            pump (600);
            ed->refreshAll();
            pump (300);
            expect (p.options.style().id == "trap.nydrill" && ! p.kits.empty() && p.kits[0].styleId == "trap.nydrill",
                    "a style sent from Song Savant stays (NY Drill): " + p.options.style().id);
            ed.reset();
        }
        f.deleteFile();
    }

    // the genre box and the description together
    {
        GrooveGrabberProcessor p;
        auto expect = [&failures] (bool ok, const juce::String& what)
        {
            std::cout << (ok ? "  ok: " : "  FAIL: ") << what << std::endl;
            if (! ok) ++failures;
        };
        p.applyDescription ("90s g-funk");
        p.setGenre (gg::Genre::boomBap);
        expect (p.options.style().id == "hiphop.west", "typed g-funk, then picked Hip-Hop: still West Coast (" + p.options.style().id + ")");
        p.applyDescription ("");
        p.applyDescription ("like Dr. Dre");
        expect (p.options.genre == gg::Genre::boomBap && p.options.style().id == "hiphop.west" && p.options.era == 3
                && p.options.directionFor (0) == gg::directionIndex (p.options.style(), "west.knock"), "like Dr. Dre: 90s West Coast, kit 1 Westside Knock");
        p.setGenre (gg::Genre::rock);
        expect (p.options.genre == gg::Genre::rock && p.options.leadDirection < 0 && p.options.secondaryGenre == (int) gg::Genre::boomBap,
                "Dr. Dre, then Rock picked: rock with a little hip-hop");
        p.applyDescription ("Post Malone");
        expect (p.options.genre == gg::Genre::rock && p.options.directionFor (0) == gg::directionIndex (p.options.style(), "rock.hybrid"),
                "Post Malone with Rock picked: his rock take");
        p.applyDescription ("");
        p.applyDescription ("Morgan Wallen");
        expect (p.options.genre == gg::Genre::country, "cleared, then Morgan Wallen: Country");
    }
    return failures;
}
