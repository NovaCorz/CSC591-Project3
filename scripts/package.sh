#!/usr/bin/env bash
# Build the submission ZIP: report, source, raw data, plotting scripts, reproduction instructions, AI log.
# Usage: scripts/package.sh [output.zip]      (put your final report PDF in report/ first)
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=${1:-hw3_submission.zip}
rm -f "$OUT"
zip -rq "$OUT" README.md ai_log.md machines.example.txt docs common exercises scripts data report \
  -x '*/rob_test' '*/pipe_test' '*/ooo_test' '*/branch_test' '*/tlb_test' '*/btb_test' '*/bin/*' '*.o' '*/__pycache__/*' '*.pyc' '.DS_Store' '*/results/*'
echo "wrote $OUT"; unzip -l "$OUT" | tail -1
echo "Check before submitting: report PDF in report/, ai_log.md filled in, docs/machines.md and data/*/machine_card.md filled in. (The real machines.txt is intentionally not packaged.)"
