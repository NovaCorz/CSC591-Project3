#!/usr/bin/env bash
# Checks that your copy of the repo is complete and consistent (run from the repo root):  bash scripts/verify_repo.sh
# Catches the classic problem of a half-updated repo (e.g. new rob_test.c with an old Makefile). Safe to run any time.
cd "$(dirname "$0")/.." || exit 1
bad=0
need() { [ -e "$1" ] || { echo "MISSING  $1"; bad=1; }; }

for f in include/bench_common.h run_lib.sh gather_info.sh freq_ctl.sh cpu_model.sh analysis_util.py; do need common/$f; done
for f in fleet.sh collect.py summarize.py package.sh new_exercise.sh verify_repo.sh; do need scripts/$f; done
declare -A SRC=( [ex01_pipelined]=pipe_test.c [ex02_ooo]=ooo_test.c [ex03_branch_prediction]=branch_test.c
                 [ex04_rob]=rob_test.c [ex08_tlb_miss_cost]=tlb_test.c [ex10_btb_capacity]=btb_test.c )
for ex in "${!SRC[@]}"; do
  for f in "${SRC[$ex]}" Makefile run.sh; do need exercises/$ex/$f; done
done
for ex in ex01_pipelined ex02_ooo ex03_branch_prediction ex08_tlb_miss_cost ex10_btb_capacity; do need exercises/$ex/analyze.py; done
need exercises/ex04_rob/analyze_rob.py; need exercises/ex04_rob/run_rob.sh; need exercises/ex15_pipeline_depth/analyze_ex15.py

# consistency markers: new files must go together
grep -q "aarch64" exercises/ex04_rob/rob_test.c 2>/dev/null       || { echo "OUTDATED exercises/ex04_rob/rob_test.c (no AArch64 kernel): replace it with the current version"; bad=1; }
grep -q "COMMON" exercises/ex04_rob/Makefile 2>/dev/null           || { echo "OUTDATED exercises/ex04_rob/Makefile (does not use common/): replace it"; bad=1; }
grep -q "march=native" exercises/ex04_rob/Makefile 2>/dev/null     && { echo "OUTDATED exercises/ex04_rob/Makefile still has -march=native"; bad=1; }
grep -q "except ImportError" exercises/ex04_rob/analyze_rob.py 2>/dev/null || { echo "OUTDATED exercises/ex04_rob/analyze_rob.py (a missing matplotlib will abort runs)"; bad=1; }
grep -q "SSH_OPTS" scripts/fleet.sh 2>/dev/null                    || { echo "OUTDATED scripts/fleet.sh (no ssh connection sharing)"; bad=1; }

# syntax
for f in scripts/*.sh common/*.sh exercises/*/*.sh; do [ -f "$f" ] && { bash -n "$f" 2>/dev/null || { echo "SYNTAX ERROR  $f"; bad=1; }; }; done
for f in scripts/*.py common/*.py exercises/*/*.py; do [ -f "$f" ] && { python3 -m py_compile "$f" 2>/dev/null || { echo "SYNTAX ERROR  $f"; bad=1; }; }; done
find . -name __pycache__ -type d -prune -exec rm -rf {} + 2>/dev/null

# can this computer build the x86 exercises? (skipped on non-x86)
if [ "$(uname -m)" = x86_64 ]; then
  for ex in ex01_pipelined ex02_ooo ex03_branch_prediction ex04_rob ex08_tlb_miss_cost ex10_btb_capacity; do
    if (cd exercises/$ex && make -s -B >/dev/null 2>&1 && make -s clean >/dev/null 2>&1); then :; else echo "BUILD FAILED  exercises/$ex (run: cd exercises/$ex && make)"; bad=1; fi
  done
else echo "(non-x86 computer: build check skipped)"; fi

[ -f machines.txt ] || echo "NOTE: machines.txt not found (cp machines.example.txt machines.txt and edit it)"
[ $bad = 0 ] && echo "OK: repo is complete and consistent." || { echo "PROBLEMS FOUND (see above). Easiest fix: copy the current exercises/, common/ and scripts/ folders from the latest zip over yours (keep machines.txt and data/)."; exit 1; }
