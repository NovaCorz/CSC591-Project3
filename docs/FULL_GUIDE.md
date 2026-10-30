# Complete project guide: Homework 3, Track B (microarchitecture reverse engineering)

ECE 492/592, Fall 2026. Due **Tuesday, November 3, 2026, 11:59 PM Eastern**. Your selection (100 points):
ex01 pipelined (10), ex02 out-of-order (10), ex03 branch prediction (10), **ex04 ROB size (30, done)**,
ex08 TLB miss cost (10), ex10 BTB capacity (20), ex15 pipeline depth (10, analysis of ex03 data).

This is the single document to follow from here to the submission. It tells you what to run, what files appear, what the
output should look like, how to tell a good run from a bad one, and what goes into the report.

> **How to read the sample outputs.** I generated every "expected output" block below by actually running the code, all in one
> session on one sandbox virtual machine (an Intel Xeon @ 2.8 GHz running at about 3.3 GHz; the hypervisor hides the exact model, and
> the behavior is consistent with a Skylake-generation core: about 4 adds per cycle, a 64-entry L1 DTLB, a 1536-entry L2 TLB). The sandbox
> was moved between physical hosts while I developed, so a few remarks below refer to an *earlier* host and say so.
> **Your machines will print different numbers.** What should match is the
> *shape* of the data, the file names, the `KEY=value` lines and the `STATUS=ok` line. Where I list numbers you might expect
> for your machines, they are hypotheses taken from published specs: check them against your measurements, never quote them as results.

---

## 0. Status: what is done, what is tested, what is not

| Item | Code | Tested where | Notes |
|---|---|---|---|
| ex04 ROB size | done (x86-64 and AArch64) | **your 7 x86 machines**; AArch64 only in an emulator | you reported it complete |
| ex01 pipelined | done, x86-64 only | sandbox VM only | not run on your machines yet |
| ex02 out-of-order | done, x86-64 only | sandbox VM only | |
| ex03 branch prediction | done, x86-64 only | sandbox VM only | feeds ex15 |
| ex15 pipeline depth | done (analysis script) | sandbox VM only | needs ex03 data |
| ex08 TLB miss cost | done, x86-64 only | sandbox VM only | needs transparent huge pages |
| ex10 BTB capacity | done, x86-64 only | sandbox VM only | needs executable memory |
| **Enclave (SGX) variants** | **not written** | **not tested anywhere** | see section 12: first find out whether any machine can run enclaves |

Everything new is x86-64 only, so **thunderbird (Arm) is skipped for ex01, ex02, ex03, ex08, ex10**. That is fine: the
assignment needs two machines per exercise, and you have seven x86 machines. Thunderbird still takes part in ex04 (and the bonus).

---

## 1. Suggested schedule (6 days left from Wed Oct 28)

| Day | Do |
|---|---|
| Wed 10/28 | Update the repo (section 2). Local smoke tests. Run **ex01, ex02, ex03** on all x86 machines, 3 runs each. |
| Thu 10/29 | Run **ex08, ex10** (3 runs each). Run ex15 analysis. Finish ex04 extras (>= 3 confirmed runs, huge-page variant). |
| Fri 10/30 | **SGX feasibility check** (section 12). Decide: build enclave variants, or document the limitation. |
| Sat 10/31 | Enclave runs (if possible) or limitation text. Fill in machine cards. Generate all tables and figures. |
| Sun 11/1 | Write the report (section 14). |
| Mon 11/2 | Polish, `verify_repo.sh`, package, test-unzip. Leave buffer. |
| Tue 11/3 | Submit well before 11:59 PM. |

---

## 2. One-time setup: update and verify your repo

Your last run mixed old and new files, which caused avoidable failures. Replace the code folders as a set.

```bash
# in WSL; adjust the path to where you saved hw3_repo.zip
unzip -o /mnt/c/Users/<you>/Downloads/hw3_repo.zip -d /tmp/hw3_new

# dry run first: shows what would change (keeps your machines.txt, data/, report/ and ai_log.md)
rsync -avn --exclude machines.txt --exclude data --exclude report --exclude ai_log.md /tmp/hw3_new/hw3/ ~/CSC591/CSC591-Project3/
# if the list looks right, run it for real (remove the n):
rsync -av  --exclude machines.txt --exclude data --exclude report --exclude ai_log.md /tmp/hw3_new/hw3/ ~/CSC591/CSC591-Project3/

cd ~/CSC591/CSC591-Project3
bash scripts/verify_repo.sh
```

Expected last line: `OK: repo is complete and consistent.` The script checks that all files exist, that old and new files match, that
the scripts have no syntax errors, and that every exercise **builds on this computer** (your WSL is x86, so it can). If it prints
`MISSING`, `OUTDATED` or `BUILD FAILED`, fix that first; the message says what to do.

Also: `chmod +x scripts/*.sh common/*.sh exercises/*/*.sh` if the executable bits were lost.

**Define your list of x86 machines once** (thunderbird excluded) and reuse it:

```bash
X86="sunbird charnwood ookay upgrade crux skylark artemisia"
```

---

## 3. The standard loop (identical for every exercise)

```bash
cd ~/CSC591/CSC591-Project3
bash scripts/fleet.sh status                      # 1. are the machines idle? (load near 0, nobody logged in)
bash scripts/fleet.sh all <exercise> $X86         # 2. deploy + build + run + pull, run 1
bash scripts/fleet.sh all <exercise> $X86         #    run 2 (later, ideally at a different time)
bash scripts/fleet.sh all <exercise> $X86         #    run 3
python3 scripts/collect.py <exercise>             # 3. table over machines and runs -> report/tables/
```

What `fleet.sh all` prints per machine (shape, with real names in place of `<...>`):

```
[<machine>] deploy <exercise> (transfer: rsync)
[<machine>] run <exercise> label=cpu2 core=2
== <exercise> on CPU 2 (cpu2) ==
...the analysis printout shown in each exercise section...
PRIMARY results (timing only) are on THIS machine in: /home/<you>/CSC591-Project3/<machine>/<exercise>/results/<run>/
[<machine>] pull results
[<machine>] results copied to: /home/<you>/CSC591/CSC591-Project3/data/<machine>/<exercise>/  (N run folder(s) there now)
```

