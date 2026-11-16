# 1.0.31 in progress: FX in your kits (the Song FX bank)

Work happens on branch `claude/loving-knuth-7hdwu7`; `main` only gets the finished, tested release (one commit whose
files are the branch's, parent origin/main: `git commit-tree <branch>^{tree} -p origin/main`). Delete this file in that
release commit.

## Decided with the owner
- Song FX bank: shared by all four kits, saved with the song, kept through GENERATE / REGENERATE / LOAD.
- Bank pads by kit size: 17 -> 4, 23 -> 6, 32 -> 8, on notes 69..76 (A3.. in Cubase naming). An empty bank exports
  exactly as 1.0.30 did.
- All 8 stay saved; a smaller kit plays / exports only its first N, and BUILD says "N more FX waiting".
- FX tab: ADD TO SONG FX (next free bank pad; full: pick one to replace) and PUT ON THIS KIT (the kit's own FX 1..4).
- BUILD: the bank as its own row ("SONG FX") under the kit's pads: hear, edit (tune / start / length / gain / reverse),
  lock, clear, drop a file on it.
- SAUCE: bank sounds are grid rows (BLANK too); DRAG MIDI has their notes.
- Export: a "Song FX" folder, files "01 Song FX 1.wav"... (FL: "... - <kit>.wav"), dropped on the A3 pad (note 69);
  each DAW guide gets a step for it; MPC program and Kit Info.txt include it.
- Song Savant (repo DK7602/Song-Savant, changed from this session): its DrumLanes learn notes 69..76 "Song FX 1..8".

## Design
- `Slot` gets sfx1..sfx8 appended after fx4 (never reorder). numSlots 40, numPads 41 (36..76).
  `bankSlots (kitSize)`, `isBankSlot`, `bankSizeFor (kitSize)`. Not in `kitSlots` (the generator never fills them).
- Processor: `songFx` (8 SlotStates, in the state as "songFx"); `pushSongFx()` copies the bank into every kit (first N
  by its size, the rest cleared) and re-renders; edits made on a kit's sfx pad (PadDetail, setSlotSound, locks) are
  pulled back into the bank in `kitChanged`.
- Kit::fromVar keeps sfx slots (older versions ignore the unknown ids).

## Checklist
- [x] SoundTypes: slots, pads, names, ids, helpers (Kit::bankCount / bankUsed, renderPad plays bank pads)
- [x] Processor: bank, sync, state, ADD / PUT ON KIT API (setSlotSound on a bank slot -> setSongFx; loadKit adopts a saved kit's bank when the song has none)
- [x] Player / render: bank pads play
- [x] BUILD: SONG FX row (PadGrid bottom strip, "+N waiting", no lock on bank pads)
- [x] FX tab: ADD TO SONG FX (REMOVE when in), PUT ON THIS KIT, "In your song" line
- [x] SAUCE: rows (BLANK + ADD DRUM), MIDI via padLayout; Song FX pads are amber (slotColour)
- [x] Export: Song FX folder, MPC, Kit Info, FL names, DAW guide step (Daws::songFxStep), export panel line
- [x] Song Savant DrumLanes (branch claude/gg-song-fx-notes in Song-Savant, CI there)
- [ ] Tests (engine + screenshots), manual, CLAUDE.md, version 1.0.31
- [ ] Release: one commit on main, CI, Windows host annotations, tell the other session
