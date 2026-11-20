# Qwen3.5-4B Work tag76: checked peer tile prefetch

Tag76 adds a one-tile lookahead to the checked persistent Qwen35 peer. The
peer fetches each original-size weight tile into alternating L1 buffers. It
waits for the exact NoC response, idle bit, and error proof before consuming
that tile, while the previous tile's existing Tensix projection runs. It
keeps the original quantized unpacking, dot checks, completion counters, and
physical peer proofs. The option is isolated behind
`PIONEER_Q35_WORK_TILE_PREFETCH` and is not enabled for other images.

The candidate was exercised on hardware, but faulted during the first Work
request after loading. See `RUN.md`; this is a failed experiment, not a
throughput qualification.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --peer-tile-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag76-tile-prefetch-buildup
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 56,112 | `18929d20d17783429d6afc5bfcf39c20e6a9f612b4f296ab7c8bb71e6eb0a3ff` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 8,176 | `b7b5a31bf65d7e08937af15cc4d86c0ee3b6d9b9463393d6878d6aec38929709` |
| Candidate driver55 | 251,296 | `b1b0142d6eae8a00025979b206bcbddbe07fff125c94194a141e80946f1edcd0` |
| Work client | 304,848 | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The ROOT image is byte-identical to tag75. The new peer stage's reported
stack frame is 64 bytes; `q38_compute_once` remains 304 bytes. The plan test
passed 5,136 cases across supported Qwen35 input/type shapes, alternating
buffer boundaries, the last tile, and refusal atomicity. Shell syntax checks
and `git diff --check` passed.

The required read-only Haiku preflight passed before staging a candidate:
the prior inference/27B diagnostic process was absent, stable driver55 was
idle at generation30, and the preserved Qwen3.8-27B model remained
`0:1:600:18973870528`. The guarded trial restored stable driver55 at
generation32 without reboot; the same 27B file remained intact.
