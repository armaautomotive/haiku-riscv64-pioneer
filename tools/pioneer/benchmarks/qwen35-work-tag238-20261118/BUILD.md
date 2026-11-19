# Qwen3.5-4B Work tag238 diagnostic build

Built on 2026-11-18 from the current workspace with the local Haiku RISC-V64
build at `/Volumes/HaikuBuildLocal/generated.riscv64`:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-238
```

Tag238 extends tag237's failure-only padded-scale evidence with snapshots
after `StageInput`, at `StageBand` entry, after batch construction, after root
job setup, and after worker-table construction. It retains the before-clear,
after-clear, before-dispatch, and before-validation snapshots. It changes no
projection arithmetic, scale validation, core schedule, or fallback behavior;
the ROOT continues to use all 120 Tensix cores for projection waves.

The full offline build passed its undefined-import and unsupported-instruction
checks. The Work owner regression passed, including clearing all nine evidence
words at request start and copying all nine words on a tagged dispatcher
fault. The Haiku client compiled with warnings as errors, and both guarded
scripts pass Bash syntax checks.

Offline build output: `/private/tmp/pioneer-qwen35-work-offline-238`.
The guarded trial stage is
`/boot/home/pioneer-qwen35-work-238.Z6nR3`.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010038` | `399b5ae39e7a066b7fdc8a10fb50ae68856f5d74cffeaee8514ec2e06685ce66` |
| Work client | `8e70e6fd4b9db0cb62e4f8e7143b6051d297fadcf85dfa758dccdfa6e1839eda` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `6e0aaabebf4731d3b3a5c67d88b2d5bbf5ca99e7fda00d1e2ac401448f358e5c` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `9c8d0836228c37e020b45c348b06972dabd7b7c88b5b6a54002b44481c08abb1` |
| Recovery helper | `00949936354778102d7515ab94f7e2f0e6f09218be4ffbca8b67fe92f0634ab5` |

The tag238 hardware run failed before prefill. The nine snapshots show the
padded scale remains zero through `StageBand` entry and changes during
`StageBand` work before the post-batch snapshot. It produced no valid output
or decode-rate measurement. Run timings, recovery, and artifact hashes are
recorded in [RUN.md](RUN.md).
