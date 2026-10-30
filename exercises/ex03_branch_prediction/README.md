# ex03_branch_prediction: Branch prediction and misprediction cost (10 points)

Status: implemented for x86-64 (tested in a sandbox VM; run it on your machines). Not ported to AArch64 (skip the Arm machine).
Full walkthrough with expected output: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

## Method
One data-dependent branch per element (inline asm, no cmov). Random data with p% high values (9 fractions) + sorted + periodic patterns. Linear fit of time vs expected mispredictions gives the penalty; periodic patterns show the predictor learns history. Its output feeds ex15.

## Files
`branch_test.c` (measurement, inline asm) | `analyze.py` (analysis + plot) | `run.sh` (entry point used by `scripts/fleet.sh`) | `Makefile`

## Run
```bash
bash scripts/fleet.sh all ex03_branch_prediction                 # deploy + run + pull on every machine in machines.txt
python3 scripts/collect.py ex03_branch_prediction                # table over machines/runs -> report/tables/
```
Repeat at least 3 times per machine. Env knobs: `CORE`, `LABEL`, `REPS` (see run.sh). Results per run: `data.csv`, `summary.txt`, `plot.png`, `sysinfo/`.

## Result keys (summary.txt, `KEY=value`)
PREDICTION_USED, PENALTY_NS, PENALTY_CYCLES, PENALTY_CYCLES_TWO_POINT, FIT_R2, T_SORTED_NS, T_RAND50_NS, PATTERN_*_COST_REMAINING

## Caveat
The penalty includes the L1 load and compare latency before the branch resolves, so it is an upper bound for the front-end depth.

## Evidence rules (course)
Primary evidence is timing only; hardware counters, if used at all, only as a labeled double-check afterwards.
