# Groove Grabber manual

## Welcome

Groove Grabber builds four drum kits for the song you're making, using only the sounds you already own. You describe the song, it picks kicks, 808s, snares, claps, hats, toms, cymbals, percussion and FX that fit together, tunes them to your key, matches their levels and exports the kit ready for your DAW's drum sampler (Groove Agent, Drum Rack, FPC, Drum Machine Designer, Impact and more). The SAUCE page shows how the drums go in your style, so you can build the parts too.

This manual covers version {VERSION}. Groove Grabber runs as a plug-in in your DAW (VST3, AU on Mac) and as a standalone app. Open this manual any time with the **?** button in the header or the **F1** key.

The workflow in six steps:

1. **LIBRARY:** point it at your drum folders once. It listens to every sound and remembers it.
2. **BUILD:** describe the song in your own words (genre, decade, feel, tempo, key, even an artist or song).
3. **GENERATE KITS:** get four kits, each a different take on that style.
4. **Edit:** lock the pads you love, regenerate the rest, swap in alternatives, tweak each pad.
5. **EXPORT:** a numbered folder you drop straight onto your drum sampler's first pad, with steps for your DAW.
6. **SAUCE:** how the drums are played in your genre, style and decade, with patterns to play, edit and drag into your DAW.

## Install and first run

Run the installer for your system, open your DAW, rescan plug-ins, then add Groove Grabber to an instrument track. It's an instrument, so it takes MIDI in and plays audio out.

| System | What installs | Where |
| --- | --- | --- |
| Windows | VST3 plug-in | C:\Program Files\Common Files\VST3\Groove Grabber.vst3 |
| Windows | Standalone app (+ optional desktop shortcut) | C:\Program Files\Groove Grabber |
| Mac | Audio Unit (Logic Pro, GarageBand) | /Library/Audio/Plug-Ins/Components |
| Mac | VST3 plug-in | /Library/Audio/Plug-Ins/VST3 |
| Mac | Standalone app | /Applications |
| Linux | VST3, LV2 and app (run install.sh) | ~/.vst3, ~/.lv2, ~/.local/bin/groove-grabber |

The Windows and Mac installers let you choose the parts. Pick "Plug-in + standalone app" if you're not sure. To update, run the new installer over the old one: your library, settings and saved kits stay.

**Standalone app:** open the gear menu (top right), choose **Audio / MIDI settings...** and pick your audio device. A MIDI keyboard or pad controller plugged in plays the pads.

**First run:** the app opens on the LIBRARY page. Add your drum folders there before building kits.

**Where your files live**

| What | Where |
| --- | --- |
| Exported kits | Documents\Groove Grabber\Exports\<genre>\<kit name> |
| Patterns dragged from SAUCE | Documents\Groove Grabber\Sauce MIDI |
| My grooves | Documents\Groove Grabber\My Grooves |
| Saved kits | Documents\Groove Grabber\Kits\<genre> |
| Harvested hits | Documents\Groove Grabber\Harvested |
| Your own recordings | Documents\Groove Grabber\My Recordings |
| Settings and the library memory | AppData\Roaming\Groove Grabber (Mac: ~/Library/Application Support/Groove Grabber) |

## The screen at a glance

Everything sits on four pages under one header bar. Hover over almost anything to see a tip.

**The header, left to right**

| Control | What it does |
| --- | --- |
| Logo | Click to switch between the dark and light look |
| BUILD / LIBRARY / MY KITS / SAUCE | The four pages |
| KEY | The song's key. The kick, 808 and toms are tuned to it. Minor keys are listed first. |
| BPM | The song's tempo. Drag up or down, scroll, or double-click to type. |
| KEY COMPASS | Takes the key and tempo from a Key Compass song or a song file, or uses a song sent from Song Savant |
| ? | This manual (also the F1 key) |
| Gear | Settings: audio / MIDI (standalone app), export format and folder, the manual, About |
| Sun / moon | Light / dark look (same as clicking the logo) |

**The pages**

- **BUILD:** describe the song, generate the four kits, edit pads, save and export. It's where you spend most of your time.
- **LIBRARY:** your sound folders, scanning, Harvest Hits, and a searchable list of every analysed sound.
- **MY KITS:** kits you saved, plus recent ones, filed by genre and searchable.
- **SAUCE:** how the drums go in a genre, style and decade: the patterns (which you can edit), and how to play and program them.

The app opens on LIBRARY the very first time (nothing to build with yet) and on BUILD after that. The window can be resized from the bottom-right corner.

## LIBRARY: your sounds

Add your drum folders once. Groove Grabber listens to every sound, works out what it is and how it sounds, and remembers it, so only new files are analysed after that.

**Adding folders**

- **+ ADD DRUM FOLDER** adds a folder of drum kits or one shots. Sub-folders are included, so you can add a whole "Drums" or "Samples" folder in one go.
- **+ MY RECORDINGS** adds a folder of hits you recorded yourself.
- You can also drag folders from Explorer or Finder straight onto the page.
- **RESCAN** looks for new, changed or removed files. Only new ones are analysed.
- After an update that measures sounds in a new way, the first scan goes through your whole library once more ("Updating the analysis..."). Your favourites, never-use marks and the types you set by hand are kept.