(The `PRIMARY results ... in:` path is on the **remote** machine; your copy is under `data/<machine>/<exercise>/` in the repo.)

What lands in your repo for each run, `data/<machine>/<exercise>/<host>_<label>_hp0_<runid>/`:

```
data.csv            raw measurements (the source of truth)
summary.txt         the analysis printout, ending with KEY=value lines and STATUS=ok or STATUS=unconfirmed
plot.png            the figure (made on the machine if matplotlib exists there, otherwise rebuilt by collect.py on your computer)
run_config.txt      core=... label=... hp=... run=...
freq_during_run.txt frequency settings at the time of the run
sysinfo/            lscpu, cpu_model, caches, SMT, SGX flags, kernel, gcc, ...
```

**Rules of thumb**
- **At least 3 runs per machine and exercise.** `collect.py` counts a run as *confirmed* only if its summary says `STATUS=ok`, and warns when a machine has fewer than 3.
- **Reading `STATUS=unconfirmed`:** the run completed but the data did not meet the exercise's quality check (noise, missing huge pages, no clear structure). Read the printout, rerun when the machine is idle. Never delete data to hide a bad run; mention excluded runs in the report.
- **Run one exercise at a time per machine**, and check `fleet.sh status` first. Other users on a machine are the main source of bad data.
- **Core choice:** the number after the colon in `machines.txt` is the logical CPU used. Keep the same core for all your repeats of an exercise.
- **Counters** are never the main evidence. Nothing in these exercises uses them. If you want to cross-check ex04 with counters, do it afterwards (`exercises/ex04_rob/perf_check.sh`).
- **Fixing the CPU frequency is optional**, needs permission and a restore (`common/freq_ctl.sh save|fix|restore`). The ratios and knees in these experiments do not depend on it; it only reduces noise.

---

## 4. Which two machines for the side-by-side comparison

The assignment asks for results from **two machines per exercise, side by side**, with an explanation of differences. Run everything on all seven x86
machines (it is cheap), then pick a contrasting pair for the figures, and use the other machines in the tables.

| Pair | Why |
|---|---|
| **sunbird (Haswell, 2014) vs artemisia (Sapphire Rapids, 2023)** | widest generation gap: largest expected differences (ROB 192 vs 512, TLB sizes, predictor) |
| charnwood (Skylake) vs skylark (AMD Zen 2) | same era, different vendor: shows vendor differences |

Use the same two machines for every exercise so the report reads consistently.

---

## 5. Exercise 4 (ROB size): what is left

You reported it complete. Before moving on, confirm each of these:

