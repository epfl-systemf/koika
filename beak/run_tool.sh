#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# ----------------------------
# Defaults
# ----------------------------

CMD="${1:-help}"
[[ $# -gt 0 ]] && shift || true

TEST_DIR="collatz"
IN_DIR="input"

MODE="init"          # init | inputs | init-and-inputs
OPT_LEVEL="O3"

LAF=1
CMPLOG=0
CLEAN=1
AUTO_REPLAY=1
FORCE_REPLAY=0

OUT_DIR=""
BIN_NAME=""

AFL_ARGS_STR=""
HARNESS_ARGS_STR=""
AFL_SEED=""
BEAK_MAX_CYCLES=""

SUMMARY_SCRIPT="fuzz_output_summary.py"

# ----------------------------
# Helpers
# ----------------------------

die() {
  echo "error: $*" >&2
  exit 2
}

print_cmd() {
  printf '+'
  printf ' %q' "$@"
  printf '\n'
}

usage() {
  cat <<EOF
Usage:
  $0 <command> [options]

Commands:
  run       Build, fuzz, replay crashes, and generate summary.
  beak      Alias for run.
  build     Build the fuzz target only.
  fuzz      Run afl-fuzz using an existing binary.
  replay    Replay/decode crashes. Skips already-decoded crashes by default.
  summary   Generate summary for a result directory.
  status    Show afl-whatsup for a result directory.
  help      Show this help.

Required/important options:
  --test DIR                 Test/design directory. Default: $TEST_DIR
  --mode MODE                init | inputs | init-and-inputs. Default: $MODE

Build options:
  --opt LEVEL                O0 | O1 | O2 | O3 | Os ... Default: $OPT_LEVEL
  --laf / --no-laf           Enable/disable AFL LLVM LAF splitting. Default: laf
  --cmplog / --no-cmplog     Enable/disable AFL++ CmpLog. Default: no-cmplog
  --max-cycles N             Adds -DBEAK_MAX_CYCLES=N
  --bin NAME                 Binary name. Default: auto-generated.

Fuzzing options:
  --in-dir DIR               Input corpus directory inside test dir. Default: $IN_DIR
  --out-dir DIR              Result directory. Default: auto-generated from config.
  --clean / --no-clean       Remove OUT_DIR before fuzzing. Default: clean
  --auto-replay / --no-auto-replay
                             Replay + summarize after fuzz stops. Default: auto-replay
  --seed N                   AFL seed, passed as -s N.
  --afl-args "ARGS"          Extra afl-fuzz args, e.g. "--afl-args '-m none -t 2000+'"
  --harness-args "ARGS"      Extra harness args before @@.

Replay/summary options:
  --force-replay             Decode crashes again even if *_decoded exists.

Examples:
  $0 run --test collatz --mode init
  $0 run --test collatz --mode inputs --opt O3 --laf --cmplog
  $0 build --test collatz --mode init --opt O3
  $0 fuzz --test collatz --mode init --no-clean
  $0 replay --test collatz --mode init
  $0 summary --out-dir collatz/results/init_O3_laf_nocmplog
  $0 status --test collatz --mode init
EOF
}

# ----------------------------
# Argument parsing
# ----------------------------

while [[ $# -gt 0 ]]; do
  case "$1" in
    --test)
      TEST_DIR="${2:?missing value for --test}"
      shift 2
      ;;
    --mode)
      MODE="${2:?missing value for --mode}"
      shift 2
      ;;
    --opt)
      OPT_LEVEL="${2:?missing value for --opt}"
      shift 2
      ;;
    --laf)
      LAF=1
      shift
      ;;
    --no-laf)
      LAF=0
      shift
      ;;
    --cmplog)
      CMPLOG=1
      shift
      ;;
    --no-cmplog)
      CMPLOG=0
      shift
      ;;
    --clean)
      CLEAN=1
      shift
      ;;
    --no-clean)
      CLEAN=0
      shift
      ;;
    --auto-replay)
      AUTO_REPLAY=1
      shift
      ;;
    --no-auto-replay)
      AUTO_REPLAY=0
      shift
      ;;
    --force-replay)
      FORCE_REPLAY=1
      shift
      ;;
    --in-dir)
      IN_DIR="${2:?missing value for --in-dir}"
      shift 2
      ;;
    --out-dir)
      OUT_DIR="${2:?missing value for --out-dir}"
      shift 2
      ;;
    --bin)
      BIN_NAME="${2:?missing value for --bin}"
      shift 2
      ;;
    --seed)
      AFL_SEED="${2:?missing value for --seed}"
      shift 2
      ;;
    --afl-args)
      AFL_ARGS_STR="${2:?missing value for --afl-args}"
      shift 2
      ;;
    --harness-args)
      HARNESS_ARGS_STR="${2:?missing value for --harness-args}"
      shift 2
      ;;
    --max-cycles)
      BEAK_MAX_CYCLES="${2:?missing value for --max-cycles}"
      shift 2
      ;;
    -h|--help|help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

