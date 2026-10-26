# ECE 492/592 HW3: microarchitecture reverse engineering (Track B)

Selected exercises (100 points): **1** pipelined, **2** out-of-order, **3** branch prediction, **4** ROB size (30),
**8** TLB miss cost, **10** BTB capacity (20), **15** pipeline depth. Spare: 14 fetch width.
Every exercise runs natively on at least two machines **and** inside an SGX enclave where available.
Findings are established from **timing**; performance counters are only a labeled double-check.

> Keep this repository **private**: it contains homework code and measurements.

## Status

| Exercise | Pts | Native code | Enclave code | Data collected |
|---|---|---|---|---|
| ex01 pipelined | 10 | scaffold | scaffold | none |
| ex02 ooo | 10 | scaffold | scaffold | none |
| ex03 branch prediction | 10 | scaffold | scaffold | none |
| ex04 ROB | 30 | **implemented** (tested only in a sandbox VM) | scaffold | none |
| ex08 TLB miss cost | 10 | scaffold | scaffold | none |
| ex10 BTB capacity | 20 | scaffold | scaffold | none |
| ex15 pipeline depth | 10 | scaffold (analysis of ex03 data) | via ex03 enclave | none |

"Scaffold" = folder with README (planned method), `run.sh` stub and Makefile stub, ready for code. Update this table as you go.

## Repository layout

```
hw3/
├── common/
│   ├── include/
│   │   └── bench_common.h
│   ├── freq_ctl.sh
│   └── gather_info.sh
├── data/
│   ├── m1/
│   │   └── machine_card.md
│   ├── m2/
│   │   └── machine_card.md
│   ├── m3/
│   │   └── machine_card.md
│   ├── m4/
│   │   └── machine_card.md
│   ├── m5/
│   │   └── machine_card.md
│   ├── m6/
│   │   └── machine_card.md
│   ├── m7/
│   │   └── machine_card.md
│   └── m8/
│       └── machine_card.md
├── docs/
│   ├── enclave_setup.md
│   ├── machines.md
│   └── requirements.md
├── exercises/
│   ├── ex01_pipelined/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex01_pipelined_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex02_ooo/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex02_ooo_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex03_branch_prediction/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex03_branch_prediction_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex04_rob/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   ├── analyze_rob.py
│   │   ├── perf_check.sh
│   │   ├── rob_test.c
│   │   ├── run.sh
│   │   └── run_rob.sh
│   ├── ex04_rob_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex08_tlb_miss_cost/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex08_tlb_miss_cost_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex10_btb_capacity/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex10_btb_capacity_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex14_fetch_width/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   └── ex15_pipeline_depth/
│       ├── .fleetignore
│       ├── Makefile
│       ├── README.md
│       └── run.sh
├── report/
│   ├── figures/
│   │   └── .gitkeep
│   ├── tables/
│   │   └── .gitkeep
│   ├── README.md
│   └── report.md
├── scripts/
│   ├── fleet.sh
│   ├── new_exercise.sh
│   ├── package.sh
│   └── summarize.py
├── .gitattributes
├── .gitignore
├── README.md
├── ai_log.md
└── machines.example.txt
```

- `exercises/exNN_*`: one self-contained folder per exercise; `*_sgx` is the enclave variant.
- `common/`: shared scripts (`gather_info.sh`, `freq_ctl.sh`) and C helpers (`include/bench_common.h`); deployed next to the exercises.
- `scripts/`: `fleet.sh` (run on many machines), `summarize.py` (tables and plots), `package.sh` (submission ZIP), `new_exercise.sh` (scaffold).
- `data/<machine>/<exercise>/<run>/`: raw results pulled from machines, plus one `machine_card.md` per machine. Committed (part of the submission).
- `docs/`: `requirements.md` (checklist), `machines.md` (inventory table), `enclave_setup.md`.
- `report/`: report skeleton, generated tables and figures.

## Quick start

```bash
git clone <your private repo> && cd hw3
cp machines.example.txt machines.txt      # then edit: ssh targets, CPUs to test, sgx yes/no (git-ignored)
bash scripts/fleet.sh status              # check machines are idle; see hybrid/SGX
bash scripts/fleet.sh all ex04_rob        # deploy + build + run + pull on every machine
python3 scripts/summarize.py ex04_rob     # -> report/tables, report/figures
```

More options (`RUN_ENV`, `PARALLEL`, machine filters, `TRANSFER=tar`) are documented at the top of `scripts/fleet.sh`.
Repeat each configuration at least 3 times; every run gets its own timestamped folder, so nothing is overwritten.
Details for the finished experiment: `exercises/ex04_rob/README.md`.

## Rules that shape the workflow

1. **Counters are never the main evidence.** Establish each result from timing first; use `perf` afterwards, as a clearly labeled confirmation (see `exercises/ex04_rob/perf_check.sh`, which refuses to run before a confirmed timing result exists).
2. **Fixing CPU frequency is allowed only with coordination and must be restored** (`common/freq_ctl.sh save|fix|restore`; not automated).
3. **Hybrid CPUs: test both P-cores and E-cores** (`machines.txt` runs column: `pcore:0,ecore:20`).
4. Keep measured findings separate from published specifications. Cache and prefetcher reverse engineering is out of scope.

## Adding or changing an exercise

```bash
bash scripts/new_exercise.sh ex12_something "Title" 20 --sgx   # scaffold native + enclave folders
```
Then follow the contract in the generated README. Update `docs/requirements.md` as items are completed.

## Submission

```bash
bash scripts/package.sh        # -> hw3_submission.zip (report PDF, sources, raw data, scripts, instructions, AI log)
```
Put the final `report.pdf` in `report/` and fill in `ai_log.md` first. The real `machines.txt` is not packaged.
