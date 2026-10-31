#!/bin/bash
# Real drum one-shots with known types, for checking the sound sorting (not part of the build).
# Downloads into $AUDIT (default /tmp/gg-audit): ~25 MB of Dirt-Samples, ~140 MB of tidal-drum-machines.
set -e
AUDIT=${AUDIT:-/tmp/gg-audit}
mkdir -p "$AUDIT" && cd "$AUDIT"
[ -d dirt ] || git clone -q --depth 1 --filter=blob:none --sparse https://github.com/tidalcycles/Dirt-Samples.git dirt
(cd dirt && git sparse-checkout set 808 808bd 808cy 808hc 808ht 808lc 808lt 808mc 808mt 808oh 808sd bass1 bassdm bd cb clubkick cp cr \
    drumtraks electro1 glitch gretsch hardkick hc hh hit ho house ht jungbass kicklinn linnhats lt mt noise noise2 perc popkick \
    realclaps reverbkick rm rs sd sn space stab tabla wind)
[ -d tdm ] || git clone -q --depth 1 --filter=blob:none --sparse https://github.com/ritchse/tidal-drum-machines.git tdm
# (don't run "git ls-tree -l" on these: with blob:none it downloads every file one by one)
(cd tdm && git sparse-checkout set --no-cone $(for m in RolandTR808 RolandTR909 RolandTR707 LinnDrum OberheimDMX AkaiMPC60 EmuSP12 \
    AlesisHR16 KorgM1 YamahaRX5 BossDR660 RolandR8 SimmonsSDS5 CasioRZ1 RolandTR626 MPC1000 AkaiXR10 YamahaRM50 RolandTR505 Linn9000; do echo "/machines/$m/"; done))
echo "fetched into $AUDIT"
