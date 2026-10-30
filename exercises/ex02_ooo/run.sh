#!/usr/bin/env bash
# ex02 entry point (used by scripts/fleet.sh). Env: CORE LABEL [REPS=15] [STEP=16] [MIB=512] [ITERS=20000] [MAXF=320]
set -euo pipefail
cd "$(dirname "$0")"
COMMON=$(ls -d ../common ../../common 2>/dev/null | head -1 || true)
source "$COMMON/run_lib.sh"
rl_init ex02_ooo
./ooo_test -c "$CORE" -m "${MIB:-512}" -i "${ITERS:-20000}" -r "${REPS:-15}" -e "${MAXF:-320}" -t "${STEP:-16}" > "$OUT/data.csv"
python3 analyze.py "$OUT/data.csv" --plot "$OUT/plot.png" | tee "$OUT/summary.txt"
rl_finish