Scanning runs in the background on at most half your computer's cores, so your session keeps playing. A progress bar shows how far it's got.

**Your sound folders** (the list on the left) shows each folder with:

- a **type** (Drum Packs, One Shots, My Recordings, Sample Snagger, Harvested). The BUILD page can include or leave out each type.
- an **on / off switch** to stop using a folder without removing it.
- a **remove** button. Your files are never touched.
- a **show folder** button.

Three folders are added for you: My Recordings, Harvested, and your Sample Snagger library (Documents\Sample Snagger\Samples) if you have one.

**What it learns about each sound**

- **Type:** Kick, 808, Snare, Clap, Rim, Closed Hat, Open Hat, Tom, Crash, Ride, Perc or FX. It uses the sound itself plus clues in the file and folder names, including drum-machine shorthand (BD, SD, CP, CH, OH, LT, MT, HT, CY, RS, CB). From the sound alone it tells a kick from a tom by its note and its sub, a clap from a snare by the clap's separate bursts, and a closed hat from an open hat and an open hat from a crash by how long each one rings.
- **"808" in a name:** "808 Bass" or a folder of 808s is the bass; "808 Snare", "808 LT" or a "TR-808" kit folder is the drum machine's own sound.
- **Character:** punch, warmth, brightness, room, vintage, aggression and live feel.
- **Pitch** (the NOTE column), length, and problems such as clipping or hiss. An 808 that drops from an octave up or slides into its note is tuned by the note it lands on.

Loops and vocal or melodic sounds (vocal chops, keys, guitar phrases) are recognised and **left out of kits automatically**, so a pack that mixes drums with loops is fine.

**The sounds list** (right side)

- Search by name, pack or folder, or click a type chip (ALL, KICK, 808...) to filter.
- Click a column heading to sort. Click a sound to hear it.
- Right-click a sound for more:
    - **It's really a...** fixes a wrong type. Your fix sticks, even after a rescan.
    - **Favourite** makes it get picked more often.
    - **Never use this sound** keeps it out of every kit.
    - **Put it on the selected pad** swaps it into the kit you're editing.
    - **Show the file** opens its folder.

**HARVEST HITS** (the drop zone at the bottom left)

Drop a drums-only bounce, a drum stem or a break here. Every hit is found and sorted, and the cleanest soft, medium and hard version of each is kept in Documents\Groove Grabber\Harvested. Harvested hits are used in kits like any other sound. Audio with vocals or music on top harvests poorly, because those parts come along with each hit.

## BUILD: describe your song

Type what you're making in plain words in the YOUR SONG box, and the genre, style, decade, sliders, tempo, key and options set themselves. Press Enter to generate straight away.

Examples that work:

- "dark 90s boom bap, punchy but warm, vinyl vibe, 88 BPM in A minor"
- "80s pop rock, big gated drums"
- "modern country rock, 108 BPM, live drums with a little hip-hop influence"
- "like Dr. Dre" or "Metro Boomin type beat"

**What it reads from your words**

| You type | It sets |
| --- | --- |
| A genre: trap, hip-hop, rap, r&b, pop, rock, country, worship, gospel, edm, house, lofi, cinematic... | The genre |
| A style: g-funk, boom bap, uk drill, phonk, honky tonk, pop rock, hard rock, neo soul, drum and bass... | The style within the genre |
| A decade: 60s, 70s, 80s, 90s, 2000s, 2010s, today | The era (and, with just a genre, the style that decade is known for: "80s pop" = Synth Pop) |
| Feel words: punchy, warm, dark, bright, roomy, dry, dusty, vintage, clean, modern, hard, heavy, smooth, gated... | The character sliders. "Not too bright" works too. |
| live, acoustic, programmed, drum machine, hybrid | Live feel |
| 808, no 808, fx, risers, lots of percussion, no percussion, minimal | The options |
| "92 BPM", "A minor", "key of F#" | Tempo and key |
| "...with a little trap influence" | Blends in a touch of a second genre |

The small blue line under the box shows what it understood, for example "90s - Hip-Hop - West Coast / G-Funk - punchy - 92 BPM". If a word you meant isn't there, it wasn't recognised.

**Genre, era and style boxes**

Under the description are three drop-downs: genre, era ("Any era" or a decade) and style. The style list changes with the genre, and its first entry is "General" for that genre. You can type, pick, or both:

- Picking a genre keeps a style you typed for it. "90s g-funk" then Hip-Hop stays West Coast / G-Funk.
- Picking a decade shifts all the sliders toward that era's sound but keeps your own slider tweaks.
- Picking a style sets the four kit directions, the tempo and the preview groove.

**Artists and songs**

Name an artist or a song and kit 1 takes their sound: "like Dr. Dre", "Still D.R.E.", "Bon Jovi drums", "Morgan Wallen", "Kirk Franklin", "Hillsong". Hundreds of artists and producers and about 90 songs are built in, covering every genre.

