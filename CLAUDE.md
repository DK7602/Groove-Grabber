# Groove Grabber: notes for Claude

Groove Grabber is a JUCE 8 plug-in and standalone app (VST3, AU, Standalone; LV2 on Linux). It builds four drum kits from the user's own sample library to fit a typed song description, and exports them for the DAW's drum sampler (Groove Agent, Drum Rack, FPC, Drum Machine Designer, Impact...). The SAUCE page teaches how the drums go in each genre, style and decade. The owner may sell it later, so keep it DAW-neutral: anything Cubase-only belongs in the Cubase guide in `Daws.cpp`. The owner uses **Cubase on Windows**. They install builds from the GitHub Actions artifacts (Groove-Grabber-Windows). They make rap, rock, country and pop, and don't use profanity, so keep kit names, examples and UI text clean.

## Build and test (Linux container)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release        # JUCE 8.0.10 comes in via FetchContent
cmake --build build --target GGTests -j8              # the test runner
cmake --build build --target GrooveGrabber_VST3 -j8   # the plug-in
./build/GGTests_artefacts/Release/GGTests             # engine tests (all must pass)
xvfb-run -a -s "-screen 0 1920x1200x24" ./build/GGTests_artefacts/Release/GGTests --shots /tmp/shots   # + UI screenshots
xvfb-run -a ./build/GGTests_artefacts/Release/GGTests --host "build/GrooveGrabber_artefacts/Release/VST3/Groove Grabber.vst3"
./build/GGTests_artefacts/Release/GGTests --classify <folder>   # TSV of what the analyser measures and decides per file
```

- `--host` loads the VST3 the way a DAW does: scan, play, save and restore state, open the window, close. It runs three rounds. Windows CI runs it too. A crash there is what kept the plug-in out of Cubase until 1.0.6.
- Don't download and run pluginval or other outside executables: the environment blocks it. Use `--host` instead.
- Look at the screenshots: Read `/tmp/shots/*.png` after UI changes.

## Releasing a version

1. Bump `VERSION` in `CMakeLists.txt` (`project(GrooveGrabber VERSION x.y.z ...)`). The app, the About page and the manual's `{VERSION}` all read it.
2. Update `Resources/Manual.md`, the in-app manual (F1 / the ? button), whenever behaviour changes.
3. Commit with the session's attribution trailer and push to `main`. CI (`.github/workflows/build.yml`) builds macOS, Windows and Linux, and runs the tests and the Windows host test.
4. `gh run watch <id> -R DK7602/Groove-Grabber --exit-status`. Artifact and log downloads are blocked by the proxy. Read the Windows host-test result from the job's annotations: `gh api repos/DK7602/Groove-Grabber/check-runs/<job id>/annotations`.
5. Give the user the run URL (artifact "Groove-Grabber-Windows").

## Where things are

- `Source/core/Analyzer.*`: what each sound is.
  - `guessFromName` and `catFromTokens`: name and folder words, drum-machine shorthand.
  - `measure`: envelope, ring time, spectrum, flatness, clap bursts, pitch, settled 808 pitch, glide.
  - `classifyAudio`: per-category 0..1 scores.
  - `finish`: name vs audio, open/closed hats, 808 vs kick, loops.
- `Source/core/SoundInfo.*`: the measured fields and the library cache format.
  - New measurements: bump `SoundInfo::currentMeasure` (the scan re-measures older sounds and keeps user marks), and `cacheVersion` in `Library.cpp` if the file layout changes.
  - Rule-only changes: bump `rulesVersion` in `Library.cpp` (cached sounds are re-sorted with no files read).
- `Source/core/Library.*`: background scan, cache (`library.ggl` in AppData), user edits.
- `Source/core/KitGenerator.*`: picks the sounds for each slot.
  - `baseScore` (traits, quality), `contextScore` (kit chemistry: kick vs 808, hats from one pack, toms spread, FX kinds differ).
  - Tuning: to key, or with no key the kick goes to the 808's note.
- `Source/core/KitNames.*`: a fresh name for each kit on every Build, from the style vocabulary, place and mood words in the description, and each kit's standout trait. Re-rolling one kit keeps its name.
- `Source/core/Styles.*`: genres, sub-styles (four directions each), eras, KitOptions.
- `Source/core/SongContext.*` and `References.*`: reading the description, and the artist/song table.
- `Source/core/SampleRender.*`: trim, tune, length, level matching. `Exporter.*`: the WAV / MPC export and Kit Info.txt.
- `Source/core/Daws.*`: one loading guide per DAW (steps, choke, saving, what the DAW calls note 36), host detection, the "daw" setting ("" = automatic). The manual's `{DAW_GUIDES}` is generated from it, so fix steps there, not in Manual.md.
- `Source/core/Sauce.*` and `Resources/Sauce.md`: the SAUCE write-ups, one `@@ key` section per style id, decade (`era.N`) and decade per genre (`era.N.<genre>`). A new style needs a section (the tests check every one). Plain ASCII, clean words.
- `Source/core/Grooves.*`: the preview grooves (16-step lanes). `swingEighths` makes a groove swing its 8ths (the country Shuffle). Step editing (`stepHits`, `withStep`, `nextLevel`) backs the SAUCE grid. Pattern MIDI is on channel 1.
- Edited grooves live in the processor (`editGroove` / `resetGroove` / `getGroove`, saved in the state as "grooveEdits"). The player holds a raw pointer, so a replaced edit is kept in `retiredGrooves` for a while; always fetch grooves through `getGroove`, never keep a `const Groove*` to an edit.
- `Source/core/Harvester.*`: cutting hits out of a drum bounce (audio-only classification).
- `Source/ui/*`: pages (Build, Library, My Kits, Sauce), ExportPanel (the FOR picker and the DAW's steps), HelpPanel (in-app manual and About; its `DocView` also renders SAUCE), Theme.
- `Tests/`:
  - `TestMain.cpp`: engine tests, plus a synthetic library from `TestSounds.cpp`, which must stay at 45/45 classified.
  - `TestScreens.cpp`: screenshots.
  - `TestHost.cpp`: `--host` and `--classify`.

## Checking the sound sorting on real samples

`tools/audit/` scores the analyser against about 1,900 labelled drum-machine and kit one-shots. They come from tidalcycles/Dirt-Samples and ritchse/tidal-drum-machines (not shipped, downloaded on demand):

```
bash tools/audit/fetch.sh && python3 -I tools/audit/label.py   # once (AUDIT=/tmp/gg-audit by default)
bash tools/audit/run.sh                                         # after building GGTests
```

At 1.0.7 the scores were:

| Set | With names | From sound alone |
|---|---|---|
| Dirt | 86.7% | 62.8% |
| TDM | 87.6% | 69.0% |

Before the 1.0.7 audit they were 61–79% with names and 34–37% from sound alone. Don't let a change lower them. Keep the synthetic 45/45 too. `score.py ... -w` lists the misses.

## Things learned the hard way

- Build used to keep each kit's old name forever, so names from old saved sessions stuck. Names are now set in `GrooveGrabberProcessor::generate` via `nameKits`.
- 808s with a pitch drop or slide read 2–8 semitones sharp from a single pitch window. Tune by the note they settle on (the `measure` windows), and don't let one window decide.
- Spectral flatness over empty top octaves (8-bit and band-limited samples) reads "tonal": measure it only over the band the sound covers.
- Plain noise flutters, so the old burst counter called noisy snares claps. A burst needs a real gap (−6 dB and back up).
- Ring time has to stop above the hiss floor, or hissy closed hats read as open.
- "808" alone in a name can be the drum machine ("808 Cow", "TR-808 Kit"), not the bass.
- DAWs spread a multi-file drop differently: Groove Agent needs the lowest of three drop icons (C1 is in pad group 3, files 13-22 go to group 4), Studio One's Impact needs Shift, Live / Bitwig / Logic spread on their own, FPC, REAPER's RS5K and Kong layer instead. Pro Tools can't load the plug-in (AAX only). Note 36 is C1 in most DAWs, C2 in REAPER, C3 in FL Studio.
- FL Studio's FPC finds samples by file name, so FL exports name each file "01 Kick - <kit>.wav" and skip the MPC program (it can only name 16-character files). Other DAWs keep "01 Kick.wav".
