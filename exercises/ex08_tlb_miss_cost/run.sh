#!/usr/bin/env bash
# ex08 entry point (used by scripts/fleet.sh). Env: CORE LABEL [REPS=9]
set -euo pipefail
cd "$(dirname "$0")"
COMMON=$(ls -d ../common ../../common 2>/dev/null | head -1 || true)
source "$COMMON/run_lib.sh"
rl_init ex08_tlb_miss_cost
./tlb_test -c "$CORE" -r "${REPS:-9}" > "$OUT/data.csv"
python3 analyze.py "$OUT/data.csv" --plot "$OUT/plot.png" | tee "$OUT/summary.txt"
rl_finish
