# Qwen3.5 dataflow rewrite tag0112

Tag0112 fixes the tag0111 kernel-stack panic. The per-layer sequences now reuse
a projection descriptor allocated with the kernel session, and the projection
schedule scratch is held by the heap-owned model authority. The program builder
fills the caller's descriptor directly instead of keeping a second 64 KiB
candidate on the stack. Projections remain on the resident Tensix matrix-engine
path across all 120 cores; there is no CPU inference or fallback.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0112-rewrite7
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010112`, dataflow ABI 3 | `b2f3a6be660a07a5c76b363b7ab116ba81b68095126e221c5a741f972a37f664` |
| Haiku dataflow client | `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text (52,616 bytes) | `605cebc2507f86ff3feae51f30edb7f4fbdba12aa376f9bc037fc9faeadfd29f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded two-warm trial wrapper | `ce0f74375383e92f78354da52f6956426fe368836c2782f1303424630f789484` |
| Post-cycle recovery helper | `a003f4d40d8c5a4b0489180967725344672db7e8cf55a8180e94306da6bf1bab` |

Offline validation passed:

- Full variant-60 driver, resident service image, and Haiku client build.
- The builder parsed the driver's `-fstack-usage` report. No Qwen35 dataflow
  function frame exceeds 8192 bytes. A deliberately conservative sum across
  generation, the larger decoder branch, projection construction, and device
  audit calls is `15696` bytes against an `18432`-byte budget. Haiku's kernel
  stack is 20 KiB.
- Seven focused AddressSanitizer/UndefinedBehaviorSanitizer tests passed:
  program authority, projection owner, service owner, FFN sequence, recurrent
  layer sequence, dense layer sequence, and decoder/generation sequence.
- The RV32IMF compile passed without A-extension atomics or unresolved runtime
  helpers. The driver build checks that result and the stack budget on every
  tag0112 build.
- `sh -n` passed for the driver builder, `bash -n` passed for both guarded
  tag0112 scripts, and `git diff --check` passed.

Tag0112 was installed for a guarded hardware trial. It uploaded the model,
then failed on the first embedding read; it produced no valid token and did
not unload cleanly. The stable driver was restored after a cold recovery.
Detailed timings, diagnostics, and evidence hashes are in [`RUN.md`](RUN.md).
The 100 decode tokens/s target remains unverified. Tag0111's earlier panic
and recovery evidence are in
[`../qwen35-dataflow-tag0111-20261121/RUN.md`](../qwen35-dataflow-tag0111-20261121/RUN.md).
