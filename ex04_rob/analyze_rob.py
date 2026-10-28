#!/usr/bin/env python3
"""Find the ROB-size knee in rob_test output and (optionally) plot it.

usage: analyze_rob.py data.csv [--plot out.png] [--title "CPU name"]
Prints a summary; the line  KNEE_N=<n>  is machine-readable (used by run_rob.sh).
"""
import argparse, csv, statistics as st, sys
from collections import defaultdict

ap = argparse.ArgumentParser()
ap.add_argument("csv")
ap.add_argument("--plot")
ap.add_argument("--title", default="")
ap.add_argument("--col", default="ns_per_iter", choices=["ns_per_iter", "tsc_per_iter"])
a = ap.parse_args()

meta, rows = [], defaultdict(list)
with open(a.csv) as f:
    lines = [l for l in f if not l.startswith("#") or meta.append(l.strip())]
for r in csv.DictReader(lines):
    rows[int(r["nops"])].append(float(r[a.col]))

ns = sorted(rows)
med = [st.median(rows[n]) for n in ns]
k = max(3, len(ns) // 10)
lo, hi = st.median(med[:k]), st.median(med[-k:])
thr = (lo + hi) / 2

knee = None
# The knee is the first N at which the median is "slow" AND the sweep stays slow afterwards (>= 90% of the
# remaining points). A short noise burst that falls back to the low plateau is therefore not accepted.
for i in range(len(ns) - 3):
    rest = med[i:]
    if med[i] >= thr and sum(m >= thr for m in rest) / len(rest) >= 0.9 and all(m >= thr for m in med[i:i + 4]):
        knee = ns[i]
        break

print("\n".join(meta))
print(f"low plateau  (overlapped misses) : {lo:8.1f} {a.col}")
print(f"high plateau (serialized misses) : {hi:8.1f} {a.col}   ratio hi/lo = {hi / lo:.2f}")
if hi / lo < 1.4 or knee is None:
    print("WARNING: no clear 2x step. ROB may be outside the sweep range, the buffer may fit in cache, "
          "or the process is not pinned / machine is noisy.")
    print("KNEE_N=")
else:
    print(f"first slow N (N_knee)            : {knee}")
    print(f"ESTIMATED ROB SIZE ~ N_knee + 2  : {knee + 2}  (+/- the sweep step; refine with -t 1)")
    print(f"KNEE_N={knee}")

try:
    if a.plot:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
except ImportError:
    print("NOTE: matplotlib is not installed here, so no plot was made. The numbers above are unaffected. "
          "Plots are regenerated on your own computer by: python3 scripts/summarize.py <exercise>")
    a.plot = None

if a.plot:
    q1 = [st.quantiles(rows[n], n=4)[0] if len(rows[n]) > 3 else min(rows[n]) for n in ns]
    q3 = [st.quantiles(rows[n], n=4)[2] if len(rows[n]) > 3 else max(rows[n]) for n in ns]
    plt.figure(figsize=(8, 4.5))
    plt.fill_between(ns, q1, q3, alpha=0.3, label="IQR over repetitions")
    plt.plot(ns, med, ".-", label="median")
    if knee is not None:
        plt.axvline(knee + 2, color="r", ls="--", label=f"ROB ~ {knee + 2}")
    plt.xlabel("NOPs between the two loads (N)")
    plt.ylabel(a.col.replace("_", " "))
    plt.title(a.title or "ROB size experiment")
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(a.plot, dpi=150)
    print("plot written to", a.plot)