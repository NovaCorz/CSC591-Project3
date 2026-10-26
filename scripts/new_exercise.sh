#!/usr/bin/env bash
# Scaffold a new exercise folder that follows the fleet.sh contract.
#   scripts/new_exercise.sh <dir_name> "<Title>" <points> [--sgx]
# Example: scripts/new_exercise.sh ex10_btb_capacity "BTB capacity" 20 --sgx
# Creates exercises/<dir_name>/ (README, run.sh stub, Makefile stub, .fleetignore) and, with --sgx,
# exercises/<dir_name>_sgx/ for the enclave variant (fleet.sh only runs *_sgx exercises on machines with sgx=yes).
set -euo pipefail
cd "$(dirname "$0")/.."
NAME=${1:?dir name, e.g. ex10_btb_capacity}; TITLE=${2:?title}; PTS=${3:?points}; SGX=${4:-}
D=exercises/$NAME
[ -e "$D" ] && { echo "$D already exists"; exit 1; }
mkdir -p "$D"

cat > "$D/README.md" <<EOT
# $NAME: $TITLE ($PTS points)

Status: [ ] code written  [ ] tested locally  [ ] run on both required machines (all 8 if doing the bonus)  [ ] >=3 repeats
        [ ] enclave run (or limitation documented)  [ ] plots  [ ] in report

## Planned method
<!--METHOD-->

## What to measure / expected shape
<!--EXPECTED-->

## Evidence rules (course)
- Primary evidence is **timing only**. Hardware counters may be used afterwards, clearly labeled as a double-check.
- Hybrid CPUs: test both P-cores and E-cores. Fixed frequency is allowed only with coordination, and must be restored.
- Separate measured findings from published specifications in the report.

## Contract with scripts/fleet.sh
\`run.sh\` reads env \`CORE\` and \`LABEL\` (plus optional tuning variables), pins to \`CORE\`, and writes
\`results/<host>_<label>_hp<HP>_<runid>/\` containing \`run_config.txt\` (\`core= label= hp= run=\`), the raw CSV(s),
summaries/plots, and \`sysinfo/\` (call \`../common/gather_info.sh\`). Use \`../common/include/bench_common.h\` for timing helpers.
See \`exercises/ex04_rob/run_rob.sh\` for a complete example.

## Results log
| machine | CPU / microarchitecture | label | result | notes |
|---|---|---|---|---|
EOT

cat > "$D/run.sh" <<EOT
#!/usr/bin/env bash
# STUB: replace with the real entry point. Env: CORE LABEL (HP and tuning variables optional).
echo "$NAME: not implemented yet" >&2
exit 2
EOT
cat > "$D/Makefile" <<EOT
# STUB: build your code here (fleet.sh deploy runs 'make -B' on each machine). Suggested: output to bin/
all:
	@echo "$NAME: no code yet"
EOT
printf 'bin/\nresults/\n*.o\n__pycache__/\n' > "$D/.fleetignore"
chmod +x "$D/run.sh"
echo "created $D"

if [ "$SGX" = "--sgx" ]; then
  S=exercises/${NAME}_sgx; mkdir -p "$S"
  cat > "$S/README.md" <<EOT
# ${NAME}_sgx: enclave variant of $TITLE

Runs the same kernel as \`$NAME\` **inside an SGX enclave** on an enclave-capable machine (machines.txt: \`sgx=yes\`),
then compare with the native results from the same machine. Setup notes: \`docs/enclave_setup.md\`.

Status: [ ] enclave builds  [ ] runs  [ ] compared with native  [ ] differences explained  [ ] in report

## Points to settle and document
- Enclave platform and configuration (Intel SGX SDK / Gramine / Open Enclave, version, SGX1 or SGX2, heap/EPC size).
- Timing: \`rdtsc\` is not allowed in SGX1 enclaves, so time outside the enclave around the ECALL (or use SGX2 if the CPU allows it).
- Memory: buffers must fit in the EPC; enclave memory is encrypted, so latencies are expected to be higher.
- If enclave support or access is unavailable on a machine, report that limitation (the ordinary experiments still count).

## Contract
Same as the native exercise: \`run.sh\` reads \`CORE\`/\`LABEL\` and writes \`results/<host>_<label>_hp<HP>_<runid>/\`.
EOT
  cat > "$S/run.sh" <<EOT
#!/usr/bin/env bash
# STUB: enclave entry point. Env: CORE LABEL
echo "${NAME}_sgx: not implemented yet" >&2
exit 2
EOT
  cp "$D/Makefile" "$S/Makefile"; sed -i "s/$NAME: no code yet/${NAME}_sgx: no code yet/" "$S/Makefile"
  cp "$D/.fleetignore" "$S/.fleetignore"; chmod +x "$S/run.sh"
  echo "created $S"
fi
