#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

BUILD_DIR="$SCRIPT_DIR/build"

BINARY="$BUILD_DIR/AnimatedNeofetch"

mkdir -p "$BUILD_DIR"

if command -v cmake >/dev/null 2>&1; then

    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "$BUILD_DIR" --parallel

else

    COMPILER="$(command -v cc || command -v gcc || command -v clang || true)"

    if [[ -z "$COMPILER" ]]; then
        echo "Error: no C compiler found. Install build-essential or clang." >&2
        exit 1
    fi

    "$COMPILER" -std=c11 -O2 -Wall -Wextra -Wpedantic \
        "$SCRIPT_DIR/AnimatedNeofetch.c" -o "$BINARY"

fi

exec "$BINARY" "$@"