- [ ] at least **3 confirmed runs** per machine: `python3 scripts/summarize.py ex04_rob` and look at `runs_confirmed`
- [ ] the **fine-sweep** estimate (in each run's `fine_summary.txt`), not only the coarse one, for the numbers you quote
- [ ] huge-page variant for at least your two comparison machines: `RUN_ENV="HP=1" bash scripts/fleet.sh all ex04_rob sunbird artemisia`
- [ ] thunderbird run attempted (the Arm port is untested on real hardware; if it shows no knee, see `exercises/ex04_rob/README.md` section 11 for the ALU-filler variant)
- [ ] published ROB sizes noted separately from your measurements

Your coarse results from the earlier all-machines run (step 4, so about +/-4; use the fine sweep for the final value):

| Machine | CPU (microarchitecture, year) | Knee N | Estimate | Published ROB | Ratio hi/lo |
|---|---|---|---|---|---|
| sunbird | Xeon E5-2680 v3 (Haswell, 2014) | 192 | 194 | 192 | 2.08 |
| charnwood | i7-6700 (Skylake, 2015) | 224 | 226 | 224 | 2.47 |
| ookay | i7-7700 (Kaby Lake, 2017) | 224 | 226 | 224 | 2.35 |
| upgrade | i7-8700 (Coffee Lake, 2017) | 224 | 226 | 224 | 2.34 |
| crux | i7-9700 (Coffee Lake Refresh, 2018) | 224 | 226 | 224 | 2.27 |
| skylark | EPYC 7532 (Zen 2, 2019) | 224 | 226 | 224 | 2.18 |
| artemisia | Xeon Gold 5420+ (Sapphire Rapids, 2023) | 496 | 498 | **512** | 2.11 |
| thunderbird | Ampere Altra (Neoverse N1, 2020) | ? | ? | ~128 (public reviews; verify) | ? |

Six of your seven x86 machines match the published ROB size to within about 2 entries; **artemisia is about 14 entries below 512**, in
two separate runs. That is a genuine finding to investigate and report honestly (try `HP=1`, check the fine sweep, check whether SMT is on; discuss
whether it is a property of the hardware or of the method, and do not claim to know which). Expected sources of a gap: another structure limits the window
before the ROB fills, or the published figure counts entries differently.

**Be ready to explain (ex04):** why two DRAM-missing loads separated by N filler instructions overlap only while all N+3 instructions fit in the ROB;
why the next iteration's addresses depend on the previous load (so iterations cannot overlap); why NOPs are used as filler; what `ROB = N_knee + 2` means.

---

## 6. Exercise 1: is the processor pipelined? (10 points)

**What it proves.** A dependent chain of N instructions takes N x latency. If the functional unit is *pipelined*, K independent chains run concurrently, so throughput
grows ~K-fold until a hardware limit. `imul` is the cleanest test: latency 3, one issue per cycle, so the speedup saturates at K=3. The `add` rows show the machine's
width (independent adds per cycle). Ratios of throughputs do not depend on the clock, so no counters and no frequency assumptions are needed.

**Files.** `exercises/ex01_pipelined/`: `pipe_test.c` (kernels), `analyze.py`, `run.sh`, `Makefile`.

**Run** (about 2 seconds per run):
```bash
# optional local smoke test on your own computer (x86 only), from the repo root:
cd exercises/ex01_pipelined && make && CORE=2 bash run.sh && cd ../..
# on the machines:
bash scripts/fleet.sh all ex01_pipelined $X86      # x3
python3 scripts/collect.py ex01_pipelined
```

**Expected output (`summary.txt`):**
```
== ex01_pipelined on CPU 0 (cpu0) ==
core clock estimate (dependent register-add chain): 3.28 GHz
K       add_reg speedup   add_imm speedup      imul speedup   add_reg IPC   imul IPC
1                  1.00              1.00              1.00          1.01       0.33
2                  1.94              1.94              2.01          1.95       0.67
3                  2.62              2.64              3.03          2.64       1.00
4                  3.33              3.38              3.02          3.35       1.00
5                  3.20              3.25              3.02          3.22       1.00
6                  3.55              3.66              3.03          3.58       1.00
8                  3.88              3.92              3.02          3.90       1.00
12                 3.93              3.93              3.02          3.96       1.00

add_reg: 3.9x speedup, saturating at K=6  -> machine sustains ~3.9 independent adds per cycle
imul   : 3.0x speedup, saturating at K=3  -> dependent imul latency ~3.0 cycles, issue rate ~1/cycle
add_imm vs add_reg at K=1: 1.00x  (no special handling of immediate adds)
PIPELINED=yes
IMUL_SPEEDUP_MAX=3.03
IMUL_SATURATION_K=3
IMUL_LATENCY_CYCLES=3.02
ADD_PEAK_IPC=3.93
ADD_SATURATION_K=6
ADD_IMM_FOLD_FACTOR=1.00
CORE_GHZ=3.280
K1_SPREAD=0.078
STATUS=ok
```

**How to read it**
- `add_reg IPC` at K=1 is 1.00 by construction (the clock estimate comes from that chain). Its growth with K, saturating, is the machine width in adds per cycle.
- `imul speedup` reaches ~3 at K=3 and stays flat: **a pipelined multiplier** (a new multiply starts every cycle while earlier ones are in flight). `IMUL_LATENCY_CYCLES` ~ 3 confirms the assumption behind the clock.
- `add_imm vs add_reg at K=1`: about 1.0 on most cores. On some newer Intel cores (Golden Cove class) dependent immediate adds are folded at the rename stage and run several per cycle (I saw a factor of 4.1 on one sandbox host). If artemisia shows this, it is a nice cross-machine difference to discuss. This is also why the clock estimate uses a register-source add.
- `PIPELINED=yes` requires an `imul` speedup of at least 2. `STATUS=ok` requires run-to-run spread of the K=1 rate below 20%.

**Hypotheses to check (published/typical, not results):** `imul` latency is 3 cycles on Haswell through Sapphire Rapids and on Zen 2. Peak independent adds per cycle: about 4 on Haswell, Skylake-family and Zen 2, about 5 on Golden Cove / Sapphire Rapids.

**If it goes wrong**

| Symptom | Fix |
|---|---|
| `STATUS=unconfirmed`, `K1_SPREAD` large | machine busy or frequency changing: rerun when idle |
| `ADD_SATURATION_K` = 12 | machine wider than the sweep; harmless, note it |
| `x86-64 only` message | you included thunderbird; use `$X86` |

**Report.** Figure: `plot.png` of two machines side by side. Table: `report/tables/ex01_pipelined_summary.md`. Discussion: why a latency-3 unit with throughput 1 is pipelined; why width differs between machines.
**Be ready to explain:** the `KERNEL` macro (K accumulator registers, `.rept` unrolling, `r15` loop counter, why the compiler cannot touch inline asm), why r14 is a constant source register.

---

## 7. Exercise 2: is execution out of order? (10 points)

**What it proves.** A load that misses all caches takes ~100+ ns. We place F filler `add`s after it in two variants that are *identical instructions*: **independent** filler (does not use the loaded value) and **dependent** filler (uses it).
An out-of-order core executes the independent filler *under* the miss: time stays flat. The dependent filler cannot start until the load returns: ~1 cycle each is added. An in-order core would show linear growth for both.

**Files.** `exercises/ex02_ooo/`: `ooo_test.c`, `analyze.py`, `run.sh`, `Makefile`. Uses a 512 MiB buffer.

**Run** (about 6 seconds per run):
```bash
bash scripts/fleet.sh all ex02_ooo $X86            # x3
python3 scripts/collect.py ex02_ooo
```

**Expected output:**
```
== ex02_ooo on CPU 0 (cpu0) ==
core clock estimate: 3.26 GHz   |   miss latency (F=0): 145.1 ns
    F    independent filler    dependent filler   hidden by OoO
    0               145.1ns             147.9ns           2.8ns
   32               145.2ns             148.7ns           3.5ns
   64               149.3ns             163.5ns          14.2ns
   96               143.1ns             176.2ns          33.1ns
  128               157.8ns             180.8ns          23.1ns
  160               151.3ns             189.8ns          38.5ns
  192               146.7ns             205.6ns          58.9ns
  224               148.8ns             213.2ns          64.4ns
  256               154.8ns             223.1ns          68.3ns
  288               168.7ns             230.6ns          61.9ns
  320               174.0ns             238.5ns          64.4ns

dependent filler: 0.302 ns per filler = 0.99 cycles each (regression over all F)  -> at F=128 it adds 38.7 ns,
                  independent filler adds 8.0 ns  ->  79% of the filler work was hidden under the miss
independent curve starts rising at F = 272   (compare with the ROB size from ex04)
OUT_OF_ORDER=yes
MISS_LATENCY_NS=145.1
DEP_COST_NS=38.7
INDEP_COST_NS=8.0
DEP_CYCLES_PER_FILLER=0.99
INDEP_RISE_F=272
CORE_GHZ=3.258
STATUS=ok
```

**How to read it**
- The two columns start together at F=0 (the miss latency) and separate: **dependent grows ~1 cycle per filler instruction** (the regression line says how many cycles each; expect ~1.0), **independent stays flat**. `OUT_OF_ORDER=yes` when the independent filler costs less than a quarter of the dependent one at F=128.
- "hidden by OoO" is the time the machine saved by running work under the miss: tens of ns.
- Where the independent curve eventually rises (`INDEP_RISE_F`) is a *window limit indicator*. It need **not** equal your ROB size from ex04: waiting dependent instructions also occupy the scheduler. In the sample it starts rising at F=272. Compare it with the ROB size of the *same machine* from ex04: if they are close, the ROB is the limit; if the rise starts far below the ROB size (I saw that on an earlier sandbox host whose ROB was about 500), another structure such as the scheduler limits first. Do not assume either; report what you see.
- Individual points wobble by +/-10 ns (the miss latency itself varies); the regression and the 15 repetitions exist to average that out.

**Hypotheses:** every machine on your list is an out-of-order core, so `OUT_OF_ORDER=yes` everywhere is expected. The interesting differences are the miss latency (server parts are slower) and where the independent curve rises.

**If it goes wrong:** `DEP_CYCLES_PER_FILLER` far from 1 (say < 0.7) means noise: rerun idle. `STATUS=unconfirmed` means the dependent cost was tiny. `mmap` errors: not enough free RAM (needs ~512 MiB).

**Report.** Figure: both curves for two machines. One sentence tying it to ex04: the same mechanism, filler under a miss, until the window fills.
**Be ready to explain:** why the next load's address depends on both the loaded value and the filler chain end (it puts the chain on the critical path; without it the dependent chain hides behind the *next* iteration's miss; I hit this exact flaw while developing it); why the two filler instructions are the same size; the sled-entry trick.

