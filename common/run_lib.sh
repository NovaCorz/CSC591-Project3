#!/usr/bin/env bash
# run_lib.sh - shared setup for the run.sh of every exercise. Usage (from an exercise's run.sh):
#   COMMON=$(ls -d ../common ../../common 2>/dev/null | head -1 || true); source "$COMMON/run_lib.sh"; rl_init <exercise_name>
# Env understood by every exercise: CORE (logical CPU, default 2)  LABEL (default cpu$CORE)  HP (0/1, default 0)  RUN (run id)
# Provides: $CORE $LABEL $HP $RUN $TAG $OUT (results dir, created) and rl_finish for the closing reminders.

rl_init() {
  EX=$1
  CORE=${CORE:-2}; LABEL=${LABEL:-cpu$CORE}; HP=${HP:-0}
  if [ "$(uname -m)" != x86_64 ]; then
    echo "$EX: x86-64 only for now (this machine is $(uname -m)); skip this machine for this exercise." >&2; exit 2
  fi
  # hybrid CPUs: both core types must be tested, one labelled run each
  HYB=0; [ -r /sys/devices/cpu_atom/cpus ] && HYB=1
  if [ "$HYB" = 1 ]; then
    case $LABEL in
      pcore) PMU=cpu_core; OTHER=ecore ;;
      ecore) PMU=cpu_atom; OTHER=pcore ;;
      *) echo "Hybrid CPU detected: set LABEL=pcore or LABEL=ecore. Both core types must be tested." >&2; exit 1 ;;
    esac
    python3 - "$(cat /sys/devices/$PMU/cpus)" "$CORE" <<'PYEOF' || { echo "CPU $CORE is not a $LABEL (see /sys/devices/$PMU/cpus)" >&2; exit 1; }
import sys
want = int(sys.argv[2]); ok = False
for part in sys.argv[1].split(","):
    a, _, b = part.strip().partition("-")
    if a: ok |= int(a) <= want <= int(b or a)
sys.exit(0 if ok else 1)
PYEOF
  fi
  TAG=$(hostname)_${LABEL}_hp${HP}
  RUN=${RUN:-$(date +%Y%m%d-%H%M%S)}
  OUT=results/${TAG}_${RUN}
  mkdir -p "$OUT"
  ln -sfn "${TAG}_${RUN}" "results/${TAG}_latest"
  make -s >/dev/null 2>&1 || make
  "$COMMON/gather_info.sh" "$OUT/sysinfo" >/dev/null
  "$COMMON/freq_ctl.sh" status > "$OUT/freq_during_run.txt" 2>&1 || true
  echo "core=$CORE label=$LABEL hp=$HP hybrid=$HYB run=$RUN" > "$OUT/run_config.txt"
  echo "== $EX on CPU $CORE ($LABEL) =="
}

rl_finish() {
  echo "PRIMARY results (timing only) are on THIS machine in: $(pwd)/$OUT/"
  echo "(if you started this with fleet.sh, the pull step copies them into data/<machine>/<exercise>/ in your repo)"
  if [ "${HYB:-0}" = 1 ] && [ -z "$(ls -d results/$(hostname)_${OTHER}_hp${HP}_* 2>/dev/null)" ]; then
    echo "REMINDER: hybrid CPU. Still to do: the $OTHER run."
  fi
  if [ -f "/var/tmp/freq_state_$(hostname).txt" ]; then
    echo "WARNING: CPU frequency settings are still modified. Restore the originals: sudo $COMMON/freq_ctl.sh restore"
  fi
}
