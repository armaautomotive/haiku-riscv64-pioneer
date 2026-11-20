# Qwen3.5-4B Work tag67: O3 ROOT trial

The tag67 candidate builds the ROOT image at `-O3` and keeps the persistent
peer image at `-Os`. ROOT text is 73,576 bytes, within the fixed 128 KiB
limit; peer text is unchanged at 7,572 bytes.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 3 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-267-root3-peerS
```

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `0dde4b4658cda6f2d58558a452cc79890fe33000b830f9ece7508d37024ec5d4` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver67 | `dcd1983d670b08782a43037d16ee9928190d7a7b64639a11000f9af2cfcc4bdb` |
| Work client | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

## Haiku result

Tag67 passed preflight from stable driver55 generation12 and hot-loaded at
generation13 without reboot. Authentication took `69.038742` s,
vocabulary/tokenization took `12.896546` s, and model upload/load took
`226.282604` s for 3,450 resident spans (`2,740,498,432` bytes).

Both warm runs returned the exact expected answer. Each processed 23 prompt
tokens in about `194.058` s and decoded 16 tokens in about `139.705` s. Rates
were `0.114526` and `0.114529` tokens/s, slower than O2 tag66's `0.11618`.
Each run still recorded 7,136 full-core waves and 14,128 projection waves.
The O2 image remains the fastest tested build; 100 tokens/s is unmet by over
three orders of magnitude. Raw output and checksums are in
`evidence/root-o3-16tokens/`.

The client unloaded cleanly. Stable driver55 was hot-restored at generation14
without reboot; postflight verified no active owner and no trial lock.
