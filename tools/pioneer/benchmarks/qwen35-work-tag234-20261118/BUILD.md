# Qwen3.5-4B Work tag234 build

Built locally on 2026-11-18 after tag233 reached the dispatcher and reported
peer1 fault `0x50` (`panels != count`). Qwen35 Work stores its ROOT projection
scale cache at `0x94000`; the reused Qwen38 persistent peer path had fetched
scales from its older ROOT address `0xab000`. The Qwen35 peer now fetches from
the Qwen35 ROOT's `0x94000` cache. Other persistent images retain their
existing address selection.

The failure path also stores the actual and expected panel counts in the
existing peer status block, without adding NoC transfers. The ROOT checkpoint
encodes worker and peer reason in the stage and both 16-bit counts in the
detail, so a repeat refusal can be distinguished from an ordinary matrix
completion.

The trial is still Work-only: admission requires `hostInference==0`; no CPU
inference path is present; projection and tied-output matrices use all 120
Tensix cores when the row schedule fills the fleet. The wrapper requires two
valid warm outputs, at least 100 decode tokens/sec for each, nonzero full-core
waves, projection waves at least equal to full-core waves, clean unload, and
stable041 restoration.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010034` | `b22f267b5008de22765086d488b14136c1a6961d026f0e06bf7a403cc62256c6` |
| Work client | `2a430e7fc80e41eacf2ccef526e32f2badabfdf83cae19527492fe427f047ea4` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `e8a28cff6ab12732f1a4b8c747ea061a0d23dfa947ea3db9ba7412208398c494` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `663e9f1e15df92abde157c5665a0ae41c076adc11db924617eb8188f4f326da3` |
| Post-cycle recovery script | `ebd7d9fdc806a887e0200061e8eddaaafceb73d124c6c64e360a54139faa30d1` |

The build completed with undefined-import and unsupported-instruction checks
enabled. Linked peer disassembly shows the Qwen35 path loading the `0x94`
address page for scale sharing.

Remote trial stage: `/boot/home/pioneer-qwen35-work-234.Gc44x`.
