#!/usr/bin/env python3
"""ex02 analysis: time per iteration vs number of filler instructions F after a cache-missing load,
independent filler vs filler that depends on the load.   usage: analyze.py data.csv [--plot out.png]"""
import argparse, os, sys
from collections import defaultdict
here = os.path.dirname(os.path.abspath(__file__))
for c in ("../common", "../../common"):
    if os.path.isdir(os.path.join(here, c)):
        sys.path.insert(0, os.path.join(here, c)); break
from analysis_util import read_csv, median, emit, get_plt

ap = argparse.ArgumentParser(); ap.add_argument("csv"); ap.add_argument("--plot"); ap.add_argument("--fref", type=int, default=128)
a = ap.parse_args()
meta, rows = read_csv(a.csv)
ghz = float(meta.get("core_ghz", "nan"))
v = defaultdict(list)
for r in rows:
    v[(r["mode"], int(r["f"]))].append(float(r["ns_per_iter"]))
T = {k: median(x) for k, x in v.items()}
fs = sorted({f for (_, f) in T})
fref = min(fs, key=lambda f: abs(f - a.fref))
L = T[("indep", fs[0])]

print(f"core clock estimate: {ghz:.2f} GHz   |   miss latency (F=0): {L:.1f} ns")
print(f"{'F':>5}{'independent filler':>22}{'dependent filler':>20}{'hidden by OoO':>16}")
for f in fs:
    if f % 32 == 0 or f == fref:
        print(f"{f:>5}{T[('indep', f)]:>20.1f}ns{T[('dep', f)]:>18.1f}ns{T[('dep', f)] - T[('indep', f)]:>14.1f}ns")

# robust estimates: least-squares slope of the dependent curve over ALL F (ns per filler), and window averages for the
# independent curve (single points are noisy because the miss latency itself wobbles by +-10 ns)
n = len(fs); mx = sum(fs) / n; my = sum(T[("dep", f)] for f in fs) / n
slope_ns = sum((f - mx) * (T[("dep", f)] - my) for f in fs) / sum((f - mx) ** 2 for f in fs)
dep_cost = slope_ns * fref
slope_cyc = slope_ns * ghz
low = [T[("indep", f)] for f in fs if f <= 32]
near = [T[("indep", f)] for f in fs if abs(f - fref) <= 32]
indep_cost = sum(near) / len(near) - sum(low) / len(low)
ooo = "yes" if dep_cost > 0 and indep_cost <= 0.25 * dep_cost else "no"
rise = next((f for f in fs if f > fs[0] and all(T[("indep", g)] > 1.10 * L for g in fs if g >= f)), None)
print()
print(f"dependent filler: {slope_ns:.3f} ns per filler = {slope_cyc:.2f} cycles each (regression over all F)  -> at F={fref} it adds {dep_cost:.1f} ns,")
print(f"                  independent filler adds {indep_cost:.1f} ns  ->  {100 * (1 - max(indep_cost, 0) / dep_cost):.0f}% of the filler work was hidden under the miss")
print("independent curve starts rising at F =", rise if rise else f"(not within F<={fs[-1]}: the window is larger than the sweep)",
      "  (compare with the ROB size from ex04)")
emit("OUT_OF_ORDER", ooo)
emit("MISS_LATENCY_NS", f"{L:.1f}")
emit("DEP_COST_NS", f"{dep_cost:.1f}")
emit("INDEP_COST_NS", f"{indep_cost:.1f}")
emit("DEP_CYCLES_PER_FILLER", f"{slope_cyc:.2f}")
emit("INDEP_RISE_F", rise if rise else "none")
emit("CORE_GHZ", f"{ghz:.3f}")
emit("STATUS", "ok" if dep_cost > 5 else "unconfirmed")

plt = get_plt(a.plot)
if plt:
    fig, ax = plt.subplots(figsize=(7.5, 4.5))
    ax.plot(fs, [T[("indep", f)] for f in fs], "o-", ms=3, label="independent filler (does not wait for the load)")
    ax.plot(fs, [T[("dep", f)] for f in fs], "s-", ms=3, label="dependent filler (waits for the load)")
    ax.set_xlabel("filler instructions after the load (F)"); ax.set_ylabel("ns per iteration")
    ax.set_title("ex02: out-of-order execution under a cache miss"); ax.grid(alpha=0.3); ax.legend(fontsize=8)
    fig.tight_layout(); fig.savefig(a.plot, dpi=150); print("plot written to", a.plot)
