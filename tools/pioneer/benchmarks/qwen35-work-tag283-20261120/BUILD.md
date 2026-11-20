# Qwen3.5-4B Work Tag83: verify alternating peer buffers

Tag83 fixes the ROOT completion proof exposed by Tag82. Peers use the final
alternating batch slot (`0x98000` or `0x9a000`), and the ROOT now derives the
expected slot from the checked projection block count before accepting the
terminal command receipt. The asynchronous NoC completion checks and full-core
projection schedule remain unchanged.

The peer design follows Tenstorrent's reader/compute overlap pattern using the
existing Haiku Work roles: peer RISC-V cores coordinate bounded NoC reads and
completion proofs while Tensix matrix engines execute projections. This does
not use TT-Metalium's runtime or circular-buffer API. The ROOT matrix image
keeps the existing bounded batch-fetch path because its private result map
overlaps the peer scratch addresses. Work mode has no CPU inference/fallback.

## Build

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --batch-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag83-batched-prefetch-v10
```

The cross-build passed the fixed 8 KiB peer image/descriptor boundary and the
ROOT/peer role checks. The host plan test passed 5,136 cases, including the
expected terminal slot for all supported type/input shapes.

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `cc9edb9f8c6fc961003d5bd7e09e268b2dfd6c7b2c8912c43ef343d4e38bcc72` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `c718047280329a045a0e89bcd6317884a468807d30b084fe0d9f9ba09575f8f3` |
| Candidate driver55 | `6cdd901d281b4b65904ec43d99b6551aa5e40ed82271fe569d22e8667b318e12` |
| Work client | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The guarded Tag83 result and phase timings are recorded in `RUN.md`; raw logs
and terminal state are in `evidence/`.
