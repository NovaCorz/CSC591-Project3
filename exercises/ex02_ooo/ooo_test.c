// ooo_test.c - Is execution out of order? (x86-64, Linux, GCC/Clang)
//
// IDEA. A load that misses all caches takes ~100 ns (hundreds of cycles). Put F filler instructions right after it:
//   indep : filler `add %r11,%rbx`  does NOT depend on the load  -> an out-of-order core executes it WHILE the load is
//                                   outstanding, so time per iteration stays ~ the miss latency (flat in F)
//   dep   : filler `add %rax,%rbx`  depends on the loaded value  -> it cannot start before the load returns, so the F
//                                   adds (1 cycle each) are added AFTER the miss: time grows linearly in F
// An in-order core would show linear growth for BOTH. The two filler instructions are identical in size and cost; only
// the dependence differs. The next iteration's load address depends on BOTH this load's value and the end of the filler
// chain (all added values are 0, so the address is unchanged): the filler chain is on the critical path in both variants,
// and misses of different iterations cannot overlap. Independent filler therefore costs max(L, F cycles), dependent
// filler costs L + F cycles. The F fillers are entered through an indirect jump into a sled of identical
// 3-byte instructions (no JIT, same technique as ex04).
//
// OUTPUT (CSV on stdout): mode,f,rep,ns_per_iter

#include "bench_common.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#if !defined(__x86_64__)
#error "ooo_test.c is x86-64 only"
#endif

#define XSTR(x) #x
#define STR(x) XSTR(x)
#define MAX_F 512
#define LINE 64UL
#define HUGE_SZ (2UL << 20)

// FILLER is 3 bytes (REX + 01 + modrm) in both variants, so the sled entry is 3*F bytes before its end.
#define KERNEL(name, FILLER_INSN)                                                                              \
    __attribute__((noinline)) static void name(const uint64_t *offs, const char *base, uint64_t iters,         \
                                               uint64_t nbytes) {                                               \
        __asm__ volatile("xor   %%eax, %%eax          \n\t"   /* rax = value of previous load (always 0)  */  \
                         "xor   %%ebx, %%ebx          \n\t"   /* rbx = end of the filler chain (stays 0)  */  \
                         "xor   %%r11d, %%r11d        \n\t"   /* r11 = constant 0, independent of the load */ \
                         "lea   3f(%%rip), %%r10      \n\t"                                                     \
                         "sub   %[nb], %%r10          \n\t"   /* entry point: F fillers before label 3    */  \
                         "1:                          \n\t"                                                     \
                         "mov   (%%rdi), %%rcx        \n\t"   /* offset for this iteration's load         */  \
                         "add   %%rax, %%rcx          \n\t"   /* address depends on previous load (adds 0) */ \
                         "add   %%rbx, %%rcx          \n\t"   /* ... and on the filler chain end (adds 0)  */ \
                         "mov   (%%rsi,%%rcx), %%rax  \n\t"   /* the load: DRAM miss                       */ \
                         "jmp   *%%r10                \n\t"                                                     \
                         ".rept " STR(MAX_F) "        \n\t" FILLER_INSN "\n\t" ".endr\n\t"                      \
                         "3:                          \n\t"                                                     \
                         "add   $8, %%rdi             \n\t"                                                     \
                         "dec   %%rdx                 \n\t"                                                     \
                         "jnz   1b                    \n\t"                                                     \
                         : "+D"(offs), "+d"(iters)                                                              \
                         : "S"(base), [nb] "r"(nbytes)                                                          \
                         : "rax", "rbx", "rcx", "r10", "r11", "cc", "memory");                                  \
    }
KERNEL(k_indep, "add %%r11, %%rbx")
KERNEL(k_dep, "add %%rax, %%rbx")

static uint64_t rng = 88172645463325252ULL;

int main(int argc, char **argv) {
    int core = -1, reps = 9, s = 0, e = 320, t = 8, c;
    size_t mb = 512;
    uint64_t iters = 20000;
    while ((c = getopt(argc, argv, "c:m:i:r:s:e:t:h")) != -1) {
        switch (c) {
        case 'c': core = atoi(optarg); break;
        case 'm': mb = strtoul(optarg, 0, 10); break;
        case 'i': iters = strtoull(optarg, 0, 10); break;
        case 'r': reps = atoi(optarg); break;
        case 's': s = atoi(optarg); break;
        case 'e': e = atoi(optarg); break;
        case 't': t = atoi(optarg); break;
        default: fprintf(stderr, "usage: %s [-c core] [-m MiB] [-i iters] [-r reps] [-s start] [-e end] [-t step]\n", argv[0]); return 1;
        }
    }
    if (e > MAX_F || t < 1) { fprintf(stderr, "max F is %d\n", MAX_F); return 1; }
    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }

    size_t size = mb << 20;
    char *raw = mmap(NULL, size + HUGE_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) { perror("mmap"); return 1; }
    char *buf = (char *)(((uintptr_t)raw + HUGE_SZ - 1) & ~(HUGE_SZ - 1));
    madvise(buf, size, MADV_NOHUGEPAGE);
    for (size_t i = 0; i < size; i += 4096) ((volatile char *)buf)[i] = 0;   // private zero pages (the loaded value must be 0)
    uint64_t *offs = malloc(iters * sizeof(uint64_t));
    uint64_t nlines = size / LINE;

    printf("# arch=%s core=%d buf_MiB=%zu iters=%lu reps=%d core_ghz=%.4f\n", BENCH_ARCH, core, mb, (unsigned long)iters, reps, core_ghz());
    printf("mode,f,rep,ns_per_iter\n");
    for (int r = -1; r < reps; r++) {                   // rep-major; r=-1 is a warm-up pass
        for (int f = s; f <= e; f += t) {
            for (int m = 0; m < 2; m++) {
                for (uint64_t i = 0; i < iters; i++) offs[i] = (xorshift64(&rng) % nlines) * LINE;
                for (uint64_t i = 0; i < iters; i++) clflush_line(buf + offs[i]);
                mfence();
                double t0 = now_ns();
                (m == 0 ? k_indep : k_dep)(offs, buf, iters, 3UL * (uint64_t)f);
                double t1 = now_ns();
                if (r >= 0) printf("%s,%d,%d,%.2f\n", m == 0 ? "indep" : "dep", f, r, (t1 - t0) / iters);
            }
        }
        fflush(stdout);
    }
    return 0;
}
