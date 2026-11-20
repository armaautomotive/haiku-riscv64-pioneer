# Qwen3.5-4B Work tag70: two-tile batch-fetch validation

Tag70 uses the same two-tile, at-most-8-KiB fetch for the Qwen3.5 ROOT and all
119 peer images. ROOT uses `-O2`; peers use `-Os`. The dispatcher accounts for
ROOT batch reads identically in its software counter and expected aggregate.
ROOT text is 55,184 bytes; peer text is 7,952 bytes.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s --batch-fetch \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-270-root2-batch
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | — | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 55,184 | `63aeeca15c67eb83046637665a0d3ac79547c1d8ca416d910568faa55cdee6ea` |
| Peer image boot | — | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,952 | `bc2a5b3fa4846a461a5377241d5efbca1589fbdd921e338b30840919d22db204` |
| Candidate driver70 | 250,976 | `e9e001f6e67d2ff5b359f1e9c8cadc8a3a180e3c99d53a822b736022c6856026` |
| Work client | 304,736 | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

## Haiku result

Tag70 passed preflight from stable driver55 generation18, hot-loaded as
driver64 generation19 without reboot, and ran both warm requests on ROOT plus
all 119 Tensix peers with host inference disabled. Each request recorded 7,136
full-core waves and 14,128 projection waves. Both produced the exact expected
`A B C D E F G H I J K L M N O P` output.

Authentication took `67.643567` s, vocabulary/tokenization took `13.588297` s,
and model load took `223.903580` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode measurements are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 192.732584 | 16 | 138.861988 | 0.115222 |
| 2 | 23 | 192.736828 | 16 | 138.856075 | 0.115227 |

This is valid all-core inference, but it misses 100 tokens/s by about 868x and
is slightly slower than O2 tag66 (`0.11618` tokens/s). The client exited
successfully and unloaded cleanly (`state=2 active=0 cold=0`). Stable driver55
was hot-restored at generation20; postflight found no owner or trial lock and
no reboot was needed. The wrapper reports
`TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET` and exits nonzero to keep the 100
tokens/s goal open. `work.stdout`, `work.stderr`, `work.exit`, and `state`
preserve the raw result.

Evidence SHA-256: `state` `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`,
`work.stdout` `d22ac3460b681486e940066bb72cf48bbef7f6c17fcc3d5e25af6837e390ab2f`,
`work.stderr` `ec4fa98c2424498ef2a37a8533c3bf799695373eb398f507bb37319ec66af076`.
