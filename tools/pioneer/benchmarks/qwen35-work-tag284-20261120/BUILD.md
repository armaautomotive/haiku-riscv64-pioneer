# Qwen3.5-4B Work Tag84: concurrent peer physical audit reads

Tag84 keeps Tag83's Qwen35 batch prefetch and ROOT receipt fix. It changes
only ROOT's read-only physical peer audit: after the peer status record passes
all existing checks, the ROOT uses three independent Blackhole NoC request
initiators to sample the peer's two RISC-V PCs and reset register concurrently.
Each request still has its own exact command readback and contributes to the
expected per-counter deltas. The ROOT waits for all three responses, verifies
all four read counters, confirms every NoC initiator is idle, then applies the
same PC/reset checks as before. A failed or incomplete transaction faults into
the existing recoverable Work path.

This is a narrow dispatch experiment in the Tenstorrent dataflow direction;
it does not introduce TT-Metalium or change model arithmetic. All 120 cores
remain assigned to Tensix projection work, and Work has no CPU inference or
fallback.

## Build

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --batch-prefetch /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag84-multi-niu-proof-v2
```

The cross-build passed. The peer image is unchanged from Tag83. ROOT image
text changes only for the concurrent read-only audit. Exact hashes:

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `761ff708998b9182e4d92b0ab77655e17c2edb4d08c1d2407c71e847543b3fed` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `c718047280329a045a0e89bcd6317884a468807d30b084fe0d9f9ba09575f8f3` |
| Candidate driver55 | `3ff51c4f7ebcc1cf99ef91d7340403c342680ad6df721938dbbae82193141c6b` |
| Work client | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The guarded hardware trial uses a private Tag284 stage and requires stable
driver55 to be idle before exchanging the candidate. Hardware timings and raw
evidence will be added to `RUN.md` after the run.
