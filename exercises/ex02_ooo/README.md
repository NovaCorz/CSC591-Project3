# ex02_ooo: Is execution out of order? (10 points)

Status: implemented for x86-64 (tested in a sandbox VM; run it on your machines). Not ported to AArch64 (skip the Arm machine).
Full walkthrough with expected output: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

## Method
A cache-missing load followed by F filler adds: filler that does NOT depend on the load is executed under the miss (flat time), identical filler that DEPENDS on the load adds ~1 cycle each (linear). The next load's address depends on the filler chain end, so the chain is on the critical path in both variants.

## Files
`ooo_test.c` (measurement, inline asm) | `analyze.py` (analysis + plot) | `run.sh` (entry point used by `scripts/fleet.sh`) | `Makefile`

## Run
```bash
bash scripts/fleet.sh all ex02_ooo                 # deploy + run + pull on every machine in machines.txt
python3 scripts/collect.py ex02_ooo                # table over machines/runs -> report/tables/
```
Repeat at least 3 times per machine. Env knobs: `CORE`, `LABEL`, `REPS` (see run.sh). Results per run: `data.csv`, `summary.txt`, `plot.png`, `sysinfo/`.

## Result keys (summary.txt, `KEY=value`)
OUT_OF_ORDER, MISS_LATENCY_NS, DEP_COST_NS, INDEP_COST_NS, DEP_CYCLES_PER_FILLER, INDEP_RISE_F

## Caveat
INDEP_RISE_F (where the independent curve starts to rise) need not equal the ROB size from ex04: other structures can limit the window first. Report it as a window indicator, not as the ROB size.

## Evidence rules (course)
Primary evidence is timing only; hardware counters, if used at all, only as a labeled double-check afterwards.
