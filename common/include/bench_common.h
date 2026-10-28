// bench_common.h - timing / pinning / flushing helpers shared by the C exercises.
// Supports x86-64 and AArch64 (Linux, GCC/Clang). One header, no per-machine scripts.
// Timing only: a cycle/timer counter and clock_gettime. No performance-monitoring counters (course requirement R1).
//
//   x86-64 : tsc_*  = TSC (lfence/rdtsc, rdtscp/lfence); clflush; mfence
//   AArch64: tsc_*  = generic timer cntvct_el0 (+isb). Its frequency (cntfrq_el0, typically 25-50 MHz) is far below the CPU
//            clock, so ticks are coarse: use the ns columns (clock_gettime) for results. Flush = dc civac; fence = dsb sy.
#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H
#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#if defined(__x86_64__)
#define BENCH_ARCH "x86-64"
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
static inline void clflush_line(const void *p) { __asm__ volatile("clflush (%0)" ::"r"(p) : "memory"); }
static inline void mfence(void) { __asm__ volatile("mfence" ::: "memory"); }
static inline double timer_ghz_nominal(void) { return 0.0; }   // x86: calibrate against clock_gettime (tsc_ghz below)
#elif defined(__aarch64__)
#define BENCH_ARCH "aarch64"
static inline uint64_t tsc_begin(void) {
    uint64_t v;
    __asm__ volatile("isb\n\tmrs %0, cntvct_el0" : "=r"(v)::"memory");
    return v;
}
static inline uint64_t tsc_end(void) {
    uint64_t v;
    __asm__ volatile("mrs %0, cntvct_el0\n\tisb" : "=r"(v)::"memory");
    return v;
}
static inline void clflush_line(const void *p) { __asm__ volatile("dc civac, %0" ::"r"(p) : "memory"); }
static inline void mfence(void) { __asm__ volatile("dsb sy" ::: "memory"); }
static inline double timer_ghz_nominal(void) {                 // generic timer frequency in GHz (e.g. 0.025 for 25 MHz)
    uint64_t f;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
    return (double)f / 1e9;
}
#else
#error "bench_common.h supports x86-64 and AArch64 only"
#endif

static inline double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}
static inline int pin_to_cpu(int cpu) {   // returns 0 on success
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    return sched_setaffinity(0, sizeof(set), &set);
}
static inline uint64_t xorshift64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return *state = x;
}
static inline double tsc_ghz(void) {      // timer ticks per ns
#if defined(__aarch64__)
    return timer_ghz_nominal();
#else
    double t0 = now_ns(); uint64_t c0 = tsc_begin();
    usleep(200000);
    uint64_t c1 = tsc_end(); double t1 = now_ns();
    return (double)(c1 - c0) / (t1 - t0);
#endif
}
#endif