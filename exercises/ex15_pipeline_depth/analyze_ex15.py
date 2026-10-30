#!/usr/bin/env python3
"""ex15: pipeline depth from the branch misprediction penalty measured in ex03 (native and, if present, enclave).

usage (repo root):  python3 exercises/ex15_pipeline_depth/analyze_ex15.py
No new measurement: it reads data/<machine>/ex03_branch_prediction[_sgx]/<run>/summary.txt (KEY=value lines).
penalty (cycles) = ns lost per misprediction x core clock. Both estimates from ex03 are used:
  PENALTY_CYCLES            slope of time vs fraction of unpredictable branches (random data, 9 fractions)
  PENALTY_CYCLES_TWO_POINT  (random 50% - sorted) / 0.5
Writes report/tables/ex15_pipeline_depth.csv/.md and report/figures/ex15_pipeline_depth.png.
What the number means: it is the time from fetching a branch to resteering the front end after the branch resolves, i.e. the
front-end pipeline length PLUS the latency of the data the branch waits for (here an L1 load and a compare). So it is an UPPER
bound on the front-end depth; discuss that gap in the report.
"""
import csv, os, re, statistics as st, sys
from collections import defaultdict

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
data = os.path.join(root, "data")
read = lambda p: open(p, errors="replace").read() if os.path.exists(p) else ""
kvs = lambda t: dict(m.groups() for m in re.finditer(r"^([A-Z][A-Z0-9_]*)=(.*)$", t, re.M))

G = defaultdict(list)
for machine in sorted(os.listdir(data)) if os.path.isdir(data) else []:
    for ex in ("ex03_branch_prediction", "ex03_branch_prediction_sgx"):
        exdir = os.path.join(data, machine, ex)
        if not os.path.isdir(exdir):
            continue
        for run in sorted(os.listdir(exdir)):
            rd = os.path.join(exdir, run)
            if run.endswith("_latest") or os.path.islink(rd) or not os.path.isdir(rd):
                continue
            kv = kvs(read(os.path.join(rd, "summary.txt")))
            if kv.get("STATUS") != "ok":
                continue
            cfg = dict(x.split("=", 1) for x in read(os.path.join(rd, "run_config.txt")).split() if "=" in x)
            m = re.search(r"model name\s*:\s*(.*)", read(os.path.join(rd, "sysinfo", "cpu_model.txt")))
            G[(machine, "enclave" if ex.endswith("_sgx") else "native", cfg.get("label", "?"))].append(
                (float(kv["PENALTY_CYCLES"]), float(kv["PENALTY_CYCLES_TWO_POINT"]), float(kv["CORE_GHZ"]), m.group(1).strip() if m else "?"))
if not G:
    sys.exit("no confirmed ex03 runs found: run  bash scripts/fleet.sh all ex03_branch_prediction  first")

rows = []
for (machine, env, label), v in sorted(G.items()):
    pen, two, ghz = [x[0] for x in v], [x[1] for x in v], [x[2] for x in v]
    rows.append({"machine": machine, "cpu_model": v[0][3], "environment": env, "core_label": label, "runs": len(v),
                 "penalty_cycles_median": round(st.median(pen), 1), "penalty_min": round(min(pen), 1), "penalty_max": round(max(pen), 1),
                 "two_point_cycles_median": round(st.median(two), 1), "core_ghz": round(st.median(ghz), 2)})
os.makedirs(os.path.join(root, "report", "tables"), exist_ok=True)
os.makedirs(os.path.join(root, "report", "figures"), exist_ok=True)
base = os.path.join(root, "report", "tables", "ex15_pipeline_depth")
with open(base + ".csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
with open(base + ".md", "w") as f:
    c = list(rows[0]); f.write("| " + " | ".join(c) + " |\n|" + "---|" * len(c) + "\n")
    for r in rows: f.write("| " + " | ".join(str(r[k]) for k in c) + " |\n")
print(read(base + ".md"))
print(f"wrote {base}.csv / .md")
if any(r["runs"] < 3 for r in rows):
    print("NOTE: some configurations have fewer than 3 confirmed ex03 runs")
try:
    import matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
except ImportError:
    sys.exit("matplotlib not installed: table written, plot skipped")
fig, ax = plt.subplots(figsize=(9, 4.5))
lab = [f"{r['machine']}\n{r['environment']}" for r in rows]
ax.bar(range(len(rows)), [r["penalty_cycles_median"] for r in rows],
       yerr=[[r["penalty_cycles_median"] - r["penalty_min"] for r in rows], [r["penalty_max"] - r["penalty_cycles_median"] for r in rows]], capsize=3)
ax.set_xticks(range(len(rows))); ax.set_xticklabels(lab, fontsize=7)
ax.set_ylabel("misprediction penalty (cycles)"); ax.set_title("ex15: branch misprediction penalty (upper bound for pipeline depth)")
ax.grid(alpha=0.3, axis="y"); fig.tight_layout()
out = os.path.join(root, "report", "figures", "ex15_pipeline_depth.png"); fig.savefig(out, dpi=150); print("wrote", out)
