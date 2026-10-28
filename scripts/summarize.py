#!/usr/bin/env python3
"""Aggregate one exercise's results across all machines.

usage (from the hw3/ folder):  python3 scripts/summarize.py ex04_rob

Reads   data/<machine>/<exercise>/<run dir>/   (run dirs written by run_rob.sh; *_latest symlinks are skipped)
Writes  report/tables/<exercise>_summary.csv and .md      one row per machine + core label + page setting
        report/figures/<exercise>_overlay.png             all machines' fine sweeps on one normalized plot

Only runs whose fine sweep CONFIRMED a knee count toward the estimate (see analyze_rob.py); unconfirmed runs are
counted separately so noisy runs are visible instead of silently dropped.
"""
import csv, os, re, statistics as st, sys
from collections import defaultdict

ex = sys.argv[1] if len(sys.argv) > 1 else "ex04_rob"
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
data = os.path.join(root, "data")


def read(path):
    try:
        return open(path, errors="replace").read()
    except OSError:
        return ""


def num(pattern, text):
    m = re.search(pattern, text)
    return float(m.group(1)) if m else None


# Raw CSVs are the source of truth. If a machine had no matplotlib (or a run aborted after measuring), rebuild the
# summaries and plots here, on your own computer.
import subprocess
ANALYZER = os.path.join(root, "exercises", ex, "analyze_rob.py")
def rebuild(rd, run, machine, label, hp):
    for kind in ("coarse", "fine"):
        csvp = os.path.join(rd, f"{kind}.csv")
        if not os.path.exists(csvp):
            continue
        summ, png = os.path.join(rd, f"{kind}_summary.txt"), os.path.join(rd, f"{kind}.png")
        need_summary = not read(summ).strip() or "KNEE_N=" not in read(summ)
        need_png = not os.path.exists(png)
        if (need_summary or need_png) and os.path.exists(ANALYZER):
            cmd = [sys.executable, ANALYZER, csvp, "--title", f"{machine}/{label}/hp{hp} {kind} sweep"]
            if need_png:
                cmd += ["--plot", png]
            out = subprocess.run(cmd, capture_output=True, text=True).stdout
            if need_summary:
                open(summ, "w").write(out)

groups = defaultdict(list)  # (machine, label, hp) -> list of run dicts
for machine in sorted(os.listdir(data)) if os.path.isdir(data) else []:
    exdir = os.path.join(data, machine, ex)
    if not os.path.isdir(exdir):
        continue
    for run in sorted(os.listdir(exdir)):
        rd = os.path.join(exdir, run)
        if run.endswith('_latest') or os.path.islink(rd) or not os.path.isdir(rd) or not os.path.exists(os.path.join(rd, "run_config.txt")):
            continue
        cfg = dict(kv.split("=", 1) for kv in read(os.path.join(rd, "run_config.txt")).split() if "=" in kv)
        rebuild(rd, run, machine, cfg.get("label", "?"), cfg.get("hp", "?"))
        fine = read(os.path.join(rd, "fine_summary.txt"))
        model = re.search(r"model name\s*:\s*(.*)", read(os.path.join(rd, "sysinfo", "cpu_model.txt")))
        groups[(machine, cfg.get("label", "?"), cfg.get("hp", "?"))].append({
            "dir": rd, "run": cfg.get("run", run), "model": model.group(1).strip() if model else "?",
            "rob": num(r"ESTIMATED ROB SIZE[^:]*:\s*(\d+)", fine),
            "knee": num(r"KNEE_N=(\d+)", fine),
            "low": num(r"low plateau[^:]*:\s*([\d.]+)", fine),
            "ratio": num(r"ratio hi/lo = ([\d.]+)", fine),
        })

if not groups:
    sys.exit(f"no runs found under {data}/<machine>/{ex}/ (run: scripts/fleet.sh all {ex})")

rows = []
for (machine, label, hp), runs in sorted(groups.items()):
    ok = [r for r in runs if r["rob"] is not None]
    robs = [r["rob"] for r in ok]
    rows.append({
        "machine": machine, "cpu_model": runs[0]["model"], "core_label": label, "hugepages": hp,
        "runs_total": len(runs), "runs_confirmed": len(ok),
        "rob_median": st.median(robs) if robs else "", "rob_min": min(robs) if robs else "",
        "rob_max": max(robs) if robs else "",
        "low_plateau_ns": round(st.median([r["low"] for r in ok if r["low"]]), 1) if ok else "",
        "hi_lo_ratio": round(st.median([r["ratio"] for r in ok if r["ratio"]]), 2) if ok else "",
    })

os.makedirs(os.path.join(root, "report", "tables"), exist_ok=True)
os.makedirs(os.path.join(root, "report", "figures"), exist_ok=True)
base = os.path.join(root, "report", "tables", f"{ex}_summary")
with open(base + ".csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0]))
    w.writeheader()
    w.writerows(rows)
with open(base + ".md", "w") as f:
    cols = list(rows[0])
    f.write("| " + " | ".join(cols) + " |\n|" + "---|" * len(cols) + "\n")
    for r in rows:
        f.write("| " + " | ".join(str(r[c]) for c in cols) + " |\n")
print(read(base + ".md"))
print(f"wrote {base}.csv / .md")
bad = [r for r in rows if r["runs_confirmed"] < 3]
if bad:
    print("NOTE: fewer than 3 confirmed runs for:", ", ".join(f"{r['machine']}/{r['core_label']}/hp{r['hugepages']}" for r in bad))

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit("matplotlib not installed: table written, overlay plot skipped")

plt.figure(figsize=(9, 5))
for (machine, label, hp), runs in sorted(groups.items()):
    r = next((r for r in runs if r["knee"] is not None), None)
    if not r or not os.path.exists(os.path.join(r["dir"], "fine.csv")):
        continue
    by = defaultdict(list)
    lines = [l for l in open(os.path.join(r["dir"], "fine.csv")) if not l.startswith("#")]
    for row in csv.DictReader(lines):
        by[int(row["nops"])].append(float(row["ns_per_iter"]))
    ns = sorted(by)
    med = [st.median(by[n]) for n in ns]
    k = max(3, len(ns) // 10)
    lo = st.median(med[:k])
    plt.plot([n - r["knee"] for n in ns], [m / lo for m in med], ".-", label=f"{machine}/{label}/hp{hp}")
plt.axhline(1, color="gray", lw=0.5)
plt.axhline(2, color="gray", lw=0.5, ls="--")
plt.xlabel("N minus knee position (NOPs)")
plt.ylabel("time per iteration / low plateau")
plt.title(f"{ex}: fine sweeps, normalized (knee at 0)")
plt.legend(fontsize=7)
plt.grid(alpha=0.3)
plt.tight_layout()
out = os.path.join(root, "report", "figures", f"{ex}_overlay.png")
plt.savefig(out, dpi=150)
print("wrote", out)