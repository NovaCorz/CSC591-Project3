#!/usr/bin/env bash
# SECONDARY validation with hardware performance counters (perf).
# COURSE REQUIREMENT: counters must NOT be the main evidence. The ROB size must first be established from the
# timing sweep (run_rob.sh). This script refuses to run until that primary result exists for the same
# CORE/LABEL/HP, and it labels its output "SECONDARY".
#
#   CORE=2 [LABEL=cpu2] [HP=0] ./perf_check.sh [N_below_knee N_above_knee]
#
# Runs two points that differ only in N (default: knee-40 and knee+40) and prints counter deltas, so the common
# setup cost cancels. Event names differ per CPU and counters are often unavailable in VMs: check 'perf list',
# override with EVENTS=...   Hybrid CPUs: cpu_core/<event>/ (P-cores) or cpu_atom/<event>/ (E-cores).
#   Intel example: EVENTS=cycles,instructions,l1d_pend_miss.pending,l1d_pend_miss.pending_cycles
set -u
cd "$(dirname "$0")"
CORE=${CORE:-2}; LABEL=${LABEL:-cpu$CORE}; HP=${HP:-0}; MIB=${MIB:-512}; ITERS=${ITERS:-200000}
EVENTS=${EVENTS:-cycles,instructions}
TAG=$(hostname)_${LABEL}_hp${HP}; DIR=results/${TAG}_${RUN:-latest}   # RUN=<run id> to pick a specific run

KNEE=$(sed -n 's/^KNEE_N=//p' "$DIR/fine_summary.txt" 2>/dev/null | grep -m1 .)
if [ -z "$KNEE" ]; then
  echo "REFUSING: no CONFIRMED timing-based knee (fine sweep) found in $DIR/."
  echo "Counters cannot be the main evidence. Run: CORE=$CORE LABEL=$LABEL HP=$HP ./run_rob.sh first."
  exit 1
fi
LOW=${1:-$(( KNEE > 40 ? KNEE - 40 : 0 ))}; HIGH=${2:-$(( KNEE + 40 > 1024 ? 1024 : KNEE + 40 ))}
command -v perf >/dev/null || { echo "perf not installed: report this limitation (primary result is unaffected)"; exit 1; }

for N in $LOW $HIGH; do
  perf stat -x, -e "$EVENTS" -o /tmp/perf_$N.csv -- ./rob_test -c "$CORE" -m "$MIB" -i "$ITERS" -r 1 -s $N -e $N -t 1 >/dev/null \
    || { echo "perf stat failed (check /proc/sys/kernel/perf_event_paranoid, VM support, event names). Report the limitation."; exit 1; }
done

{
echo "=== SECONDARY EVIDENCE (hardware counters) for $TAG ==="
echo "Primary, timing-based knee: N_knee=$KNEE (ROB ~ $((KNEE+2))). Counters below only CONFIRM it; they do not establish it."
python3 - "$LOW" "$HIGH" <<'PY'
import sys
def load(n):
    d = {}
    for l in open(f"/tmp/perf_{n}.csv"):
        f = l.strip().split(",")
        if len(f) > 2 and f[0].replace(".", "").isdigit(): d[f[2]] = float(f[0])
    return d
lo, hi = load(sys.argv[1]), load(sys.argv[2])
print(f"{'event':45s} {'N='+sys.argv[1]:>16s} {'N='+sys.argv[2]:>16s} {'delta':>16s}")
for e in lo:
    if e in hi: print(f"{e:45s} {lo[e]:16.0f} {hi[e]:16.0f} {hi[e]-lo[e]:16.0f}")
pend = [e for e in lo if e.endswith("l1d_pend_miss.pending")]
pc = [e for e in lo if e.endswith("l1d_pend_miss.pending_cycles")]
if pend and pc and pend[0] in hi and pc[0] in hi:
    print(f"\nraw MLP (pending/pending_cycles, includes setup phase): N={sys.argv[1]}: "
          f"{lo[pend[0]]/lo[pc[0]]:.2f}   N={sys.argv[2]}: {hi[pend[0]]/hi[pc[0]]:.2f}")
print("\nIf the timing result is right: cycles rise sharply from the low N to the high N (~2x kernel time)")
print("while instructions change only by the extra NOPs. If they disagree, say so and discuss; do not")
print("replace the timing-based estimate with a counter-based one.")
PY
} | tee "$DIR/counter_check_SECONDARY.txt"
