# ROB-size experiment (Homework 3, Track B, exercise 4)

## 1. What the experiment does

Each loop iteration issues two DRAM-missing loads, **A** and **B**, with **N one-byte NOPs** between them:

```
A: load (miss)  ->  N x nop  ->  B: load (miss)
```

| Condition | What happens | Time per iteration |
|---|---|---|
| N + 3 <= ROB size | A, the NOPs and B all fit in the ROB, so B issues while A is outstanding and the two misses overlap | ~1 x DRAM latency |
| N + 3 > ROB size | B cannot be allocated until A retires, so the misses serialize | ~2 x DRAM latency |

The knee of time/iteration vs N gives the ROB size: **ROB ~ N_knee + 2**, where N_knee is the first "slow" N.

Design details worth explaining in your report:

- **Dependency chain across iterations.** The addresses of A and B in iteration i+1 depend on the value B loaded in iteration i. The buffer is all zeros, so the address is unchanged, but the chain stops different iterations from overlapping.
- **No JIT.** The asm contains a sled of 1024 NOPs and an indirect `jmp` enters it N bytes before the end. This needs no RWX memory, so the same kernel can go into an SGX enclave.
- **Always DRAM misses.** Random cache lines in a 512 MiB buffer, regenerated and `clflush`ed before every timed run.
- **NOPs as filler** because they take ROB entries but (on most cores) not scheduler entries. Dependent `add`s would fill the scheduler first and give a smaller, misleading knee.
- **Interleaved repetitions.** Repetitions loop over all N (rep-major), so a burst of external noise hits every N once instead of corrupting one block of N; the per-N median then discards it. `analyze_rob.py` additionally requires the slow plateau to *persist* (a short bump that falls back is not a knee), and `run_rob.sh` warns if the fine sweep does not confirm the coarse knee.
- **Timing only.** `lfence; rdtsc` ... `rdtscp; lfence`, plus `clock_gettime` for ns. The TSC is a timestamp clock, not a performance-monitoring counter (PMC). If you are unsure whether the course treats it differently, confirm with the instructor.

## 2. Course requirements that shape this workflow

These come from the instructor's clarifications and are built into the scripts.

**R1. Performance counters cannot be the main evidence.** Establish the ROB size first from the timing sweep alone, without counters. Counters may be used *afterward* to double-check.

- Primary evidence: `rob_test` timing sweeps, the knee found by `analyze_rob.py`, and the plots. `rob_test.c` uses no performance counters.
- Secondary evidence: `perf_check.sh`. It **refuses to run** until the timing-based knee exists for the same CPU/label, and writes `counter_check_SECONDARY.txt`.
- In the report: state your ROB estimate from timing first; present counter data in a separate, clearly labeled "confirmation" subsection. Never derive the estimate from counters. If counters disagree with timing, report and discuss the disagreement.
- If counters are unavailable (VM, `perf_event_paranoid`, missing event names), report that limitation. The primary result is unaffected.

**R2. Fixing the frequency (disabling DVFS) is allowed, but needs coordination, and you must restore the original settings afterward.**

- Fixing frequency is not required. The knee is measured in instructions, so it should not move with frequency; fixing mostly reduces noise and lets you convert TSC ticks to core cycles.
- Before touching anything: get permission from whoever manages the machine, and check nobody else is using it (`who`).
- Use `../common/freq_ctl.sh` (paths in this README are relative to the exercise folder on the machine, `hw3_run/ex04_rob`; `common/` is deployed next to it). `save` snapshots the original settings and refuses to overwrite an existing snapshot, so the originals are never lost. `restore` puts them back and prints the state.
- `run_rob.sh` records the frequency state in `freq_during_run.txt` for every run, and warns at the end if a snapshot still exists (settings not yet restored).
- In the report: state whether frequency was fixed, to what, and that the original settings were restored.

