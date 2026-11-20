# Qwen3.5-4B Work Tag81: batched peer weight reads

Tag81 enables the existing bounded Qwen35 batch-fetch path. Each peer reads
two adjacent original-layout quantized weight tiles together (up to the
path's existing 8 KiB limit), then consumes the original tiles in order. The
Tensix matrix math, output format, per-peer counter expectations, and root
completion and output proofs remain in place. This candidate has no host CPU
inference or fallback.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --batch-fetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag81-batched-peer-reads
```

The cross-build validates the fixed peer image ABI and bounded scratch map.
Hardware acceptance requires the guarded trial to produce two exact warm
outputs, report load/prompt/decode separately, and restore stable driver55.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `2d6c9281ee9f3fcdb7135f1ad0b470d72df5edc99051c8a774e2bf0f07f0685d` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `7fc1175623fcec517370ddec303f7a8bc7c67c6237fe6e0abf59d013b1fb3382` |
| Candidate driver55 | `97b2da8d9331f92244ae220baabc25134ceddc67ed34910ed181c7cd3e0f4717` |
| Work client | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The guarded run completed successfully and produced valid outputs, but batch
fetch did not improve decode throughput. See `RUN.md` for the measured result.
