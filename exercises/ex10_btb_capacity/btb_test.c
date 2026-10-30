// btb_test.c - How many branches can the BTB (branch target buffer) hold? (x86-64, Linux)
//
// IDEA. Generate a chain of N unconditional jumps; each one jumps to the next, the last one returns to the first
// (through a counted loop). Every executed jump is a TAKEN branch whose target the front end must know early:
//   * if all N branches fit in the BTB, the front end redirects immediately: ~1 taken branch per cycle or two
//   * once N exceeds a BTB level, targets are not found early and each jump pays a re-steer bubble: time per branch steps up
// Branches are spaced S bytes apart (S = 4, 8, 16, 32, 64). The spacing changes how the branches map onto BTB sets and
// how much code the loop touches, so we measure several S: a step that moves with S points at cache/ITLB capacity,
// a step at the same N for every S points at a BTB capacity. The code is generated at run time (mmap + mprotect).
//   S=4 : EB 02 90 90                       (jmp +2 over two padding bytes -> next block)
//   S>=8: E9 rel32 (rel32 = S-5) + padding  (jmp to the next block)
//   end : dec %rdi ; jnz first_block ; ret  (the loop-closing branch is a taken branch too: N+1 taken branches/iteration)
//
// OUTPUT (CSV on stdout): S,N,rep,ns_per_branch

#include "bench_common.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#if !defined(__x86_64__)
#error "btb_test.c is x86-64 only"
#endif

static const int spacings[] = {4, 8, 16, 32, 64};
static const int grid[] = {8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072,
                           4096, 6144, 8192, 12288, 16384, 24576, 32768};
#define NS (int)(sizeof(spacings) / sizeof(spacings[0]))
#define NG (int)(sizeof(grid) / sizeof(grid[0]))
typedef void (*fn_t)(uint64_t);

static fn_t build_chain(size_t n, size_t S) {
    size_t len = n * S + 16, maplen = (len + 4095) & ~4095UL;
    uint8_t *code = mmap(NULL, maplen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) { perror("mmap"); exit(1); }
    memset(code, 0x90, maplen);
    for (size_t i = 0; i < n; i++) {
        uint8_t *p = code + i * S;
        if (S == 4) { p[0] = 0xEB; p[1] = 0x02; }                         // jmp +2: skips the two padding bytes
        else { p[0] = 0xE9; int32_t rel = (int32_t)(S - 5); memcpy(p + 1, &rel, 4); }   // jmp to p + S
    }
    uint8_t *ep = code + n * S;                                            // loop end: dec rdi; jnz code; ret
    ep[0] = 0x48; ep[1] = 0xFF; ep[2] = 0xCF;
    ep[3] = 0x0F; ep[4] = 0x85;
    int32_t rel = (int32_t)(code - (ep + 9));
    memcpy(ep + 5, &rel, 4);
    ep[9] = 0xC3;
    if (mprotect(code, maplen, PROT_READ | PROT_EXEC)) {
        perror("mprotect(PROT_EXEC) (is this directory/system W^X-restricted?)"); exit(1);
    }
    return (fn_t)code;
}

int main(int argc, char **argv) {
    int core = -1, reps = 7, c;
    uint64_t target = 20000000;                    // taken branches per timed run
    while ((c = getopt(argc, argv, "c:r:t:h")) != -1) {
        if (c == 'c') core = atoi(optarg);
        else if (c == 'r') reps = atoi(optarg);
        else if (c == 't') target = strtoull(optarg, 0, 10);
        else { fprintf(stderr, "usage: %s [-c core] [-r reps] [-t taken_branches_per_run]\n", argv[0]); return 1; }
    }
    if (core >= 0 && pin_to_cpu(core)) { perror("sched_setaffinity"); return 1; }

    fn_t fn[NS][NG];
    for (int s = 0; s < NS; s++) for (int g = 0; g < NG; g++) fn[s][g] = build_chain(grid[g], spacings[s]);

    printf("# arch=%s core=%d reps=%d taken_per_run=%lu core_ghz=%.4f\n", BENCH_ARCH, core, reps, (unsigned long)target, core_ghz());
    printf("S,N,rep,ns_per_branch\n");
    for (int r = -1; r < reps; r++) {              // rep-major, r=-1 warm-up
        for (int s = 0; s < NS; s++) {
            for (int g = 0; g < NG; g++) {
                uint64_t per = (uint64_t)grid[g] + 1, iters = target / per;
                if (iters < 2) iters = 2;
                fn[s][g](2);                       // prime the front end / BTB with this exact chain
                double t0 = now_ns();
                fn[s][g](iters);
                double t1 = now_ns();
                if (r >= 0) printf("%d,%d,%d,%.4f\n", spacings[s], grid[g], r, (t1 - t0) / (double)(iters * per));
            }
        }
        fflush(stdout);
    }
    return 0;
}
