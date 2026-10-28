// rob_test.c - Reorder-buffer (ROB) size microbenchmark for x86-64 / Linux / GCC or Clang.
//
// IDEA
//   Each loop iteration issues two DRAM-missing loads, A and B, separated by N one-byte NOPs:
//
//        A: load (miss)  -> N x nop -> B: load (miss)
//
//   * If the ROB can hold A, the N NOPs and B (N + 3 entries), B enters the window while A is still
//     outstanding, so both misses overlap:               time/iter ~ 1 x DRAM latency.
//   * If N + 3 > ROB size, B cannot be allocated until A retires, so the misses serialize:
//                                                        time/iter ~ 2 x DRAM latency.
//   The knee of time/iter vs N gives the ROB size:  ROB ~ N_knee + 2  (N_knee = first "slow" N).
//
//   Iterations are chained through a data dependence (the address of A and B in iteration i+1 depends
//   on the value loaded by B in iteration i; the buffer holds zeros, so the address is unchanged).
//   That stops the out-of-order window from overlapping *different* iterations.
//
//   The NOP count is varied WITHOUT JIT: the asm contains a sled of MAX_NOPS NOPs and an indirect jump
//   enters the sled N instructions before its end. (No RWX memory needed -> portable to SGX enclaves.)
//   Written for x86-64 and AArch64 in this one file; the compiler selects the matching kernel.
//
// OUTPUT (stdout, CSV):  nops,rep,tsc_per_iter,ns_per_iter      ('#' lines are metadata)

#include "bench_common.h"   // timing, pinning, flushing helpers for x86-64 and AArch64 (common/include)
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define MAX_NOPS 1024
#define LINE 64UL
#define HUGE_SZ (2UL << 20)

// ---------- the kernel ----------
// offs : 2*iters byte-offsets (multiples of 64) into base; offs[2i] -> load A, offs[2i+1] -> load B
// base : buffer of zeros, much larger than the LLC
// nops : number of filler instructions between A and B (0..MAX_NOPS)
// The same experiment is written twice, once per ISA; the compiler picks the right one. Everything else is shared.
#if defined(__x86_64__)
__attribute__((noinline)) static void rob_kernel(const uint64_t *offs, const char *base,
                                                 uint64_t iters, uint64_t nops) {
    __asm__ volatile(
        "xor   %%r9d, %%r9d            \n\t" // r9 = value loaded by previous B (always 0)
        "lea   3f(%%rip), %%r10        \n\t" // r10 = end of NOP sled
        "sub   %[n], %%r10             \n\t" // r10 = entry point: exactly n NOPs (1 byte each) before label 3
        "1:                            \n\t"
        "mov   (%%rdi), %%rax          \n\t" // offset for A
        "mov   8(%%rdi), %%rcx         \n\t" // offset for B
        "add   %%r9, %%rax             \n\t" // A address depends on previous B (adds 0)
        "add   %%r9, %%rcx             \n\t" // B address depends on previous B (adds 0)
        "mov   (%%rsi,%%rax), %%r8     \n\t" // load A  (DRAM miss)
        "jmp   *%%r10                  \n\t" // enter NOP sled
        ".rept " STR(MAX_NOPS) "       \n\t"
        "nop                           \n\t" // 1-byte NOPs (0x90)
        ".endr                         \n\t"
        "3:                            \n\t"
        "mov   (%%rsi,%%rcx), %%r9     \n\t" // load B  (DRAM miss)
        "add   $16, %%rdi              \n\t"
        "dec   %%rdx                   \n\t"
        "jnz   1b                      \n\t"
        : "+D"(offs), "+d"(iters)
        : "S"(base), [n] "r"(nops)
        : "rax", "rcx", "r8", "r9", "r10", "cc", "memory");
}
#elif defined(__aarch64__)
// Every A64 instruction is 4 bytes, so the entry point is 4*n bytes before the end of the sled.
// Filler instruction: `nop` by default. If a core drops NOPs before the ROB, no knee appears; then rebuild with the
// independent ALU filler:   make CFLAGS_EXTRA=-DROB_FILLER_ALU     (via fleet.sh: MAKE_ARGS="CFLAGS_EXTRA=-DROB_FILLER_ALU")
// and discuss in the report that ALU fillers also occupy scheduler entries, so that knee may be lower than the ROB size.
#ifndef FILLER
#  ifdef ROB_FILLER_ALU
#    define FILLER "add x11, xzr, xzr"
#  else
#    define FILLER "nop"
#  endif
#endif
__attribute__((noinline)) static void rob_kernel(const uint64_t *offs, const char *base,
                                                 uint64_t iters, uint64_t nops) {
    __asm__ volatile(
        "mov   x9, xzr                          \n\t" // x9 = value loaded by previous B (always 0)
        "adr   x10, 3f                          \n\t" // x10 = end of filler sled
        "sub   x10, x10, %[n], lsl #2           \n\t" // x10 = entry point: n instructions (4 bytes each) before label 3
        "1:                                     \n\t"
        "ldr   x4, [%[o]]                       \n\t" // offset for A
        "ldr   x5, [%[o], #8]                   \n\t" // offset for B
        "add   x4, x4, x9                       \n\t" // A address depends on previous B (adds 0)
        "add   x5, x5, x9                       \n\t" // B address depends on previous B (adds 0)
        "ldr   x6, [%[b], x4]                   \n\t" // load A  (DRAM miss)
        "br    x10                              \n\t" // enter the sled
        ".rept " STR(MAX_NOPS) "                \n\t"
        FILLER "                                \n\t"
        ".endr                                  \n\t"
        "3:                                     \n\t"
        "ldr   x9, [%[b], x5]                   \n\t" // load B  (DRAM miss)
        "add   %[o], %[o], #16                  \n\t"
        "subs  %[it], %[it], #1                 \n\t"
        "b.ne  1b                               \n\t"
        : [o] "+r"(offs), [it] "+r"(iters)
        : [b] "r"(base), [n] "r"(nops)
        : "x4", "x5", "x6", "x9", "x10", "x11", "cc", "memory");
}
#endif