---

## 8. Exercise 3: branch prediction and misprediction cost (10 points)

**What it proves.** One data-dependent branch per array element (inline asm: a conditional move is impossible). Random data with a fraction p of high values has an unavoidable misprediction rate e = min(p, 1-p) for any predictor, so time per element is **linear in e**, and the slope is the cost of one misprediction. Periodic patterns (`alt`, `per16`, `per64`) are learned by a history-based predictor, random data cannot be. This is the evidence that a predictor exists *and* that it uses history.

**Files.** `exercises/ex03_branch_prediction/`: `branch_test.c`, `analyze.py`, `run.sh`, `Makefile`.

**Run** (about 3 seconds per run):
```bash
bash scripts/fleet.sh all ex03_branch_prediction $X86     # x3
python3 scripts/collect.py ex03_branch_prediction
```

**Expected output:**
```
== ex03_branch_prediction on CPU 0 (cpu0) ==
core clock estimate: 3.27 GHz
    mode   mispredict frac e    ns / element
     p00                0.00           0.671
     p01                0.01           0.770
     p02                0.02           0.810
     p05                0.05           1.203
     p10                0.10           1.691
     p20                0.20           2.745
     p30                0.30           3.476
     p40                0.40           4.464
     p50                0.50           5.053
  sorted           (pattern)           0.632
     alt           (pattern)           0.768
   per16           (pattern)           0.955
   per64           (pattern)           0.998

linear fit over random data: T = 0.734 ns + 9.04 ns x e      (R^2 = 0.995)
misprediction penalty ~ 9.0 ns = 29.5 cycles   (two-point estimate (p50-sorted)/0.5: 8.8 ns = 28.9 cycles)
random 50% data is 8.0x slower than sorted data -> branch prediction is clearly in use
     alt:  3.1% of the random-data misprediction cost remains  ->  pattern learned by the predictor
   per16:  7.3% of the random-data misprediction cost remains  ->  pattern learned by the predictor
   per64:  8.3% of the random-data misprediction cost remains  ->  pattern learned by the predictor
(the penalty above also contains the data-load and compare latency before the branch resolves, so it is an upper bound
 for the front-end pipeline length; see ex15)
PREDICTION_USED=yes
T_SORTED_NS=0.632
T_RAND50_NS=5.053
SLOWDOWN_RAND50=8.00
PENALTY_NS=9.04
PENALTY_CYCLES=29.54
PENALTY_CYCLES_TWO_POINT=28.90
FIT_R2=0.995
PATTERN_ALT_COST_REMAINING=0.031
PATTERN_PER16_COST_REMAINING=0.073
PATTERN_PER64_COST_REMAINING=0.083
CORE_GHZ=3.268
STATUS=ok
```

**How to read it**
- The table is the evidence: time per element rises smoothly from the p00 row to p50, with sorted data and periodic patterns far below random data.
- `FIT_R2` close to 1 (>= 0.9 required for `STATUS=ok`) shows the linear model fits. `PENALTY_CYCLES` is the slope times the clock; the two-point estimate (random50 - sorted)/0.5 should agree within a few cycles.
- "x% of the random-data cost remains" for the patterns: a few percent means the predictor learned the pattern. If a machine shows a large percentage for `per64`, its history is too short for that period: report it as a predictor-capacity observation.
- The penalty here **includes** the L1 load and compare latency before the branch resolves, so it is an **upper bound** for the front-end depth (ex15).

**Hypotheses:** published misprediction penalties are roughly 15-20 cycles on recent Intel and AMD cores; this loop adds a few cycles of data dependence on top, so 20-30 is plausible. Differences between machines are the interesting part.

**If it goes wrong:** `FIT_R2` low or penalty negative: noisy machine, rerun. In the shared sandbox the penalty varied from 25 to 30 cycles between runs; report the spread over your 3 runs.

**Report.** Figure: `plot.png` (data points, fitted line, pattern markers) for two machines. State the penalty in cycles with min..max over runs.
**Be ready to explain:** why `jb` is written in asm (no cmov), why data is regenerated every pass (the predictor cannot memorize 4 MiB), why e = min(p, 1-p).

---

## 9. Exercise 15: pipeline depth (10 points, analysis only)

**What it proves.** The misprediction penalty (ex03) is the time from fetching a branch to restarting the front end after it resolves: roughly the **front-end pipeline length plus the latency of what the branch waits for**. It is an upper bound for pipeline depth, not an exact stage count.
There is nothing to run on the machines; this reuses the ex03 data.

**Run** (on your own computer, after ex03 data exists):
```bash
python3 exercises/ex15_pipeline_depth/analyze_ex15.py
```

