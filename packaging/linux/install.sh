#!/usr/bin/env bash
# Installs Groove Grabber for the current user on Linux.
#   VST3 -> ~/.vst3   LV2 -> ~/.lv2   app -> ~/.local/bin/groove-grabber
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

mkdir -p ~/.vst3 ~/.lv2 ~/.local/bin
rm -rf ~/.vst3/"Groove Grabber.vst3" ~/.lv2/"Groove Grabber.lv2"
cp -R "$HERE/Groove Grabber.vst3" ~/.vst3/
cp -R "$HERE/Groove Grabber.lv2" ~/.lv2/
install -m 755 "$HERE/Groove Grabber" ~/.local/bin/groove-grabber

echo "Installed. Rescan plug-ins in your DAW, or run: groove-grabber"
