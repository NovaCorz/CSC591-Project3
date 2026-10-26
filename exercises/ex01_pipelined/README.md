# ex01_pipelined: Is the processor pipelined? (10 points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
Time a long chain of **dependent** integer adds (each needs the previous result, so time/instruction = ALU latency) against several chains of **independent** adds (throughput-bound). Use inline asm so the compiler cannot reorder or remove the work, and pin to one core.

## What to measure / expected shape
Independent adds complete at a rate well above 1 per latency (several per cycle on modern cores), while the dependent chain runs at ~1 per cycle. A ratio clearly above 1 shows overlapped execution of instructions (pipelining / superscalar issue). Report cycles per instruction for both and the ratio on each machine.

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
