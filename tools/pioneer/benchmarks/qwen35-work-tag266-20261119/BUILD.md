# Qwen3.5-4B Work tag66: O2 ROOT trial

This candidate compiles the ROOT image with `-O2` and keeps all persistent
peers at `-Os`. The O2 ROOT image is 54,108 bytes, within the fixed 128 KiB
text limit. The peer image remains 7,572 bytes, within its fixed 8 KiB limit.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-266-root2-peerS
```

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `1e1bbddc68ca04a02f2c3377c034ead001a99eeb597d9811e1809fc68bf5a8ca` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver66 | `eeaf9314c803dccad60e7b040e24de33956955c83bd52dbaf3e19251ce0e66e0` |
| Work client | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

## Haiku result

Tag66 passed guarded preflight from stable driver55 generation10, then
hot-loaded at generation11 without reboot. Work reported host inference
disabled. Authentication took `67.572652` s, vocabulary/tokenization took
`13.482827` s, and model upload/load took `223.667993` s for 3,450 resident
spans (`2,740,498,432` bytes).

Both warm runs returned exactly `A B C D E F G H I J K L M N O P`. Each
processed 23 prompt tokens in about `191.20` s and decoded 16 tokens in about
`137.72` s. Decode rates were `0.116179` and `0.116181` tokens/s. Each run
recorded 7,136 full-core waves and 14,128 projection waves across ROOT and all
119 Tensix peers. O2 is about 4.4% faster than O1 and about 25.8% faster than
the `0.092316` tokens/s tag64 baseline. It remains about 861 times below the
100 tokens/s goal.

The client unloaded cleanly (`state=2 active=0 cold=0`). Stable driver55 was
hot-restored at generation12 without reboot. Read-only postflight confirmed
no active owner and no trial lock. Raw output and hashes are in
`evidence/root-o2-16tokens/`.
