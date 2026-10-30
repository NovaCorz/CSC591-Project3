# Runbook: running the exercises in order

> **Superseded for the full project by [`FULL_GUIDE.md`](FULL_GUIDE.md)**, which covers ex01-ex15, the enclave question, the report and packaging.
> This file stays as the detailed ex04 walkthrough (Part B).

Start here. Part A gives the order for all exercises; Part B is a complete walkthrough for the first exercise to
finish, **ex04 (ROB size)**, with the commands to run, the files each step creates, and the output you should expect.

> The sample outputs below come from a real run of this code in a shared sandbox VM (an Intel Xeon @ 2.1 GHz whose
> exact model is hidden), with reduced settings. **Your numbers will differ.** What should match is the *shape* and
> the *kinds* of files and messages.

---

## Part A. Order of work

| Order | Exercise | Pts | Status today | Why this position |
|---|---|---|---|---|
| 1 | **ex04 ROB size** | 30 | **code ready** | biggest item, already implemented, and it validates the whole fleet workflow (SSH, deploy, pull, summarize) |
| 2 | ex03 branch prediction | 10 | implemented (see FULL_GUIDE) | its data also feeds ex15 |
| 3 | ex15 pipeline depth | 10 | implemented (see FULL_GUIDE) | pure analysis of ex03 data, almost free once ex03 exists |
| 4 | ex01 pipelined | 10 | implemented (see FULL_GUIDE) | short; reuses the timing helpers |
| 5 | ex02 out-of-order | 10 | implemented (see FULL_GUIDE) | short; same idea as ex04 with a dependent-filler control |
| 6 | ex08 TLB miss cost | 10 | implemented (see FULL_GUIDE) | new careful code (pointer chasing) |
| 7 | ex10 BTB capacity | 20 | implemented (see FULL_GUIDE) | most delicate code (jump chains, code layout) |
| 8 | all `*_sgx` enclave variants | | **not written** (see FULL_GUIDE section 12) | do after each native exercise is solid; same kernels, different harness |

Exercises 1, 2, 3, 8, 10 and 15 are now implemented (x86-64): follow `FULL_GUIDE.md`. Everything below is the ex04 walkthrough.

---

## Part B. ex04 (ROB size), step by step

### What you run, and what runs where

| Where | File | Purpose |
|---|---|---|
| your laptop | `scripts/fleet.sh` | deploys, runs and collects results over SSH |
| your laptop | `scripts/summarize.py` | table + overlay plot from the collected data |
| each machine (automatic) | `exercises/ex04_rob/run.sh` -> `run_rob.sh` | the experiment: builds nothing itself, runs the sweeps |
| each machine (automatic) | `exercises/ex04_rob/rob_test.c` (built to `rob_test` by `make`) | the measurement kernel |
| each machine (automatic) | `exercises/ex04_rob/analyze_rob.py` | finds the knee, draws the plots |
| each machine (automatic) | `common/gather_info.sh` | records CPU/OS/SGX/hybrid/frequency info |
| each machine (manual, optional, later) | `exercises/ex04_rob/perf_check.sh` | counter cross-check, only after timing results exist |
| each machine (manual, optional, coordinated) | `common/freq_ctl.sh` | fix and restore CPU frequency |

You normally only type `fleet.sh` and `summarize.py` commands on your laptop; `fleet.sh` runs the machine-side files for you.

### Step 0. One-time setup (laptop)

```bash
cd hw3
cp machines.example.txt machines.txt      # edit: one line per machine
# e.g.  m1  m1  cpu2:2  no      m3  m3  pcore:0,ecore:20  no
ssh m1 true                               # must work without a password prompt (SSH keys + ~/.ssh/config)
```

Pick the logical CPU (`cpu2:2`) for each machine; on a hybrid CPU give both a P-core and an E-core
(`pcore:0,ecore:20`; use the CPU ids from `lscpu -e` or `/sys/devices/cpu_core/cpus` and `/sys/devices/cpu_atom/cpus`).
Needed on each machine: `gcc`, `make`, `python3`, and about 600 MiB of free RAM. No root is needed.

### Step 1. Check the machines are idle

```bash
bash scripts/fleet.sh status
```

Expected, per machine (sample):

```
== m1 (m1)
host1
 19:38:52 up 12 days,  2 users, load average: 0.02, 0.05, 0.02
users logged in: 0
model name	: Intel(R) Core(TM) i7-xxxx CPU @ x.xxGHz
governor: powersave            <- or performance; n/a on a VM
hybrid: no                     <- yes means: test BOTH core types
sgx device: none               <- /dev/sgx_enclave if enclaves are possible
```

