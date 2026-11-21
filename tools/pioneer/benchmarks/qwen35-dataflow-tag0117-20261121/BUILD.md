# Qwen3.5 dataflow rewrite tag0117

Tag0117 adds a one-run diagnostic mode to the Haiku client. It runs one prompt
through the resident model, requests exactly two output tokens, and skips the
second generation. This validates the embedding read and first decode step
without starting the two-run throughput benchmark. The guarded card wrapper
still checks the stable55 idle state, exact model hashes, exclusive lock, and
clean unload; it restores stable55 when the service closes cleanly. If a
failure cold-latches the candidate, it preserves the stable backup and lock
for the saved post-power-cycle recovery helper.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0117-diagnostic-build2
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010117`, dataflow ABI 3 | `e5387ede717a52354f64fe8cb41ca73bf9a9cc1bc5ab86de0155dca6ec59283c` |
| Haiku dataflow client | `7faaabe33f14399c717b91efc1f690c7defbb7a1d3b74f981d91ced47510276f` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `e48aba4107df19fd5c1e5c888f772459a3d69ba4f53a8dc83f9efe24f62352bd` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build pass.
- Conservative kernel stack use is `15712/18432` bytes; no Qwen35 frame is
  above 8192 bytes.
- The focused runtime recovery, embedding worker, owner, and service-owner
  ASan/UBSan fixtures pass. The worker role contract compiles as RV32IMF
  without A-extension atomics or unresolved runtime helpers.
- Both diagnostic and recovery shell wrappers pass `sh -n`.

The tag0117 diagnostic run is pending. Tag0116's failure-detail changes and
previous hardware outcomes are documented in
[`tag0116 BUILD`](../qwen35-dataflow-tag0116-20261121/BUILD.md) and
[`tag0115 RUN`](../qwen35-dataflow-tag0115-20261121/RUN.md).