**Expected output** (one row per machine/environment; the sample uses two simulated machines, `alpha` and `beta`, that are really the same sandbox VM, with 3 and 2 confirmed ex03 runs):
```
| machine | cpu_model | environment | core_label | runs | penalty_cycles_median | penalty_min | penalty_max | two_point_cycles_median | core_ghz |
|---|---|---|---|---|---|---|---|---|---|
| alpha | Intel(R) Xeon(R) Processor @ 2.80GHz | native | cpu0 | 3 | 26.1 | 24.9 | 27.3 | 24.7 | 3.17 |
| beta | Intel(R) Xeon(R) Processor @ 2.80GHz | native | cpu1 | 2 | 28.8 | 28.4 | 29.2 | 25.1 | 3.31 |
```
Files: `report/tables/ex15_pipeline_depth.csv/.md` and `report/figures/ex15_pipeline_depth.png` (bars with min..max error bars). If you build the enclave variant of ex03, its rows appear automatically as `environment=enclave`.

**What to write.** The penalty in cycles per machine, ordered by introduction year; whether newer cores show a longer or shorter penalty; what the number includes and what it does not (data-dependency latency, resteer cost, one-cycle fetch effects); why you cannot read off an exact number of pipeline stages from a timing experiment.
**Be ready to explain:** the difference between pipeline depth and misprediction penalty.

---

## 10. Exercise 8: cost of a TLB miss (10 points)

**What it proves.** Pointer-chasing through P nodes in random order exposes latency. Two layouts with the same number of nodes and cache footprint: **dense** (nodes packed 64 B apart, backed by huge pages, so almost no TLB entries) and **sparse** (one node per 4 KiB page, so P translations). The per-access difference, as P grows, isolates address translation and shows two steps: where the **L1 DTLB** is exhausted (an L2-TLB hit costs a few cycles) and where the **L2 TLB** is exhausted (a page walk costs tens of cycles, more when the page tables themselves miss the caches).

**Files.** `exercises/ex08_tlb_miss_cost/`: `tlb_test.c`, `analyze.py`, `run.sh`, `Makefile`. Needs ~260 MiB and **transparent huge pages enabled for madvise** (`cat /sys/kernel/mm/transparent_hugepage/enabled` shows `always` or `[madvise]`, not `[never]`).

**Run** (about 12 seconds per run):
```bash
bash scripts/fleet.sh all ex08_tlb_miss_cost $X86      # x3
python3 scripts/collect.py ex08_tlb_miss_cost
```

**Expected output:**
```
== ex08_tlb_miss_cost on CPU 0 (cpu0) ==
core clock estimate: 3.27 GHz   |   THP setting: madvise   |   huge-page-backed memory: 4096 kB
 P (pages)   dense ns  sparse ns   translation cost ns   cycles
         8       1.54       1.60                  0.04      0.1
        16       1.54       1.58                  0.01      0.0
        24       1.56       1.57                 -0.01     -0.0
        32       1.55       1.54                 -0.04     -0.1
        48       1.55       1.53                 -0.04     -0.1
        64       1.54       1.65                  0.08      0.3
        96       1.56       3.78                  2.19      7.2
       128       1.56       3.77                  2.18      7.1
       192       1.58       3.79                  2.19      7.1
       256       1.60       3.88                  2.26      7.4
       384       1.61       3.92                  2.29      7.5
       512       1.63       3.88                  2.23      7.3
       768       1.73       3.81                  2.06      6.7
      1024       4.41       6.64                  2.21      7.2
      1536       4.28       6.58                  2.28      7.4
      2048       4.43       8.30                  3.84     12.5
      3072       4.33      14.11                  9.75     31.9
      4096       4.30      14.91                 10.58     34.6
      6144       4.29      16.52                 12.21     39.9
      8192       4.53      16.48                 11.93     39.0
     12288       4.77      21.45                 16.66     54.4
     16384       6.50      30.49                 23.96     78.3
     32768      25.65     113.56                 87.88    287.1
     65536     101.66     160.77                 59.09    193.0

L1-DTLB miss (L2-TLB hit) cost ~ 2.23 ns = 7.3 cycles   (window P=192..768)
page walk, page tables cached  ~ 10.58 ns = 34.6 cycles   (3 points from the second step, P>=3072)
page walk, page tables cold     ~ 73.49 ns = 240.1 cycles   (P>=32768)
first step up at P ~ 96  (L1 DTLB reach in entries: between the previous grid point and this one)
L2 TLB starts to be exceeded at P ~ 2048  (capacity: between the previous grid point and 2048)
full page-walk level reached at P ~ 3072
TLB_L1MISS_NS=2.23
TLB_L1MISS_CYCLES=7.3
TLB_WALK_NS=10.58
TLB_WALK_CYCLES=34.6
TLB_WALK_COLD_NS=73.49
TLB_WALK_COLD_CYCLES=240.1
TLB_STEP1_P=96
TLB_STEP2_START_P=2048
TLB_STEP2_P=3072
DENSE_HUGEPAGES=yes
CORE_GHZ=3.267
STATUS=ok
```

**How to read it**
- Columns `dense ns` and `sparse ns` are the two chases. Both rise at P=1024 (the 64 KiB working set leaves L1 data cache) and later; those cache effects are **identical in both layouts and cancel** in the "translation cost" column.
- The translation cost is ~0 up to the L1 DTLB reach, jumps to a plateau (the L2-TLB hit cost; ~7 cycles in the sample), then jumps again (page walk, 30+ cycles), then keeps growing for very large P as the page tables stop fitting in cache. `TLB_WALK_*` is the first plateau after the second step; `TLB_WALK_COLD_*` is the large-P region.
- **Reading the capacities.** `TLB_STEP1_P` is the first grid point where the L1 DTLB is exceeded, so the L1 DTLB size lies between the previous grid point and it (grid: 8, 16, 24, 32, 48, 64, 96, 128 ... 1536, 2048, 3072 ...; sample: first step at 96, so the size is between 64 and 96). The L2 TLB is exceeded gradually: a random chase slightly over capacity still hits part of the time. `TLB_STEP2_START_P` is where the rise *starts* (sample: 2048, so the capacity lies between 1536 and 2048), and `TLB_STEP2_P` is where the full page-walk level is reached (sample: 3072); that second value can overestimate the capacity by up to about 2x, so quote the start of the rise.
- `DENSE_HUGEPAGES=yes` means the dense layout really got huge pages. If `no`, the analyzer warns and the run is `unconfirmed`: the dense chase suffers TLB misses too and the costs are underestimated.

