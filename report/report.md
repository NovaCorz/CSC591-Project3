# Microarchitecture Reverse Engineering Across Machines
ECE 492/592, Fall 2026, Homework 3, **Track B**

Name: ____    Date: ____

## Selected exercises (first page)

| # | Exercise | Points |
|---|---|---|
| 1 | Pipelined? | 10 |
| 2 | Out-of-order? | 10 |
| 3 | Branch prediction? | 10 |
| 4 | ROB size | 30 |
| 8 | TLB miss cost | 10 |
| 10 | BTB capacity | 20 |
| 15 | Pipeline depth | 10 |
| | **Total** | **100** |

AI use: (state "No AI was used." or see `ai_log.md`)

## 1. Machines and setup
Table of machines used (id, CPU model, microarchitecture, SMT, hybrid, SGX), frequency handling (changed? restored?), and
limitations. Source: `docs/machines.md`. Evidence policy: all findings are established from timing measurements;
hardware counters, where used, appear only in "confirmation" subsections.

## 2. Exercise 1: Pipelined?
### Method
### Code snippet
### Results (machine A | machine B, side by side)
### Enclave comparison
### Discussion (differences, published specs kept separate)

## 3. Exercise 2: Out-of-order?
(same subsections)

## 4. Exercise 3: Branch prediction
(same subsections)

## 5. Exercise 4: ROB size
(same subsections; include coarse and fine sweeps, run-to-run spread, and a "Confirmation with counters" subsection if used)

## 6. Exercise 8: TLB miss cost
(same subsections)

## 7. Exercise 10: BTB capacity
(same subsections)

## 8. Exercise 15: Pipeline depth
(same subsections; derived from the Exercise 3 data, native and enclave)

## 9. Enclave comparison summary
Platform and configuration, what changed, challenges (timing source, EPC size, memory encryption).

## 10. Limitations and unresolved issues

## 11. (Optional bonus) Trend across >= 8 machines
Proposed law, supporting measurements, scope and exceptions.

## Appendix: reproduction
See the repository `README.md`; commands used to produce each figure.