# ----------------------------
# Derived configuration
# ----------------------------

case "$MODE" in
  init|register|registers|reg)
    MODE_TAG="init"
    MODE_ID=1
    ;;
  inputs|input|io)
    MODE_TAG="inputs"
    MODE_ID=2
    ;;
  init-and-inputs|init_and_inputs|both)
    MODE_TAG="init_and_inputs"
    MODE_ID=3
    ;;
  *)
    die "invalid --mode '$MODE'. Use init, inputs, or init-and-inputs."
    ;;
esac

LAF_TAG=$([[ "$LAF" == "1" ]] && echo "laf" || echo "nolaf")
CMPLOG_TAG=$([[ "$CMPLOG" == "1" ]] && echo "cmplog" || echo "nocmplog")

BUILD_DIR="$TEST_DIR/build"

if [[ -z "$BIN_NAME" ]]; then
  BIN_NAME="harness_${MODE_TAG}_${OPT_LEVEL}_${LAF_TAG}"
fi

BIN_PATH="$BUILD_DIR/$BIN_NAME"
CMPLOG_BIN_PATH="$BUILD_DIR/${BIN_NAME}_cmplog"

if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="$TEST_DIR/results/${MODE_TAG}_${OPT_LEVEL}_${LAF_TAG}_${CMPLOG_TAG}"
fi

IN_PATH="$TEST_DIR/$IN_DIR"
HARNESS_CPP="$TEST_DIR/harness/harness.cpp"
IMPL_DIR="$TEST_DIR/implementation"

# Simple whitespace splitting. Good enough for normal AFL/HARNESS args.
# For complex quoted values, prefer putting them directly into the script or using env-specific wrappers.
read -r -a AFL_EXTRA <<< "$AFL_ARGS_STR"
read -r -a HARNESS_EXTRA <<< "$HARNESS_ARGS_STR"

# ----------------------------
# Checks
# ----------------------------

check_layout() {
  [[ -d "$TEST_DIR" ]] || die "test directory not found: $TEST_DIR"
  [[ -f "$HARNESS_CPP" ]] || die "harness not found: $HARNESS_CPP"
  [[ -d "$IMPL_DIR" ]] || die "implementation include dir not found: $IMPL_DIR"
}

check_input_corpus() {
  [[ -d "$IN_PATH" ]] || die "input corpus directory not found: $IN_PATH"
}

check_binary() {
  [[ -x "$BIN_PATH" ]] || die "binary not found or not executable: $BIN_PATH. Run build or run first."
}

# ----------------------------
# Build
# ----------------------------

build_one() {
  local out_bin="$1"
  local cmplog_build="$2"

  mkdir -p "$BUILD_DIR"

  local defs=(
    -DSIM_FUZZER
    "-DBEAK_FUZZ_MODE=$MODE_ID"
  )

  if [[ -n "$BEAK_MAX_CYCLES" ]]; then
    defs+=("-DBEAK_MAX_CYCLES=$BEAK_MAX_CYCLES")
  fi

  local cmd=(
    afl-clang-fast++
    -g
    "-$OPT_LEVEL"
    -std=c++17
    "${defs[@]}"
    -I"$IMPL_DIR"
    -o "$out_bin"
    "$HARNESS_CPP"
  )

  if [[ "$cmplog_build" == "1" ]]; then
    echo "[build] CmpLog binary: $out_bin"
    print_cmd env AFL_LLVM_CMPLOG=1 "${cmd[@]}"
    env \
      -u AFL_LLVM_LAF_ALL \
      -u AFL_LLVM_LAF_SPLIT_COMPARES \
      -u AFL_LLVM_LAF_SPLIT_SWITCHES \
      -u AFL_LLVM_LAF_TRANSFORM_COMPARES \
      AFL_LLVM_CMPLOG=1 \
      "${cmd[@]}"
  elif [[ "$LAF" == "1" ]]; then
    echo "[build] target binary with LAF: $out_bin"
    print_cmd env AFL_LLVM_LAF_ALL=1 "${cmd[@]}"
    AFL_LLVM_LAF_ALL=1 "${cmd[@]}"
  else
    echo "[build] target binary without LAF: $out_bin"
    print_cmd env -u AFL_LLVM_LAF_ALL -u AFL_LLVM_CMPLOG "${cmd[@]}"
    env \
      -u AFL_LLVM_LAF_ALL \
      -u AFL_LLVM_LAF_SPLIT_COMPARES \
      -u AFL_LLVM_LAF_SPLIT_SWITCHES \
      -u AFL_LLVM_LAF_TRANSFORM_COMPARES \
      -u AFL_LLVM_CMPLOG \
      "${cmd[@]}"
  fi

  sha256sum "$out_bin"
}

