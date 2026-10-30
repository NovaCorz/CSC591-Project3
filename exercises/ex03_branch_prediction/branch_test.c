// branch_test.c - Does the processor use branch prediction? How much does a misprediction cost? (x86-64, Linux)
//
// IDEA. One data-dependent conditional branch per array element, written in inline asm so the compiler cannot turn it
// into a conditional move:      if (data[i] >= 128) sum += data[i];
// The work per element is the same for every data set; only the PREDICTABILITY of the branch differs:
//   pNN     i.i.d. random data, a fraction NN% of elements >= 128. Best possible predictor mispredicts min(p,1-p) of them,
//           so time per element should grow LINEARLY with that fraction; the slope is the cost of one misprediction.
//   sorted  same data as p50 but sorted: one transition in the whole array -> ~0 mispredictions
//   alt     50,200,50,200...             period 2
//   per16   a random 16-element pattern repeated    period 16
//   per64   a random 64-element pattern repeated    period 64
// A history-based predictor learns the periodic patterns (fast, like `sorted`); random data cannot be learned.
// The array is 4 MiB and each pass uses fresh random data, so the predictor cannot memorize it.
//
// OUTPUT (CSV on stdout): mode,e,rep,ns_per_elem     (e = expected mispredictions per element for an ideal predictor,
//                                                      -1 for the periodic/sorted modes: not used in the regression)

#include "bench_common.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__x86_64__)
#error "branch_test.c is x86-64 only"
#endif

static uint64_t rng = 88172645463325252ULL;
static volatile uint64_t sink;

// the measured kernel: the branch is `jb` on the compare
__attribute__((noinline)) static uint64_t kernel(const uint8_t *d, uint64_t n) {
    uint64_t sum;
    __asm__ volatile("xor   %%eax, %%eax        \n\t"
                     "1:                        \n\t"
                     "movzbl (%[d]), %%ecx      \n\t"
                     "cmp   $128, %%ecx         \n\t"
                     "jb    2f                  \n\t"   // data-dependent branch (taken when value < 128)
                     "add   %%rcx, %%rax        \n\t"   // only executed when value >= 128
                     "2:                        \n\t"
                     "inc   %[d]                \n\t"
                     "dec   %[n]                \n\t"
                     "jnz   1b                  \n\t"
                     : "=&a"(sum), [d] "+r"(d), [n] "+r"(n)
                     :
                     : "rcx", "cc", "memory");
    return sum;
}

struct mode { const char *name; int pct; double e; };   // pct >= 0: i.i.d. with pct% high values; pct < 0: special
static const struct mode modes[] = {
    {"p00", 0, 0.00}, {"p01", 1, 0.01}, {"p02", 2, 0.02}, {"p05", 5, 0.05}, {"p10", 10, 0.10}, {"p20", 20, 0.20},
    {"p30", 30, 0.30}, {"p40", 40, 0.40}, {"p50", 50, 0.50},
    {"sorted", -1, -1}, {"alt", -2, -1}, {"per16", -3, -1}, {"per64", -4, -1},
};
#define NM (int)(sizeof(modes) / sizeof(modes[0]))

static void fill(uint8_t *d, uint64_t n, int pct) {
    if (pct >= 0 || pct == -1) {                       // i.i.d. (or sorted version of p50)
        int p = pct >= 0 ? pct : 50;
        uint64_t thr = (uint64_t)((double)p / 100.0 * 4294967296.0), hi = 0;
        for (uint64_t i = 0; i < n; i++) {
            int high = (xorshift64(&rng) & 0xFFFFFFFFULL) < thr;
            d[i] = high ? 200 : 50;
            hi += high;
        }
        if (pct == -1) { memset(d, 50, n - hi); memset(d + (n - hi), 200, hi); }   // sorted
    } else if (pct == -2) {
        for (uint64_t i = 0; i < n; i++) d[i] = (i & 1) ? 200 : 50;
    } else {
        int period = pct == -3 ? 16 : 64;
        uint8_t pat[64];
        for (int i = 0; i < period; i++) pat[i] = (xorshift64(&rng) & 1) ? 200 : 50;
        for (uint64_t i = 0; i < n; i++) d[i] = pat[i % period];
    }
}

int main(int argc, char **argv) {
    int core = -1, reps = 9, c;
    uint64_t n = 1UL << 22;
    while ((c = getopt(argc, argv, "c:r:n:h")) != -1) {
        if (c == 'c') core = atoi(optarg);
        else if (c == 'r') reps = atoi(optarg);
        else if (c == 'n') n = strtoull(optarg, 0, 10);
        else { fprintf(stderr, "usage: %s [-c core] [-r reps] [-n elements]\n", argv[0]); return 1; }
    }
    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }
    uint8_t *d = malloc(n);
    if (!d) { perror("malloc"); return 1; }

    printf("# arch=%s core=%d elements=%lu reps=%d core_ghz=%.4f\n", BENCH_ARCH, core, (unsigned long)n, reps, core_ghz());
    printf("mode,e,rep,ns_per_elem\n");
    for (int r = -1; r < reps; r++) {                  // rep-major, r=-1 warm-up
        for (int m = 0; m < NM; m++) {
            fill(d, n, modes[m].pct);
            double t0 = now_ns();
            sink = kernel(d, n);
            double t1 = now_ns();
            if (r >= 0) printf("%s,%.2f,%d,%.4f\n", modes[m].name, modes[m].e, r, (t1 - t0) / (double)n);
        }
        fflush(stdout);
    }
    return 0;
}
