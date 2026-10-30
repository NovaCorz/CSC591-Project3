#!/usr/bin/env bash
# ex03 entry point (used by scripts/fleet.sh). Env: CORE LABEL [REPS=9]
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
rl_init ex03_branch_prediction
./branch_test -c "$CORE" -r "${REPS:-9}" > "$OUT/data.csv"
python3 analyze.py "$OUT/data.csv" --plot "$OUT/plot.png" | tee "$OUT/summary.txt"
rl_finish
