# 1.0.32 in progress: fit FX to bars, tune pitched FX to the key

Branch `claude/loving-knuth-7hdwu7`; `main` gets one commit at the end (`git commit-tree <branch>^{tree} -p origin/main`),
without this file.

## Decided with the owner (1.0.30 plan, Step 2)
- FIT: 1, 2, 4 or 8 bars at the song's tempo. Too long: trim from the start (a build-up keeps its end). Too short: stretch
  without changing pitch (Signalsmith Stretch, MIT, in ThirdParty/). Build-ups end on the downbeat, hits start on it,
  a whoosh peaks on it. Applies to the pads, HEAR IT IN TIME, DRAG and the export.
- Tune pitched FX (zaps, lasers, blips, booms, impacts with a tone) to the key, only when the pitch is sure and steady;
  noise FX untouched; "tuned +2 st to Am".

## Design
- SlotState: fitBars (0 off), fitAnchor (0 end, 1 peak, 2 start), fitStretch (stretch when short). RenderParams the same
  + fitBpm (the kit's bpm, which follows the song). Render: fit after LENGTH, before the level.
- fx::fitSettings (Type) -> anchor / stretch; fx::keyTune (Shape, Type, key) -> semitones (hits to the root, the rest
  to the nearest note of the key's scale).
- Processor: retuneFx() sets autoTune on every FX pad (kits' FX 1-4 and the bank) from the catalog; runs on key change,
  generate, a new FX sound, and when the catalog finishes sorting. Option text: "Tune kick, 808, toms, FX to key".
- FX tab: FIT (OFF 1 2 4 8) for the picked effect; HEAR IT IN TIME / DRAG / ADD / PUT use it and the tuning.
- BUILD pad editor: FIT for FX pads.

## Checklist
- [x] Core: SlotState, RenderParams, fit in render (render::fitTo, anchorSample), fx::fitSettings / keyTune, cache key; tests
- [ ] Processor: retuneFx, fit into ADD / PUT, hear in time + drag render
- [ ] FX tab: FIT control, tune line, drag a rendered file
- [ ] BUILD pad editor: FIT for FX pads
- [ ] Tests, manual, CLAUDE.md, THIRD_PARTY_NOTICES, version 1.0.32
- [ ] Release
