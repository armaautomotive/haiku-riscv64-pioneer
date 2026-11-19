# Qwen3.5-4B Work tag237 diagnostic build

Built on 2026-11-18 with the local Haiku RISC-V64 toolchain using
`tools/pioneer/build_qwen35_work.py` and build tag `2026111800010037`.
Tag237 adds four failure-evidence words around the Q5_K padded-scale tail
clear and dispatcher validation. It does not change scale data, matrix math,
core scheduling, or fallback behavior. The all-120-core projection schedule
is unchanged.

The full build passed its undefined-import and unsupported-instruction checks.
The Work owner regression passed, including zeroing the evidence range at
request start, returning its four words on a tagged dispatcher failure, and
restoring the saved L1 image on clean unload. Both guarded scripts pass Bash
syntax checks.

Offline build output: `/private/tmp/pioneer-qwen35-work-offline-237c`.
The guarded trial stage is
`/boot/home/pioneer-qwen35-work-237.K9sQ3`.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010037` | `ff0883e59a7bdf16a39d1ec51042ffd460e78709cf4abd1f22fdf11310b9a5f1` |
| Work client | `d7cb9b8065411afa032da7de35b4aeb0e34fac8811dfacfc3cb54170e92b7643` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `a7499bae3d421de1d6b7d556d92ff198bc363cbcb36dc08543435ab0ffa27534` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `b76e50057227689b982e2a8308d40385052634ae8c592557751a259b98dcf75d` |
| Recovery helper | `bcdefaf2d8c51e2d028aae340fa23ac7ff126f9d53f0e217bc89da5209622441` |

The tag237 hardware run failed before prefill. Its phase evidence, separate
load and prompt timings, artifact hashes, and stable041 recovery are recorded
in [RUN.md](RUN.md). The padded scale is zero immediately after clearing but
has been overwritten by matrix dispatch; no valid output or decode-rate
measurement was produced.
