#!/usr/bin/env bash
# Run an exercise on many machines listed in machines.txt.
#   scripts/fleet.sh status                              # CPU, load, who is logged in, governor, hybrid/SGX
#   scripts/fleet.sh deploy  <exercise> [machine ...]    # copy exercises/<exercise>/ to each machine and build
#   scripts/fleet.sh run     <exercise> [machine ...]    # run it there (one run per LABEL:CORE in machines.txt)
#   scripts/fleet.sh pull    <exercise> [machine ...]    # copy results back to data/<machine>/<exercise>/
#   scripts/fleet.sh all     <exercise> [machine ...]    # deploy + run + pull
# Env:  RUN_ENV="HP=1 REPS=20"   extra variables passed to the exercise's run.sh
#       PARALLEL=1               with 'all': machines run concurrently (logs go to data/<m>/<ex>/fleet_all.log)
#       REMOTE_DIR=CSC591-Project3       working directory on each machine (relative to its home); each machine id
#                                        gets its own subfolder: <REMOTE_DIR>/<id>/{common,<exercise>}
#       MAKE_ARGS="..."          extra arguments for 'make' on each machine, e.g. CFLAGS_EXTRA=-DROB_FILLER_ALU
#       TRANSFER=tar             copy with tar-over-ssh instead of rsync (automatic if rsync is not installed locally)
# Notes: use SSH keys (password prompts break loops). Long runs: start fleet.sh inside tmux/screen.
#        Frequency fixing (freq_ctl.sh) needs sudo and coordination, so it is NOT automated here.
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
REMOTE_DIR=${REMOTE_DIR:-CSC591-Project3}
RUN_ENV=${RUN_ENV:-}
CMD=${1:?usage: fleet.sh status | deploy|run|pull|all <exercise> [machine_id ...]}; shift
EX=""
if [ "$CMD" != status ]; then EX=${1:?give an exercise folder name from exercises/}; shift; fi
FILTER=("$@")
[ -f "$ROOT/machines.txt" ] || { echo "machines.txt missing: cp machines.example.txt machines.txt, then edit it"; exit 1; }
ids_in_file() { grep -vE '^\s*(#|$)' "$ROOT/machines.txt" | sed 's/#.*//' | awk 'NF{print $1}'; }
for f in ${FILTER[@]+"${FILTER[@]}"}; do
  ids_in_file | grep -qx -- "$f" || { echo "unknown machine id '$f'. Ids in machines.txt: $(ids_in_file | tr '\n' ' ')"; exit 1; }
done
[ -z "$EX" ] || [ -d "$ROOT/exercises/$EX" ] || { echo "no such exercise: exercises/$EX"; exit 1; }

