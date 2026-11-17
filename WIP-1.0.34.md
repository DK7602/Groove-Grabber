# WIP 1.0.34: IMPORT SONG (replaces the KEY COMPASS button)

User's answers: 1. show "what I found" first (strip under the description, USE IT / X) 2. MIDI drum track -> offer
to save as one of MY GROOVES 3. harvest: my call -> no (LIBRARY keeps HARVEST).

Design
- core/SongImport.*: gg::ImportedSong {from audio/midi/songSavant/keyCompass, name, file, bpm, otherBpm (audio:
  double/half), key, keySure, keyFrom, songSavant (whole song), drums (a MIDI file of the drum part, 4 bars max),
  drumBars}; importSong (file) by extension (.mid/.midi, .sssong, .kcsong, audio); songFromMidi (key signature,
  else from the notes (Krumhansl, keyFromChroma shared with SongAnalysis); a 0-sharp major signature can be a DAW
  default: the notes decide), tempo, drums = channel 10 or a drum-named track or the "drums:" tag, the busiest
  4-bar stretch; otherTempo (bpm).
- Processor: offeredImport (not saved), offerImport, useImport (otherTempo), dismissImport, saveImportedDrums,
  importSongFile (background). Song Savant sends become an offer (songSavantSeen on use / dismiss). Menu picks of
  recent songs apply at once (the menu already shows key and BPM).
- Header: IMPORT SONG (Icon::music): Song file..., Use my DAW's tempo (plug-in, when known), FROM SONG SAVANT (if
  any), FROM KEY COMPASS (if its folder exists) + choose folder. A .kcsong imported by file sets the folder.
  Files drop on the button and on YOUR SONG.
- SongPanel strip: text line + [USE IT] [92][184] or [DRUMS TO MY GROOVES] [X].
- Song Savant: its text "KEY COMPASS menu" -> "IMPORT SONG menu" (branch + message to its session).

Checklist
- [x] core + engine tests (815/815)
- [x] processor, header, strip, drops; screen tests; screenshots (815/815)
- [ ] manual, CLAUDE.md, FX tempo hint text, version 1.0.34; full tests + host; release; CI; Song Savant
