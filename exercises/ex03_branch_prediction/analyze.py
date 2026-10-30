#!/usr/bin/env python3
"""ex03 analysis: cost of unpredictable branches; estimates the misprediction penalty (also the input of ex15).
usage: analyze.py data.csv [--plot out.png]"""
import argparse, os, sys
from collections import defaultdict
here = os.path.dirname(os.path.abspath(__file__))
for c in ("../common", "../../common"):
    if os.path.isdir(os.path.join(here, c)):
        sys.path.insert(0, os.path.join(here, c)); break
from analysis_util import read_csv, median, emit, get_plt

ap = argparse.ArgumentParser(); ap.add_argument("csv"); ap.add_argument("--plot"); a = ap.parse_args()
meta, rows = read_csv(a.csv)
ghz = float(meta.get("core_ghz", "nan"))
v, e = defaultdict(list), {}
for r in rows:
    v[r["mode"]].append(float(r["ns_per_elem"])); e[r["mode"]] = float(r["e"])
T = {m: median(x) for m, x in v.items()}
rand = sorted([m for m in T if m.startswith("p") and e[m] >= 0], key=lambda m: e[m])
xs, ys = [e[m] for m in rand], [T[m] for m in rand]

# least-squares line  T = T0 + penalty * e
n = len(xs); mx, my = sum(xs) / n, sum(ys) / n
sxx = sum((x - mx) ** 2 for x in xs); sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
slope = sxy / sxx; icpt = my - slope * mx
ss_res = sum((y - (icpt + slope * x)) ** 2 for x, y in zip(xs, ys)); ss_tot = sum((y - my) ** 2 for y in ys)
r2 = 1 - ss_res / ss_tot if ss_tot else float("nan")
two_point = (T["p50"] - T["sorted"]) / 0.5

print(f"core clock estimate: {ghz:.2f} GHz")
print(f"{'mode':>8}{'mispredict frac e':>20}{'ns / element':>16}")
for m in rand:
    print(f"{m:>8}{e[m]:>20.2f}{T[m]:>16.3f}")
for m in ("sorted", "alt", "per16", "per64"):
    if m in T: print(f"{m:>8}{'(pattern)':>20}{T[m]:>16.3f}")
print()
print(f"linear fit over random data: T = {icpt:.3f} ns + {slope:.2f} ns x e      (R^2 = {r2:.3f})")
print(f"misprediction penalty ~ {slope:.1f} ns = {slope * ghz:.1f} cycles   (two-point estimate (p50-sorted)/0.5: {two_point:.1f} ns = {two_point * ghz:.1f} cycles)")
slow = T["p50"] / T["sorted"]
print(f"random 50% data is {slow:.1f}x slower than sorted data -> branch prediction is {'clearly in use' if slow >= 1.3 else 'NOT visible'}")
gap = T["p50"] - T["sorted"]
remain = {m: (T[m] - T["sorted"]) / gap for m in ("alt", "per16", "per64") if m in T}
for m, f in remain.items():
    print(f"  {m:>6}: {100 * f:4.1f}% of the random-data misprediction cost remains  ->  {'pattern learned by the predictor' if f < 0.25 else 'pattern NOT learned'}")
print("(the penalty above also contains the data-load and compare latency before the branch resolves, so it is an upper bound")
print(" for the front-end pipeline length; see ex15)")
emit("PREDICTION_USED", "yes" if slow >= 1.3 else "no")
emit("T_SORTED_NS", f"{T['sorted']:.3f}")
emit("T_RAND50_NS", f"{T['p50']:.3f}")
emit("SLOWDOWN_RAND50", f"{slow:.2f}")
emit("PENALTY_NS", f"{slope:.2f}")
emit("PENALTY_CYCLES", f"{slope * ghz:.2f}")
emit("PENALTY_CYCLES_TWO_POINT", f"{two_point * ghz:.2f}")
emit("FIT_R2", f"{r2:.3f}")
for m, f in remain.items():
    emit(f"PATTERN_{m.upper()}_COST_REMAINING", f"{f:.3f}")
emit("CORE_GHZ", f"{ghz:.3f}")
emit("STATUS", "ok" if r2 >= 0.9 and slope > 0 else "unconfirmed")

plt = get_plt(a.plot)
if plt:
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(xs, ys, "o", label="random data (fraction p of high values)")
    ax.plot([0, max(xs)], [icpt, icpt + slope * max(xs)], "-", label=f"fit: {slope * ghz:.1f} cycles per misprediction")
    for m, mk in (("sorted", "s"), ("alt", "^"), ("per16", "v"), ("per64", "D")):
        if m in T: ax.plot([0], [T[m]], mk, label=m)
    ax.set_xlabel("expected mispredictions per element, e = min(p, 1-p)"); ax.set_ylabel("ns per element")
    ax.set_title("ex03: cost of unpredictable branches"); ax.grid(alpha=0.3); ax.legend(fontsize=8)
    fig.tight_layout(); fig.savefig(a.plot, dpi=150); print("plot written to", a.plot)