if [ ${#FILTER[@]} -gt 0 ]; then
  KNOWN=$(grep -vE '^\s*(#|$)' "$ROOT/machines.txt" | sed 's/#.*//' | awk 'NF{print $1}')
  for f in "${FILTER[@]}"; do
    echo "$KNOWN" | grep -qx "$f" || { echo "unknown machine id '$f'. Ids in machines.txt: $(echo $KNOWN)"; exit 1; }
  done
fi

# Reuse ONE ssh connection for all commands to a machine (a password is typed once, not once per command).
# Disable with SSH_MUX=0. The shared connection stays open 15 minutes; close it early with:  ssh -O exit <target>
SSH_MUX=${SSH_MUX:-1}; SSH_OPTS=()
if [ "$SSH_MUX" = 1 ]; then
  mkdir -p "$HOME/.ssh" && chmod 700 "$HOME/.ssh"
  SSH_OPTS=(-o ControlMaster=auto -o "ControlPath=$HOME/.ssh/fleet-%C" -o ControlPersist=15m)
fi
RSYNC_SSH="ssh ${SSH_OPTS[*]:-}"

rdir()  { if [ "$1" = local ] && [[ "$REMOTE_DIR" != /* ]]; then echo "$HOME/$REMOTE_DIR"; else echo "$REMOTE_DIR"; fi; }
rpath() { if [ "$1" = local ]; then echo "$2"; else echo "$1:$2"; fi; }
rsh()   { local t=$1; shift; if [ "$t" = local ]; then bash -c "$*"; else ssh -n ${SSH_OPTS[@]+"${SSH_OPTS[@]}"} "$t" "$*"; fi; }

rsh_in() { local t=$1; shift; if [ "$t" = local ]; then bash -c "$*"; else ssh ${SSH_OPTS[@]+"${SSH_OPTS[@]}"} "$t" "$*"; fi; }   # keeps stdin (for tar pipes)
TRANSFER=${TRANSFER:-$(command -v rsync >/dev/null && echo rsync || echo tar)}   # TRANSFER=tar forces tar-over-ssh (no rsync needed)

push_dir() {   # push_dir <target> <src dir> <dest dir on target> <ignore file or empty>
  local t=$1 src=$2 dst=$3 ign=${4:-} rc exf=() tx=()
  if [ "$TRANSFER" = rsync ]; then
    [ -n "$ign" ] && exf=(--exclude-from="$ign")
    rsh "$t" "mkdir -p $dst" || return 1        # rsync only creates the last path component, so make parents first
    rsync -az -e "$RSYNC_SSH" --delete ${exf[@]+"${exf[@]}"} "$src/" "$(rpath "$t" "$dst/")"
  else
    if [ -n "$ign" ]; then sed 's:/$::' "$ign" > "/tmp/fleet_ign.$$"; tx=(--exclude-from="/tmp/fleet_ign.$$"); fi
    tar -C "$src" ${tx[@]+"${tx[@]}"} -cf - . | rsh_in "$t" "mkdir -p $dst && tar -C $dst -xf -"
  fi
  rc=$?; rm -f "/tmp/fleet_ign.$$"; return $rc
}

pull_dir() {   # pull_dir <target> <remote dir> <local dir>
  local t=$1 src=$2 dst=$3
  if [ "$TRANSFER" = rsync ]; then rsync -az -e "$RSYNC_SSH" --exclude='*_latest' "$(rpath "$t" "$src/")" "$dst/"
  else rsh "$t" "tar -C $src --exclude='*_latest' -cf - ." | tar -C "$dst" -xf -; fi
}

selected() {   # prints "id target runs sgx" for the machines to use
  grep -vE '^\s*(#|$)' "$ROOT/machines.txt" | sed 's/#.*//' | while read -r id target runs sgx _; do
    [ -n "${id:-}" ] || continue
    if [ ${#FILTER[@]} -gt 0 ]; then [[ " ${FILTER[*]} " == *" $id "* ]] || continue; fi
    if [[ "$EX" == *_sgx && "${sgx:-no}" != yes ]]; then echo "skip $id: sgx=${sgx:-no} (set yes in machines.txt once verified)" >&2; continue; fi
    echo "$id $target $runs ${sgx:-no}"
  done
}

do_deploy() {
  local id=$1 t=$2 d; d=$(rdir "$t")/$id      # per-machine folder: safe even if the home directory is shared between machines
  local ign=""; [ -f "$ROOT/exercises/$EX/.fleetignore" ] && ign="$ROOT/exercises/$EX/.fleetignore"
  echo "[$id] deploy $EX (transfer: $TRANSFER)"
  push_dir "$t" "$ROOT/common" "$d/common" "" || return 1          # shared scripts/headers
  push_dir "$t" "$ROOT/exercises/$EX" "$d/$EX" "$ign" || return 1
  rsh "$t" "cd $d/$EX && chmod +x *.sh *.py 2>/dev/null; if [ -f Makefile ]; then make -B ${MAKE_ARGS:-} 2>&1 | tail -3; fi"
}

do_run() {
  local id=$1 t=$2 runs=$3 d; d=$(rdir "$t")/$id
  local out="$ROOT/data/$id/$EX"; mkdir -p "$out"
  local log="$out/fleet_run_$(date +%Y%m%d-%H%M%S).log" specs spec
  IFS=, read -ra specs <<< "$runs"
  for spec in "${specs[@]}"; do
    local label=${spec%%:*} core=${spec##*:}
    echo "[$id] run $EX label=$label core=$core ${RUN_ENV}" | tee -a "$log"
    rsh "$t" "cd $d/$EX && env CORE=$core LABEL=$label $RUN_ENV ./run.sh" 2>&1 | tee -a "$log"
  done
}

do_pull() {
  local id=$1 t=$2 d; d=$(rdir "$t")/$id
  mkdir -p "$ROOT/data/$id/$EX"
  echo "[$id] pull results"
  pull_dir "$t" "$d/$EX/results" "$ROOT/data/$id/$EX" || return 1
  local n; n=$(find "$ROOT/data/$id/$EX" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
  echo "[$id] results copied to: $ROOT/data/$id/$EX/  ($n run folder(s) there now)"
}

one() {
  case $CMD in
    deploy) do_deploy "$@" ;;
    run)    do_run "$@" ;;
    pull)   do_pull "$@" ;;
    all)    do_deploy "$@" && do_run "$@" && do_pull "$@" ;;
    *)      echo "unknown command $CMD"; exit 1 ;;
  esac
}

if [ "$CMD" = status ]; then
  while read -r id t runs sgx; do
    echo "== $id ($t)"
    rsh "$t" 'hostname; uptime; echo "users logged in: $(who | wc -l)"; lscpu | grep -m1 "Model name"; echo "arch: $(uname -m)";
              echo "governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo n/a)";
              echo "hybrid: $([ -r /sys/devices/cpu_atom/cpus ] && echo yes || echo no)";
              echo "sgx cpu flags: $(grep -m1 -ow -E "sgx|sgx_lc|sgx1|sgx2" /proc/cpuinfo | sort -u | tr "\n" " ")";
              echo "sgx devices: $(ls /dev/sgx_enclave /dev/sgx/enclave /dev/isgx 2>/dev/null | tr "\n" " ")(empty = none)"'
  done < <(selected)
  exit 0
fi

pids=(); NSEL=0
while read -r id t runs sgx; do
  NSEL=$((NSEL+1))
  if [ "${PARALLEL:-0}" = 1 ] && [ "$CMD" = all ]; then
    mkdir -p "$ROOT/data/$id/$EX"
    ( one "$id" "$t" "$runs" ) > "$ROOT/data/$id/$EX/fleet_all.log" 2>&1 &
    pids+=($!); echo "[$id] started in background (log: data/$id/$EX/fleet_all.log)"
  else
    one "$id" "$t" "$runs" || echo "[$id] FAILED ($CMD)"
  fi
done < <(selected)
for p in "${pids[@]:-}"; do [ -n "$p" ] && wait "$p"; done
if [ "$NSEL" -eq 0 ]; then echo "No machines were selected (check machines.txt, the machine ids you passed, and sgx=yes for *_sgx exercises)."; exit 1; fi
echo "done. Next: python3 scripts/summarize.py $EX"
