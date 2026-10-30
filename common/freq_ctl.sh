#!/usr/bin/env bash
# Save / fix / restore CPU frequency settings (DVFS). Course rule: coordinate first, and ALWAYS restore the original settings afterward.
# Needs root and PERMISSION from whoever runs the machine. Fixing the frequency is allowed but not required.
#   ./freq_ctl.sh status
#   sudo ./freq_ctl.sh save           # snapshot ORIGINAL settings (refuses to overwrite an existing snapshot)
#   sudo ./freq_ctl.sh fix [KHZ]      # performance governor, turbo off, optionally min=max=KHZ
#   sudo ./freq_ctl.sh restore        # put everything back, then verify with 'status'
# Hybrid CPUs: P- and E-cores have different frequency ranges; pass a KHZ that every CPU supports, or omit it.
set -uo pipefail
STATE=/var/tmp/freq_state_$(hostname).txt
SYS=/sys/devices/system/cpu
NT=$SYS/intel_pstate/no_turbo
BOOST=$SYS/cpufreq/boost

status() {
  for c in $SYS/cpu[0-9]*; do
    [ -d $c/cpufreq ] || continue
    printf "%s gov=%s min=%s max=%s cur=%s\n" "$(basename $c)" "$(cat $c/cpufreq/scaling_governor)" \
      "$(cat $c/cpufreq/scaling_min_freq)" "$(cat $c/cpufreq/scaling_max_freq)" "$(cat $c/cpufreq/scaling_cur_freq 2>/dev/null)"
  done
  [ -f $NT ] && echo "intel_pstate no_turbo=$(cat $NT)"
  [ -f $BOOST ] && echo "cpufreq boost=$(cat $BOOST)"
  return 0
}

need_root() { [ "$(id -u)" = 0 ] || { echo "run with sudo"; exit 1; }; [ -d $SYS/cpu0/cpufreq ] || { echo "no cpufreq interface (VM?). Nothing to do; document this."; exit 1; }; }

save() {
  need_root
  [ -f "$STATE" ] && { echo "snapshot $STATE already exists (original settings). Restore first."; exit 1; }
  who | sed 's/^/logged in: /'; echo "^ check nobody else needs the machine before changing frequency"
  for c in $SYS/cpu[0-9]*; do
    [ -d $c/cpufreq ] || continue
    echo "cpu $(basename $c) $(cat $c/cpufreq/scaling_governor) $(cat $c/cpufreq/scaling_min_freq) $(cat $c/cpufreq/scaling_max_freq)"
  done > "$STATE"
  [ -f $NT ] && echo "no_turbo $(cat $NT)" >> "$STATE"
  [ -f $BOOST ] && echo "boost $(cat $BOOST)" >> "$STATE"
  echo "saved original settings to $STATE"
}

fix() {
  need_root; [ -f "$STATE" ] || save
  KHZ=${1:-}
  for c in $SYS/cpu[0-9]*; do
    [ -d $c/cpufreq ] || continue
    echo performance > $c/cpufreq/scaling_governor
    if [ -n "$KHZ" ]; then                       # order matters so min<=max always holds
      cat $c/cpufreq/cpuinfo_min_freq > $c/cpufreq/scaling_min_freq
      echo "$KHZ" > $c/cpufreq/scaling_max_freq
      echo "$KHZ" > $c/cpufreq/scaling_min_freq
    fi
  done
  [ -f $NT ] && echo 1 > $NT
  [ -f $BOOST ] && echo 0 > $BOOST
  echo "frequency fixed. REMEMBER: sudo ./freq_ctl.sh restore"; status
}

restore() {
  need_root; [ -f "$STATE" ] || { echo "no snapshot at $STATE"; exit 1; }
  while read -r kind a b c d; do
    case $kind in
      cpu) p=$SYS/$a/cpufreq
           cat $p/cpuinfo_min_freq > $p/scaling_min_freq
           echo "$d" > $p/scaling_max_freq
           echo "$c" > $p/scaling_min_freq
           echo "$b" > $p/scaling_governor ;;
      no_turbo) echo "$a" > $NT ;;
      boost) echo "$a" > $BOOST ;;
    esac
  done < "$STATE"
  mv "$STATE" "$STATE.restored"; echo "restored. current state:"; status
}

case ${1:-status} in status) status;; save) save;; fix) fix "${2:-}";; restore) restore;; *) sed -n 2,8p "$0";; esac
