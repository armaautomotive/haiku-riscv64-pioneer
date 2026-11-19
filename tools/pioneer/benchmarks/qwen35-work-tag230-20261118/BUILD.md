# Qwen3.5-4B Work tag230 build

Built locally on 2026-11-18 from the current HaikuOS workspace for the next
guarded Pioneer trial. Tag230 adds a Work-only peer-fault return-address
capture after tag229 stopped on `Q8PF` before prefill. `Peer()` reports
`dispatcher_stage=0x54000216`; `dispatcher_detail` packs the worker ID in its
high byte and the low 24 bits of the peer call-site PC. The peer stores that
PC in reserved status word5. The generated peer disassembly contains the
expected `sw ra,12(t0)` at `block_fault_park`.

The trial wrapper syncs its output files and exit marker before attempting the
stable-driver restore, so failure evidence is durable before a power-cycle
recovery.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, build tag `2026111800010030` | `65991df9318d052aa8af419b21993f672ac06f4e9257a9e60996fa2b047a8a5e` |
| Work client | `c588a5cafd5a76e09ddf71a00139da783e6b9754850914e9881d9a86694cf513` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4ed80b904ba87ae283c255fc278f36defab4d57916949e0554d1d047e7ada424` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `7341fd15fbe13a79f9ae59d913f530870c3419604e8fa18e2282fafa8107a3bc` |
| `PioneerQwen35WorkImage.h` | `29dd0956ac311e87154acad3c5ffdc6604f961189a59a7a5cbe1cb7193a9df82` |
| `PioneerQwen35WorkPeerImage.h` | `9fe1556f7c9d4b107713881483755695b05f724665e9defdfeb02eee1db4ea4a` |
| Trial wrapper | `8747331f9a185d06c65edd7ebc951759334f9d00ae376b4ea731b63f954211be` |
| Post-power-cycle recovery script | `c0f9c67c67406915378d02d67cb1cc61bf088a5ceb0f1b029024fa8a43d6ef90` |

Expected admission: stable041 generation2, idle/no owner, all64 CPUs and full
RAM, 4B and 27B files unchanged, no inference processes, no shared lock.
Remote trial stage: `/boot/home/pioneer-qwen35-work-230.A7mC92`.
