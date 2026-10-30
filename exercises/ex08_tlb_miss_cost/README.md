# ex08_tlb_miss_cost: TLB miss cost (10 points)

Status: implemented for x86-64 (tested in a sandbox VM; run it on your machines). Not ported to AArch64 (skip the Arm machine).
Full walkthrough with expected output: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

## Method
Pointer chase over P nodes: dense (packed, huge pages) vs sparse (one node per 4 KiB page). Same cache footprint; the difference is the translation cost, vs P. Steps show the L1 DTLB and L2 TLB reach; levels give the L2-TLB-hit cost and the page-walk cost.

## Files
`tlb_test.c` (measurement, inline asm) | `analyze.py` (analysis + plot) | `run.sh` (entry point used by `scripts/fleet.sh`) | `Makefile`

## Run
```bash
bash scripts/fleet.sh all ex08_tlb_miss_cost                 # deploy + run + pull on every machine in machines.txt
python3 scripts/collect.py ex08_tlb_miss_cost                # table over machines/runs -> report/tables/
```
Repeat at least 3 times per machine. Env knobs: `CORE`, `LABEL`, `REPS` (see run.sh). Results per run: `data.csv`, `summary.txt`, `plot.png`, `sysinfo/`.

## Result keys (summary.txt, `KEY=value`)
TLB_L1MISS_NS/CYCLES, TLB_WALK_NS/CYCLES (page tables cached), TLB_WALK_COLD_NS/CYCLES, TLB_STEP1_P (L1 DTLB exceeded), TLB_STEP2_START_P (L2 TLB starts to be exceeded), TLB_STEP2_P (full walk level)

## Caveat
Needs transparent huge pages for the dense layout (`madvise` or `always`); the analyzer warns and marks the run unconfirmed otherwise.

## Evidence rules (course)
Primary evidence is timing only; hardware counters, if used at all, only as a labeled double-check afterwards.
