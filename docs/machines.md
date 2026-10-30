# Machines

| id | CPU model | Microarchitecture | Intro year | ISA | Hybrid | SGX in the CPU spec | SGX status on the machine | Notes |
|---|---|---|---|---|---|---|---|---|
| sunbird | 2x Xeon E5-2680 v3 | Haswell-EP | 2014 | x86-64 | no | no (predates SGX) | no | dual socket (NUMA) |
| charnwood | Core i7-6700 | Skylake | 2015 | x86-64 | no | yes (with Intel ME) | verify | likely legacy launch control -> `/dev/isgx` driver |
| ookay | Core i7-7700 | Kaby Lake | 2017 | x86-64 | no | yes (with Intel ME) | verify | same as above |
| upgrade | Core i7-8700 | Coffee Lake | 2017 | x86-64 | no | yes | verify | same as above |
| crux | Core i7-9700 | Coffee Lake Refresh | 2018 | x86-64 | no | yes (with Intel ME) | verify | same as above |
| skylark | 2x AMD EPYC 7532 | Zen 2 / Rome | 2019 | x86-64 | no | no (AMD) | no | dual socket (NUMA) |
| thunderbird | Ampere Altra Q80-30 | Arm Neoverse N1 | 2020 | **AArch64** | no | no (x86 feature) | no | ex04 now has an AArch64 kernel (same source file); untested on real Arm hardware. Other exercises need the same treatment |
| artemisia | 2x Xeon Gold 5420+ | Sapphire Rapids | 2023 | x86-64 | no | yes (max enclave size 128 GB) | verify | best SGX candidate; dual socket |

"SGX in the CPU spec" is from Intel's specifications. "SGX status on the machine" also depends on the BIOS setting and the OS driver, which
only you can verify (see `docs/enclave_setup.md`). Copy details into `data/<machine>/machine_card.md`.

Track B needs two machines per exercise (native) plus the enclave comparison on at least one enclave-capable machine. All five Intel x86
machines can run the x86 kernels natively; the Arm machine needs a port.