**Hypotheses (published/typical, verify):** L1 DTLB 64 entries on Haswell, Skylake-family and Zen 2, 96 on Golden Cove; L2 TLB 1024 (Haswell), 1536 (Skylake-family), 2048 (Zen 2, Golden Cove). The page-walk cost varies a lot between runs in a shared machine (13-24 ns in my sandbox runs); report min..max.

**If it goes wrong:** THP `never` on a machine: it is a limitation; ask the administrator, or report it and keep the run as supporting data. `mmap` failure: low free RAM.

**Report.** Figure: both panels of `plot.png` for two machines. Table: the four cost numbers and the two step positions per machine.
**Be ready to explain:** why the node offset inside each page varies (so lines spread over cache sets like the dense layout), why a random cyclic order (no prefetching, one load in flight), why the difference cancels the cache effects.

---

## 11. Exercise 10: how many branches can the BTB hold? (20 points)

**What it proves.** A chain of N unconditional jumps (each to the next; the last returns to the first) is generated at run time. Every executed jump is a *taken* branch whose target the front end must know early. While all N fit in a BTB level the cost per branch is small and constant; when N exceeds a level, targets are found later and each jump pays a re-steer bubble: **a staircase in cycles per taken branch**. The branches are spaced S = 4, 8, 16, 32, 64 bytes apart: a step at the *same N for every spacing* points at a capacity; steps that move with S point at set mapping, fetch-window or cache/ITLB limits.

**Files.** `exercises/ex10_btb_capacity/`: `btb_test.c` (code generator + timing), `analyze.py`, `run.sh`, `Makefile`. Needs permission to map executable memory (`mmap` + `mprotect(PROT_EXEC)`).

**Run** (about 25 seconds per run):
```bash
bash scripts/fleet.sh all ex10_btb_capacity $X86      # x3
python3 scripts/collect.py ex10_btb_capacity
```

**Expected output:**
```
== ex10_btb_capacity on CPU 0 (cpu0) ==
core clock estimate: 3.21 GHz   (1 cycle = 0.312 ns)
time per taken branch, in CYCLES (median of repetitions)
     N   S=4      S=8      S=16     S=32     S=64
     8     1.84     1.43     1.01     1.23     1.00
    12     1.78     1.45     1.08     1.16     0.98
    16     2.25     1.48     1.12     1.10     0.98
    24     2.06     1.48     1.39     1.05     1.01
    32     2.23     1.66     1.02     1.05     1.01
    48     2.08     1.53     1.01     1.05     0.99
    64     2.11     1.54     1.14     1.01     0.99
    96     2.16     1.53     1.05     1.00     0.97
   128     2.15     1.48     1.17     1.17     0.98
   192     2.14     1.50     1.15     1.10     1.80
   256     2.16     1.57     1.01     1.03     1.91
   384     2.19     1.55     1.46     1.90     1.96
   512     2.15     1.85     1.71     1.95     2.02
   768     2.07     1.86     1.95     1.97     3.90
  1024     2.16     2.09     1.96     1.95     3.91
  1536     2.16     2.18     2.10     1.98     3.95
  2048     2.24     2.19     1.99     2.04     4.00
  3072     2.29     2.33     2.06     2.15     7.84
  4096     2.37     2.45     2.34     2.16     8.31
  6144     5.97     8.74     7.63     8.01    10.18
  8192     8.07     9.06     9.01     8.53    10.26
 12288    10.27    11.33    10.17    10.03    10.89
 16384    10.35    10.16    10.68    10.81    11.41
 24576    10.38    11.07    10.79    10.55    13.63
 32768    10.18    10.34    10.73    10.38    14.56

S=4   steps at N = 3072 (x1.29), 4096 (x2.51), 6144 (x1.35), 8192 (x1.27)
S=8   steps at N = 384 (x1.30), 2048 (x1.26), 4096 (x3.76)
S=16  steps at N = 256 (x1.44), 512 (x1.34), 4096 (x3.92), 8192 (x1.33)
S=32  steps at N = 256 (x1.54), 4096 (x4.22), 8192 (x1.25)
S=64  steps at N = 128 (x1.80), 512 (x2.17), 2048 (x2.01), 4096 (x1.30), 16384 (x1.34)

first plateau (small N, S=16): 1.01 cycles per taken branch;  last plateau (N=32768): ~10.4 cycles
largest big step (>=1.5x) at N ~ 4096 branches and it is at about the same N for every spacing 4..32 -> a BTB capacity
steps at small N that DO move with S point at set-mapping limits or fetch-window limits; steps whose N is constant across
S point at capacity. Compare with the published BTB size in the report (kept separate).
KNEES_S4=3072,4096,6144,8192
LAST_BIG_KNEE_S4=4096
KNEES_S8=384,2048,4096
LAST_BIG_KNEE_S8=4096
KNEES_S16=256,512,4096,8192
LAST_BIG_KNEE_S16=4096
KNEES_S32=256,4096,8192
LAST_BIG_KNEE_S32=4096
KNEES_S64=128,512,2048,4096,16384
LAST_BIG_KNEE_S64=2048
BTB_CAPACITY_EST=4096
BTB_AGREE_ACROSS_S=yes
BASE_CYCLES_PER_BRANCH=1.01
FAR_CYCLES_PER_BRANCH=10.4
CORE_GHZ=3.208
STATUS=ok
```

**How to read it**
- Read the table downwards in the S=16 column: ~1 cycle per taken branch (fastest level), ~1.5-2 cycles (next level), then ~8-10 cycles. The big jump around N=4096-6144 is at about the same N for S=4, 8, 16, 32: **a BTB capacity**. `BTB_CAPACITY_EST` is the median of those knees and `BTB_AGREE_ACROSS_S=yes` says they agree within 1.5x.
- S=64 behaves differently (steps at smaller N, higher plateaus): with 64-byte spacing every branch is on its own cache line and BTB sets are used unevenly; its large-N region also stresses the instruction cache and ITLB. That is a *spacing effect*, not capacity: say so in the report.
- Small-N steps (a few hundred branches) are fast-BTB-level or fetch-window effects. Report them as levels, with the N where they occur.
- `STATUS=ok` needs the big step to be found and steps in at least three spacings.

