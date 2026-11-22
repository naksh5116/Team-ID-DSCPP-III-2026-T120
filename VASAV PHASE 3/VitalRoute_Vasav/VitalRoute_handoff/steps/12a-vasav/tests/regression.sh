#!/bin/sh
# Console regression check.
#
# Runs the console program on every tests/regression/NN-name.in against a
# fresh copy of data/ (no saved records) and compares the output with
# tests/regression/NN-name.expected. If NN-name.2.in exists, the program is
# started a second time on the same data folder (saved records reload).
#
# Clock times (with the " (+1d)" suffix a time past midnight gets), "in N
# min" and calendar dates are masked so the expected files stay valid on any
# day and at any hour. Patient IDs carry the year, so the expected
# files must be regenerated (--update) after the year changes.
#
#   sh tests/regression.sh                  compare bin/vitalroute
#   sh tests/regression.sh path/to/binary   compare another build
#   sh tests/regression.sh --update [bin]   rewrite the expected files
cd "$(dirname "$0")/.." || exit 1
root=$(pwd)

update=0
if [ "$1" = "--update" ]; then
    update=1
    shift
fi
bin=${1:-bin/vitalroute}
[ -f "$bin" ] || [ -f "$bin.exe" ] || { echo "Build $bin first (make)."; exit 1; }
case $bin in /*) ;; *) bin="$root/$bin" ;; esac

work=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/vr_regression_$$")
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

mask() {
    tr -d '\r' | sed -E \
        -e 's/[0-9]{2}:[0-9]{2}( \(?\+[0-9]+d\)?)?/HH:MM/g' \
        -e 's/\(in [0-9]+ min\)/(in N min)/g' \
        -e 's/[0-9]{4}-[0-9]{2}-[0-9]{2}/YYYY-MM-DD/g'
}

# Equality needs only coreutils; showing the changed lines uses diff
# (MSYS2: pacman -S diffutils) or git when one of them is installed.
same_file() {
    [ "$(cksum < "$1")" = "$(cksum < "$2")" ]
}
show_diff() {
    if command -v diff >/dev/null 2>&1; then
        diff -u "$1" "$2"
    elif command -v git >/dev/null 2>&1; then
        git diff --no-index --no-color -- "$1" "$2"
    else
        echo "  (install diffutils to see the changed lines)"
    fi
}

run_case() {   # $1 = case name without extension; output on stdout
    rm -rf "$work/data"
    cp -r data "$work/data"
    rm -f "$work/data/records/"*.dat
    (cd "$work" && "$bin" data < "$root/tests/regression/$1.in" 2>&1)
    if [ -f "tests/regression/$1.2.in" ]; then
        echo "----- restart -----"
        (cd "$work" && "$bin" data < "$root/tests/regression/$1.2.in" 2>&1)
    fi
}

status=0
count=0
for input in tests/regression/[0-9][0-9]-*.in; do
    name=$(basename "$input" .in)
    case $name in *.2) continue ;; esac
    count=$((count + 1))
    expected="tests/regression/$name.expected"
    run_case "$name" | mask > "$work/actual.txt"
    if [ "$update" -eq 1 ]; then
        cp "$work/actual.txt" "$expected"
        echo "[UPDATED] $name"
    elif [ ! -f "$expected" ]; then
        echo "[FAIL] $name (no expected output; run with --update)"
        status=1
    elif same_file "$expected" "$work/actual.txt"; then
        echo "[PASS] $name"
    else
        echo "[FAIL] $name"
        show_diff "$expected" "$work/actual.txt"
        status=1
    fi
done

echo
if [ "$update" -eq 1 ]; then
    echo "Expected output rewritten for $count cases."
elif [ "$status" -eq 0 ]; then
    echo "CONSOLE OUTPUT UNCHANGED ($count cases)"
else
    echo "CONSOLE OUTPUT CHANGED"
fi
exit $status
