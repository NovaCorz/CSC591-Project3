#!/usr/bin/env bash
# Prints the CPU model as "model name : <text>" on x86 and AArch64 (lscpu fallback; Arm has no "model name" in /proc/cpuinfo).
m=$(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | sed 's/^[^:]*:[[:space:]]*//')
[ -n "$m" ] || m=$(lscpu 2>/dev/null | sed -n 's/^Model name:[[:space:]]*//p' | head -1)
[ -n "$m" ] || m=unknown
echo "model name : $m"
echo "architecture : $(uname -m)"
