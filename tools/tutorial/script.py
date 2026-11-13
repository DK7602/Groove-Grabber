# The tutorial's script: what's said, and what's on screen while it's said.
#
# Each entry is a title card ("card"), a chapter card ("chapter"), or a line of narration ("say") with the shots
# shown while it's spoken. A shot shows one of the captured frames (GGTests --tutorial):
#   f      the frame
#   pre    the frame shown first, until the cursor clicks (the click is what changes it to f)
#   click  where the cursor clicks (a mark: see frames.json), point: where it moves without clicking
#   drag   where the cursor grabs and drags off to the right (DRAG MIDI)
#   hl     marks to light up (the rest of the screen dims a little)
#   zoom   marks to move the camera in on (none: the whole window)
#   w      how much of the line's time it gets (default 1)
# UI words are in capitals as on screen; the narrator says them as words.

SCRIPT = [
    {"card": "title"},

    {"chapter": "1", "title": "Your sounds"},
    {"say": "Groove Grabber has four pages: BUILD, SAUCE, MY KITS and LIBRARY. Start on LIBRARY.",
     "shots": [{"f": "lib-empty", "hl": ["tabs"], "point": "tab0"}, {"f": "lib-empty", "hl": ["tab1"], "point": "tab1"}]},
    {"say": "Press ADD DRUM FOLDER and choose the folder where your drum samples live. Groove Grabber only uses sounds you already own.",
     "shots": [{"pre": "lib-empty", "f": "lib-full", "click": "addFolder", "hl": ["addFolder"], "zoom": ["addFolder", "tab0"]},
               {"f": "lib-full", "w": 1.4}]},
    {"say": "It listens to every one-shot and sorts it: kicks, 808s, snares, claps, hats, toms, cymbals, percussion and effects. It does this once, and remembers.",
     "shots": [{"f": "lib-full", "hl": ["table"]}]},
    {"say": "Click a type to see only those sounds, and click any sound to hear it.",
     "shots": [{"pre": "lib-full", "f": "lib-808", "click": "chip808", "hl": ["chips"]}]},
    {"say": "It reads file names, but it also listens. These downloads are all called Untitled, and each one still landed in the right place.",
     "shots": [{"f": "lib-untitled", "hl": ["table"], "point": "search"}]},
    {"say": "If it ever gets one wrong, right-click the sound to set what it really is, make it a favourite, or never use it.",
     "shots": [{"pre": "lib-untitled", "f": "lib-menu", "click": "row", "hl": ["menu", "submenu"], "zoom": ["menu", "submenu", "row"]}]},

    {"chapter": "2", "title": "Describe your song"},
    {"say": "On BUILD, describe your song in your own words: the genre, the decade, the feel, the tempo and the key. You can even name an artist.",
     "shots": [{"pre": "build-empty", "f": "build-empty", "click": "description", "hl": ["description"], "zoom": ["description", "songBoxes"]},
               {"f": "type-1", "zoom": ["description", "songBoxes"], "w": 0.3},
               {"f": "type-2", "zoom": ["description", "songBoxes"], "w": 0.3},
               {"f": "type-3", "zoom": ["description", "songBoxes"], "w": 0.3},
               {"f": "type-4", "zoom": ["description", "songBoxes"], "w": 0.3},
               {"f": "type-5", "zoom": ["description", "songBoxes"], "w": 0.3},
               {"f": "type-6", "zoom": ["description", "songBoxes"], "w": 0.9}]},
    {"say": "Groove Grabber reads it back. Here it picked hip-hop, East Coast boom bap, the 90s, 88 BPM and A minor. You can change any of them by hand.",
     "shots": [{"f": "build-described", "hl": ["understood", "songBoxes", "keyBpm"]}]},
    {"say": "KIT CHARACTER shapes the sound: punch, warmth, brightness, room, vintage, aggression and live feel. Your description sets them, and you can move them.",
     "shots": [{"f": "build-described", "hl": ["character"], "zoom": ["character", "songBoxes", "options"]}]},
    {"say": "Under OPTIONS, KIT SIZE picks 17, 23 or 32 sounds, and you choose the feel, the low end and how much percussion.",
     "shots": [{"f": "build-described", "hl": ["options"], "zoom": ["character", "options", "toggles"]}]},

    {"chapter": "3", "title": "Generate and shape your kits"},
    {"say": "Press GENERATE KITS, and you get four different kits from your library, each with its own name and character, with the kick, 808 and toms tuned to your key.",
     "shots": [{"pre": "build-described", "f": "build-kits", "click": "generate", "hl": ["generate"]},
               {"f": "build-kits", "hl": ["cards"], "w": 2.2}]},
    {"say": "Click a kit to hear it play a groove at your song's tempo. The bar at the bottom picks the groove and the swing, and Follow DAW keeps it in time with your project.",
     "shots": [{"pre": "build-kits", "f": "build-kit2", "click": "card1", "hl": ["card1"]},
               {"f": "build-kit2", "hl": ["transport"], "point": "follow", "w": 1.4}]},
    {"say": "Click a pad to hear it and shape it: tune it, trim the start, set the length and gain, or reverse it.",
     "shots": [{"pre": "build-kit2", "f": "build-pad", "click": "snarePad", "hl": ["snarePad"]},
               {"f": "build-pad", "hl": ["detail"], "zoom": ["detail"], "w": 1.6}]},
    {"say": "Not feeling a sound? The alternatives underneath are close matches. Click one to hear it, and press USE to swap it in.",
     "shots": [{"f": "build-pad", "hl": ["alts"], "zoom": ["detail"], "point": "alts"}]},
    {"say": "Lock the pads you love, then press REGENERATE for new sounds everywhere else. SAVE KIT keeps the kit in MY KITS.",
     "shots": [{"f": "build-pad", "hl": ["lockAll"], "point": "lockAll"},
               {"pre": "build-pad", "f": "build-regen", "click": "regenerate", "hl": ["pads"]},
               {"f": "build-regen", "hl": ["saveKit"], "point": "saveKit"}]},

    {"chapter": "4", "title": "Export to your DAW"},
    {"say": "When the kit feels right, press EXPORT.",
     "shots": [{"pre": "build-before-export", "f": "export", "click": "export", "hl": ["export"]}]},
    {"say": "Pick your DAW under FOR. Groove Grabber knows the drum sampler in each one.",
     "shots": [{"pre": "export", "f": "export-daws", "click": "forBox", "hl": ["menu"], "zoom": ["exportCard"]}]},
    {"say": "It writes one WAV per pad, trimmed, tuned and level matched, in the order your sampler expects, plus the groove as a MIDI file.",
     "shots": [{"f": "export-cubase", "hl": ["exportToggles", "format"], "zoom": ["exportToggles", "format", "forBox"]}]},
    {"say": "Then it shows the steps for your sampler. Here it's Groove Agent in Cubase: drag all the files onto the first pad, and the kit spreads across the pads in order.",
     "shots": [{"pre": "export-cubase", "f": "export-done", "click": "exportKit", "zoom": ["exportCard"]},
               {"f": "export-done", "hl": ["steps"], "zoom": ["steps"], "w": 2.5}]},

    {"chapter": "5", "title": "The SAUCE"},
    {"say": "SAUCE shows how the drums go in your genre, style and decade: the tempo, the swing, and what the kick, snare and hats usually play.",
     "shots": [{"f": "sauce", "hl": ["left"], "point": "recipe"}, {"f": "sauce", "hl": ["doc"], "w": 1.3}]},
    {"say": "Each pattern plays with your kit. Press play to hear it.",
     "shots": [{"pre": "sauce", "f": "sauce-playing", "click": "play", "hl": ["grid", "grooveChips"]}]},
    {"say": "Click a step to add or take away a hit. Right-click for hard, medium or soft. Ctrl-click for a quick roll.",
     "shots": [{"pre": "sauce-playing", "f": "sauce-edit-1", "click": "cell1", "zoom": ["grid"], "hl": ["cell1"]},
               {"pre": "sauce-edit-1", "f": "sauce-edit-2", "click": "cell2", "zoom": ["grid"], "hl": ["cell2"]},
               {"pre": "sauce-edit-2", "f": "sauce-edit-3", "click": "cell3", "zoom": ["grid"], "hl": ["cell3"]}]},
    {"say": "SWING runs from straight, at 50%, to a full triplet at 66%.",
     "shots": [{"pre": "sauce-edit-3", "f": "sauce-swing", "click": "swingSlider", "hl": ["swingSlider"], "zoom": ["swingSlider", "grid"]}]},
    {"say": "When you like it, drag DRAG MIDI straight onto your drum track. The notes match every Groove Grabber kit.",
     "shots": [{"f": "sauce-swing", "drag": "drag", "hl": ["drag"]}]},

    {"chapter": "6", "title": "MY GROOVES and COACH MY BEAT"},
    {"say": "Press SAVE AS MY GROOVE to keep a pattern for every song. It keeps the kit you made it with, too.",
     "shots": [{"pre": "sauce-swing", "f": "sauce-naming", "click": "saveMine", "hl": ["nameBox"]},
               {"f": "sauce-mine", "hl": ["myChip"], "w": 1.2}]},
    {"say": "Later, open it from MY GROOVES. Load it with its original kit, or try it on the kit you're building now. Your kits on BUILD stay as they are.",
     "shots": [{"pre": "sauce", "f": "sauce-mygrooves-menu", "click": "myChip", "hl": ["menu", "submenu"], "zoom": ["menu", "submenu", "grooveChips"]},
               {"f": "sauce-original", "hl": ["original"], "w": 1.2}]},
    {"say": "BLANK gives you an empty grid with a row for every sound in your kit, to build a beat from scratch.",
     "shots": [{"pre": "sauce-original", "f": "sauce-blank-empty", "click": "blank", "hl": ["grid"]},
               {"f": "sauce-blank", "hl": ["grid"]}]},
    {"say": "Then press COACH MY BEAT. It compares your beat with the style and gives you direct tips, with the reason behind each one.",
     "shots": [{"pre": "sauce-blank", "f": "sauce-coach", "click": "coachBtn", "hl": ["coach"]}]},
    {"say": "TRY IT makes the change for you, and UNDO takes it back. If you did it that way on purpose, press ON PURPOSE and the tip goes away.",
     "shots": [{"pre": "sauce-coach", "f": "sauce-coach-after", "click": "tryIt", "hl": ["grid"]},
               {"f": "sauce-coach-after", "hl": ["coach"]}]},

    {"chapter": "7", "title": "MY KITS and help"},
    {"say": "MY KITS keeps everything you save: your kits by genre, your recent builds, and your grooves. Load a kit back into BUILD, or export it again any time.",
     "shots": [{"pre": "sauce-coach-after", "f": "kits", "click": "tab2", "hl": ["list"]},
               {"f": "kits-grooves", "hl": ["list"]}]},
    {"say": "Press the question mark, or F1, for the full manual, with the loading steps for every DAW.",
     "shots": [{"pre": "build-final", "f": "manual", "click": "help"}]},
    {"say": "And the sun button switches between dark and light.",
     "shots": [{"pre": "build-final", "f": "build-light", "click": "theme"}]},

    {"card": "outro"},
]

TITLE = "Getting started"
OUTRO_LINE = "Your sounds. Built to fit your song."
OUTRO_SAY = "That's Groove Grabber: your sounds, built to fit your song. Now go make something great."
INTRO_SAY = "Welcome to Groove Grabber. In the next few minutes, you'll go from your own drum samples to four kits that fit your song, ready to play in your DAW."
