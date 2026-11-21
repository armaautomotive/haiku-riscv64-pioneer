# Qwen3.5 dataflow rewrite tag0111

Tag0111 is an offline recovery and diagnostic checkpoint for the resident
120-core Tensix rewrite. The tag adds:

- An embedding-operation receipt that identifies the owner result, failed
  core, worker state/failure/phase, and pending command sequence through
  `QUERY` and `RECOVER`.
- An authority transition for aborting a pending embedding after the physical
  all-core worker audit. The destination activation and vocabulary result
  remain invalid after an aborted operation.
- A same-owner recovery request before service close, plus a close retry after
  a recoverable stop-audit failure.

Projections remain on the resident Tensix matrix-engine path across all 120
cores. The Work client has no CPU inference or fallback path.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0111-rewrite5
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010111`, dataflow ABI 3 | `34b8680b70b9664176492832be457a26bc82263efaf2be39a233f3e1bad29bd4` |
| Haiku dataflow client | `a043d2c1ba96ed28133bbf46ced63e6e1b9e70811d0ce5fedac764c3011419ae` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text (52,616 bytes) | `605cebc2507f86ff3feae51f30edb7f4fbdba12aa376f9bc037fc9faeadfd29f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

Offline validation passed:

- Full driver, resident service image, and Haiku client build. The service
  image is compiled for RV32IMF, has no unresolved imports, and contains no
  A-extension atomic instructions.
- AddressSanitizer/UndefinedBehaviorSanitizer program-authority,
  service-owner, and projection-owner fixtures, including an embedding worker
  failure, recovery audit, activation invalidation, and clean close.
- `sh -n tools/pioneer/build_qwen38_banks_driver.sh` and
  `git diff --check` on the tag0111 source paths.

This checkpoint has not been installed on the Pioneer. The preceding tag0110
hardware attempt failed at the first embedding operation; stable driver55
generation2 was restored and the 4B and 27B model files were verified intact.
Tag0111 has no silicon correctness result, generated output, or tokens/sec
measurement. Continue offline rewrite work before the next benchmark.
