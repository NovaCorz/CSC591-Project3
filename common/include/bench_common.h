// bench_common.h - timing / pinning / flushing helpers shared by the C exercises (x86-64, Linux, GCC/Clang).
// Timing only: TSC and clock_gettime. No performance-monitoring counters (course requirement R1).
#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H
#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

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
static inline double tsc_ghz(void) {      // TSC ticks per ns, calibrated against CLOCK_MONOTONIC (~0.2 s)
    double t0 = now_ns(); uint64_t c0 = tsc_begin();
    usleep(200000);
    uint64_t c1 = tsc_end(); double t1 = now_ns();
    return (double)(c1 - c0) / (t1 - t0);
}
#endif
