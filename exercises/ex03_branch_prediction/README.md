# ex03_branch_prediction: Does the processor use branch prediction? (10 points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
Run the same branchy loop over (a) sorted or constant/repeating data and (b) random data of the same size. Keep the work per iteration identical, avoid branchless compiler transformations (check the asm), and generate data outside the timed region. This experiment's data also feeds ex15.

## What to measure / expected shape
Random data is slower by roughly (misprediction rate x penalty) per branch; predictable data runs at full speed. A large gap is the evidence of prediction. Save per-iteration times and the exact number of unpredictable branches (known from the data, not from counters).

(Expectations are hypotheses to check against your data, not results to quote.)

## Evidence rules (course)
- Primary evidence is **timing only**. Hardware counters may be used afterwards, clearly labeled as a double-check.
- Hybrid CPUs: test both P-cores and E-cores. Fixed frequency is allowed only with coordination, and must be restored.
- Separate measured findings from published specifications in the report.

## Contract with scripts/fleet.sh
`run.sh` reads env `CORE` and `LABEL` (plus optional tuning variables), pins to `CORE`, and writes
`results/<host>_<label>_hp<HP>_<runid>/` containing `run_config.txt` (`core= label= hp= run=`), the raw CSV(s),
summaries/plots, and `sysinfo/` (call `../common/gather_info.sh`). Use `../common/include/bench_common.h` for timing helpers.
See `exercises/ex04_rob/run_rob.sh` for a complete example.

## Results log
| machine | CPU / microarchitecture | label | result | notes |
|---|---|---|---|---|