```bash
../common/freq_ctl.sh status
sudo ../common/freq_ctl.sh save
sudo ../common/freq_ctl.sh fix            # performance governor + turbo off (or: fix <KHZ>)
CORE=2 ./run_rob.sh
sudo ../common/freq_ctl.sh restore
```

On a VM without a cpufreq interface the script does nothing; just note that.

**R3. Hybrid CPUs: if you choose such a machine, test both P-cores and E-cores and compare.**

- `run_rob.sh` detects a hybrid CPU and **requires** `LABEL=pcore` or `LABEL=ecore`, checks that `CORE` really is of that type, and reminds you if the other core type has not been run yet.
- `gather_info.sh` records which logical CPUs are P and E (`hybrid_cores.txt`) and the SMT siblings (`smt_siblings.txt`). Use one logical CPU per P-core and keep its sibling idle.

```bash
CORE=0  LABEL=pcore ./run_rob.sh
CORE=20 LABEL=ecore ./run_rob.sh
```

## 3. Files

| File | Purpose |
|---|---|
| `rob_test.c` | harness: C driver + inline-asm kernel (timing only) |
| `Makefile` | builds `rob_test` |
| `run_rob.sh` | **primary** run: system info, coarse + fine sweep, plots; enforces hybrid rule |
| `analyze_rob.py` | finds the knee, prints the estimate, plots (`matplotlib` needed for `--plot`) |
| `../common/gather_info.sh` | (shared) collects CPU/OS/SGX/hybrid/frequency info into text files |
| `../common/freq_ctl.sh` | (shared) save / fix / restore CPU frequency settings (R2; needs root) |
| `perf_check.sh` | **secondary** counter cross-check, gated on the primary result (R1) |

## 4. Prerequisites (per machine)

```bash
gcc --version                    # any gcc/clang for x86-64
python3 -c "import matplotlib"   # only for plots; otherwise copy the CSVs and plot elsewhere
free -h                          # need >= ~600 MiB free
chmod +x *.sh analyze_rob.py     # in case the executable bits were lost when copying/unzipping
```

Use a directory that allows executables (some mounts are `noexec`). Check the machine is idle (`uptime`, `top`) and the SMT sibling of your core is not busy (`cat /sys/devices/system/cpu/smt/active`, `lscpu -e`).

## 5. Workflow on each machine

**Step 1: primary, timing only (R1).**

```bash
make
CORE=2 ./run_rob.sh          # writes results/<host>_cpu2_hp0_<runid>/ ; add LABEL=pcore/ecore on hybrid CPUs
HP=1 CORE=2 ./run_rob.sh     # repeat with transparent huge pages -> ..._hp1_<runid>/
```

Run each configuration **at least 3 separate times** and report the spread. Every run gets its own timestamped directory (`..._<runid>/`) so repeats never overwrite each other; `..._latest` points to the newest one. Manual use:

```bash
./rob_test -c 2 -m 512 -i 20000 -r 15 -s 0 -e 800 -t 4 > coarse.csv
python3 analyze_rob.py coarse.csv --plot coarse.png --title "CPU name"
./rob_test -c 2 -s 200 -e 260 -t 1 -r 30 > fine.csv          # refine around the knee
```

**Step 2: record your timing-based estimate** (the `ESTIMATED ROB SIZE` line in `fine_summary.txt`) before looking at any counters.

**Step 3: secondary counter check (allowed only now).**

```bash
CORE=2 ./perf_check.sh       # same CORE/LABEL/HP as Step 1; uses knee-40 and knee+40 by default
# e.g. EVENTS=cycles,instructions,l1d_pend_miss.pending,l1d_pend_miss.pending_cycles CORE=2 ./perf_check.sh
```

It runs two points that differ only in N and prints counter deltas. If the timing result is right, cycles rise sharply while instructions change only by the extra NOPs. `perf_check.sh` has not been tested against real counters (my development sandbox had none), so expect to adjust event names.

