#!/usr/bin/env bash
# Collects everything the report needs to identify a machine. Usage: ./gather_info.sh <outdir>
OUT=${1:-sysinfo}
mkdir -p "$OUT"
run() { local name=$1; shift; { echo "\$ $*"; "$@" 2>&1; } > "$OUT/$name.txt"; }

run hostname      hostname
run uname         uname -a
run os_release    cat /etc/os-release
run lscpu         lscpu
run lscpu_caches  lscpu -C
run cpuinfo_core0 bash -c "awk -v RS= 'NR==1' /proc/cpuinfo"        # model name, family/model/stepping, flags
run cpu_model     bash -c "grep -m1 'model name' /proc/cpuinfo"
run smt_active    cat /sys/devices/system/cpu/smt/active           # 1 = SMT/Hyper-Threading on
run governor      bash -c "cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | sort | uniq -c"
run thp           cat /sys/kernel/mm/transparent_hugepage/enabled
run meminfo       bash -c "grep -E 'MemTotal|HugePages|Hugepagesize' /proc/meminfo"
run numa          bash -c "lscpu | grep -i numa"
run tsc_flags     bash -c "grep -m1 flags /proc/cpuinfo | tr ' ' '\n' | grep -E 'tsc|constant|nonstop' | tr '\n' ' '"
run sgx_flags     bash -c "grep -m1 flags /proc/cpuinfo | tr ' ' '\n' | grep -i sgx | tr '\n' ' '; echo; ls -l /dev/sgx* /dev/isgx 2>&1"
run gcc_version   gcc --version
command -v cpuid >/dev/null && run cpuid_leaf1 bash -c "cpuid -1 -l 1 -r; cpuid -1 | grep -iE 'family|model|stepping|synth|(uarch)'"
command -v lstopo-no-graphics >/dev/null && run topology lstopo-no-graphics

run hybrid_cores  bash -c 'echo "P-cores: $(cat /sys/devices/cpu_core/cpus 2>/dev/null || echo none)"; echo "E-cores: $(cat /sys/devices/cpu_atom/cpus 2>/dev/null || echo none)"'
run smt_siblings  bash -c 'for c in /sys/devices/system/cpu/cpu[0-9]*; do echo "$(basename $c): $(cat $c/topology/thread_siblings_list)"; done'
run cpu_max_mhz   lscpu -e=CPU,CORE,MAXMHZ,MINMHZ
run freq_state    "$(dirname "$0")/freq_ctl.sh" status
run perf_avail    bash -c 'command -v perf || echo "perf not installed"; cat /proc/sys/kernel/perf_event_paranoid'

echo "wrote $OUT/"
