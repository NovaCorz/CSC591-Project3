# Requirements checklist (Track B)

Tick items as they are completed. "Where" points to the file or folder that satisfies each item.

## Selection (>= 100 points)
| Exercise | Points | Native folder | Enclave folder |
|---|---|---|---|
| 1 Pipelined? | 10 | `exercises/ex01_pipelined` | `..._sgx` |
| 2 Out-of-order? | 10 | `exercises/ex02_ooo` | `..._sgx` |
| 3 Branch prediction? | 10 | `exercises/ex03_branch_prediction` | `..._sgx` |
| 4 ROB size | 30 | `exercises/ex04_rob` | `..._sgx` |
| 8 TLB miss cost | 10 | `exercises/ex08_tlb_miss_cost` | `..._sgx` |
| 10 BTB capacity | 20 | `exercises/ex10_btb_capacity` | `..._sgx` |
| 15 Pipeline depth | 10 | `exercises/ex15_pipeline_depth` (analysis of ex03 data) | via ex03 enclave data |
| **Total** | **100** | | |
| 14 Fetch width (spare, 10) | | `exercises/ex14_fetch_width` | only if needed |

- [ ] Selected questions and their total are listed on the **first page** of the report (`report/report.md`).

## Per-experiment requirements
- [ ] Each selected exercise run on **two course machines**, results presented side by side with an explanation of differences.
- [ ] Brief method per exercise: what is tested, how it works, what is measured, how the results support the conclusion.
- [ ] A code snippet per experiment in the report.
- [ ] Repeated measurements, plots, short explanations (>= 3 runs per configuration; `scripts/summarize.py` counts them).
- [ ] Experimental findings kept separate from published specifications.
- [ ] Cache and prefetcher reverse engineering (including DMPs) is excluded: do not include it.

## Enclave comparison
- [ ] Every selected experiment run inside and outside an enclave on the same enclave-capable machine, with differences and challenges explained.
- [ ] Enclave platform and configuration identified (`docs/enclave_setup.md`, machine cards).
- [ ] Where enclave support/access is unavailable: limitation reported.

## Machine identification
- [ ] CPU model and microarchitecture of each tested machine (`docs/machines.md`, `data/m*/machine_card.md`).
- [ ] Unsupported features and access limitations documented.

## Instructor clarifications
- [ ] **Counters are never the main evidence.** Findings are established from timing first; counters (if used) come afterwards in a labeled "confirmation" subsection (`perf_check.sh`-style gating).
- [ ] **Frequency fixing**, if used: coordinated with the machine's owner, original settings restored, stated in the report (`common/freq_ctl.sh`).
- [ ] **Hybrid machines**, if chosen: both P-cores and E-cores tested and compared.

## Submission ZIP (`bash scripts/package.sh`)
- [ ] PDF report in `report/`
- [ ] source code (`exercises/`, `common/`, `scripts/`)
- [ ] raw data (`data/`)
- [ ] plotting scripts (`scripts/summarize.py` and any per-exercise plotting scripts)
- [ ] reproduction instructions (`README.md` + per-exercise READMEs)
- [ ] AI-use log (`ai_log.md`) or the statement that no AI was used

## Optional bonus (both tracks)
- [ ] Compare one reverse-engineering result (e.g. ROB size) across >= 8 course machines, propose a trend/law, support it with measurements, and state its scope and exceptions.
