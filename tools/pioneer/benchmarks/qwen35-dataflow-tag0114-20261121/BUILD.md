# Qwen3.5 dataflow rewrite tag0114

Tag0114 adds a runtime-stability subcheck code to the all-core QUERY/RECOVER
diagnostic after tag0113 reported only `runtime unstable, stage6, core0`. It
keeps the recovery predicates intact and does not change the 64-byte worker
mailbox or the ioctl request size. Projection work remains on the Tensix
matrix engines across all 120 resident workers; there is no CPU inference or
fallback path.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0114-rewrite14-build
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010114`, dataflow ABI 3 | `ea4fa51d250c143bd5144156aa6aba71f43e11c9f1c89ece41280ba7a08c3964` |
| Haiku dataflow client | `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `28226c21e05582247946f5cfefe5dc6391455d631e09d8c53dc92948c3e96980` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- The full variant-60 driver, resident service image, and Haiku client build
  passed.
- The stack audit reports `15456/18432` bytes; the generation sequence is
  inlined into the measured ioctl control frame. No Qwen35 frame exceeds
  8192 bytes.
- ASan/UBSan owner and service-owner tests pass, including propagation of the
  audit detail. The embedding-worker tests and RV32IMF compile also pass; the
  object contains no A-extension atomics or unresolved runtime helpers.
- The runtime audit detail codes are: `1` context, `2` mapping, `3` service
  image, `4` reset, `5` dispatch, `6` local route, `7` NoC metadata, `8` route,
  `9` runtime source proof, `10` service PC, `11` resident NoC idle proof.
  Despite the old label, this checks NoC command/queue status registers; it is
  not a Tensix matrix-engine idle proof. Detail `11` on the card followed the
  failed embedding-weight read and showed that NoC quiet had not been proven.

## Hardware status

Tag0114 loaded the full model, then failed on core0's first embedding weight
read. QUERY/RECOVER identified runtime audit stage6/core0/detail11, the
resident NoC idle check; CLOSE failed the core0 stop audit. Its wrapper retained
the cold candidate, exact stable backup, and lock. The attempted user power
reset did not clear the latch, and an authorized host reboot was required.
See [`tag0114 RUN`](RUN.md) for timings and recovery state. The stable55 restore
is pending the SD-free Haiku handoff; the 27B and 4B files are intact.
