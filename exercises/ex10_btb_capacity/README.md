# ex10_btb_capacity: BTB capacity (20 points)

Status: implemented for x86-64 (tested in a sandbox VM; run it on your machines). Not ported to AArch64 (skip the Arm machine).
Full walkthrough with expected output: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

## Method
Run-time generated chain of N taken jumps (mmap + mprotect), spacings S=4,8,16,32,64 bytes. Cycles per taken branch vs N shows a staircase: ~1 cycle (fast level), ~2 cycles (next level), then ~8-10 cycles once the BTB is exceeded. A big step at the same N for several spacings points at capacity.

## Files
`btb_test.c` (measurement, inline asm) | `analyze.py` (analysis + plot) | `run.sh` (entry point used by `scripts/fleet.sh`) | `Makefile`

## Run
```bash
bash scripts/fleet.sh all ex10_btb_capacity                 # deploy + run + pull on every machine in machines.txt
python3 scripts/collect.py ex10_btb_capacity                # table over machines/runs -> report/tables/
```
Repeat at least 3 times per machine. Env knobs: `CORE`, `LABEL`, `REPS` (see run.sh). Results per run: `data.csv`, `summary.txt`, `plot.png`, `sysinfo/`.

## Result keys (summary.txt, `KEY=value`)
BTB_CAPACITY_EST, BTB_AGREE_ACROSS_S, KNEES_S<S>, LAST_BIG_KNEE_S<S>, BASE_CYCLES_PER_BRANCH, FAR_CYCLES_PER_BRANCH

## Caveat
Needs executable memory (mprotect PROT_EXEC). Large footprints also hit L1I/ITLB limits: judge steps by whether they move with the spacing. Not portable to an SGX enclave as is (no run-time code generation there).

## Evidence rules (course)
Primary evidence is timing only; hardware counters, if used at all, only as a labeled double-check afterwards.
