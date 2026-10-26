#!/usr/bin/env bash
# PRIMARY experiment on the current machine, timing only (no performance counters):
# system info + coarse sweep + fine sweep + plots.
# Usage:  CORE=2 ./run_rob.sh                          (env: MIB ITERS REPS MAXN HP LABEL)
# Hybrid CPUs (Intel Alder Lake and later) REQUIRE both core types to be tested, one logical CPU each:
#   CORE=0  LABEL=pcore ./run_rob.sh      # a P-core (from /sys/devices/cpu_core/cpus; keep its SMT sibling idle)
#   CORE=20 LABEL=ecore ./run_rob.sh      # an E-core (from /sys/devices/cpu_atom/cpus)
set -euo pipefail
cd "$(dirname "$0")"
CORE=${CORE:-2}; MIB=${MIB:-512}; ITERS=${ITERS:-20000}; REPS=${REPS:-15}; MAXN=${MAXN:-800}; HP=${HP:-0}
LABEL=${LABEL:-cpu$CORE}
HPFLAG=""; [ "$HP" = "1" ] && HPFLAG="-H"

# ---- hybrid check: must declare the core type and the CPU must really be of that type ----
HYB=0; [ -r /sys/devices/cpu_atom/cpus ] && HYB=1
if [ "$HYB" = 1 ]; then
  case $LABEL in
    pcore) PMU=cpu_core; OTHER=ecore ;;
    ecore) PMU=cpu_atom; OTHER=pcore ;;
    *) echo "Hybrid CPU detected: set LABEL=pcore or LABEL=ecore. Both core types must be tested."; exit 1 ;;
  esac
  python3 - "$(cat /sys/devices/$PMU/cpus)" "$CORE" <<'PY' || { echo "CPU $CORE is not a $LABEL (see /sys/devices/$PMU/cpus)"; exit 1; }
import sys
want = int(sys.argv[2]); ok = False
for part in sys.argv[1].split(","):
    a, _, b = part.strip().partition("-")
    if a: ok |= int(a) <= want <= int(b or a)
sys.exit(0 if ok else 1)
PY
fi

TAG=$(hostname)_${LABEL}_hp${HP}
RUN=${RUN:-$(date +%Y%m%d-%H%M%S)}          # every run gets its own directory: repeats never overwrite each other
OUT=results/${TAG}_${RUN}
mkdir -p "$OUT"
ln -sfn "${TAG}_${RUN}" "results/${TAG}_latest"   # convenience pointer used by perf_check.sh

[ -x rob_test ] || make
../common/gather_info.sh "$OUT/sysinfo" >/dev/null
../common/freq_ctl.sh status > "$OUT/freq_during_run.txt" 2>&1 || true   # frequency state under which the data was taken
echo "core=$CORE label=$LABEL hp=$HP hybrid=$HYB run=$RUN" > "$OUT/run_config.txt"

echo "== coarse sweep (step 4) on CPU $CORE ($LABEL) =="
./rob_test -c "$CORE" -m "$MIB" -i "$ITERS" -r "$REPS" -s 0 -e "$MAXN" -t 4 $HPFLAG > "$OUT/coarse.csv"
KNEE=$(python3 analyze_rob.py "$OUT/coarse.csv" | tee "$OUT/coarse_summary.txt" | sed -n 's/^KNEE_N=//p')
cat "$OUT/coarse_summary.txt"

if [ -n "$KNEE" ]; then
  LO=$(( KNEE > 24 ? KNEE - 24 : 0 )); HI=$(( KNEE + 24 ))
  echo "== fine sweep (step 1) around N=$KNEE =="
  ./rob_test -c "$CORE" -m "$MIB" -i "$ITERS" -r $(( REPS * 2 )) -s "$LO" -e "$HI" -t 1 $HPFLAG > "$OUT/fine.csv"
  FKNEE=$(python3 analyze_rob.py "$OUT/fine.csv" --plot "$OUT/fine.png" --title "$TAG fine sweep" | tee "$OUT/fine_summary.txt" | sed -n 's/^KNEE_N=//p')
  if [ -z "$FKNEE" ]; then
    echo "WARNING: the fine sweep did NOT confirm the coarse knee at N=$KNEE. Treat the estimate as unreliable:"
    echo "         rerun on an idle machine, with more REPS/ITERS, and inspect the plots."
  fi
else
  echo "WARNING: no knee found in the coarse sweep (see warning above). No ROB estimate was produced."
fi
python3 analyze_rob.py "$OUT/coarse.csv" --plot "$OUT/coarse.png" --title "$TAG coarse sweep" >/dev/null || true
echo "PRIMARY results (timing only) in $OUT/"

# ---- reminders ----
if [ "$HYB" = 1 ] && [ -z "$(ls -d results/$(hostname)_${OTHER}_hp${HP}_* 2>/dev/null)" ]; then
  echo "REMINDER: hybrid CPU. Still to do: the $OTHER run (LABEL=$OTHER, CORE=<a CPU from /sys/devices/$([ $OTHER = pcore ] && echo cpu_core || echo cpu_atom)/cpus>)."
fi
if [ -f "/var/tmp/freq_state_$(hostname).txt" ]; then
  echo "WARNING: CPU frequency settings are still modified. Restore the originals: sudo ../common/freq_ctl.sh restore"
fi
if [ -n "${FKNEE:-}" ]; then
  echo "Timing-based estimate confirmed. Only now may you run the secondary counter check: CORE=$CORE LABEL=$LABEL HP=$HP ./perf_check.sh"
else
  echo "No confirmed timing-based estimate yet, so perf_check.sh will refuse to run (counters cannot be the main evidence)."
fi
