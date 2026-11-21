# Qwen3.5 dataflow rewrite tag0115

Tag0115 adds a bounded, read-only wait for resident NoC status to become quiet
after the worker reports the specific embedding weight read failure (failure7).
It then runs the existing strict runtime audit before acknowledging any worker.
Other failure types keep the immediate audit. The wait never clears a command,
resets a Tensix core, relaxes an audit predicate, or changes the fixed 64-byte
worker mailbox or ioctl request size. Projection work remains on the Tensix
matrix engines across all 120 workers; there is no CPU inference or fallback.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0115-rewrite15-build
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010115`, dataflow ABI 3 | `52a4a16ceb170767089edac960925aac0fa549160f47cfd949ba657f25e21885` |
| Haiku dataflow client | `80d28f4d75a48612773b07339010308ced09e8c8debf8e7708c35b85f7908da5` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `28226c21e05582247946f5cfefe5dc6391455d631e09d8c53dc92948c3e96980` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build pass.
- Conservative kernel stack use is `15456/18432` bytes; no Qwen35 frame is
  above 8192 bytes. Generation remains inlined into the measured ioctl frame.
- The focused recovery test passes with ASan/UBSan. It covers an already-idle
  core, a transient queue status, a stuck initiator that remains untouched at
  timeout, and a mapping instability.
- The role contracts compile as RV32IMF with no A-extension atomics or
  unresolved runtime helpers.

Tag0115's hardware run reproduced the core0 embedding weight read failure. Its
bounded wait reached timeout on NoC0 queue0 status, leaving the driver cold
latched; the exact stable55 backup and lock were retained. The guarded
post-cycle helper has since restored stable55 generation2 and cleared the
lock. No valid output or decode rate was produced. See [`tag0115 RUN`](RUN.md).
