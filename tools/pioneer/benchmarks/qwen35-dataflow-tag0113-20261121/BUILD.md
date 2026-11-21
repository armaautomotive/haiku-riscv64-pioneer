# Qwen3.5 dataflow rewrite tag0113

Tag0113 adds failure localization for the first-prompt embedding failure and
the all-core recovery audit. The worker failure field now distinguishes an
invalid embedding invocation, an IO binding failure, a NoC weight read, Q6_K
unpack, and an output write. QUERY/RECOVER reports the first recovery-audit
stage and core. The worker mailbox remains 64 bytes, and the request structure
retains its current size. Projection products continue to use the Tensix
matrix engines across the 120-core resident service; there is no CPU inference
or Work-mode fallback.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0113-rewrite13
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010113`, dataflow ABI 3 | `e2f7d4bb865cab020c6e52cb411eb99b15ec204b1faa7b1c683496493a0d42cf` |
| Haiku dataflow client | `152c5136d707b899e139ed674d3cfb43c85f74f9058c8773795ed6becbbf94e3` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `28226c21e05582247946f5cfefe5dc6391455d631e09d8c53dc92948c3e96980` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build passed.
- The RV32IMF compile passed without A-extension atomics or unresolved
  runtime helpers.
- Focused AddressSanitizer/UndefinedBehaviorSanitizer tests passed for the
  projection owner, service owner, and embedding worker. The owner fixture also
  checks that a refused recovery reports its audit stage and core.
- The stack audit found no Qwen35 frame over 8192 bytes. The conservative
  bound is `15456/18432` bytes. GCC emitted no separate generation-sequence
  stack record because that template was inlined into the ioctl control frame;
  the audit counts that measured frame once.
- `sh -n` passed for the driver builder and target compile test script.

## Hardware status

Tag0113 has not been installed on the Pioneer. Tag0112's last trial completed
the model upload and failed at the first prompt embedding; same-owner recovery
and close audit failed. The board was recovered to stable driver55 generation2
and the 4B/27B files were verified intact. The next hardware preflight must
recheck the stable driver, idle state, absence of the prior diagnostic, and
both model files after the user's recent power reset. The 100 decode tokens/s
target, two valid warm runs, and clean unload remain unverified.