Proceed when `load average` is near 0 and nobody else is logged in. If someone is, wait or pick another time:
shared load is the main source of bad data.

### Step 2. Smoke test on ONE machine

(`m1` below is just an example: use the **id in the first column of your `machines.txt`**, e.g. `sunbird`. An unknown id now produces an error that lists the valid ids.)

```bash
bash scripts/fleet.sh all ex04_rob m1
```

This copies `common/` and `exercises/ex04_rob/` to `~/CSC591-Project3/m1/` on m1, runs `make`, runs the experiment pinned to your
chosen CPU, and copies the results to `data/m1/ex04_rob/`. With the default settings expect roughly **1-3 minutes**
(the reduced-settings sample took 18 s).

Expected console output (real sample, trimmed):

```
[m1] deploy ex04_rob (transfer: rsync)
cc -O2 -Wall -Wextra -march=native -o rob_test rob_test.c
[m1] run ex04_rob label=cpu2 core=2
== coarse sweep (step 4) on CPU 2 (cpu2) ==
low plateau  (overlapped misses) :    152.1 ns_per_iter
high plateau (serialized misses) :    349.6 ns_per_iter   ratio hi/lo = 2.30
first slow N (N_knee)            : 500
ESTIMATED ROB SIZE ~ N_knee + 2  : 502  (+/- the sweep step; refine with -t 1)
KNEE_N=500
== fine sweep (step 1) around N=500 ==
PRIMARY results (timing only) in results/host1_cpu2_hp0_20261027-193856/
Timing-based estimate confirmed. Only now may you run the secondary counter check: CORE=2 LABEL=cpu2 HP=0 ./perf_check.sh
[m1] pull results
done. Next: python3 scripts/summarize.py ex04_rob
```

**It worked if:** there is a ratio hi/lo of roughly 1.8-2.3, a `KNEE_N=` line with a number, no `WARNING` lines, and the
line "Timing-based estimate confirmed".

### Step 3. Look at what was produced

```
data/m1/ex04_rob/
├── fleet_run_20261027-193856.log                 full console log of the run
├── host1_cpu2_hp0_20261027-193856/               one directory per run (timestamped; repeats never overwrite)
│   ├── coarse.csv      raw data, N = 0..800 step 4        (sample: 1810 data rows)
│   ├── coarse.png      plot of the coarse sweep
│   ├── coarse_summary.txt
│   ├── fine.csv        raw data, step 1 around the knee   (sample: 883 data rows)
│   ├── fine.png / fine_summary.txt
│   ├── run_config.txt  core=2 label=cpu2 hp=0 hybrid=0 run=<id>
│   ├── freq_during_run.txt   frequency settings during the run (empty on a VM without cpufreq)
│   └── sysinfo/        lscpu, cpu_model, caches, SMT, hybrid_cores, sgx_flags, perf_avail, gcc, kernel, ...
└── host1_cpu2_hp0_latest -> host1_cpu2_hp0_20261027-193856    symlink to the newest run
```

CSV format (`coarse.csv`, real sample):

```
# core=0 buf_MiB=256 iters=8000 reps=9 hugepages=0 tsc_ghz=2.1000
nops,rep,tsc_per_iter,ns_per_iter
0,0,309.92,147.63
4,0,373.56,177.98
8,0,315.03,150.11
```

One row per (N, repetition). `ns_per_iter` is the time for one loop iteration (two loads); `tsc_per_iter` is the same in TSC ticks.

**Expected plot shape** (`coarse.png`): a roughly flat low plateau (the two misses overlap, about one DRAM latency,
~70-150 ns depending on the machine), a **sharp step up to about twice that** at one value of N (the red dashed line is
the estimate), then a noisier high plateau. In the sample, the low plateau drifted gently from ~150 to ~200 ns before the
step. A slow drift like that is normal (more NOPs cost some front-end time); the **step** is the signal.

The fine-sweep summary (`fine_summary.txt`, real sample):

```
low plateau  (overlapped misses) :    217.4 ns_per_iter
high plateau (serialized misses) :    359.8 ns_per_iter   ratio hi/lo = 1.65
first slow N (N_knee)            : 497
ESTIMATED ROB SIZE ~ N_knee + 2  : 499  (+/- the sweep step; refine with -t 1)
KNEE_N=497
```