// ---------- misc ----------
static uint64_t rng = 88172645463325252ULL;

static void usage(const char *p) {
    fprintf(stderr,
            "usage: %s [-c core] [-m MiB] [-i iters] [-r reps] [-s start] [-e end] [-t step] [-H]\n"
            "  -c core   pin to this logical CPU (default: no pinning)\n"
            "  -m MiB    buffer size, must be >> LLC (default 512)\n"
            "  -i iters  loop iterations per timed run (default 20000)\n"
            "  -r reps   timed repetitions per N (default 15)\n"
            "  -s/-e/-t  sweep NOP count from start to end (<= %d) by step (default 0 800 4)\n"
            "  -H        ask for transparent huge pages (fewer TLB misses); default is 4 KiB pages\n",
            p, MAX_NOPS);
}

int main(int argc, char **argv) {
    int core = -1, hp = 0, reps = 15, warm = 1, s = 0, e = 800, t = 4, c;
    size_t mb = 512;
    uint64_t iters = 20000;
    while ((c = getopt(argc, argv, "c:m:i:r:s:e:t:Hh")) != -1) {
        switch (c) {
        case 'c': core = atoi(optarg); break;
        case 'm': mb = strtoul(optarg, 0, 10); break;
        case 'i': iters = strtoull(optarg, 0, 10); break;
        case 'r': reps = atoi(optarg); break;
        case 's': s = atoi(optarg); break;
        case 'e': e = atoi(optarg); break;
        case 't': t = atoi(optarg); break;
        case 'H': hp = 1; break;
        default: usage(argv[0]); return 1;
        }
    }
    if (e > MAX_NOPS || s < 0 || t < 1) { usage(argv[0]); return 1; }

    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }

    // Buffer: 2 MiB aligned, explicitly written (so no shared zero page), contents all zero.
    size_t size = mb << 20;
    char *raw = mmap(NULL, size + HUGE_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) { perror("mmap"); return 1; }
    char *buf = (char *)(((uintptr_t)raw + HUGE_SZ - 1) & ~(HUGE_SZ - 1));
    madvise(buf, size, hp ? MADV_HUGEPAGE : MADV_NOHUGEPAGE);
    for (size_t i = 0; i < size; i += 4096) ((volatile char *)buf)[i] = 0;

    uint64_t *offs = malloc(2 * iters * sizeof(uint64_t));
    if (!offs) { perror("malloc"); return 1; }
    uint64_t nlines = size / LINE;

    printf("# arch=%s core=%d buf_MiB=%zu iters=%lu reps=%d hugepages=%d tsc_ghz=%.4f\n", BENCH_ARCH, core, mb,
           (unsigned long)iters, reps, hp, tsc_ghz());
    printf("nops,rep,tsc_per_iter,ns_per_iter\n");

    // Repetitions are INTERLEAVED across all N (rep-major order): a burst of external noise (other tenants on a
    // VM, interrupts, frequency changes) then hits every N once instead of corrupting one contiguous block of N.
    // The per-N median over repetitions (analyze_rob.py) discards such bursts. Pass r < 0 are warm-up passes.
    for (int r = -warm; r < reps; r++) {
        for (int n = s; n <= e; n += t) {
            // Fresh random lines every run, then flush them so every load goes to DRAM.
            for (uint64_t i = 0; i < 2 * iters; i++) offs[i] = (xorshift64(&rng) % nlines) * LINE;
            for (uint64_t i = 0; i < 2 * iters; i++) clflush_line(buf + offs[i]);
            mfence();

            double n0 = now_ns();
            uint64_t c0 = tsc_begin();
            rob_kernel(offs, buf, iters, (uint64_t)n);
            uint64_t c1 = tsc_end();
            double n1 = now_ns();

            if (r >= 0) printf("%d,%d,%.2f,%.2f\n", n, r, (double)(c1 - c0) / iters, (n1 - n0) / iters);
        }
        fflush(stdout);
    }
    return 0;
}