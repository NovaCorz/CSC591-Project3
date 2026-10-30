#!/usr/bin/env python3
"""ex08 analysis: per-access time of one-node-per-page vs packed pointer chases -> TLB miss costs.
usage: analyze.py data.csv [--plot out.png]
Definitions (documented; adjust --l1 if your CPU needs it):
  baseline   P <= 32            everything fits the L1 DTLB
  L1-miss    192 <= P <= 768    beyond a 64-96 entry L1 DTLB, inside a 1024-2048 entry L2 TLB (L2-TLB hit cost)
  rise 2     first P where the cost exceeds the L1-miss cost by 1 ns: the L2 TLB starts to be exceeded (capacity lies between
             the previous grid point and this one; a random chase slightly over capacity still hits part of the time)
  step 2     first P where the translation cost exceeds the L1-miss cost by 4 ns (full page-walk level reached; this is
             up to ~2x above the capacity, because the rise is gradual)
  walk-near  median cost at the 3 grid points starting at step 2: a page walk whose page-table entries are cache-resident
  walk-far   median cost for P >= 32768: page walks whose page-table entries themselves miss the caches"""
import argparse, os, sys
from collections import defaultdict
here = os.path.dirname(os.path.abspath(__file__))
for c in ("../common", "../../common"):
    if os.path.isdir(os.path.join(here, c)):
        sys.path.insert(0, os.path.join(here, c)); break
from analysis_util import read_csv, median, emit, get_plt

ap = argparse.ArgumentParser(); ap.add_argument("csv"); ap.add_argument("--plot")
ap.add_argument("--l1", default="192-768")
a = ap.parse_args()
lo1, hi1 = (int(x) for x in a.l1.split("-"))
meta, rows = read_csv(a.csv)
ghz = float(meta.get("core_ghz", "nan"))
v = defaultdict(list)
for r in rows:
    v[(int(r["P"]), r["layout"])].append(float(r["ns_per_access"]))
T = {k: median(x) for k, x in v.items()}
Ps = sorted({p for p, _ in T})
delta = {p: T[(p, "sparse")] - T[(p, "dense")] for p in Ps}
base = median([delta[p] for p in Ps if p <= 32])
d = {p: delta[p] - base for p in Ps}
l1m = median([d[p] for p in Ps if lo1 <= p <= hi1])
step1 = next((p for p in Ps if l1m > 0.5 and d[p] >= 0.5 * l1m), None)
step2 = next((p for p in Ps if p > (step1 or 0) and d[p] >= l1m + 4.0), None)
rise2 = next((p for p in Ps if p > (step1 or 0) and d[p] >= l1m + 1.0), None)
i2 = Ps.index(step2) if step2 else None
walk = median([d[p] for p in Ps[i2:i2 + 3]]) if step2 else float("nan")      # walk-near
walk_far = median([d[p] for p in Ps if p >= 32768])
huge_kb = int(meta.get("anon_huge_kb", "-1")); thp = meta.get("thp", "?")
dense_huge = huge_kb >= 2048

print(f"core clock estimate: {ghz:.2f} GHz   |   THP setting: {thp}   |   huge-page-backed memory: {huge_kb} kB")
print(f"{'P (pages)':>10}{'dense ns':>11}{'sparse ns':>11}{'translation cost ns':>22}{'cycles':>9}")
for p in Ps:
    print(f"{p:>10}{T[(p, 'dense')]:>11.2f}{T[(p, 'sparse')]:>11.2f}{d[p]:>22.2f}{d[p] * ghz:>9.1f}")
print()
print(f"L1-DTLB miss (L2-TLB hit) cost ~ {l1m:.2f} ns = {l1m * ghz:.1f} cycles   (window P={lo1}..{hi1})")
print(f"page walk, page tables cached  ~ {walk:.2f} ns = {walk * ghz:.1f} cycles   (3 points from the second step, P>={step2})")
print(f"page walk, page tables cold     ~ {walk_far:.2f} ns = {walk_far * ghz:.1f} cycles   (P>=32768)")
print(f"first step up at P ~ {step1}  (L1 DTLB reach in entries: between the previous grid point and this one)")
print(f"L2 TLB starts to be exceeded at P ~ {rise2}  (capacity: between the previous grid point and {rise2})")
print(f"full page-walk level reached at P ~ {step2}")
if not dense_huge:
    print("WARNING: the dense layout was NOT backed by huge pages (THP off), so it also suffers TLB misses at large P and")
    print("         the costs above are underestimates. Enable THP (madvise/always) or note this limitation in the report.")
emit("TLB_L1MISS_NS", f"{l1m:.2f}")
emit("TLB_L1MISS_CYCLES", f"{l1m * ghz:.1f}")
emit("TLB_WALK_NS", f"{walk:.2f}")
emit("TLB_WALK_CYCLES", f"{walk * ghz:.1f}")
emit("TLB_WALK_COLD_NS", f"{walk_far:.2f}")
emit("TLB_WALK_COLD_CYCLES", f"{walk_far * ghz:.1f}")
emit("TLB_STEP1_P", step1 if step1 else "none")
emit("TLB_STEP2_START_P", rise2 if rise2 else "none")
emit("TLB_STEP2_P", step2 if step2 else "none")
emit("DENSE_HUGEPAGES", "yes" if dense_huge else "no")
emit("CORE_GHZ", f"{ghz:.3f}")
emit("STATUS", "ok" if (step2 is not None and walk >= l1m - 0.5 and dense_huge) else "unconfirmed")

plt = get_plt(a.plot)
if plt:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.3))
    ax1.semilogx(Ps, [T[(p, "dense")] for p in Ps], "o-", ms=3, label="dense (few translations)")
    ax1.semilogx(Ps, [T[(p, "sparse")] for p in Ps], "s-", ms=3, label="sparse (one page per node)")
    ax1.set_xlabel("nodes P"); ax1.set_ylabel("ns per access"); ax1.set_title("pointer chase latency"); ax1.grid(alpha=0.3, which="both"); ax1.legend(fontsize=8)
    ax2.semilogx(Ps, [d[p] for p in Ps], "o-", ms=3, color="C3")
    ax2.axhline(l1m, ls=":", color="gray"); ax2.axhline(walk, ls="--", color="gray")   # L2-TLB hit / page walk (cached tables)
    ax2.set_xlabel("pages P"); ax2.set_ylabel("translation cost (ns)"); ax2.set_title("sparse - dense"); ax2.grid(alpha=0.3, which="both")
    fig.tight_layout(); fig.savefig(a.plot, dpi=150); print("plot written to", a.plot)
