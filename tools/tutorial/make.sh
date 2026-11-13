#!/usr/bin/env bash
# Makes the video tutorial: the app's real screens (GGTests --tutorial), a narrator (Piper, an offline voice) and a
# beat the first kit plays. Needs the GGTests build, xvfb-run, ffmpeg, curl, and python3 with numpy and Pillow.
#
#   bash tools/tutorial/make.sh ["OUT.mp4"]
#
# WORK (default /tmp/gg-tutorial) keeps the voice, the pictures and the spoken lines between runs.
# VOICE is a Piper voice name; the default, LJ Speech, is public domain. Check a voice's licence before using
# another one in a video you'll share (several are non-commercial).
# TUT_HOME is the throwaway home folder the app runs in. Its paths show on screen, so /Users/you is used when it
# can be made.
set -euo pipefail
cd "$(dirname "$0")/../.."
WORK=${WORK:-/tmp/gg-tutorial}
VOICE=${VOICE:-en_US-ljspeech-high}
OUT=${1:-$WORK/Groove Grabber Tutorial.mp4}
if [ -z "${TUT_HOME:-}" ]; then
  if mkdir -p /Users/you 2>/dev/null && [ -w /Users/you ]; then TUT_HOME=/Users/you; else TUT_HOME=$WORK/home; fi
fi
mkdir -p "$WORK/voices"

# the voice
if [ ! -x "$WORK/venv/bin/piper" ]; then
  python3 -m venv "$WORK/venv"
  "$WORK/venv/bin/pip" install -q piper-tts
fi
IFS=- read -r LANGCODE NAME QUALITY <<< "$VOICE"
for ext in onnx onnx.json; do
  f="$WORK/voices/$VOICE.$ext"
  [ -s "$f" ] || curl -sSfL -o "$f" "https://huggingface.co/rhasspy/piper-voices/resolve/main/${LANGCODE%%_*}/$LANGCODE/$NAME/$QUALITY/$VOICE.$ext"
done

# the pictures and the beat, from a fresh run of the app
cmake --build build --target GGTests -j"$(nproc)"
rm -rf "$TUT_HOME" && mkdir -p "$TUT_HOME"
HOME="$TUT_HOME" xvfb-run -a -s "-screen 0 1920x1200x24" ./build/GGTests_artefacts/Release/GGTests --tutorial "$WORK/capture"

# the video
python3 tools/tutorial/make_video.py "$WORK/capture" "$OUT" --piper "$WORK/venv/bin/piper" --voice "$WORK/voices/$VOICE.onnx"