- **It knows how their drums are known to sound**, not the recordings themselves. It sets the style, the decade and the sliders, and makes their signature direction kit 1, then picks the closest sounds in your library.
- **The genre you pick decides which side of an artist you get.** Post Malone with Country picked gives his country sound; with Trap picked, his trap sound.
- **An artist from another genre lends their decade and feel.** "Dr. Dre" with Rock picked gives a punchy, dry 90s rock kit with a little hip-hop in it.
- **A genre typed in the box wins over the artist's own.** "Dr. Dre style rock song" stays Rock.
- **Names that are everyday words need a cue:** "like Future", "Prince type beat", "in the style of Queen". Otherwise "future bass" would mean the rapper.
- Names it doesn't know are ignored. For the real drums of a specific song, harvest them (see Working with Key Compass, Song Savant and Sample Snagger).

## BUILD: kit character and options

The sliders and switches under the description fine-tune what the four kits aim for. They start where your style, decade and words put them, and you can move any of them before you generate.

**KIT CHARACTER sliders** (0 to 100; double-click a slider to reset it to the middle)

| Slider | Low | High |
| --- | --- | --- |
| Punch | Soft, slow attacks | Sharp, fast attacks |
| Warmth | Thin | Full low-mids |
| Brightness | Dark | Crisp top end |
| Room | Short and dry | Long and roomy |
| Vintage | Clean and modern | Dusty, vinyl, sampler grit |
| Aggression | Gentle | Hard and compressed |
| Live Feel | Programmed, drum machine | Real, acoustic drums |

Moving a slider shifts all four kits the same way, so each kit keeps its own personality.

**OPTIONS**

