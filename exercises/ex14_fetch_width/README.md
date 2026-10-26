# ex14_fetch_width: SPARE: instructions fetched per cycle (10 points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
Spare exercise, only if one of the others fails. Sweep independent 1-byte/multi-byte NOPs or adds and find the maximum sustained rate. Careful: the result may reflect decode, rename or retire width rather than fetch.

## What to measure / expected shape
A plateau at the narrowest stage's width; discuss which stage limits it.

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
