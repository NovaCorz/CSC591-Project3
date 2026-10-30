# ex01_pipelined: Is the processor pipelined? (10 points)

Status: implemented for x86-64 (tested in a sandbox VM; run it on your machines). Not ported to AArch64 (skip the Arm machine).
Full walkthrough with expected output: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

## Method
Throughput of K independent chains (K=1..12) vs one dependent chain, for `add` (register source), `add $imm` and `imul`. A pipelined multiplier shows ~3x speedup saturating at K=3 (latency 3, one issue per cycle). Ratios cancel the clock, so no frequency assumption or counters are needed.

## Files
`pipe_test.c` (measurement, inline asm) | `analyze.py` (analysis + plot) | `run.sh` (entry point used by `scripts/fleet.sh`) | `Makefile`

## Run
```bash
bash scripts/fleet.sh all ex01_pipelined                 # deploy + run + pull on every machine in machines.txt
python3 scripts/collect.py ex01_pipelined                # table over machines/runs -> report/tables/
```
Repeat at least 3 times per machine. Env knobs: `CORE`, `LABEL`, `REPS` (see run.sh). Results per run: `data.csv`, `summary.txt`, `plot.png`, `sysinfo/`.

## Result keys (summary.txt, `KEY=value`)
PIPELINED, IMUL_SPEEDUP_MAX, IMUL_SATURATION_K, IMUL_LATENCY_CYCLES, ADD_PEAK_IPC, ADD_IMM_FOLD_FACTOR, CORE_GHZ

## Caveat
`add $imm` chains are folded at rename on some newer Intel cores (several per cycle): that is why the clock estimate uses a register-source add chain.

## Evidence rules (course)
Primary evidence is timing only; hardware counters, if used at all, only as a labeled double-check afterwards.
