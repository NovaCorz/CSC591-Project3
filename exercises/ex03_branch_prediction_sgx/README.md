# ex03_branch_prediction_sgx: enclave variant of Does the processor use branch prediction?

Runs the same kernel as `ex03_branch_prediction` **inside an SGX enclave** on an enclave-capable machine (machines.txt: `sgx=yes`),
then compare with the native results from the same machine. Setup notes: `docs/enclave_setup.md`.

Status: [ ] enclave builds  [ ] runs  [ ] compared with native  [ ] differences explained  [ ] in report

## Points to settle and document
- Enclave platform and configuration (Intel SGX SDK / Gramine / Open Enclave, version, SGX1 or SGX2, heap/EPC size).
- Timing: `rdtsc` is not allowed in SGX1 enclaves, so time outside the enclave around the ECALL (or use SGX2 if the CPU allows it).
- Memory: buffers must fit in the EPC; enclave memory is encrypted, so latencies are expected to be higher.
- If enclave support or access is unavailable on a machine, report that limitation (the ordinary experiments still count).

## Contract
Same as the native exercise: `run.sh` reads `CORE`/`LABEL` and writes `results/<host>_<label>_hp<HP>_<runid>/`.
