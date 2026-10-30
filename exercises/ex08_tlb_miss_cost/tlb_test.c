// tlb_test.c - What does a TLB miss cost? (x86-64, Linux)
//
// IDEA. Pointer-chase through P nodes in random order (every access depends on the previous one, so latency is exposed
// and prefetchers cannot help). Two layouts with the SAME number of nodes and the same cache footprint (P cache lines):
//   dense  : nodes packed 64 B apart (P*64 bytes; huge pages requested, so almost no TLB entries are needed)
//   sparse : one node per 4 KiB page (P pages -> P different translations); the node's offset inside the page varies
//            so the P lines spread evenly over the cache sets, like the dense layout
// Per-access time(sparse) - time(dense) isolates the cost of address translation as P grows:
//   P up to the L1 DTLB size : ~0     P beyond it: L1 DTLB miss, L2 TLB (STLB) hit    P beyond the STLB: page walk
// Interpretation caveat: for large P the page-table entries themselves may miss the caches, which is part of the cost.
//
// OUTPUT (CSV on stdout): P,layout,rep,ns_per_access

#include "bench_common.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#if !defined(__x86_64__)
#error "tlb_test.c is x86-64 only"
#endif

#define PAGE 4096UL
#define HUGE_SZ (2UL << 20)
#define MAXP 65536
static const int grid[] = {8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048,
                           3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536};
#define NG (int)(sizeof(grid) / sizeof(grid[0]))

static uint64_t rng = 88172645463325252ULL;

// n dependent loads (n multiple of 8); returns the final pointer so the chain cannot be optimized away
__attribute__((noinline)) static void *chase(void *p, uint64_t n) {
    __asm__ volatile("1:\n\t"
                     ".rept 8\n\t mov (%0), %0\n\t .endr\n\t"
                     "sub $8, %1\n\t"
                     "jnz 1b\n\t"
                     : "+r"(p), "+r"(n)
                     :
                     : "cc", "memory");
    return p;
}

static void *map_region(size_t bytes, int huge) {
    char *raw = mmap(NULL, bytes + HUGE_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) { perror("mmap"); exit(1); }
    char *p = (char *)(((uintptr_t)raw + HUGE_SZ - 1) & ~(HUGE_SZ - 1));
    madvise(p, bytes, huge ? MADV_HUGEPAGE : MADV_NOHUGEPAGE);
    for (size_t i = 0; i < bytes; i += 4096) ((volatile char *)p)[i] = 0;    // fault everything in now
    return p;
}

static long anon_huge_kb(void) {           // how much of this process is really backed by huge pages
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    char line[256]; long kb = -1;
    if (!f) return -1;
    while (fgets(line, sizeof line, f)) if (sscanf(line, "AnonHugePages: %ld kB", &kb) == 1) break;
    fclose(f);
    return kb;
}

static void thp_setting(char *out, size_t n) {
    FILE *f = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    char line[128] = "unknown";
    if (f) { if (!fgets(line, sizeof line, f)) strcpy(line, "unknown"); fclose(f); }
    char *l = strchr(line, '['), *r = l ? strchr(l, ']') : NULL;
    if (l && r) { *r = 0; snprintf(out, n, "%s", l + 1); } else snprintf(out, n, "unknown");
}

int main(int argc, char **argv) {
    int core = -1, reps = 9, c;
    uint64_t steps = 2000000;
    while ((c = getopt(argc, argv, "c:r:s:h")) != -1) {
        if (c == 'c') core = atoi(optarg);
        else if (c == 'r') reps = atoi(optarg);
        else if (c == 's') steps = strtoull(optarg, 0, 10) & ~7UL;
        else { fprintf(stderr, "usage: %s [-c core] [-r reps] [-s accesses_per_run]\n", argv[0]); return 1; }
    }
    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }

    char *sparse = map_region((size_t)MAXP * PAGE, 0);        // 4 KiB pages only
    char *dense = map_region((size_t)MAXP * 64, 1);           // huge pages if the kernel allows
    uint32_t *order = malloc(MAXP * sizeof(uint32_t));
    char thp[64]; thp_setting(thp, sizeof thp);

    printf("# arch=%s core=%d reps=%d steps=%lu thp=%s anon_huge_kb=%ld core_ghz=%.4f\n", BENCH_ARCH, core, reps,
           (unsigned long)steps, thp, anon_huge_kb(), core_ghz());
    printf("P,layout,rep,ns_per_access\n");
    for (int r = -1; r < reps; r++) {                          // rep-major, r=-1 warm-up
        for (int g = 0; g < NG; g++) {
            int P = grid[g];
            for (int i = 0; i < P; i++) order[i] = i;
            for (int i = P - 1; i > 0; i--) { int j = xorshift64(&rng) % (i + 1); uint32_t t = order[i]; order[i] = order[j]; order[j] = t; }
            for (int lay = 0; lay < 2; lay++) {               // 0 = dense, 1 = sparse
                char *base = lay ? sparse : dense;
                #define NODE(i) ((void **)(lay ? base + (size_t)(i) * PAGE + ((size_t)(i) % 64) * 64 : base + (size_t)(i) * 64))
                for (int k = 0; k < P; k++) *NODE(order[k]) = NODE(order[(k + 1) % P]);   // random cyclic chain
                void *p = NODE(order[0]);
                p = chase(p, (uint64_t)((P + 7) & ~7));       // one warm lap
                double t0 = now_ns();
                p = chase(p, steps);
                double t1 = now_ns();
                __asm__ volatile("" ::"r"(p));
                if (r >= 0) printf("%d,%s,%d,%.3f\n", P, lay ? "sparse" : "dense", r, (t1 - t0) / (double)steps);
            }
        }
        fflush(stdout);
    }
    return 0;
}
