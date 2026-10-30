"""Helpers shared by every exercise's analyze.py (no third-party packages needed except optional matplotlib)."""
import csv, statistics as st, sys


def read_csv(path):
    """Returns (meta dict from '# key=value' header lines, list of row dicts)."""
    meta, lines = {}, []
    for l in open(path):
        if l.startswith("#"):
            for kv in l[1:].split():
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    meta[k] = v
        elif l.strip():
            lines.append(l)
    return meta, list(csv.DictReader(lines))


def median(xs):
    return st.median(xs)


def emit(key, val):
    """Machine-readable result line (collected by scripts/collect.py)."""
    print(f"{key}={val}")


def get_plt(plot_path):
    """Returns pyplot, or None (with a note) when no plot is wanted or matplotlib is missing. Never raises."""
    if not plot_path:
        return None
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        return plt
    except ImportError:
        print("NOTE: matplotlib is not installed here, so no plot was made. The numbers are unaffected; "
              "plots are rebuilt on your own computer by: python3 scripts/collect.py <exercise>")
        return None
