# ECE 492/592 HW3: microarchitecture reverse engineering (Track B)

Selected exercises (100 points): **1** pipelined, **2** out-of-order, **3** branch prediction, **4** ROB size (30),
**8** TLB miss cost, **10** BTB capacity (20), **15** pipeline depth. Spare: 14 fetch width.
Every exercise runs natively on at least two machines **and** inside an SGX enclave where available.
Findings are established from **timing**; performance counters are only a labeled double-check.

## Status

| Exercise | Pts | Native code | 
|---|---|---|---|---|
| ex01 pipelined | 10 | **implemented** (x86-64, sandbox-tested) | 
| ex02 ooo | 10 | **implemented** (x86-64, sandbox-tested) | 
| ex03 branch prediction | 10 | **implemented** (x86-64, sandbox-tested) | 
| ex04 ROB | 30 | **implemented**, x86-64 and AArch64 (x86 run on real machines; AArch64 only emulator-tested) |
| ex08 TLB miss cost | 10 | **implemented** (x86-64, sandbox-tested) |
| ex10 BTB capacity | 20 | **implemented** (x86-64, sandbox-tested) |
| ex15 pipeline depth | 10 | **implemented** (analysis of ex03 data) |


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
│   ├── charnwood/
│   ├── crux/
│   ├── ookay/
│   ├── skylark/
│   ├── sunbird/
│   ├── thunderbird/
│   └── upgrade/
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
│   │   ├── analyze.py
│   │   ├── pipe_test.c
│   │   └── run.sh
│   ├── ex02_ooo/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── analyze.py
│   │   ├── ooo_test.c
│   │   └── run.sh
│   ├── ex03_branch_prediction/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── analyze.py
│   │   ├── branch_test.c
│   │   └── run.sh
│   ├── ex04_rob/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── analyze_rob.py
│   │   ├── perf_check.sh
│   │   ├── rob_test.c
│   │   ├── run.sh
│   │   └── run_rob.sh
│   ├── ex08_tlb_miss_cost/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── analyze.py
│   │   ├── run.sh
│   │   └── tlb_test.c
│   ├── ex10_btb_capacity/
│   │   ├── .fleetignore
│   │   ├── Makefile
│   │   ├── analyze.py
│   │   ├── btb_test.c
│   │   └── run.sh
│   └── ex15_pipeline_depth/
│       ├── .fleetignore
│       ├── Makefile
│       ├── README.md
│       ├── analyze_ex15.py
│       └── run.sh
├── report/
│   ├── CSC591_Project3.pdf
│   ├── figures/
│   │   └── .gitkeep
│   ├── tables/
│   │   └── .gitkeep
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
└── machines.example.txt
```

- `exercises/exNN_*`: one self-contained folder per exercise;
- `common/`: shared scripts (`gather_info.sh`, `freq_ctl.sh`) and C helpers (`include/bench_common.h`); deployed next to the exercises.
- `scripts/`: `fleet.sh` (run on many machines), `summarize.py` (tables and plots), `package.sh` (submission ZIP), `new_exercise.sh` (scaffold).
- `data/<machine>/<exercise>/<run>/`: raw results pulled from machines
- `docs/`: `requirements.md` (checklist), `machines.md` (inventory table), `enclave_setup.md`.
- `report/`: generated tables and figures.

## Quick start

```bash
git clone <your private repo> && cd CSC-Project3
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