build() {
  check_layout

  echo "[config] TEST_DIR=$TEST_DIR"
  echo "[config] MODE=$MODE_TAG BEAK_FUZZ_MODE=$MODE_ID"
  echo "[config] OPT_LEVEL=$OPT_LEVEL LAF=$LAF CMPLOG=$CMPLOG"
  echo "[config] BIN_PATH=$BIN_PATH"
  echo "[config] OUT_DIR=$OUT_DIR"

  build_one "$BIN_PATH" 0

  if [[ "$CMPLOG" == "1" ]]; then
    build_one "$CMPLOG_BIN_PATH" 1
  fi
}

# ----------------------------
# AFL++ commands
# ----------------------------

status() {
  if command -v afl-whatsup >/dev/null 2>&1; then
    print_cmd afl-whatsup "$OUT_DIR"
    afl-whatsup "$OUT_DIR" || true
  else
    echo "afl-whatsup not found."
  fi
}

run_summary() {
  [[ -f "$SUMMARY_SCRIPT" ]] || die "summary script not found: $SUMMARY_SCRIPT"

  echo "[summary] generating fuzzing summary for $OUT_DIR"
  print_cmd python3 "$SUMMARY_SCRIPT" "$OUT_DIR"
  python3 "$SUMMARY_SCRIPT" "$OUT_DIR"
}

replay() {
  check_binary

  local crash_dir="$OUT_DIR/default/crashes"

  if [[ ! -d "$crash_dir" ]]; then
    echo "[replay] crash directory not found: $crash_dir"
    return 0
  fi

  shopt -s nullglob
  local crashes=( "$crash_dir"/id:* )
  shopt -u nullglob

  if [[ ${#crashes[@]} -eq 0 ]]; then
    echo "[replay] no crashes found in $crash_dir"
    return 0
  fi

  local decoded=0
  local skipped=0

  echo "[replay] found ${#crashes[@]} crash(es)"

  for f in "${crashes[@]}"; do
    [[ -f "$f" ]] || continue

    local decoded_file="${f}_decoded"

    if [[ "$FORCE_REPLAY" != "1" && -s "$decoded_file" ]]; then
      ((skipped += 1))
      continue
    fi

    echo "[replay] decoding $(basename "$f")"
    "$BIN_PATH" --replay "$f" >/dev/null 2>&1 || true
    ((decoded += 1))
  done

  echo "[replay] decoded=$decoded skipped_existing=$skipped"
}

fuzz() {
  check_layout
  check_input_corpus
  check_binary

  if [[ "$CLEAN" == "1" ]]; then
    echo "[fuzz] cleaning OUT_DIR=$OUT_DIR"
    rm -rf "$OUT_DIR"
  fi

  mkdir -p "$OUT_DIR"

  local afl_cmd=(afl-fuzz)

  if [[ -n "$AFL_SEED" ]]; then
    afl_cmd+=(-s "$AFL_SEED")
  fi

  if [[ "$CMPLOG" == "1" ]]; then
    [[ -x "$CMPLOG_BIN_PATH" ]] || die "CmpLog binary not found: $CMPLOG_BIN_PATH. Run build first."
    afl_cmd+=(-c "$CMPLOG_BIN_PATH")
  fi

  afl_cmd+=("${AFL_EXTRA[@]}")
  afl_cmd+=(-i "$IN_PATH" -o "$OUT_DIR")
  afl_cmd+=(-- "$BIN_PATH")
  afl_cmd+=("${HARNESS_EXTRA[@]}")
  afl_cmd+=(@@)

  echo "[fuzz] IN_DIR=$IN_PATH"
  echo "[fuzz] OUT_DIR=$OUT_DIR"
  echo "[fuzz] BIN_PATH=$BIN_PATH"
  echo "[fuzz] MODE=$MODE_TAG OPT_LEVEL=$OPT_LEVEL LAF=$LAF CMPLOG=$CMPLOG"
  echo "[fuzz] command:"
  print_cmd "${afl_cmd[@]}"
  echo

  finish_fuzz() {
    local status=$?
    trap - EXIT

    echo
    echo "[fuzz] stopped with status=$status"

    if [[ "$AUTO_REPLAY" == "1" ]]; then
      set +e
      replay
      run_summary
      set -e
    fi

    exit "$status"
  }

  trap finish_fuzz EXIT

  "${afl_cmd[@]}"
}

run_all() {
  build
  fuzz
}

# ----------------------------
# Dispatch
# ----------------------------

case "$CMD" in
  run|beak)
    run_all
    ;;
  build)
    build
    ;;
  fuzz)
    fuzz
    ;;
  replay)
    replay
    ;;
  summary)
    run_summary
    ;;
  status)
    status
    ;;
  ""|-h|--help|help)
    usage
    ;;
  *)
    die "unknown command: $CMD"
    ;;
esac