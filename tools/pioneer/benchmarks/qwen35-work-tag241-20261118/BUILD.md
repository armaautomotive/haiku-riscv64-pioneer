# Qwen3.5-4B Work tag241 L1 layout fix

Built on 2026-11-18 from the current workspace with the local Haiku RISC-V64
build:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-241
```

Tag240 evidence exposed an L1 address collision. The 119 peer output buffers
start at `0x80000` and occupy 256 float words per worker. Worker80 therefore
starts at `0x80000 + 80 * 256 * 4 = 0x94000`, which was also the Qwen3.5
activation-scale base. Its row10 output overwrote the padded Q5_K scale at
`0x94028` during a multi-wave projection.

Tag241 moves the scale buffer to `0x9dc00`, immediately after the complete
119-worker output range. The largest supported scale vector is 80 words
(320 bytes); it fits before the ROOT result buffer at `0x9e000`. Shared
compile-time assertions enforce this separation and the runtime staging code
checks the scale bound. Projection math, dispatcher validation, the 120-core
schedule, and Work's no-CPU-fallback behavior are unchanged.

The offline build completed with its generated ROOT/peer image import and
instruction checks. The Work owner regression passed with warnings as errors.
The guarded trial/recovery scripts pass `bash -n`, and `git diff --check` is
clean.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010041` | `698c7ea4e7ad0fe6882b2e94af87a9fcc3d52c3c7d2577b5d7b698de2bd01390` |
| Work client | `dd3e91d51f9cc2f08002494cc55ed478b2272d2e42ec65f450e64377e58bfe2a` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `73c1f30a78d8f19d88ecd469f3222e6a7900e01683cd7517669c5ae268704f9e` |
| Recovery helper | `a436385245cb19a68bbd7de5300087c258a110c3576a2ff060de78cad924e3e2` |

Offline output is `/private/tmp/pioneer-qwen35-work-offline-241`. The planned
Haiku stage is `/boot/home/pioneer-qwen35-work-241.K3mP7`.
