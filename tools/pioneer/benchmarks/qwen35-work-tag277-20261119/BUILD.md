# Qwen3.5-4B Work tag77: checked peer tile prefetch, dispatch fix

Tag77 carries the same checked one-tile lookahead as tag76, with the
fallthrough corrected. When `PIONEER_Q35_WORK_TILE_PREFETCH` selects the
Qwen35 stage, the peer no longer enters the legacy synchronous fetch after
starting the next-tile read. The original unpacking, quantized matrix math,
dot checks, and completion proofs are unchanged.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --peer-tile-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag77-tile-prefetch-fix1
```

The candidate cross-build succeeded. Disassembly shows the call to
`Qwen35TilePrefetchStage` followed directly by projection code, with no
legacy `FetchBlock` call in that path. The data-only plan test passed 5,136
valid/refusal cases; the trial wrapper passes `bash -n`. The guarded hardware
trial produced two valid warm outputs, verified all-core waves, unloaded
cleanly, and restored stable driver55. Decode remained far below the 100 tok/s
target; see `RUN.md`.

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 56,112 | `18929d20d17783429d6afc5bfcf39c20e6a9f612b4f296ab7c8bb71e6eb0a3ff` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 8,116 | `8e165c2bdbc754106f51e8ab84203645bca0da1e6f663d8da4e4dda31eae72d9` |
| Candidate driver55 | 251,240 | `322d74d97428a9c0846d832c9df35cfb2217f114a02e74ccb3727a37d4906acc` |
| Work client | 304,848 | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The peer stack report lists 64 bytes for `Qwen35TilePrefetchStage` and 288
bytes for `q38_compute_once`. The ROOT image remains byte-identical to tag75.
