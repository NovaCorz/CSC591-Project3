#!/usr/bin/env bash
# Standard entry point used by scripts/fleet.sh (every exercise has one). Env: CORE LABEL HP MIB ITERS REPS MAXN RUN
cd "$(dirname "$0")" && exec ./run_rob.sh "$@"
