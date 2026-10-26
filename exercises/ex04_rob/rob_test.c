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
//   enters the sled N bytes before its end. (No RWX memory needed -> portable to SGX enclaves.)
//
// OUTPUT (stdout, CSV):  nops,rep,tsc_per_iter,ns_per_iter      ('#' lines are metadata)

#define _GNU_SOURCE
#include <getopt.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define MAX_NOPS 1024
#define LINE 64UL
#define HUGE_SZ (2UL << 20)

// ---------- timing / cache helpers (inline asm) ----------
static inline uint64_t tsc_begin(void) {
    uint32_t lo, hi;
    __asm__ volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi)::"memory");
    return ((uint64_t)hi << 32) | lo;
}
static inline uint64_t tsc_end(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtscp\n\tlfence" : "=a"(lo), "=d"(hi)::"rcx", "memory");
    return ((uint64_t)hi << 32) | lo;
}
static inline void clflush(const void *p) { __asm__ volatile("clflush (%0)" ::"r"(p) : "memory"); }
static inline void mfence(void) { __asm__ volatile("mfence" ::: "memory"); }

static inline double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// ---------- the kernel ----------
// offs : 2*iters byte-offsets (multiples of 64) into base; offs[2i] -> load A, offs[2i+1] -> load B
// base : buffer of zeros, much larger than the LLC
// nops : number of NOPs between A and B (0..MAX_NOPS)
__attribute__((noinline)) static void rob_kernel(const uint64_t *offs, const char *base,
                                                 uint64_t iters, uint64_t nops) {
    __asm__ volatile(
        "xor   %%r9d, %%r9d            \n\t" // r9 = value loaded by previous B (always 0)
        "lea   3f(%%rip), %%r10        \n\t" // r10 = end of NOP sled
        "sub   %[n], %%r10             \n\t" // r10 = entry point: exactly n NOPs before label 3
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

// ---------- misc ----------
static uint64_t rng = 88172645463325252ULL;
static inline uint64_t xorshift64(void) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static double tsc_ghz(void) {
    double t0 = now_ns();
    uint64_t c0 = tsc_begin();
    usleep(200000);
    uint64_t c1 = tsc_end();
    double t1 = now_ns();
    return (double)(c1 - c0) / (t1 - t0);
}

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

    if (core >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(core, &set);
        if (sched_setaffinity(0, sizeof(set), &set)) { perror("sched_setaffinity"); return 1; }
    }

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

    printf("# core=%d buf_MiB=%zu iters=%lu reps=%d hugepages=%d tsc_ghz=%.4f\n", core, mb,
           (unsigned long)iters, reps, hp, tsc_ghz());
    printf("nops,rep,tsc_per_iter,ns_per_iter\n");

    // Repetitions are INTERLEAVED across all N (rep-major order): a burst of external noise (other tenants on a
    // VM, interrupts, frequency changes) then hits every N once instead of corrupting one contiguous block of N.
    // The per-N median over repetitions (analyze_rob.py) discards such bursts. Pass r < 0 are warm-up passes.
    for (int r = -warm; r < reps; r++) {
        for (int n = s; n <= e; n += t) {
            // Fresh random lines every run, then flush them so every load goes to DRAM.
            for (uint64_t i = 0; i < 2 * iters; i++) offs[i] = (xorshift64() % nlines) * LINE;
            for (uint64_t i = 0; i < 2 * iters; i++) clflush(buf + offs[i]);
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
