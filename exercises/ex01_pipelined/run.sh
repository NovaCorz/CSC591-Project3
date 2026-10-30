#!/usr/bin/env bash
# ex01 entry point (used by scripts/fleet.sh). Env: CORE LABEL [REPS=9]
set -euo pipefail
cd "$(dirname "$0")"
if [ -d ../common ]; then
    COMMON=../common
elif [ -d ../../common ]; then
    COMMON=../../common
else
    echo "ERROR: Cannot find common directory" >&2
    exit 1
fi
source "$COMMON/run_lib.sh"
rl_init ex01_pipelined
REPS=${REPS:-9}
./pipe_test -c "$CORE" -r "$REPS" > "$OUT/data.csv"
python3 analyze.py "$OUT/data.csv" --plot "$OUT/plot.png" | tee "$OUT/summary.txt"
rl_finish
