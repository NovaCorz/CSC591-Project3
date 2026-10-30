#!/usr/bin/env python3
"""ex01 analysis: throughput speedup of K independent chains vs one chain, per operation.
usage: analyze.py data.csv [--plot out.png]   Prints a table and KEY=value result lines (see scripts/collect.py)."""
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
ips = defaultdict(list)                       # (op,k) -> instructions per ns, one value per repetition
for r in rows:
    ips[(r["op"], int(r["k"]))].append(float(r["instrs"]) / float(r["ns"]))
med = {k: median(v) for k, v in ips.items()}
ops = [o for o in ("add_reg", "add_imm", "imul") if (o, 1) in med]
ks = sorted({k for (_, k) in med})
speed = {(o, k): med[(o, k)] / med[(o, 1)] for o in ops for k in ks if (o, k) in med}

print(f"core clock estimate (dependent register-add chain): {ghz:.2f} GHz")
print("K    " + "".join(f"{o + ' speedup':>18}" for o in ops) + f"{'add_reg IPC':>14}{'imul IPC':>11}")
for k in ks:
    print(f"{k:<5}" + "".join(f"{speed[(o, k)]:>18.2f}" for o in ops)
          + f"{med[('add_reg', k)] / ghz:>14.2f}{med[('imul', k)] / ghz:>11.2f}")

def sat(op):                                   # smallest K reaching 90% of the maximum speedup
    m = max(speed[(op, k)] for k in ks)
    return min(k for k in ks if speed[(op, k)] >= 0.9 * m), m

k_add, m_add = sat("add_reg"); k_mul, m_mul = sat("imul")
lat = ghz / med[("imul", 1)]                   # cycles per dependent imul = ns per imul * GHz
fold = med[("add_imm", 1)] / med[("add_reg", 1)]
k1 = ips[("add_reg", 1)]; spread = (max(k1) - min(k1)) / median(k1)
print()
print(f"add_reg: {m_add:.1f}x speedup, saturating at K={k_add}  -> machine sustains ~{m_add:.1f} independent adds per cycle")
print(f"imul   : {m_mul:.1f}x speedup, saturating at K={k_mul}  -> dependent imul latency ~{lat:.1f} cycles, issue rate ~1/cycle")
print(f"add_imm vs add_reg at K=1: {fold:.2f}x  ({'immediate adds are folded/accelerated at rename' if fold > 1.5 else 'no special handling of immediate adds'})")
pipelined = "yes" if m_mul >= 2.0 else "no"
emit("PIPELINED", pipelined)
emit("IMUL_SPEEDUP_MAX", f"{m_mul:.2f}")
emit("IMUL_SATURATION_K", k_mul)
emit("IMUL_LATENCY_CYCLES", f"{lat:.2f}")
emit("ADD_PEAK_IPC", f"{m_add:.2f}")
emit("ADD_SATURATION_K", k_add)
emit("ADD_IMM_FOLD_FACTOR", f"{fold:.2f}")
emit("CORE_GHZ", f"{ghz:.3f}")
emit("K1_SPREAD", f"{spread:.3f}")
emit("STATUS", "ok" if spread <= 0.2 else "unconfirmed")

plt = get_plt(a.plot)
if plt:
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for o in ops:
        ax.plot(ks, [speed[(o, k)] for k in ks], "o-", label=o)
    ax.plot(ks, ks, "k:", lw=0.8, label="ideal (K x)")
    ax.set_xlabel("independent chains K"); ax.set_ylabel("throughput vs 1 chain")
    ax.set_title("ex01: speedup of independent chains"); ax.grid(alpha=0.3); ax.legend()
    fig.tight_layout(); fig.savefig(a.plot, dpi=150); print("plot written to", a.plot)
