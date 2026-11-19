# Qwen3.5-4B Work tag231 build

Built locally on 2026-11-18 from the current HaikuOS workspace for the next
guarded Pioneer trial. Tag231 adds distinct Work-only peer failure codes for
startup admission, descriptor validation, completion checks, counters, and
saved descriptors. The root reports
`dispatcher_stage=0x54000300|reason`; `dispatcher_detail` packs the logical
worker ID in its high byte and the low 24 bits of the peer call-site PC.
Reason0 means an uninstrumented peer-image fault and is still localized by
the return-address PC.

The peer disassembly retains the Work-only return-address store in
`block_fault_park`. The tag231 trial wrapper also syncs stdout, stderr, and the
exit marker before attempting stable-driver restore.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, build tag `2026111800010031` | `573a657c6a52913f1c3ec15e17a2e727ab36c218dd4d14929516d2b705e8d081` |
| Work client | `c588a5cafd5a76e09ddf71a00139da783e6b9754850914e9881d9a86694cf513` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `ca887bed735b1088fde3755bec8d3abb76bf0906ab0f0ea427a3b8b74827c16a` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `6f8f3abcb725665b2a9560fedce41d98c9a532064e46f1fcad379e94d0678626` |
| `PioneerQwen35WorkImage.h` | `f1b8e653e15b069156ce294e2ed52309349a8e3ffe6bfab85968d88f74e74e1a` |
| `PioneerQwen35WorkPeerImage.h` | `f2ae047af28da039a21b72f4feb556e6884bbae869c7542110e1667103874617` |
| Trial wrapper | `8938e823bc1874cf4acdcd0924ccc1cd02a60867a44a15b5e3170633617d1fc8` |
| Post-power-cycle recovery script | `e0d0c403371024c2afab085e21181da3fd087cd0f21f6c6f14a8e57c960ff706` |

Expected admission: stable041 generation2, idle/no owner, all64 CPUs and full
RAM, 4B and 27B files unchanged, no inference processes, no shared lock.
Remote trial stage: `/boot/home/pioneer-qwen35-work-231.Z4kY8`.
