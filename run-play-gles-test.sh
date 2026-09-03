#!/usr/bin/env bash
#
# Boot a PS2 disc with the Play! core on the GLES Kodi build. Play! is the only
# PS2 core with a working OpenGL ES renderer, so this is the one that can reach
# the LibreELEC box. Play! HLEs the BIOS, so no BIOS files are needed.
#
#   ./run-play-gles-test.sh [secs] [disc]
set -uo pipefail

KODI_SRC=/home/chris/xbmc
KODI_BUILD=$KODI_SRC/build-gles
DATA=/home/chris/kodi-test-data
SHOTS=$KODI_SRC/shots-play
RPC=$KODI_SRC/rpc.py
LOG=$DATA/temp/kodi.log

RUN_SECS=${1:-60}
DISC=${2:-/home/chris/Downloads/ssx.iso}

fail() { echo "FAIL: $*" >&2; exit 1; }
[ -x "$KODI_BUILD/kodi.bin" ] || fail "no kodi.bin"
[ -f "$DISC" ]                || fail "disc missing: $DISC"

echo "== preparing =="
pkill -f "[k]odi.bin --windowing" 2>/dev/null && sleep 3
mkdir -p "$SHOTS" && rm -f "$SHOTS"/*.png
rm -rf "$DATA/saves"
: > "$LOG" 2>/dev/null || true

KODI_HOME=$KODI_BUILD KODI_DATA=$DATA \
  "$KODI_BUILD/kodi.bin" --windowing=wayland --debug \
  > "$KODI_SRC/kodi-play-run.log" 2>&1 &

python3 "$RPC" wait || fail "Kodi never answered on JSON-RPC 9090"
python3 "$RPC" Addons.SetAddonEnabled \
  '{"addonid":"game.libretro.play","enabled":true}' >/dev/null
sleep 2

echo "== render system =="
grep -m1 -oE "GL_VERSION = .*" "$LOG" | cut -c1-60

echo "== RunAddon(game.libretro.play, $(basename "$DISC")) =="
python3 "$RPC" Addons.ExecuteAddon \
  "{\"addonid\":\"game.libretro.play\",\"params\":[\"$DISC\"]}"
sleep 30

echo "== active players =="
python3 "$RPC" Player.GetActivePlayers

for i in $(seq 1 6); do
  python3 "$RPC" Input.ExecuteAction '{"action":"screenshot"}' >/dev/null
  sleep "$((RUN_SECS / 6))"
done

echo
echo "== negotiation + renderer =="
grep -E "Enabling hardware rendering|Creating renderer|maximum frame size|Failed to open stream|Using game client|which this build does not provide|not available on this display" "$LOG" | tail -12

echo
echo "== core log =="
grep -E "<game.libretro.play>" "$LOG" | grep -viE "RETRO_DEVICE|Port: " | tail -20

echo
echo "== GL errors: $(grep -icE "GL_INVALID|GL error" "$LOG") =="

echo
echo "== quitting =="
python3 "$RPC" Application.Quit >/dev/null 2>&1
sleep 8
if pgrep -f "[k]odi.bin --windowing" >/dev/null; then
  echo "!! still running after Quit"; pkill -f "[k]odi.bin --windowing"
else
  echo "clean exit"
fi
echo "captures: $(ls -1 "$SHOTS"/*.png 2>/dev/null | wc -l) in $SHOTS"
