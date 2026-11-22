#!/bin/sh
# Step 7 restart check: save in the GUI, quit, start a new process, compare;
# then let the console program read the same GUI-saved records.
#
#   sh tests/restart_check.sh
#
# Uses a fresh copy of data/ in a temporary folder; data/records is untouched.
cd "$(dirname "$0")/.." || exit 1
work=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/vr_restart_$$")
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT
cp -r data "$work/data"
rm -f "$work/data/records/"*.dat

export QT_QPA_PLATFORM=offscreen
./build-gui/vitalroute_gui_check --restart save "$work/data" || exit 1
./build-gui/vitalroute_gui_check --restart load "$work/data" || exit 1

echo
echo "-- Console program on the GUI-saved records --"
[ -f bin/vitalroute ] || [ -f bin/vitalroute.exe ] || make -s
out=$(printf '4\nVR-2026-000012\nn\n12\n' | ./bin/vitalroute "$work/data" | tr -d '\r')
echo "$out" | grep -E '^Loaded |Restart Patient|Bed       :|Unstable angina|Saved ' | sed 's/^/  /'
if echo "$out" | grep -q '^Loaded 12 patients' && echo "$out" | grep -q 'Unstable angina'; then
    echo "[PASS] R7  The console program loads the GUI-saved records and shows the visit note"
else
    echo "[FAIL] R7  The console program could not read the GUI-saved records"
    exit 1
fi
