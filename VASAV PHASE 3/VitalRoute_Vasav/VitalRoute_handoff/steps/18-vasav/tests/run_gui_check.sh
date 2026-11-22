#!/bin/sh
# Builds the GUI (CMake + Ninja) and runs the automated GUI checks.
#
#   sh tests/run_gui_check.sh                 checks only, no windows appear
#   sh tests/run_gui_check.sh --shots DIR     also save screenshots into DIR
#   sh tests/run_gui_check.sh --only nurse,map   run some groups only
#
# Windows (MSYS2 UCRT64 shell) and Linux. On Windows, Qt's offscreen platform
# draws text badly, so a screenshot run uses the normal platform: the windows
# appear for a moment (and briefly go full screen for the F11 checks).
cd "$(dirname "$0")/.." || exit 1

if [ ! -f build-gui/build.ninja ]; then
    cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release || exit 1
fi
cmake --build build-gui || exit 1

platform=offscreen
case " $* " in
*" --shots "*)
    case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) platform=windows ;;
    esac
    ;;
esac

QT_QPA_PLATFORM=$platform ./build-gui/vitalroute_gui_check "$@"