**Hypotheses:** published BTB sizes are in the thousands of entries (and vary by level and by how vendors count); your measured number is an *effective* capacity for this branch pattern. Compare with WikiChip for each microarchitecture and keep published and measured values separate.

**If it goes wrong**

| Symptom | Fix |
|---|---|
| `mprotect(PROT_EXEC) ...` error | the system forbids writable-then-executable memory (W^X / SELinux). Report it as a limitation for that machine |
| `BTB_AGREE_ACROSS_S=no` | not an error: steps differ by spacing. Discuss per spacing |
| very noisy small-N values | machine busy; rerun idle |

**Report.** Figure: the staircase plot for two machines. Table: first-level cycles/branch, last plateau, `BTB_CAPACITY_EST`.
**Be ready to explain:** the machine code bytes (`EB 02` and `E9 rel32`), why the loop-closing `jnz` counts as a taken branch (N+1 per iteration), why several spacings are used, why a step at the same N for every S means capacity.

---

## 12. Enclave (SGX) comparison: what you can and cannot do today

The requirement: run each selected experiment **inside and outside an enclave on the same enclave-capable machine**, compare, and explain differences; identify the platform and configuration. If enclave support or access is unavailable on a machine, **report that limitation** (the ordinary experiments still count).

**Honest status.** I have not written or tested any enclave code, and this sandbox has no SGX, so I could not. The `*_sgx` folders are scaffolds. Whether any of your machines can run enclaves is unknown: only artemisia (Sapphire Rapids) is a strong candidate, and the four i7 client machines (charnwood, ookay, upgrade, crux) are doubtful (see `docs/enclave_setup.md`).

**Step 1 (do this first, 5 minutes, no root needed).** On artemisia and each i7 machine:
```bash
ssh artemisia
grep -m1 -ow -E 'sgx|sgx_lc|sgx1|sgx2' /proc/cpuinfo | sort -u      # CPU flags the kernel exposes
ls -l /dev/sgx_enclave /dev/sgx/enclave /dev/isgx 2>&1               # driver device nodes
```
or from your computer: `bash scripts/fleet.sh status artemisia charnwood ookay upgrade crux` (it prints `sgx cpu flags` and `sgx devices`).

**Step 2: decide.**

| What you see | Meaning | Do |
|---|---|---|
| a device node exists (`/dev/sgx_enclave` or `/dev/isgx`) and flags include `sgx` | an enclave driver is installed | ask the course staff for the SDK/Gramine setup; tell me the machine and the output, and I will build the enclave variants |
| flags present but no device node | CPU supports SGX; driver not installed or SGX disabled in BIOS | you probably cannot fix it without root/BIOS access: ask the staff, otherwise go to the limitation path |
| no `sgx` flag | SGX unavailable (disabled or not supported) | limitation path |

**Limitation path (valid per the assignment).** In the report, state for each machine: the CPU, whether its spec lists SGX, the commands above and their output, and that no enclave runtime was available to you. Keep the ordinary experiments complete (two machines per exercise). Do not fabricate or estimate enclave numbers.

**If an enclave machine exists, what is feasible** (this is my plan, not something that exists yet):

| Exercise | Inside an enclave | Difficulty |
|---|---|---|
| ex01 pipelined | pure compute; works unchanged (time outside around the ECALL) | easy |
| ex03 branch prediction | compute over enclave memory; works | easy |
| ex02 / ex04 | needs the 512 MiB buffer inside the EPC (enclave memory is encrypted, small on client parts); same kernels | medium |
| ex08 TLB miss cost | EPC pages are 4 KiB: the dense layout cannot use huge pages inside, so the method needs a redesign | medium-hard |
| ex10 BTB capacity | enclaves cannot generate and run code at run time (SGX1): needs statically assembled chains for each N | hard |

Known differences to look for (hypotheses to test, not results): compute-only experiments (ex01, ex03) should match native closely; memory-latency plateaus (ex02, ex04) should be higher in an enclave because of memory encryption, while structural knees (ROB size) should not move; `rdtsc` is not allowed inside SGX1 enclaves, so time outside around the ECALL.
Ex15 gets its enclave comparison from the enclave version of ex03.

---

## 13. Optional extras (allowed, not required)

- **Counters** may only confirm findings afterwards; they are never the main evidence. Only ex04 has a counter helper (`perf_check.sh`, gated on a confirmed timing result, untested on real counters).
- **Fixing frequency** needs the machine owner's permission and a restore: `sudo common/freq_ctl.sh save`, `fix`, then `restore`. State in the report whether you did.
- **Hybrid CPUs:** none of your eight machines is hybrid, so there is nothing to do.

---

## 14. Assembling the report

Start from `report/report.md` (the front page already lists your selection and the 100-point total). For **each** exercise, the assignment wants: a brief method, a code snippet, results from two machines side by side, repeated measurements, plots, short explanations, enclave comparison (or the limitation), and findings kept separate from published specifications.

| Exercise | Code snippet to include | Figure | Table | Discussion points |
|---|---|---|---|---|
| ex01 | `KERNEL` macro and one `ALLK` instance in `pipe_test.c` | speedup vs K, two machines | `report/tables/ex01_pipelined_summary.md` | pipelined multiplier; width; immediate-add folding if seen |
| ex02 | `KERNEL` macro in `ooo_test.c` | the two curves | `..._ex02_ooo_summary.md` | OoO evidence; where the independent curve rises vs ROB (ex04) |
| ex03 | `kernel()` in `branch_test.c` | fit + patterns | `..._ex03_branch_prediction_summary.md` | linear cost; history learned; penalty spread |
| ex04 | `rob_kernel` in `rob_test.c` | coarse + fine sweeps | `python3 scripts/summarize.py ex04_rob` | knee vs published; artemisia gap; runs spread |
| ex08 | `chase()` and the two layouts in `tlb_test.c` | both panels | `..._ex08_tlb_miss_cost_summary.md` | L1 DTLB / L2 TLB reach; walk cost; THP |
| ex10 | `build_chain()` in `btb_test.c` | staircase | `..._ex10_btb_capacity_summary.md` | levels; capacity vs spacing; published sizes kept separate |
| ex15 | none (analysis) | penalty bars | `report/tables/ex15_pipeline_depth.md` | upper bound for depth; what the number includes |

