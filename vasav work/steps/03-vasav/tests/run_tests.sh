#!/bin/sh
# Builds and runs the VitalRoute test suite.
# Each test binary prints one [PASS]/[FAIL] line per test; this script
# exits non-zero if any binary reports a failure.

cd "$(dirname "$0")/.." || exit 1

${MAKE:-make} -s test-bins || { echo "[FAIL] build"; exit 1; }

status=0
ran=0
for t in bin/test_engine bin/test_scenarios; do
    if [ -f "$t" ] || [ -f "$t.exe" ]; then
        echo
        "./$t" || status=1
        ran=$((ran + 1))
    fi
done

echo
if [ "$ran" -eq 0 ]; then
    echo "No test binaries were built."
elif [ "$status" -eq 0 ]; then
    echo "ALL TESTS PASSED"
else
    echo "SOME TESTS FAILED"
fi
exit $status