Use the **fine** sweep for the knee position (step 1) and the **coarse** sweep for plateau latencies and the ratio: the fine
sweep only spans N_knee +/- 24, so its plateau estimates are rougher (ratio 1.65 here versus 2.30 in the coarse sweep).
The coarse (502) and fine (499) estimates agree within a few entries, which is what a clean run looks like.

### Step 4. Compare with the published ROB size

Look up the CPU in `data/m1/ex04_rob/<run>/sysinfo/cpu_model.txt` (and the microarchitecture on Intel ARK, AMD specs or
WikiChip). The knee should land within a few entries of the published size (table in `exercises/ex04_rob/README.md`,
e.g. Skylake 224, Zen 3 256, Ice Lake 352, Golden Cove 512). Keep the **measured** and **published** values separate in the
report. If your estimates sit consistently a few entries off on several machines, that is a constant offset of the method
(the code counts a handful of setup instructions); say so rather than hiding it.

### Step 5. Repeat runs and the huge-page variant

The course wants repeated measurements. Run each configuration at least 3 times, ideally at different times:

```bash
bash scripts/fleet.sh all ex04_rob m1                       # run 2
bash scripts/fleet.sh all ex04_rob m1                       # run 3
RUN_ENV="HP=1" bash scripts/fleet.sh all ex04_rob m1       # 4 KiB pages -> huge pages variant (3 runs of this too)
```

Expected: the knee N stays the same (+/- 1-4); the plateau latencies differ between 4 KiB pages and huge pages
(page walks add latency to every load with 4 KiB pages). Check:

```bash
python3 scripts/summarize.py ex04_rob
```

Expected (real format, sample with only one run):

```
| machine | cpu_model | core_label | hugepages | runs_total | runs_confirmed | rob_median | rob_min | rob_max | low_plateau_ns | hi_lo_ratio |
| loc | Intel(R) Xeon(R) Processor @ 2.10GHz | cpu0 | 0 | 1 | 1 | 499.0 | 499.0 | 499.0 | 217.4 | 1.65 |
NOTE: fewer than 3 confirmed runs for: loc/cpu0/hp0
```

You are done with a configuration when `runs_confirmed` is at least 3 and `rob_min`..`rob_max` is tight (a few entries).
A run that prints "fine sweep did NOT confirm" is counted in `runs_total` but not in `runs_confirmed`: rerun it when
the machine is idle.

### Step 6. All eight machines

```bash
bash scripts/fleet.sh all ex04_rob              # one machine after another
PARALLEL=1 bash scripts/fleet.sh all ex04_rob   # machines at the same time (they are independent); logs in data/<m>/ex04_rob/fleet_all.log
python3 scripts/summarize.py ex04_rob
```

Outputs: `report/tables/ex04_rob_summary.csv/.md` (one row per machine/core/page setting) and
`report/figures/ex04_rob_overlay.png` (all machines' fine sweeps, normalized so the knee is at 0 and the plateau is at 1;
every machine should show the same step to about 2).

**Hybrid machine** (`pcore:0,ecore:20`): two result directories appear (`..._pcore_...` and `..._ecore_...`), each with its own
knee. The script refuses to run on a hybrid CPU unless the core type is declared, and reminds you if one is missing.
Expect different ROB sizes and plateau latencies for the two core types.

### Step 7. Optional, only afterwards: counter cross-check (secondary evidence)

Counters may only confirm what timing already showed. On the machine:

```bash
ssh m1
cd CSC591-Project3/<machine>/ex04_rob
CORE=2 ./perf_check.sh                       # uses the knee from the confirmed fine sweep: knee-40 vs knee+40
exit
bash scripts/fleet.sh pull ex04_rob m1       # brings counter_check_SECONDARY.txt back
```

Before a confirmed timing result exists it refuses ("REFUSING: no CONFIRMED timing-based knee ..."). Afterwards it prints a
table with a header `=== SECONDARY EVIDENCE (hardware counters) ... ===` and deltas for `cycles` and `instructions`. What
confirms the timing result: **cycles rise sharply** between the two N values while **instructions change only by the extra
NOPs**. If `perf` is missing or event names are rejected (common in VMs), write the limitation in the report. This script has
not been tested against real counters yet, so expect to adjust event names (`EVENTS=...`, see its header).

### Step 8. Optional, only with permission: fix the CPU frequency