**Step 4: enclave run** (section 9), then repeat on the second machine.

## 6. What to gather from each machine

Copy the whole `results/` directory back. `gather_info.sh` (called by `run_rob.sh`) writes `results/<host>_<label>_hpX_<runid>/sysinfo/` containing:

- `lscpu.txt`, `cpuinfo_core0.txt`, `cpu_model.txt`: CPU model, family/model/stepping, flags. Use them to identify the **microarchitecture** (look the model up on Intel ARK / AMD specs / WikiChip).
- `lscpu_caches.txt`: cache sizes; confirms the 512 MiB buffer is far larger than the LLC.
- `hybrid_cores.txt`, `smt_siblings.txt`, `smt_active.txt`: core types and SMT layout.
- `freq_state.txt`, `governor.txt`, `cpu_max_mhz.txt`, `thp.txt`, `numa.txt`, `meminfo.txt`: conditions to state in the report.
- `tsc_flags.txt`: look for `constant_tsc nonstop_tsc`.
- `sgx_flags.txt`: whether the CPU/OS exposes SGX (for the enclave comparison, or to document the limitation).
- `perf_avail.txt`: whether `perf` exists and `perf_event_paranoid` (for the secondary check).
- `gcc_version.txt`, `uname.txt`, `os_release.txt`: toolchain and kernel.

Also per run: `freq_during_run.txt`, `run_config.txt`, the CSVs, summaries and plots. Look up by hand: the **microarchitecture name** and the **published ROB size** (keep it separate from your measurement in the report).

## 7. Expected results

**Shape of the curve** (time per iteration vs N):

```
time
 2L |                      ________________________   <- serialized misses
    |                     |
    |                     |   <- sharp step, a few NOPs wide
 1L |_____________________|                            <- overlapped misses
    +---------------------+------------------------> N
                        ROB-2
```

- **Low plateau** ~ one DRAM latency: roughly 70-110 ns on desktops, 100-150 ns on servers or VMs (the sandbox test gave ~139 ns).
- **High plateau** ~ twice the low plateau (hi/lo ratio about 1.8-2.1). Below 1.4 means the experiment is not working (section 8).
- **Knee** is sharp, usually within 1-4 NOPs. It may sit a few entries away from the published value (see the calibration note below).

**Estimated knee positions** (from published specs and third-party measurements; expectations to *check*, not results to quote):

| Microarchitecture | Published ROB entries | Expected N_knee |
|---|---|---|
| Intel Sandy / Ivy Bridge | 168 | ~166 |
| Intel Haswell / Broadwell | 192 | ~190 |
| Intel Skylake / Kaby / Coffee / Cascade Lake | 224 | ~222 |
| Intel Ice Lake / Sunny Cove | 352 | ~350 |
| Intel Golden Cove / Raptor Cove (P-cores) | 512 | ~510 |
| Intel Gracemont (E-cores) | 256 | ~254 |
| Intel Skymont (newer E-cores) | 416 (verify) | ~414 |
| Intel Lion Cove (newer P-cores) | 576 (verify) | ~574 |
| AMD Zen / Zen+ | 192 | ~190 |
| AMD Zen 2 | 224 | ~222 |
| AMD Zen 3 | 256 | ~254 |
| AMD Zen 4 | 320 | ~318 |
| AMD Zen 5 | 448 | ~446 |

Possible deviations to discuss: AMD counts retire-queue entries differently (macro-ops, not instructions); another structure (scheduler, load queue) may cap the window first; on hybrid CPUs, P and E cores differ in both plateau latency and knee.

**Sandbox check (what I actually observed, and its limits):** I tested the harness in a shared, single-vCPU virtual machine ("Intel Xeon Processor @ 2.10GHz"; the hypervisor hides the exact model, so I have no published ROB size to compare against). Results:

- When the VM was quiet, a run gave a low plateau of ~150 ns, a high plateau of ~300 ns (ratio ~2.0) and a fine-sweep knee at N=498, so ROB ~ 500. That is plausible for a 512-entry core, but I could not verify the CPU model, and the ~12-entry gap to 512 may be a small systematic offset of the method (or a real difference). **Calibrate the method on a course machine with a well-documented ROB (for example a 224-entry Skylake) and discuss any constant offset in your report.**
- Other runs in the same VM were swamped by noisy-neighbor bursts (slow stretches that look like a knee at the wrong N). The scripts caught these: the fine sweep failed to confirm the coarse knee and `run_rob.sh` printed a warning. Expect much cleaner data on bare-metal course machines; if you do see this, rerun on an idle machine with more `REPS`.
- I did not test `perf_check.sh` against real counters (no `perf` in the sandbox), and I could not test the hybrid or SGX paths.

## 8. Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| No step (ratio < 1.4) | Buffer fits in cache: raise `-m` above 4x LLC. Or ROB > sweep range: raise `-e` (max 1024). |
| Step is smeared / many outliers | Not pinned, other load, frequency scaling, SMT sibling busy. Use `-c`, more `-r` (and, with permission, R2). |
| Plateau levels differ between `HP=0` and `HP=1` | Expected: with 4 KiB pages each load also pays a page walk. The knee position should not change; report both. |
| Knee far below published ROB | Another resource limit hit first. Check that fillers are NOPs, and discuss. |
| `run_rob.sh`: "Hybrid CPU detected" | Set `LABEL=pcore` or `ecore` and pick a CPU of that type from `hybrid_cores.txt`. |
| `perf_check.sh` refuses | Run `run_rob.sh` first (R1). That is intentional. |
| `Permission denied` running `./rob_test` | Directory mounted `noexec`; copy the folder elsewhere. |
| `mmap` fails | Not enough free RAM; lower `-m` (stay well above the LLC). |

## 9. Enclave (SGX) version: notes for your report

The kernel is plain `mov`/`nop`/`jmp` with no counters, so it runs unchanged inside an enclave. What changes:

- **No `rdtsc` in SGX1 enclaves.** Time outside the enclave around the ECALL that runs `rob_kernel`; with 20000 iterations (~ms) the ~10 microsecond ECALL overhead is negligible. Document this choice. (Hardware counters are also generally unusable inside enclaves, another reason the timing method is the primary one.)
- **Memory.** The buffer must live in enclave memory (EPC). Client SGX parts have a small EPC (~93 MiB usable), so use `-m 64` or similar (ideally still larger than the LLC) and watch for EPC paging. Server SGX (Ice Lake and later) supports far larger EPC.
- **Encrypted memory** raises the DRAM latency plateau inside the enclave; the knee position should stay the same. That is the finding to confirm and discuss.
- `clflush` is allowed in enclaves; `getopt`/`mmap` are not, so allocate with the SDK heap and pass N and the iteration count as ECALL arguments.
- Report the SGX platform (Intel SGX SDK / Gramine / Open Enclave), version and configuration (heap size, SGX1 or SGX2).

## 10. Reporting checklist

- CPU model, microarchitecture and core type (P/E on hybrid CPUs) for each of your two machines.
- Method: the diagram in section 1, plus the `rob_kernel` asm snippet (annotate the dependency chain and NOP sled).
- **Primary evidence:** timing sweeps (coarse and fine plots) for both machines side by side, outside and inside the enclave, with repeated-run median and range.
- Estimated ROB size from timing vs published ROB size, clearly separated.
- **Secondary evidence (if used):** counter results in a separate subsection, labeled as confirmation only, with the events used. Or the reason counters were unavailable.
- Frequency handling: whether DVFS was changed, to what, and that the original settings were restored.
- Hybrid machines: both P-core and E-core results and the comparison.
- Explanation of differences between the two machines; unsupported features and access limits.
- AI-use log (Track B).