Also include: the machine table (`docs/machines.md` and the machine cards in `data/<machine>/machine_card.md`: fill in cores/SMT, caches, RAM, kernel, SGX result), frequency handling, the limitations you hit (matplotlib missing on machines, Arm not covered for ex01-10, THP, W^X, noisy runs excluded and why), and the AI disclosure.

**Track B is an AI-permitted track, with conditions:** disclose all AI use in `ai_log.md` and be able to explain every line of submitted code. Each exercise section above ends with "Be ready to explain" bullets; if you cannot answer them, read the code first.

Convert `report/report.md` to PDF (for example `pandoc report/report.md -o report/report.pdf`) and keep the PDF in `report/`.

---

## 15. Optional bonus: a trend across eight machines

The bonus needs one reverse-engineering result compared across **at least eight course machines** with a proposed trend ("law"), its scope and exceptions. ROB size (ex04) is the natural candidate, and with thunderbird you have exactly eight. Using your coarse results so far:

| Year | Machine | Microarchitecture | ROB (measured, coarse) |
|---|---|---|---|
| 2014 | sunbird | Haswell | ~194 |
| 2015 | charnwood | Skylake | ~226 |
| 2017 | ookay, upgrade | Kaby Lake, Coffee Lake | ~226 |
| 2018 | crux | Coffee Lake Refresh | ~226 |
| 2019 | skylark | Zen 2 (AMD) | ~226 |
| 2020 | thunderbird | Neoverse N1 (Arm) | not measured yet |
| 2023 | artemisia | Sapphire Rapids | ~498 |

A pattern you can defend with this data is that ROB size changes in **discrete generational steps, not continuously**: a plateau at 224 across 2015-2019 spanning two vendors, a small step from Haswell (192), and a large jump by 2023. Write a falsifiable prediction (for example, what a 2024-2025 core should show, and what would refute your law), state exceptions (artemisia's 14-entry gap to the published value, the Arm core on a different ISA), and use the fine-sweep numbers with their run-to-run spread. Required for credit: more than a table; a stated law with scope and exceptions.

---

## 16. Packaging and final checklist

```bash
bash scripts/verify_repo.sh                  # still consistent?
python3 scripts/summarize.py ex04_rob        # ex04 tables and overlay
for e in ex01_pipelined ex02_ooo ex03_branch_prediction ex08_tlb_miss_cost ex10_btb_capacity; do python3 scripts/collect.py $e; done
python3 exercises/ex15_pipeline_depth/analyze_ex15.py
# put report.pdf in report/, fill in ai_log.md, fill in docs/machines.md and the machine cards
bash scripts/package.sh                      # -> hw3_submission.zip
unzip -l hw3_submission.zip | less           # check: report PDF, exercises/, common/, scripts/, data/, docs/, ai_log.md, README.md
```

- [ ] report PDF with the selection list and 100-point total on the first page
- [ ] every selected exercise on **two machines**, >= 3 confirmed runs each, side by side, with figures
- [ ] enclave comparison done, or the limitation documented with the commands and their output
- [ ] published specifications kept separate from measurements; cache/prefetcher topics not included
- [ ] frequency changes (if any) restored and stated
- [ ] `ai_log.md` filled in (or the statement that no AI was used)
- [ ] the ZIP contains: PDF, source code, raw data, plotting scripts, reproduction instructions
- [ ] submitted before the deadline, with time to spare

---

## 17. Troubleshooting index

| Symptom | Meaning / fix |
|---|---|
| `Connection closed by <ip> port 22` after `Password:` | login failed: test `ssh <target>` by hand; set up SSH keys; rerun only that machine |
| `ModuleNotFoundError: matplotlib` on a machine | harmless now: plots are rebuilt on your computer by `collect.py` / `summarize.py` |
| compile errors mentioning `rax`/`rcx` on thunderbird | that was an outdated x86-only `rob_test.c`; the current one has an AArch64 kernel. New exercises are x86-only: exclude thunderbird |
| `x86-64 only for now` | expected on thunderbird for ex01-ex10 |
| `unknown machine id` | the id must match the first column of `machines.txt` |
| `rsync: mkdir ... failed` | you have an outdated `fleet.sh`: re-copy `scripts/` |
| `cannot delete non-empty directory ..._latest` | outdated `fleet.sh`; delete the stray local folder `data/<machine>/<ex>/*_latest` |
| results seem to be in the wrong machine's folder | outdated `fleet.sh` (the shared home directory problem): re-copy `scripts/`, delete old `data/*/<exercise>/` and rerun |
| `STATUS=unconfirmed` | read the printout: noise, THP off, no clear structure. Rerun idle |
| `Permission denied` running `./run.sh` | home mounted `noexec`: `REMOTE_DIR=/dev/shm/hw3 bash scripts/fleet.sh ...` |
| `mprotect(PROT_EXEC)` error (ex10) | W^X policy on that machine: report as a limitation |
| `BUILD FAILED` in `verify_repo.sh` | `cd exercises/<ex> && make` and read the compiler message |
| a run looks wrong but you cannot tell why | open `data/<machine>/<ex>/<run>/summary.txt` and `sysinfo/`, check load with `fleet.sh status`, compare with another run |

---

## 18. Appendix: published values to compare against (typical figures; verify before quoting)

| Quantity | Haswell | Skylake / Kaby / Coffee | Zen 2 | Sapphire Rapids (Golden Cove) |
|---|---|---|---|---|
| ROB entries | 192 | 224 | 224 | 512 |
| `imul` latency (cycles) | 3 | 3 | 3 | 3 |
| independent ALU adds per cycle | 4 | 4 | 4 | 5 |
| L1 DTLB entries (4 KiB pages) | 64 | 64 | 64 | 96 |
| L2 TLB entries | 1024 | 1536 | 2048 | 2048 |

Arm Neoverse N1: ROB reported as 128 by public reviews (verify). BTB sizes and misprediction penalties differ by source and by how each is
defined, so look them up on WikiChip / vendor documents per machine and cite them as *published* values, separate from what you measured.