Not required. It only reduces noise. Coordinate with whoever manages the machine, check `who`, then on the machine:

```bash
cd ~/CSC591-Project3/<machine>/common
./freq_ctl.sh status
sudo ./freq_ctl.sh save          # snapshot of the ORIGINAL settings (refuses to overwrite it)
sudo ./freq_ctl.sh fix           # performance governor, turbo off
# ... now run the experiment from your laptop:  bash scripts/fleet.sh run ex04_rob m1   (then: pull) ...
sudo ./freq_ctl.sh restore       # ALWAYS restore; it prints the final state
```

`run_rob.sh` records the frequency state with every run and warns at the end if a snapshot exists (not yet restored). Write
in the report whether you changed it and that you restored it.

### Step 9. Enclave variant (not implemented yet)

`exercises/ex04_rob_sgx/` is a scaffold. It needs an enclave harness around the same asm kernel (timing outside the enclave,
buffer sized to fit the EPC; see `docs/enclave_setup.md`). Only machines with `sgx=yes` in `machines.txt` take part. Do this
after the native results are solid; ask for it when you are ready.

### Step 10. What ex04 contributes to the report

- Method paragraph and the `rob_kernel` asm snippet (from `rob_test.c`), with the dependency chain and NOP sled explained.
- For two machines (side by side): coarse and fine plots, ROB estimate with min..max over runs, plateau latencies.
- A comparison against published ROB sizes (kept separate), and an explanation of any difference between the machines.
- Optional: a short "confirmation with counters" subsection, clearly labeled secondary.
- Limitations: VM or hybrid issues, noisy runs you excluded and why.
- For the optional bonus: all eight machines in one table and the overlay plot, with a proposed trend.

---

## What the output looks like when something is wrong

| You see | Meaning | Do this |
|---|---|---|
| `WARNING: no clear 2x step` or `ratio hi/lo` below 1.4 | no knee: buffer fits in cache, ROB beyond the sweep, or machine noisy | raise `MIB` (`RUN_ENV="MIB=1024"`) or `MAXN`; check load; rerun |
| `WARNING: the fine sweep did NOT confirm the coarse knee` | a noise burst faked the coarse knee | rerun on an idle machine (more `REPS`); do not use this run |
| knee far below the published ROB size | another resource limit hit first, or wrong core type (hybrid) | check the CPU id belongs to the intended core type; discuss in the report |
| `Hybrid CPU detected: set LABEL=pcore or LABEL=ecore` | hybrid machine with plain `cpuN:N` in `machines.txt` | use `pcore:<id>,ecore:<id>` |
| `Permission denied` running `./run.sh` | home directory mounted `noexec` | `REMOTE_DIR=/dev/shm/hw3 bash scripts/fleet.sh ...` |
| `Connection closed by <ip> port 22` right after `Password:` | login failed (typo, too many attempts, access denied). Run `ssh <target>` by hand; set up SSH keys; then rerun only that machine |
| `ModuleNotFoundError: matplotlib` | no longer fatal. Plots are rebuilt on your computer by `summarize.py` |
| compile errors mentioning `rax`/`rcx`/`rdtscp` on an Arm machine | outdated x86-only code: redeploy the current `rob_test.c` (it supports AArch64) |
| `FAILED (all)` for one machine, others continue | SSH, disk, or build problem on that machine | read `data/<m>/ex04_rob/fleet_run_*.log` or run the steps by hand (below) |

Running by hand on one machine (useful for debugging):

```bash
ssh m1
cd ~/CSC591-Project3/<machine>/ex04_rob
make && CORE=2 ./run.sh                      # same as what fleet.sh does
# quick low-effort test:  MIB=256 ITERS=8000 REPS=9 CORE=2 ./run.sh
```

## Definition of done for ex04

- [ ] Smoke test on one machine gives a confirmed knee and a ratio of about 2
- [ ] At least 2 machines (all 8 for the bonus) with >= 3 confirmed runs per configuration (`summarize.py` shows it)
- [ ] Both page settings (4 KiB, huge pages) recorded, or a note on why not
- [ ] Hybrid machines: both core types
- [ ] Machine cards filled in (`data/m*/machine_card.md`, `docs/machines.md`)
- [ ] Estimates compared with published sizes (kept separate); differences explained
- [ ] Counter cross-check done or limitation noted (only after the timing result)
- [ ] Frequency settings, if changed, restored and stated
- [ ] Enclave variant done, or limitation documented
