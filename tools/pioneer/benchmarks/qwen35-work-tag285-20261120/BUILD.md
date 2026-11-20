# Qwen3.5-4B Work Tag85: peer execution timing

Tag85 adds read-only peer `mcycle` timing around each persistent matrix
projection. Each peer writes its elapsed low word to proof word 48, which ROOT
already gathers. ROOT takes the maximum across active peers and accumulates it
only during decode, after at least one token has been generated. Three reserved
ROOT L1 words hold the 64-bit sum and sample count. This measures the slowest
peer's compute interval per gathered dispatch; it does not include all ROOT
work or prove that peer compute alone determines end-to-end latency.

The profile is advisory. Its value does not affect arithmetic, output
validation, faults, or recovery. No extra NoC transaction was added. All 120
cores remain assigned to Tensix projections, and Work has no CPU inference or
fallback. Work protocol version 10 returns the timing after clean completion.

## Build

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  --batch-prefetch --peer-exec-profile /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-qwen35-tag85-peer-execution-profile-v2
```

The cross-build passed. The peer image is 8,064 bytes, within its 8,192-byte
limit. Exact hashes:

| Artifact | SHA-256 | Size |
| --- | --- | ---: |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` | 4 B |
| ROOT image text | `c2fc090375d03291af5e197c5525a2aa2fc725305de93d3eaf47c93e7dc13132` | 60,096 B |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` | 4 B |
| Peer image text | `70ac1b0da7a21015957d4c6c22424b079192547dcae1d1473f2b649a80f18416` | 8,064 B |
| Candidate driver55 | `565600c30c9971d1d03efa6a56ae773a36db5327850dcb9e5989285b048b3dc0` | 255,240 B |
| Work client | `d8da309cdf9675c3e4e3610d2b6e6b99e0d7a519b091af26dadcf49d227d1c5d` | 304,848 B |

The guarded hardware trial uses a private Tag285 stage and requires stable
driver55 to be idle before exchanging the candidate. It runs two warm outputs,
then restores stable driver55. Hardware results and raw evidence will be added
to `RUN.md` after the trial.
