# ex15_pipeline_depth: Pipeline depth from misprediction penalty (10 points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
Analysis only: reuse the ex03 data (native and enclave). Misprediction penalty in cycles = (T_random - T_predictable) / number_of_mispredictions, with the misprediction count known from the data pattern (not from counters). The penalty approximates the front-end-to-resolution pipeline length. Its run.sh should read results of ex03 and write its own results dir.

## What to measure / expected shape
Typically in the range of roughly 15-20 cycles on recent Intel/AMD cores (verify on your machines). Explain what the number reveals and what it does not (it is not exactly the full pipeline depth).

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
