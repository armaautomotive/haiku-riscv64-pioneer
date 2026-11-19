# Qwen3.5-4B Work tag233 build

Built locally on 2026-11-18 for the next guarded Pioneer trial. Tag233 adds a
ROOT dispatcher entry stub that records entry at the existing owned L1
checkpoint and mirrors a marker into the existing progress word before tail
calling the compiled dispatcher body. This distinguishes a fault before
dispatcher entry from a fault inside the dispatcher without changing the
number or type of NoC transfers.

The stub is linked at `q35_dispatch_matrix` (`0x40068`); the body is linked at
`q35_dispatch_body` (`0x48e40`). The linked disassembly shows both stores and
the tail jump at the entry point. The normal `Checkpoint()` path mirrors each
subsequent stage into that same progress word. Successful completion still
replaces the owned status words with the strict result proof.

The trial remains Work-only. Admission requires `hostInference==0`, the client
accepts no CPU inference path, and projections and tied output are dispatched
through the Tensix matrix engine across 119 workers plus the ROOT core (120
Tensix cores total). The wrapper gates each of two warm runs on valid output,
at least 100 decode tokens/sec, nonzero full-core waves, and projection waves
at least equal to full-core waves; it also requires clean unload and restores
stable041.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, build tag `2026111800010033` | `22e6b2f803893b6e7a2aa2fd5a12eb55ece1e79a6f7a312cdebb797c4f2abf23` |
| Work client | `2a430e7fc80e41eacf2ccef526e32f2badabfdf83cae19527492fe427f047ea4` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `2d00db63889ba6e8ae524a49174b5e2986bfd527777dbbdde082348acd49a5d2` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `6f8f3abcb725665b2a9560fedce41d98c9a532064e46f1fcad379e94d0678626` |
| ROOT image header | `a9ff5c6415d18e4342c0daab80e3d060f8e4d3cf72679a45ca9f9d2114dd5579` |
| Peer image header | `f2ae047af28da039a21b72f4feb556e6884bbae869c7542110e1667103874617` |
| Trial wrapper | `61b8213340c5da954cfc8e5395b8627780ce151db0cf30e86e5b1d672f7a57d5` |

| Recovery script | `3c6c77797961b45f4e855d5b5f697b82abad1cf88f091853c557a600c9b0ce91` |

The build completed with undefined-import and unsupported-instruction checks
enabled.

Remote trial stage: `/boot/home/pioneer-qwen35-work-233.Gc43x`.
