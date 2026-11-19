# Qwen3.5-4B Work tag240 diagnostic build

Built on 2026-11-18 from the current workspace with the local Haiku RISC-V64
build:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-240
```

Tag240 keeps tag239's 15 StageBand failure snapshots and changes the Work
owner's failure readback condition: the request-cleared `RootFault` completion
marker now triggers evidence capture even when the parked PC is
`WorkFaultPc+4`. The owner regression exercises that PC+4 path and verifies
all 15 captured words. Projection arithmetic, dispatcher validation, the
120-core schedule, and Work's no-CPU-fallback contract are unchanged.

The offline build completed, including the generated ROOT/peer image import
and instruction checks. The Work owner regression passed with warnings as
errors. The guarded trial and recovery scripts pass `bash -n`, contain no
tag239 identities, and `git diff --check` is clean.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010040` | `2ec4b9598ea57b67cbf0149ac595c178e66f98aed5ea92e284149c6472c9e779` |
| Work client | `dd3e91d51f9cc2f08002494cc55ed478b2272d2e42ec65f450e64377e58bfe2a` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `1ea760acf8db762f0ef7a20cfd755000e529a590e00a56a1285f732f60e40cf6` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `b674597e6265a15a48394b29d608395b529ccd953f9485cfe38400e281763412` |
| Recovery helper | `79a5cca432403bcf8ff1dc2d5485a1cc46cda94ac7c6de3976d2a855e48eab8a` |

The local offline build is `/private/tmp/pioneer-qwen35-work-offline-240`.
The planned Haiku stage is `/boot/home/pioneer-qwen35-work-240.P8rT6`. Hardware
preflight confirmed stable041 generation2 idle, no inference process or
shared lock, 64 CPUs/full RAM, and unchanged 4B/27B model sizes before staging.
