# ECE 492/592 HW3: microarchitecture reverse engineering (Track B)

Selected exercises (100 points): **1** pipelined, **2** out-of-order, **3** branch prediction, **4** ROB size (30),
**8** TLB miss cost, **10** BTB capacity (20), **15** pipeline depth. Spare: 14 fetch width.
Every exercise runs natively on at least two machines **and** inside an SGX enclave where available.
Findings are established from **timing**; performance counters are only a labeled double-check.

> Keep this repository **private**: it contains homework code and measurements.

**Start with [`docs/FULL_GUIDE.md`](docs/FULL_GUIDE.md)**: the complete walkthrough (commands, expected output, report, packaging) for every exercise. Run `bash scripts/verify_repo.sh` after copying new files.

## Status

| Exercise | Pts | Native code | Enclave code | Data collected |
|---|---|---|---|---|
| ex01 pipelined | 10 | **implemented** (x86-64, sandbox-tested) | scaffold | none |
| ex02 ooo | 10 | **implemented** (x86-64, sandbox-tested) | scaffold | none |
| ex03 branch prediction | 10 | **implemented** (x86-64, sandbox-tested) | scaffold | none |
| ex04 ROB | 30 | **implemented**, x86-64 and AArch64 (x86 run on real machines; AArch64 only emulator-tested) | scaffold | first runs collected |
| ex08 TLB miss cost | 10 | **implemented** (x86-64, sandbox-tested) | scaffold | none |
| ex10 BTB capacity | 20 | **implemented** (x86-64, sandbox-tested) | scaffold | none |
| ex15 pipeline depth | 10 | **implemented** (analysis of ex03 data) | via ex03 enclave | none |

"Scaffold" = folder with README, `run.sh` stub and Makefile stub, no code (the enclave variants: nothing written or tested). "Sandbox-tested" = run only in a virtual machine here, not yet on your machines. Update this table as you go.

## Repository layout

```
hw3/
├── common/
│   ├── include/
│   │   └── bench_common.h
│   ├── analysis_util.py
│   ├── cpu_model.sh
│   ├── freq_ctl.sh
│   ├── gather_info.sh
│   └── run_lib.sh
├── data/
│   ├── artemisia/
│   │   └── machine_card.md
│   ├── charnwood/
│   │   └── machine_card.md
│   ├── crux/
│   │   └── machine_card.md
│   ├── ookay/
│   │   └── machine_card.md
│   ├── skylark/
│   │   └── machine_card.md
│   ├── sunbird/
│   │   └── machine_card.md
│   ├── thunderbird/
│   │   └── machine_card.md
│   └── upgrade/
│       └── machine_card.md
├── docs/
│   ├── FULL_GUIDE.md
│   ├── enclave_setup.md
│   ├── machines.md
│   ├── requirements.md
│   └── runbook.md
├── exercises/
│   ├── ex01_pipelined/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   ├── analyze.py
│   │   ├── pipe_test.c
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
│   │   ├── analyze.py
│   │   ├── ooo_test.c
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
│   │   ├── analyze.py
│   │   ├── branch_test.c
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
│   │   ├── analyze.py
│   │   ├── run.sh
│   │   └── tlb_test.c
│   ├── ex08_tlb_miss_cost_sgx/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   └── run.sh
│   ├── ex10_btb_capacity/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── README.md
│   │   ├── analyze.py
│   │   ├── btb_test.c
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
│       ├── analyze_ex15.py
│       └── run.sh
├── report/
│   ├── figures/
│   │   └── .gitkeep
│   ├── tables/
│   │   └── .gitkeep
│   ├── README.md
│   └── report.md
├── scripts/
│   ├── collect.py
│   ├── fleet.sh
│   ├── new_exercise.sh
│   ├── package.sh
│   ├── summarize.py
│   └── verify_repo.sh
├── .gitattributes
├── .gitignore
├── README.md
├── ai_log.md
└── machines.example.txt
```

- `exercises/exNN_*`: one self-contained folder per exercise; `*_sgx` is the enclave variant.
- `common/`: shared scripts (`gather_info.sh`, `freq_ctl.sh`) and C helpers (`include/bench_common.h`); deployed next to the exercises.
- `scripts/`: `fleet.sh` (run on many machines), `summarize.py` (tables and plots), `package.sh` (submission ZIP), `new_exercise.sh` (scaffold).
- `data/<machine>/<exercise>/<run>/`: raw results pulled from machines, plus one `machine_card.md` per machine (pre-filled for the 8 course machines). Committed (part of the submission).
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
