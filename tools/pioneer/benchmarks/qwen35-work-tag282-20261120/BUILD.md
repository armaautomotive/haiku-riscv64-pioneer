# Qwen3.5-4B Work Tag82: peer batch prefetch

Tag82 adapts Tenstorrent's reader/compute overlap pattern to the existing
Haiku Work peer image. Each persistent peer uses its RISC-V control core to
issue checked NoC reads into alternating 8 KiB local buffers while its Tensix
matrix engine computes from the other buffer. The existing completion
counter, idle check, source seal, and full-core schedule remain in force.
This is a small custom pipeline in the Haiku driver; it does not use the
TT-Metalium runtime or its circular-buffer API. The ROOT matrix image keeps
the existing bounded batch-fetch path because its private result map overlaps
the peer scratch addresses.

Work mode continues to use Tensix matrix engines for projections on the
distributed worker cores. Host CPU inference and fallback remain disabled.

## Build

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --batch-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag82-batched-prefetch-v9
```

The cross-build passed the peer's fixed 8 KiB image/descriptor boundary and
the ROOT/peer role checks. The host plan test passed 5,136 cases covering
original-layout tile addresses, alternating buffer order, final batches, and
failure atomicity.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `2d6c9281ee9f3fcdb7135f1ad0b470d72df5edc99051c8a774e2bf0f07f0685d` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `c718047280329a045a0e89bcd6317884a468807d30b084fe0d9f9ba09575f8f3` |
| Candidate driver55 | `6d19ccf853e6e2dfcf373edc1fc813b29bf30feb172e53e19ffd970091368aba` |
| Work client | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The guarded hardware attempt and its fault-safe restoration are recorded in
`RUN.md`. Before staging, strict Haiku SSH verified that the prior diagnostic
had exited, stable driver55 was idle at generation42, and the preserved 27B
model still had stat `0:1:600:18973870528`.
