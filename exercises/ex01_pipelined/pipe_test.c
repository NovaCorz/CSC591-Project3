// pipe_test.c - Is the processor pipelined? (x86-64, Linux, GCC/Clang)
//
// IDEA. A dependent chain of N instructions takes N x latency. If the unit is PIPELINED, several INDEPENDENT chains
// proceed at the same time: K chains finish in about the same time as one, so throughput grows ~K-fold until a hardware
// limit (the unit's latency/throughput ratio, or the machine width).
//   op add_reg : add %r14,%reg   register-source add, latency 1 -> the "one instruction per cycle" reference
//   op add_imm : add $1,%reg     immediate add (newer Intel cores fold dependent ones at rename: faster than 1/cycle)
//   op imul    : imul $3,%reg,%reg   3-cycle latency, 1/cycle throughput on x86: speedup saturating near K=3 is the
//                                    classic signature of a PIPELINED multiplier (new multiply starts every cycle)
// Ratios of throughputs are independent of the core clock, so no counters and no frequency assumptions are needed.
//
// OUTPUT (CSV on stdout): op,k,rep,ns,instrs      ('#' lines = metadata)

#include "bench_common.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__x86_64__)
#error "pipe_test.c is x86-64 only"
#endif

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define GROUPS 16                 // repetitions of the K-instruction group per loop iteration (amortizes the loop branch)

#define OP_add_reg(r) "add %%r14, %%" #r "\n\t"
#define OP_add_imm(r) "add $1, %%" #r "\n\t"
#define OP_imul(r) "imul $3, %%" #r ", %%" #r "\n\t"

// K independent accumulator chains = K different registers (r14 = constant source, r15 = loop counter)
#define B1(O) O(rax)
#define B2(O) B1(O) O(rbx)
#define B3(O) B2(O) O(rcx)
#define B4(O) B3(O) O(rdx)
#define B5(O) B4(O) O(rsi)
#define B6(O) B5(O) O(rdi)
#define B8(O) B6(O) O(r8) O(r9)
#define B12(O) B8(O) O(r10) O(r11) O(r12) O(r13)

#define KERNEL(name, O, B)                                                                                         \
    static void name(uint64_t n) {                                                                                  \
        __asm__ volatile("mov $1, %%r14\n\t"                                                                        \
                         "mov %[n], %%r15\n\t"                                                                      \
                         "1:\n\t"                                                                                   \
                         ".rept " STR(GROUPS) "\n\t" B(O) ".endr\n\t"                                               \
                         "dec %%r15\n\t"                                                                            \
                         "jnz 1b\n\t"                                                                               \
                         :                                                                                          \
                         : [n] "m"(n)                                                                               \
                         : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", \
                           "r15", "cc");                                                                            \
    }

#define ALLK(op) KERNEL(k_##op##_1, OP_##op, B1) KERNEL(k_##op##_2, OP_##op, B2) KERNEL(k_##op##_3, OP_##op, B3) \
    KERNEL(k_##op##_4, OP_##op, B4) KERNEL(k_##op##_5, OP_##op, B5) KERNEL(k_##op##_6, OP_##op, B6)              \
    KERNEL(k_##op##_8, OP_##op, B8) KERNEL(k_##op##_12, OP_##op, B12)
ALLK(add_reg)
ALLK(add_imm)
ALLK(imul)

typedef void (*kfn)(uint64_t);
struct kern { const char *op; int k; kfn fn; };
#define ENT(op, K) { #op, K, k_##op##_##K }
#define ROW(op) ENT(op, 1), ENT(op, 2), ENT(op, 3), ENT(op, 4), ENT(op, 5), ENT(op, 6), ENT(op, 8), ENT(op, 12)
static const struct kern table[] = { ROW(add_reg), ROW(add_imm), ROW(imul) };
#define NK (int)(sizeof(table) / sizeof(table[0]))

int main(int argc, char **argv) {
    int core = -1, reps = 9, c;
    uint64_t total = 16000000;            // instructions per timed run (about 5-20 ms)
    while ((c = getopt(argc, argv, "c:r:n:h")) != -1) {
        if (c == 'c') core = atoi(optarg);
        else if (c == 'r') reps = atoi(optarg);
        else if (c == 'n') total = strtoull(optarg, 0, 10);
        else { fprintf(stderr, "usage: %s [-c core] [-r reps] [-n instructions_per_run]\n", argv[0]); return 1; }
    }
    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }

    printf("# arch=%s core=%d reps=%d instrs_per_run=%lu core_ghz=%.4f\n", BENCH_ARCH, core, reps, (unsigned long)total, core_ghz());
    printf("op,k,rep,ns,instrs\n");
    // rep-major: a burst of external noise hits every kernel once instead of one kernel for all reps
    for (int r = -1; r < reps; r++) {            // r = -1: warm-up pass
        for (int i = 0; i < NK; i++) {
            uint64_t iters = total / (GROUPS * table[i].k);
            double t0 = now_ns();
            table[i].fn(iters);
            double t1 = now_ns();
            if (r >= 0) printf("%s,%d,%d,%.1f,%lu\n", table[i].op, table[i].k, r, t1 - t0, (unsigned long)(iters * GROUPS * table[i].k));
        }
        fflush(stdout);
    }
    return 0;
}
