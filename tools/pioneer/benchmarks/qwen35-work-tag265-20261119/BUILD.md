# Qwen3.5-4B Work tag65: O1 ROOT trial

This candidate changes the ROOT image to `-O1` while keeping all persistent peers at `-Os`, because an O1 peer image exceeds its fixed 8 KiB limit. The ROOT image remains within its 128 KiB fixed text limit. This is an isolated performance trial; it is not the stable driver.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 1 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-265-root1-peerS
```

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `850737c3eb9b5fe3843ebfe1d114befc401257dfa321e302d5017d26033d0f3f` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver65 | `d388a575908527b013ce4b81d37f47da5be3d6251cd0f88c3b7a35d0567c6e1a` |
| Work client | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

The tag64 baseline returned both 16-token answers on ROOT and all 119 Tensix peers at `0.092316` decode tokens/s.

## Haiku result

The first staged attempt loaded tag65 at generation7 but stopped before the
client ran because this wrapper mistakenly expected driver ABI version65.
Tag65 is the build tag; the driver ABI remains version64. The guarded
`--recover-wait-failure` path quiesced the idle candidate and restored stable
driver55 at generation8 without a reboot. The trial lock was removed and no
model request or Tensix inference ran in that attempt. The version check was
corrected before retrying in a fresh one-shot stage.

The fresh trial passed its preflight from stable generation8 and loaded tag65
at generation9 without reboot. Work reported host inference disabled.
Authentication took `68.154080` s, vocabulary/tokenization took `13.022836`
s, and model upload/load took `222.682238` s for 3,450 resident spans
(`2,740,498,432` bytes).

Both warm runs returned exactly `A B C D E F G H I J K L M N O P`. Each
processed 23 prompt tokens in about `200.31` s and decoded 16 tokens in about
`143.77` s. Decode rates were `0.111292` and `0.111285` tokens/s. Each run
recorded 7,136 full-core waves and 14,128 projection waves across ROOT and all
119 Tensix peers. This is about a 20.5% improvement over tag64, but remains
about 898 times below the 100 tokens/s goal.

The client unloaded cleanly (`state=2 active=0 cold=0`). Stable driver55 was
restored at generation10 without reboot; postflight verified no active owner
and no trial lock. The wrapper's final run path initially reported a generic
acceptance error on this valid but below-target output. The read-only
postflight correctly classified it as
`TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET`; the wrapper now reports that case
explicitly. Full logs and hashes are in `evidence/root-o1-16tokens/`.
