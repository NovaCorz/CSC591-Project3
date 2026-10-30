# Enclave (SGX) comparison: setup notes

Requirement: run each selected experiment **inside** and **outside** an enclave (for example Intel SGX) on the
**same** enclave-capable machine, compare results and trends, and explain differences and experimental challenges.
Identify the platform and configuration. If enclave support or access is unavailable on a machine, report that
limitation; the ordinary experiments must still meet the two-machines-per-exercise requirement.

## Find out which machines can do it
`bash scripts/fleet.sh status` shows `/dev/sgx_enclave`; each run's `sysinfo/sgx_flags.txt` shows CPU flags. Set `sgx=yes`
in `machines.txt` only for machines where you can actually build and run an enclave.

## Things that differ inside an enclave (document the ones you hit)
- **No `rdtsc` in SGX1 enclaves.** Time outside the enclave around the ECALL; run enough iterations (ms) that the
  ECALL overhead (~10 us) is negligible. SGX2 / newer CPUs may allow `rdtsc`; check CPUID and state what you used.
- **Memory is the EPC.** Buffers live in enclave memory. Client SGX parts have a small EPC (~93 MiB usable): size
  buffers to fit (but ideally still above the LLC) and watch for EPC paging. Server SGX supports much larger EPC.
- **Memory encryption** raises DRAM latency inside the enclave; plateaus move up, structural knees (ROB size, BTB
  capacity) should not. That contrast is a good thing to show.
- **No syscalls inside.** No `mmap`, `getopt`, file I/O in the enclave: allocate from the enclave heap, pass parameters
  through ECALL arguments, return results through ECALL outputs.
- `clflush` and plain loads/stores/jumps work inside enclaves. Hardware performance counters generally do not, which is
  another reason timing is the primary evidence.
- Hybrid CPUs: if the SGX machine is hybrid, run the enclave experiment on both core types.

## Platform choice
Intel SGX SDK, Gramine, or Open Enclave: pick one, record the version and configuration (heap size, SGX1/SGX2, debug
vs release) in `data/<machine>/machine_card.md` and in the report. Keep the shared kernels (the inline-asm loops) identical
to the native versions so differences come from the enclave, not the code.

## Layout
Each exercise has a sibling `exNN_*_sgx/` folder. `scripts/fleet.sh` runs those only on machines with `sgx=yes`.
ex15 (pipeline depth) is analysis of ex03 data, so its enclave comparison comes from `ex03_branch_prediction_sgx`.

## Your machines: which can run enclaves

| Machine(s) | In the CPU spec | What to expect on Linux |
|---|---|---|
| sunbird (Haswell-EP), skylark (EPYC / Zen 2), thunderbird (Arm Neoverse N1) | no SGX | report the limitation; native runs only (SGX is an Intel x86 feature; AMD has SEV, Arm has TrustZone/CCA) |
| charnwood (i7-6700), ookay (i7-7700), upgrade (i7-8700), crux (i7-9700) | yes ("with Intel ME") | older client platforms generally lack Flexible Launch Control, so the in-kernel driver does not enable SGX; they need Intel's out-of-tree driver (`/dev/isgx`), BIOS SGX enabled, and root to install. Small EPC (~93 MiB usable). SGX1 only: no `rdtsc` inside |
| artemisia (2x Xeon Gold 5420+) | yes (128 GB max enclave size) | server platform with Flexible Launch Control: kernel >= 5.11 exposes `/dev/sgx_enclave`; BIOS must enable SGX and allocate EPC. Large EPC, so the big benchmark buffers can fit |

**Verify on each candidate** (no root needed for the first two):
```bash
grep -m1 -ow -E 'sgx|sgx_lc|sgx1|sgx2' /proc/cpuinfo | sort -u   # CPU flags the kernel exposes
ls -l /dev/sgx_enclave /dev/sgx/enclave /dev/isgx 2>&1            # in-kernel / DCAP / legacy driver nodes
dmesg | grep -i sgx                                               # needs root: explains "disabled in BIOS" etc.
```
If the flags are missing, SGX is probably disabled in the BIOS or locked out by the kernel (no `sgx_lc` means the in-kernel
driver will not use it). If you have no root or BIOS access, ask the course staff which machine already has an enclave setup, and
record the limitation for the others. Set `sgx=yes` in `machines.txt` only when a machine passes this check.
