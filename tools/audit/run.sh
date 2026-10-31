#!/bin/bash
# Classify both sets with the current build and score them. Run from the repo root after building GGTests.
AUDIT=${AUDIT:-/tmp/gg-audit}
T=./build/GGTests_artefacts/Release/GGTests
for set in dirt-set tdm-set; do for v in named scrubbed; do
    $T --classify "$AUDIT/$set/$v" > "$AUDIT/$set/out_$v.tsv" 2>/dev/null &
done; done; wait
for set in dirt-set tdm-set; do for v in named scrubbed; do
    echo "== $set $v"; python3 -I tools/audit/score.py "$AUDIT/$set/labels_$v.json" "$AUDIT/$set/out_$v.tsv"
done; done
