#!/usr/bin/env python3
"""Aggregate one exercise across machines and runs (for ex01, ex02, ex03, ex08, ex10; ex04 uses scripts/summarize.py).

usage (from the repo root):  python3 scripts/collect.py <exercise_folder_name> [--min-runs 3]

Reads   data/<machine>/<exercise>/<run dir>/summary.txt   (KEY=value lines written by the exercise's analyze.py)
Writes  report/tables/<exercise>_summary.csv and .md      one row per machine + core label + page setting
Raw CSVs are the source of truth: a missing summary.txt / plot.png (e.g. a machine without matplotlib) is rebuilt here by
running the exercise's analyze.py on data.csv. A run counts as confirmed only when its summary says STATUS=ok.
"""
import argparse, csv, os, re, statistics as st, subprocess, sys
from collections import Counter, defaultdict

ap = argparse.ArgumentParser(); ap.add_argument("exercise"); ap.add_argument("--min-runs", type=int, default=3)
a = ap.parse_args()
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
data = os.path.join(root, "data")
analyzer = os.path.join(root, "exercises", a.exercise, "analyze.py")


def read(p):
    try:
        return open(p, errors="replace").read()
    except OSError:
        return ""


def parse_kv(text):
    return dict(m.groups() for m in re.finditer(r"^([A-Z][A-Z0-9_]*)=(.*)$", text, re.M))


def num(x):
    try:
        return float(x)
    except ValueError:
        return None


groups = defaultdict(list)
for machine in sorted(os.listdir(data)) if os.path.isdir(data) else []:
    exdir = os.path.join(data, machine, a.exercise)
    if not os.path.isdir(exdir):
        continue
    for run in sorted(os.listdir(exdir)):
        rd = os.path.join(exdir, run)
        if run.endswith("_latest") or os.path.islink(rd) or not os.path.exists(os.path.join(rd, "run_config.txt")):
            continue
        csvp, summ, png = (os.path.join(rd, n) for n in ("data.csv", "summary.txt", "plot.png"))
        if os.path.exists(csvp) and os.path.exists(analyzer):
            need_s = "STATUS=" not in read(summ)
            need_p = not os.path.exists(png)
            if need_s or need_p:
                cmd = [sys.executable, analyzer, csvp] + (["--plot", png] if need_p else [])
                out = subprocess.run(cmd, capture_output=True, text=True).stdout
                if need_s:
                    open(summ, "w").write(out)
        kv = parse_kv(read(summ))
        cfg = dict(x.split("=", 1) for x in read(os.path.join(rd, "run_config.txt")).split() if "=" in x)
        model = re.search(r"model name\s*:\s*(.*)", read(os.path.join(rd, "sysinfo", "cpu_model.txt")))
        groups[(machine, cfg.get("label", "?"), cfg.get("hp", "?"))].append(
            {"kv": kv, "model": model.group(1).strip() if model else "?", "ok": kv.get("STATUS") == "ok"})

if not groups:
    sys.exit(f"no runs found under {data}/<machine>/{a.exercise}/ (run: bash scripts/fleet.sh all {a.exercise})")

rows, keys = [], []
for (machine, label, hp), runs in sorted(groups.items()):
    ok = [r for r in runs if r["ok"]]
    row = {"machine": machine, "cpu_model": runs[0]["model"], "core_label": label, "hugepages": hp,
           "runs_total": len(runs), "runs_confirmed": len(ok)}
    allkeys = sorted({k for r in ok for k in r["kv"] if k != "STATUS"})
    for k in allkeys:
        vals = [r["kv"][k] for r in ok if k in r["kv"]]
        nums = [num(x) for x in vals]
        if all(n is not None for n in nums) and nums:
            row[k] = round(st.median(nums), 3)
            row[k + "_min"], row[k + "_max"] = round(min(nums), 3), round(max(nums), 3)
        else:
            row[k] = Counter(vals).most_common(1)[0][0]
        if k not in keys:
            keys.append(k)
    rows.append(row)

cols = ["machine", "cpu_model", "core_label", "hugepages", "runs_total", "runs_confirmed"]
for k in keys:
    cols.append(k)
    if any((k + "_min") in r for r in rows):
        cols += [k + "_min", k + "_max"]
os.makedirs(os.path.join(root, "report", "tables"), exist_ok=True)
base = os.path.join(root, "report", "tables", f"{a.exercise}_summary")
with open(base + ".csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=cols, restval=""); w.writeheader(); w.writerows(rows)
# markdown: "median [min..max]" in one cell per metric
with open(base + ".md", "w") as f:
    head = cols[:6] + keys
    f.write("| " + " | ".join(head) + " |\n|" + "---|" * len(head) + "\n")
    for r in rows:
        cells = [str(r.get(c, "")) for c in cols[:6]]
        for k in keys:
            cells.append(f"{r[k]} [{r[k + '_min']}..{r[k + '_max']}]" if (k + "_min") in r else str(r.get(k, "")))
        f.write("| " + " | ".join(cells) + " |\n")
print(read(base + ".md"))
print(f"wrote {base}.csv / .md")
bad = [r for r in rows if r["runs_confirmed"] < a.min_runs]
if bad:
    print(f"NOTE: fewer than {a.min_runs} confirmed runs for:", ", ".join(f"{r['machine']}/{r['core_label']}/hp{r['hugepages']} ({r['runs_confirmed']}/{r['runs_total']})" for r in bad))
