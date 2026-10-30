#!/usr/bin/env python3
"""ex10 analysis: time per taken branch vs number of chained branches N, for several branch spacings S.
Detects the steps (a step = the time per branch rises >= 25% and stays there). usage: analyze.py data.csv [--plot out.png]"""
import argparse, os, sys
from collections import defaultdict
here = os.path.dirname(os.path.abspath(__file__))
for c in ("../common", "../../common"):
    if os.path.isdir(os.path.join(here, c)):
        sys.path.insert(0, os.path.join(here, c)); break
from analysis_util import read_csv, median, emit, get_plt

ap = argparse.ArgumentParser(); ap.add_argument("csv"); ap.add_argument("--plot"); ap.add_argument("--thr", type=float, default=1.25)
a = ap.parse_args()
meta, rows = read_csv(a.csv)
ghz = float(meta.get("core_ghz", "nan"))
v = defaultdict(list)
for r in rows:
    v[(int(r["S"]), int(r["N"]))].append(float(r["ns_per_branch"]))
T = {k: median(x) for k, x in v.items()}
Ss = sorted({s for s, _ in T}); Ns = sorted({n for _, n in T})

def find_steps(ns, y, thr):
    out, base, j = [], y[0], 1
    while j < len(ns):
        nxt_ok = j + 1 >= len(ns) or y[j + 1] >= thr * base * 0.95
        if y[j] >= thr * base and nxt_ok:
            out.append((ns[j - 1], y[j] / base))      # knee = last N still on the old level
            base = y[j]
        j += 1
    return out

print(f"core clock estimate: {ghz:.2f} GHz   (1 cycle = {1 / ghz:.3f} ns)")
print("time per taken branch, in CYCLES (median of repetitions)")
print(f"{'N':>6}" + "".join(f"   S={s:<4}" for s in Ss))
for n in Ns:
    print(f"{n:>6}" + "".join(f"{T[(s, n)] * ghz:>9.2f}" for s in Ss))
print()
knees, big = {}, {}
for s in Ss:
    st = find_steps(Ns, [T[(s, n)] for n in Ns], a.thr)
    knees[s] = st
    big[s] = [n for n, ratio in st if ratio >= 1.5]
    print(f"S={s:<3} steps at N = " + (", ".join(f"{n} (x{r:.2f})" for n, r in st) if st else "none"))

last_big = {s: big[s][-1] for s in Ss if big[s]}
core_spacings = [s for s in (4, 8, 16, 32) if s in last_big]
est = median([last_big[s] for s in core_spacings]) if core_spacings else None
agree = bool(core_spacings) and max(last_big[s] for s in core_spacings) <= 1.5 * min(last_big[s] for s in core_spacings)
base_c = T[(16, Ns[0])] * ghz if (16, Ns[0]) in T else float("nan")
far_c = median([T[(s, Ns[-1])] for s in Ss]) * ghz
print()
print(f"first plateau (small N, S=16): {base_c:.2f} cycles per taken branch;  last plateau (N={Ns[-1]}): ~{far_c:.1f} cycles")
if est:
    print(f"largest big step (>=1.5x) at N ~ {est:.0f} branches {'and it is at about the same N for every spacing 4..32 -> a BTB capacity' if agree else 'but it moves with the spacing -> not a pure BTB capacity (cache/ITLB/set mapping effects); report per spacing'}")
print("steps at small N that DO move with S point at set-mapping limits or fetch-window limits; steps whose N is constant across")
print("S point at capacity. Compare with the published BTB size in the report (kept separate).")
for s in Ss:
    emit(f"KNEES_S{s}", ",".join(str(n) for n, _ in knees[s]) or "none")
    emit(f"LAST_BIG_KNEE_S{s}", last_big.get(s, "none"))
emit("BTB_CAPACITY_EST", f"{est:.0f}" if est else "none")
emit("BTB_AGREE_ACROSS_S", "yes" if agree else "no")
emit("BASE_CYCLES_PER_BRANCH", f"{base_c:.2f}")
emit("FAR_CYCLES_PER_BRANCH", f"{far_c:.1f}")
emit("CORE_GHZ", f"{ghz:.3f}")
emit("STATUS", "ok" if est is not None and sum(1 for s in Ss if knees[s]) >= 3 else "unconfirmed")

plt = get_plt(a.plot)
if plt:
    fig, ax = plt.subplots(figsize=(8, 4.8))
    for s in Ss:
        ax.semilogx(Ns, [T[(s, n)] * ghz for n in Ns], "o-", ms=3, label=f"spacing S={s} B")
    ax.set_xlabel("number of chained taken branches N"); ax.set_ylabel("cycles per taken branch")
    ax.set_title("ex10: BTB capacity"); ax.grid(alpha=0.3, which="both"); ax.legend(fontsize=8)
    fig.tight_layout(); fig.savefig(a.plot, dpi=150); print("plot written to", a.plot)