| Option | Choices | What it does |
| --- | --- | --- |
| FEEL | Auto, Live, Hybrid, Prog | Auto lets the style decide. Live = real drums, Prog = drum machines, Hybrid = a mix. |
| LOW END | Auto, Kick, Kick + 808 | Just a kick, or a kick and an 808. Auto follows the style (trap and G-Funk use an 808, rock doesn't). |
| PERC | Auto, 0, 1, 2, 3 | How many percussion pads (shaker, tambourine, conga...) |
| FX / sound effects | On / off | Adds two FX pads of different kinds: a build-up (riser, reverse), an impact, a texture (vinyl, noise) or an accent (scratch, stab, laser) |
| Tune kick, 808, toms to key | On / off | Tunes the 808 to the song's root, the kick to the root or fifth when it's close, and the toms to notes of the scale. With no key set, the kick is still tuned to the 808's note (or its fifth), so the low end agrees. |

**USE SOUNDS FROM**

One switch per kind of folder: Drum Packs, One Shots, My Recordings, Sample Snagger, Harvested. The number beside each is how many usable sounds it holds. Turn kinds off to build kits from only some of them, for example only Harvested to build from hits you harvested from one song.

**Generating**

- **GENERATE KITS** builds four kits. Pads you've locked stay put.
- The **dice button** (Surprise me) builds four wilder kits that take more chances.

## Generating and editing kits

You get four kits across the top of the BUILD page, each a different take on your style. They're steered apart on purpose, so they don't share their main sounds.

**Kit names** are new every time you generate. They come from your style, the places and moods you typed ("westcoast", "rainy", "summer", "late night") and each kit's own sound: the darkest kit gets a dark name, the dustiest a dusty one. "trap, westcoast" might give Westside Pulse, Diamond Lowrider, Compton Roll and Golden Hour Crenshaw. Under each name is a three-word description of its sound. Re-rolling one kit keeps its name, and you can rename a kit when you export it.

**On each kit card**

- Click the card to make it the kit you're editing.
- **Play** hears it play the preview groove at your tempo. Every kit is level-matched, so a louder kit never wins just for being louder.
- **Circular arrow:** new sounds for that kit's unlocked pads.
- **Heart:** saves it to MY KITS.

**The pads (KIT EDITOR)**

The kit you're editing shows its pads: Kick, 808, Snare, Clap, Rim, Closed Hat, Open Hat, Crash, Ride, three toms, three percussion and two FX. Each pad shows its waveform, file name and how far it was tuned to the key ("KEY +3.5"), or to the 808 when no key is set ("TO 808 +1.2").

- **Click a pad** to hear it and open it in the pad editor below.
- **Drag a pad into your DAW** to use that one sound on its own.
- **Drop a sound file on a pad** to replace it with your own.
- **The lock icon** on a pad keeps it when you regenerate.

The buttons above the pads act on the whole kit: **REGENERATE** (new sounds for every unlocked pad), **LOCK ALL**, **SAVE KIT** and **EXPORT**.

**The pad editor** (bottom panel)

- A large waveform, a play button, and notes such as "level matched", "tuned +3.52 st to Am", the sound's note, and any hiss or vinyl noise.
- **TUNE** (on top of the key tuning), **START** (skip the start of the sound), **LENGTH** (shorten it with a fade) and **GAIN** (on top of the level matching). Double-click a knob to reset it.
- **REVERSE** and **LOCK**.
- **ALTERNATIVES:** five other sounds that fit the rest of this kit. Click one to hear it, **USE** to swap it in.

**The groove bar** (very bottom)

- **Play / stop** the preview groove with the selected kit. The **Space bar** does the same.
- **Groove** picks the preview pattern. Each style has its own, and it plays at the song's BPM.
- **SWING** and **VOL** for the preview.
- **Follow DAW:** while your DAW plays, the groove locks to its timeline, so you hear the kit inside your song.

**Playing from MIDI:** the pads answer MIDI notes 36 to 57 (C1 to A2), the same layout as the export (see the Quick reference card). Closed, pedal and open hats cut each other off like a real hi-hat.

## Saving kits and MY KITS

Press the heart on a kit card, or **SAVE KIT** above the pads, to keep a kit. It keeps its name, sounds, pad settings, locks, style, decade, key, tempo and the description you typed.

The MY KITS page has three views:

- **SAVED:** every kit you kept, filed in a folder for its genre (Documents\Groove Grabber\Kits\Hip-Hop, \Rock, \Country...). Kits saved by older versions are moved into their genre folders automatically.
- **RECENT:** the last 48 kits you generated, saved or not, so a good one is never lost.
- **MY GROOVES:** the patterns you saved as your own on SAUCE (see SAUCE: how the drums go). Each shows a small picture of the pattern, its genre and style, tempo, swing and the pattern it started from, with **OPEN** (show it on SAUCE to play, change or drag it), **USE ON BUILD** (BUILD's preview and the export's Groove MIDI use it) and **DELETE** (click twice). The genre tabs and search work here too.

**Finding a kit**

- **Genre tabs** under SAVED / RECENT: ALL, then one per genre you have kits in (TRAP, HIP-HOP, ROCK, COUNTRY...), each with its count. Click one to see only that genre.
- **Search** (top right): type any words from a kit's name, genre, style, decade, key or BPM, and every word must match. "g-funk 90", "westside", "A minor" and "92 bpm" all work, and the search combines with the genre tab. Press Esc to clear it.

Each kit shows its genre, style, decade, key, BPM, how many pads it fills and when it was made, with three buttons:

- **LOAD** opens it on the BUILD page, in place of the kit you have selected there.
- **EXPORT** exports it without loading it.
- **DELETE** (saved kits) or **SAVE** (recent kits).

**OPEN KIT FILE...** opens a .ggkit file. Every export folder has one, so you can reopen any exported kit. A kit file points at your original sounds, so keep your drum folders where they are.

## Exporting to your DAW

**EXPORT** writes the kit as one WAV per pad, numbered in General MIDI order from note 36, already tuned, trimmed and level-matched. Drop them all on your drum sampler's first pad and every sound lands on the right note. Groove Agent, Drum Rack, FPC and the rest each take the files a little differently, so the export panel, Kit Info.txt and this page give the steps for your DAW.

**In the EXPORT panel**

- **Name:** the kit's name, which becomes the folder name. Change it if you like.
- **FOR:** the DAW the loading steps are written for. **Automatic** picks the DAW Groove Grabber is running in (Cubase, Live, FL Studio, Logic, Studio One, REAPER, Bitwig, Reason...). Pick one by hand in the app, or to get another DAW's steps. MPC sets 16-bit for you.
- **16-BIT / 24-BIT** and **44.1 kHz / 48 kHz.** Use 16-bit for MPCs and older samplers.
- **MPC program (.pgm):** an MPC 1000 program with every pad and note set, for MPCs and samplers that import them. (Off for FL Studio: its file names are too long for an MPC program.)
- **Groove MIDI file:** the preview groove on the same notes, on MIDI channel 1 (your edited version if you edited it on SAUCE).
- **Silent files for empty pads:** keeps the numbering steady when a kit doesn't fill every pad. Leave it on.
- **In a genre folder:** puts the kit in a folder for its genre, for example Exports\Hip-Hop\90s Westside Classic. On by default.
- **CHANGE...** picks the export folder (default Documents\Groove Grabber\Exports). The "To:" line shows exactly where the kit will go. **EXPORT KIT** writes it, **SHOW FOLDER** opens it.
- After exporting, grab the **drag handle** and drop all the files on your sampler in one go. It's labelled for your DAW, for example **DRAG TO GROOVE AGENT - PAD C1** or **DRAG TO DRUM RACK - PAD C1**.

**What you get**

```
Exports/Hip-Hop/90s Westside Classic/
  01 Kick.wav  02 Rim.wav  03 Snare.wav ... 17 808.wav ... 22 FX 2.wav
  90s Westside Classic.pgm          MPC 1000 program
  90s Westside Classic Groove.mid   the preview groove
  Kit Info.txt                      where every sound came from + the steps for your DAW
  90s Westside Classic.ggkit        reopen it in Groove Grabber
```

**For FL Studio, each file also carries the kit's name**, for example "01 Kick - 90s Westside Classic.wav". FPC finds its samples by file name, so this keeps two kits' kicks from ever getting mixed up.

**Note names differ between DAWs.** The files always sit on MIDI notes 36 to 57, but DAWs name those notes differently: note 36 is C1 in Cubase, Live, Logic, Studio One and Bitwig, C2 in REAPER and C3 in FL Studio. The export panel, Kit Info.txt and the SAUCE grid use your DAW's names.

**Loading the kit, DAW by DAW**

{DAW_GUIDES}
**Or skip the sampler.** Groove Grabber is a drum instrument too: put it on an instrument track and it plays the kit you're editing on notes 36 to 57, the same layout as the export. It's the quickest way to sketch, and the only way in GarageBand.

**Other ways to use it**

- **The groove:** drop the Groove .mid on your drum track's lane (not below the last track, which makes a new track with no instrument).
- **Kit Info.txt** lists which pack every sound came from. Check each pack's licence before releasing music made with it.

## SAUCE: how the drums go

The SAUCE page explains how drums are played and programmed in a genre, style and decade, so you can build your own parts in your DAW. It opens on the genre, style and decade picked on BUILD, so it always starts with your song.

**THE RECIPE** (left)

- **Genre, decade and style** boxes: pick any other to read up on it. BUILD isn't changed.
- When you're on another style, **MATCH BUILD** goes back to your song's, and **USE ON BUILD** sets BUILD to this style and takes you there to generate kits.
- **At a glance:** the style's starting tempo, its low end (kick, or kick + 808), its usual percussion and the decades it's known for.
- **Build it in your DAW:** the five steps from pattern to finished part.

**THE PATTERN** (top)

- The style's grooves as a step grid: one row per drum, 16 steps a bar, two bars. The beats are steps 1, 5, 9 and 13, counted 1 e & a. Brighter cells are harder hits, faded ones are soft ghost notes, and split cells are rolls.
- Each row shows the note it plays in your DAW, for example "C1 (36)".
- The buttons across the top pick between the style's grooves.
- **Play** (or the Space bar) plays it with the kit you're editing on BUILD. It plays at your song's tempo while you're on your song's style, and at the groove's own tempo when you're reading up on another.
- **DRAG MIDI** shows the tempo and swing it plays at, and drags four bars of it into your DAW, on the same notes as every Groove Grabber kit (MIDI channel 1). **Drop it on the lane of your drum track** (in Cubase, your Groove Agent SE or Groove Grabber track), at the bar where it should start. Dropped below the last track, most DAWs make a new track with no instrument on it, so it stays silent. The files are kept in Documents\Groove Grabber\Sauce MIDI.

**Editing the pattern**

- **Click a step** to add a hit or take one away. Hold the mouse down and drag along the row to do several.
- **Right-click** a hit to make it hard, medium or soft (a ghost note). Right-click an empty step for a soft ghost note.
- **Ctrl-click** (Alt-click on a Mac) a step for a roll: 2 quick hits in that step, Ctrl-click again for 3 (a triplet roll, the fast hi-hat flutter in trap and drill), and again for 1 hit. Right-click it afterwards to make the roll softer.
- **+ ADD DRUM** adds a row for a drum the pattern doesn't use yet (rim, toms, ride, percussion, FX...).
- **UNDO** takes back your last change. **RESET** goes back to the original pattern (and UNDO brings your version back).
- An edited pattern shows **EDITED**. Your version is saved with your song (in your DAW project), plays while you edit, and is also what BUILD's preview groove plays and what goes into the export's Groove MIDI file. The groove list on BUILD marks it "edited".

**My grooves: patterns you keep for every song**

An edit belongs to one song. To keep a pattern for all your songs, save it as your own:

1. Shape the pattern (or take a style's pattern as it is) and press **SAVE AS MY GROOVE**.
2. Type a name ("Late Snare Bounce"), then:
    - **SAVE** (or Enter) keeps your version under its new name. The pattern you started from goes back to its original, so BUILD's groove list still has the original under its own name. (UNDO on the original brings your changes back.)
    - **SAVE & REPLACE ON BUILD** does the same, and BUILD's preview plays your version in place of the original (and the export's Groove MIDI uses it).
    - **CANCEL** (or Esc) leaves it.
3. It's saved on your computer in Documents\Groove Grabber\My Grooves, with its tempo and swing, and listed under MY GROOVES here and on the MY KITS page.

- **MY GROOVES** (next to the pattern buttons) lists your grooves, this genre's first and the rest under Other genres. Pick one to see it, play it, change it or drag it into your DAW.
- While one of your grooves is showing, it's marked **MY GROOVE**. Changes save to it as you go (UNDO still works), **RENAME** gives it a new name, and **DELETE** removes it (click it twice: the first click asks SURE?).
- BUILD's groove list has a **MY GROOVES** section in every song: pick one to hear new kits in your groove, and it's what goes into the export's Groove MIDI file. If you delete it, BUILD goes back to the style's own groove.

**THE SAUCE** (bottom)

- **The pocket:** what makes this style feel like itself, with its tempo, swing, time feel and low end.
- **Kick, Snare and clap, Hats and cymbals, Percussion and toms, 808 and bass:** where each one goes and how it moves.
- **Fills and changes:** what changes between verse, hook, bridge and drop.
- **Sauce tips:** velocity, ghost notes, timing, layering and mix moves.
- With a decade picked, **In the 90s** (or whichever) adds how that genre's drums sounded then, the gear of the time and how to get that sound in your DAW.

Swing is given MPC-style: 50% is straight, about 54 to 58% light, 60 to 62% heavy and 66% a full triplet. The DRAG MIDI handle shows the swing the pattern plays at (for example "4 bars, 92 BPM, swing 60%"), and BUILD's swing slider shows its value the same way when you point at it. DAWs name their swing controls differently, so match the feel rather than the number.

## Working with Key Compass, Song Savant and Sample Snagger

**Key Compass**

Click **KEY COMPASS** in the header:

- **Your Key Compass songs** are listed at the top (from its My Songs folder). Pick one and Groove Grabber takes its key and tempo. If a song has no key set, it works the key out from the song's chords.
- **Detect key and tempo from a song file...** listens to a bounce or rough mix (WAV, AIFF, FLAC, MP3 or OGG) and sets the key and BPM from it.
- **Choose the Key Compass songs folder...** if your songs live somewhere other than the default (AppData\Roaming\KeyCompass\Songs on Windows).

With a key set and "Tune kick, 808, toms to key" on, the kit is tuned to your song.

**Song Savant**

Song Savant suggests the sounds for a whole song from the instruments you own. On its drum card, **SEND TO GROOVE GRABBER** passes the song over: its description, genre, style, decade, key and tempo (and the sections of its arrangement).

- If Groove Grabber is open, the BUILD page shows **Song Savant sent "...": USE IT** under your description for a day. Click it and Groove Grabber sets everything and builds four kits for the song, with the kick, 808 and toms tuned to its key. The **X** next to it hides it.
- Every song Song Savant sent is also in the **KEY COMPASS** menu, under SONG SAVANT SONGS. Picking one does the same.
- Save the kits you like (SAVE KIT). Song Savant lists your saved kits as drum choices for its songs, and its starter drum parts play the right pads of an exported kit.

The songs are small files in Documents\Song Savant\Songs.

**Sample Snagger**

Your Sample Snagger library (Documents\Sample Snagger\Samples) is added to the LIBRARY automatically and scanned like any other folder.

- **One-shot snags** (a single kick, snare, clap...) become kit sounds straight away.
- **Longer snags** count as loops and are left out of kits. To use their drums, put them through HARVEST HITS.

**Getting the drums from a specific song**

1. In Sample Snagger, snag a spot where the drums are exposed: an intro, a breakdown or a drum break. Vocals, bass or music on top come along with every hit.
2. On the LIBRARY page, drag the snag onto **HARVEST HITS**. Every hit is cut out and the cleanest soft, medium and hard version of each is filed under Harvested.
3. On the BUILD page, under **USE SOUNDS FROM**, switch off everything except Harvested to build only from that song's hits, or leave the others on to mix them with your packs.
4. Click **KEY COMPASS**, then **Detect key and tempo from a song file...**, and pick the same song to match its key and BPM.
5. Generate.

If you release a track that uses drums lifted from someone else's record, that sample needs clearing.

## Reference: styles, decades and names

**Styles.** Every genre has a General style plus the styles below. Type any of the words in the middle column, or pick the style from the drop-down. Each style has four kit directions, its own preview grooves and a starting tempo.

| Genre | Style | Type words like | Tempo (BPM) |
| --- | --- | --- | --- |
| Hip-Hop | East Coast / Boom Bap | boom bap, east coast, golden era, jazz rap | 92 |
| Hip-Hop | West Coast / G-Funk | g-funk, west coast, lowrider, talkbox | 94 |
| Hip-Hop | Memphis / Phonk | memphis, phonk, drift phonk, cowbell phonk | 136 |
| Hip-Hop | Southern / Crunk | crunk, dirty south, southern rap, chopped and screwed | 78 |
| Hip-Hop | Old School / Electro | old school, electro rap, electro funk, breakdance | 108 |
| Trap | Atlanta Trap | atlanta, atl, street trap, hard trap | 140 |
| Trap | UK Drill | uk drill, london drill, road rap | 142 |
| Trap | NY Drill | ny drill, brooklyn drill, bronx drill, sample drill | 142 |
| Trap | Rage / Hyper | rage, hyper trap, distorted trap, opium | 155 |
| Trap | Plugg / Melodic | plugg, pluggnb, melodic trap, emo rap | 150 |
| R&B | 90s R&B / New Jack | new jack swing, slow jam, quiet storm | 100 |
| R&B | Neo Soul | neo soul, nu soul | 82 |
| R&B | Modern / Alt R&B | alt r&b, trap soul, dark r&b | 70 |
| R&B | Funk / Motown | motown, funk, p-funk, classic soul | 108 |
| Pop | Synth Pop / 80s | synth pop, synthwave, new wave, retrowave | 118 |
| Pop | Dance Pop | dance pop, club pop, disco pop, electropop | 124 |
| Pop | Pop Rock | pop rock, power pop, arena pop | 128 |
| Pop | Indie / Bedroom Pop | indie pop, bedroom pop, dream pop, chill pop | 100 |
| Rock | Classic Rock | classic rock, blues rock, southern rock, psych rock | 116 |
| Rock | Hard Rock | hard rock, arena rock, stadium rock, hair metal | 124 |
| Rock | Metal | metal, heavy metal, metalcore, thrash, djent, nu metal | 150 |
| Rock | Alt / Grunge | grunge, alternative, alt rock, indie rock, post punk | 112 |
| Rock | Punk / Pop Punk | punk, pop punk, skate punk, emo | 172 |
| Country | Classic / Honky Tonk | honky tonk, outlaw, classic country, train beat | 108 |
| Country | Modern Nashville | modern country, country pop, nashville, bro country | 116 |
| Country | Country Rap / Trap | country rap, country trap, hick hop | 140 |
| Country | Americana / Folk | americana, folk, bluegrass, roots, stomp and clap | 96 |
| Worship | Modern Worship | modern worship, ccm, contemporary christian | 72 |
| Worship | Gospel / Church | gospel, church, gospel chops, shout music | 100 |
| Worship | Ambient / Hymns | ambient worship, prayer, soaking, hymns | 68 |
| EDM | House | house, deep house, tech house, uk garage | 124 |
| EDM | Techno | techno, industrial, warehouse, detroit techno | 132 |
| EDM | Drum & Bass | drum and bass, dnb, jungle, liquid dnb | 174 |
| EDM | Dubstep / Bass | dubstep, riddim, bass music | 140 |
| EDM | Disco / Nu-Disco | disco, nu disco, italo, boogie | 120 |
| Lo-Fi | Jazz Hop / Chillhop | jazz hop, chillhop, lofi hip hop, study beats | 85 |
| Lo-Fi | Lo-Fi Trap / Sad | lofi trap, sad trap, sad beat | 75 |
| Cinematic | Epic Trailer | trailer, epic trailer, action movie, blockbuster | 100 |
| Cinematic | Dark / Horror | horror, thriller, suspense | 90 |
| Cinematic | World Percussion | world percussion, tribal, taiko, hand drums | 100 |

**Decades.** A decade moves all four kits toward its sound. With a genre and no named style, it also picks the style that decade is known for.

| Decade | Sound | With just a genre, it picks |
| --- | --- | --- |
| 60s | Warm, roomy, all live | R&B: Funk / Motown. Rock: Classic. Country: Classic. |
| 70s | Dry, deadened, warm | R&B: Funk / Motown. Rock: Classic. Country: Classic. EDM: Disco. |
| 80s | Gated rooms, drum machines | Hip-Hop: Old School. Pop: Synth Pop. Rock: Hard Rock. Country: Classic. EDM: Disco. |
| 90s | Samplers, punchy and warm | Hip-Hop: East Coast. R&B: New Jack. Rock: Alt / Grunge. Country: Classic. EDM: House. |
| 2000s | Tighter, louder, cleaner | Hip-Hop: Southern / Crunk. R&B: Neo Soul. Pop: Dance Pop. Rock: Punk. |
| 2010s | Crisp and clean | Trap: Atlanta. R&B: Alt R&B. Pop: Dance Pop. Country: Modern Nashville. Worship: Modern. EDM: Dubstep. |
| Today | Modern, polished, hard | Trap: Atlanta. R&B: Alt R&B. Pop: Indie. Country: Modern Nashville. Worship: Modern. EDM: Techno. |

**Artists and songs.** A sample of the names built in, by the genre they belong to. Many artists also have a take in other genres.

| Genre | Some of the names it knows |
| --- | --- |
| Hip-Hop | Dr. Dre, Snoop Dogg, Warren G, 2Pac, Ice Cube, Nas, Biggie, Wu-Tang, DJ Premier, J Dilla, Kendrick Lamar, OutKast, Three 6 Mafia, Run-DMC, Eminem, Kanye West |
| Trap | Metro Boomin, Future, Migos, 21 Savage, Lil Baby, Travis Scott, Playboi Carti, Pop Smoke, Central Cee, Lecrae |
| R&B | Teddy Riley, TLC, Aaliyah, D'Angelo, Erykah Badu, The Weeknd, Bryson Tiller, SZA, Bruno Mars, Stevie Wonder, James Brown, Prince |
| Pop | Michael Jackson, Phil Collins, Madonna, Dua Lipa, Taylor Swift, Ed Sheeran, Billie Eilish, Post Malone, Bon Jovi, Coldplay |
| Rock | Led Zeppelin, The Beatles, AC/DC, Def Leppard, Foo Fighters, Metallica, Linkin Park, Nirvana, Red Hot Chili Peppers, Green Day, blink-182 |
| Country | Johnny Cash, Willie Nelson, George Strait, Garth Brooks, Luke Bryan, Morgan Wallen, Luke Combs, Chris Stapleton, Zach Bryan, Jelly Roll, Lil Nas X |
| Worship | Hillsong, Elevation Worship, Bethel Music, Maverick City Music, Chris Tomlin, Kirk Franklin, CeCe Winans, Tasha Cobbs Leonard |
| EDM | Daft Punk, Avicii, Calvin Harris, Skrillex, Illenium, Pendulum, Bee Gees, Giorgio Moroder |
| Lo-Fi / Cinematic | Nujabes, Lofi Girl, Joji, Hans Zimmer, John Williams, John Carpenter |

Songs work the same way, for example "Still D.R.E.", "Regulate", "N.Y. State of Mind", "Mask Off", "In the Air Tonight", "Billie Jean", "Livin' on a Prayer", "When the Levee Breaks", "Folsom Prison Blues", "Old Town Road", "Way Maker" and "Uptown Funk".

## Tips and troubleshooting

| If... | Try this |
| --- | --- |
| A sound has the wrong type (a tom filed as an 808) | LIBRARY: right-click it, **It's really a...** and pick the right type. The fix sticks. |
| One sound keeps turning up and you don't like it | Right-click it, **Never use this sound**. Use **Favourite** on sounds you want more often. |
| A vocal chop or melody slipped into a kit | Right-click it in LIBRARY, **It's really a... Vocal / Melodic**. It won't be used again. |
| "No kicks / snares / hats in your library yet" | Add a pack that has them, or check that its folder is switched on and its kind is ticked under USE SOUNDS FROM. |
| The four kits feel too alike | Use the dice (Surprise me), move the sliders further, or name a style or artist. |
| A word or artist you typed didn't change anything | Check the blue "understood" line under the box. Use the drop-downs, or give a common-word name a cue ("like Future"). |
| Pads sound out of tune with your song | Set the right KEY in the header, or switch off "Tune kick, 808, toms to key". Fine-tune with the pad's TUNE knob. |
| The plug-in doesn't show up in your DAW | It's an instrument: look in your instrument list (Cubase: under Drum, by Grabber Audio). On Windows the VST3 is in C:\Program Files\Common Files\VST3. In Cubase, if it was ever blocked, open Studio > VST Plug-in Manager, show the blocklist, select Groove Grabber and click Reactivate. |
| No sound from the standalone app | Gear menu, **Audio / MIDI settings...**, and pick your output device. |
| The groove drifts from your song | Turn on **Follow DAW** in the groove bar. |
| Sounds land on the wrong pads in your sampler | Keep "Silent files for empty pads" on, and drop all 22 files on the note 36 pad. In Groove Agent, drop on the lowest drop icon; in Studio One, hold Shift. See Exporting to your DAW. |
| All the files stacked up on one pad | The sampler layered them. Drop them again the way your DAW spreads them (Exporting to your DAW), or one file per pad in FPC, REAPER and Kong. |
| Note names in the steps don't match your DAW | Pick your DAW under FOR in the export panel. The notes are the same; only the names differ (note 36 is C1 in Cubase, C3 in FL Studio). |
| Open hats ring over closed hats | Put the Closed Hat, Pedal Hat and Open Hat pads (notes 42, 44 and 46) in the same choke / exclusive group. |
| A pattern you dragged in from SAUCE doesn't play | It was dropped on empty space, so your DAW made a new track with no instrument. Drag the part onto your drum track (Groove Agent, Groove Grabber...), or set that track's output to your drum instrument. Next time, drop it straight on the drum track's lane. |
| You changed a pattern on SAUCE and want the original back | Press RESET under the pattern. UNDO brings your version back. |
| You want a pattern in every song, not just this one | SAUCE: SAVE AS MY GROOVE. It's then under MY GROOVES on SAUCE, in BUILD's groove list and on the MY KITS page. |
| You saved your version and want BUILD to use it instead of the original | Pick it in BUILD's groove list (MY GROOVES section), or press USE ON BUILD on the MY KITS page. Next time, save with SAVE & REPLACE ON BUILD. |
| You moved or renamed your drum folders | Remove the old folder on the LIBRARY page and add it again. Saved kits point at the old file locations. |
| The first scan takes a while | Every sound is analysed once. Later scans only look at new files, and you can keep working meanwhile. |

## Quick reference card

**Keys and clicks**

| Do this | To |
| --- | --- |
| F1 or the ? button | Open this manual |
| Space | Play / stop the preview groove (on SAUCE: the pattern you're looking at) |
| Click a step (SAUCE) | Add or take away a hit; drag along the row for several |
| Right-click a step (SAUCE) | Hard, medium, soft |
| Ctrl-click a step (SAUCE; Alt-click on a Mac) | A roll: 2 quick hits, then 3 (triplet), then 1 |
| Enter (in the description box) | Generate kits |
| Esc | Close the manual or the export panel |
| Double-click a slider or knob | Reset it |
| Click the logo | Switch dark / light |
| Right-click a sound (LIBRARY) | Fix its type, favourite it, ban it, put it on a pad |
| Drag a pad to your DAW | Use that one sound |
| Drop a file on a pad | Replace that pad's sound |

**Pad map** (export file numbers and MIDI notes; shown as C1 = MIDI note 36, the Cubase, Live and Logic naming)

| File | Pad | Note |
| --- | --- | --- |
| 01 | Kick | C1 (36) |
| 02 | Rim | C#1 (37) |
| 03 | Snare | D1 (38) |
| 04 | Clap | D#1 (39) |
| 05 | Snare 2 | E1 (40) |
| 06 | Floor Tom 2 | F1 (41) |
| 07 | Closed Hat | F#1 (42) |
| 08 | Floor Tom | G1 (43) |
| 09 | Pedal Hat | G#1 (44) |
| 10 | Low Tom | A1 (45) |
| 11 | Open Hat | A#1 (46) |
| 12 | Mid Tom | B1 (47) |
| 13 | Hi-Mid Tom | C2 (48) |
| 14 | Crash | C#2 (49) |
| 15 | High Tom | D2 (50) |
| 16 | Ride | D#2 (51) |
| 17 | 808 | E2 (52) |
| 18 | Perc 1 | F2 (53) |
| 19 | Perc 2 | F#2 (54) |
| 20 | Perc 3 | G2 (55) |
| 21 | FX 1 | G#2 (56) |
| 22 | FX 2 | A2 (57) |

Snare 2, Floor Tom 2, Pedal Hat, Low Tom and Hi-Mid Tom are variations of a main pad (tuned down, quieter or shorter), so General MIDI grooves always find a sound.
