# Qwen3.5-4B Work tag239 diagnostic build

Built on 2026-11-18 from the current workspace with the local Haiku RISC-V64
build at `/Volumes/HaikuBuildLocal/generated.riscv64`:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-239
```

Tag239 retains tag238's failure-only scale snapshots and adds points after
StageBand validation, after fragment construction, after state validation,
before and after batch-object initialization, after the batch builder returns,
and after its batch fields are read. It does not change projection math,
validation, scheduling, or fallback behavior. The projection schedule still
uses all 120 Tensix cores.

The full offline build passed its undefined-import and unsupported-instruction
checks. The Work owner regression passed, including clearing and returning all
15 evidence words on the proper paths. The Haiku client compiled with warnings
as errors, and both guarded scripts pass Bash syntax checks.

Offline build output: `/private/tmp/pioneer-qwen35-work-offline-239`.
The guarded trial stage is
`/boot/home/pioneer-qwen35-work-239.D7vM2`.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010039` | `02fc553c6ae377d6af7630c4611b1512d3f7873e920d809b84b9f27d0ef42dd2` |
| Work client | `dd3e91d51f9cc2f08002494cc55ed478b2272d2e42ec65f450e64377e58bfe2a` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `1ea760acf8db762f0ef7a20cfd755000e529a590e00a56a1285f732f60e40cf6` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `8f5d63ac984ada5568a7d7c04390a3cf115a664b68679281fe0b4df4a5170f83` |
| Recovery helper | `4f2a28f80c36991eb66dd54bc173ff6267c2935c0c88739e3f0d59ff12c709fb` |

The tag239 hardware run failed before prefill and did not produce a valid
output or clean Work unload. The readback-gate defect was corrected for
tag240. See [RUN.md](RUN.md) for timings, recovery, and artifact hashes.
