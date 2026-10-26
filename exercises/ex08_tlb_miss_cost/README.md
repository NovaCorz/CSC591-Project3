# ex08_tlb_miss_cost: Cost of a TLB miss (10 points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
Pointer-chase with one access per page. Compare a working set within the TLB reach to one far beyond it. Offset each page's line so the *data* stays cache-resident (different cache sets) while the *translations* do not, so the difference isolates translation cost. Run with 4 KiB pages (and optionally huge pages).

## What to measure / expected shape
Latency per access steps up once the page count exceeds the TLB entries. The difference between the plateaus is the miss cost: a few cycles for an L2-TLB hit, tens of cycles or more for a page walk (verify on your machines).

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
