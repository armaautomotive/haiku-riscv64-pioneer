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
  `9` runtime source proof, `10` service PC, `11` Tensix idle proof. The detail
  occupies previously unused bits in the existing diagnostic word.

## Hardware status

Tag0114 has not run on the card. Tag0113's preceding hardware trial loaded the
full model and failed at core0 embedding with failure7 (`EmbeddingWeightRead`).
Its all-core recovery audit stopped at runtime-instability stage6/core0 and
the wrapper retained the candidate, stable backup, and lock. The authorized
fresh boot and guarded restore have returned the card to stable55 generation2,
idle, with the lock cleared; both model files remain intact. This did not
verify in-place recovery. Tag0114 is ready for a fresh guarded trial from that
stable state. See
[`tag0113 RUN`](../qwen35-dataflow-tag0113-20261121/RUN.md).
