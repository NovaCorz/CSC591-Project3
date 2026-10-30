# ex15_pipeline_depth: pipeline depth from the misprediction penalty (10 points)

Analysis only: it reuses the ex03 data (native, and the enclave variant if you have it). Nothing runs on the machines.
Full walkthrough: [`docs/FULL_GUIDE.md`](../../docs/FULL_GUIDE.md).

```bash
python3 exercises/ex15_pipeline_depth/analyze_ex15.py     # -> report/tables/ex15_pipeline_depth.csv/.md, report/figures/ex15_pipeline_depth.png
```

penalty (cycles) = ns lost per misprediction x core clock (both the regression slope and the (random50 - sorted)/0.5 estimate from ex03).
The penalty is the front-end pipeline length PLUS the latency of what the branch waits for (an L1 load and a compare in this loop),
so it is an upper bound on the front-end depth. Discuss that gap, and why a measured penalty cannot give an exact stage count.
