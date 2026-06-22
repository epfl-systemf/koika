#!/usr/bin/env bash
set -euo pipefail

# ========== EDIT THESE ==========
SOURCE_DIR=""
DEST_DIR=""      # new folder to create
BUILD_DIR="$SOURCE_DIR/_build/default/examples/"    # folder containing the source files
IMPLEMENTATION_NAME=""            # without .hpp (e.g. "collatz" -> collatz.hpp)
# ================================

if command -v realpath >/dev/null 2>&1; then
  SOURCE_DIR="$(realpath "$SOURCE_DIR")"
  DEST_DIR="$(realpath -m "$DEST_DIR")"
fi

SRC_HARNESS_CPP="$BUILD_DIR/harness.cpp"
SRC_IMPL_HPP="$BUILD_DIR/${IMPLEMENTATION_NAME}.hpp"
SRC_CUTTLESIM_HPP="$BUILD_DIR/cuttlesim.hpp"
SRC_HARNESS_SUPPORT_HPP="$SOURCE_DIR/beak/harness_setup/harness_support.hpp"
SRC_MODE_SUPPORT="$SOURCE_DIR/beak/harness_setup/beak_mode.hpp"
SRC_ASSERTIONS_HPP="$SOURCE_DIR/beak/harness_setup/assertions.hpp"

for f in "$SRC_HARNESS_CPP" "$SRC_IMPL_HPP" "$SRC_CUTTLESIM_HPP" "$SRC_HARNESS_SUPPORT_HPP" "$SRC_ASSERTIONS_HPP"; do
  if [[ ! -f "$f" ]]; then
    echo "Error: required source file not found: $f" >&2
    exit 1
  fi
done

mkdir -p "$DEST_DIR/harness" "$DEST_DIR/implementation" "$DEST_DIR/input"
: > "$DEST_DIR/input/seed"

cp -f "$SRC_HARNESS_CPP" "$DEST_DIR/harness/harness.cpp"
cp -f "$SRC_IMPL_HPP" "$DEST_DIR/implementation/${IMPLEMENTATION_NAME}.hpp"
cp -f "$SRC_CUTTLESIM_HPP" "$DEST_DIR/implementation/cuttlesim.hpp"
cp -f "$SRC_HARNESS_SUPPORT_HPP" "$DEST_DIR/harness/harness_support.hpp"
cp -f "$SRC_MODE_SUPPORT" "$DEST_DIR/harness/beak_mode.hpp"
cp -f "$SRC_ASSERTIONS_HPP" "$DEST_DIR/harness/assertions.hpp"

echo "Done:"
echo "  $DEST_DIR/harness/harness.cpp"
echo "  $DEST_DIR/implementation/${IMPLEMENTATION_NAME}.hpp"
echo "  $DEST_DIR/implementation/cuttlesim.hpp"
echo "  $DEST_DIR/harness/harness_support.hpp"
echo "  $DEST_DIR/harness/beak_mode.hpp"
echo "  $DEST_DIR/harness/assertions.hpp"
echo "  $DEST_DIR/input/seed"
