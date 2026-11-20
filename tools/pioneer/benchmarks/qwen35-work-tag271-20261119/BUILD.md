# Qwen3.5-4B Work tag71: Q5 operand-store reduction

Tag71 removes the Qwen-specific Q5 panel decoder's operand zero pass and
read/modify/write OR operations. It writes each packed operand word once,
after validating all row metadata. The checked dot-product and matrix-engine
path are unchanged. This build uses ROOT `-O2`, peer `-Os`, seven-bank upload,
and the default Work projection path on all 120 Tensix cores.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-271-q5-store-build2
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | — | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 53,980 | `046c437b2de526d6cd1b787c3d646a5e1575453312e38dfa01ee3bc65c05926c` |
| Peer image boot | — | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,444 | `bcf7c2a1dd48e4770f40421044542340810ea5db85d7c4b0e5faf1344e46978e` |
| Candidate driver55 | 248,024 | `7237179ce2cb60e0234d9720e946087d1b478b9ebcc64c7183c5dd7ffb38364a` |
| Reload guard | — | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | 304,736 | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

Offline compilation succeeded, then reproduced the same hashes after removing
an unbuildable SFPU experiment. Before staging, strict pinned SSH confirmed
Haiku, stable driver55 tag `2026102100010012` generation20, `quiesced0 cold0
active_owner0 open_count1 guard1;CPU_METADATA_ONLY`, and no active inference or
Qwen3.8 diagnostic process. The guarded trial verified these conditions again
before the reload and confirmed the Qwen3.8-27B model file remained intact.

## P100A result

Tag71 hot-loaded as driver64 generation21 without reboot. Work mode ran ROOT
plus all 119 Tensix peers; each warm request recorded 7,136 full-core waves and
14,128 projection waves. Both runs returned the exact expected `A B C D E F G
H I J K L M N O P` output, with no CPU inference fallback. The model unloaded
cleanly (`state=2 active=0 cold=0`). The guarded wrapper restored stable
driver55 tag `2026102100010012` at generation22, removed its exclusive trial
lock, and verified there was no active inference process. The 27B setup remained
untouched.

Authentication took `67.570347` s, vocabulary/tokenization took `13.921755` s,
and model load took `222.397310` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 182.182947 | 16 | 131.826220 | 0.121372 |
| 2 | 23 | 182.178410 | 16 | 131.837785 | 0.121361 |

This is about 5.3% faster than tag70's `0.11522` tokens/s, but remains far
below the 100 tokens/s target. The wrapper reports
`TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET`; its exit status is 1 by design for
an unmet rate target, while the inference client itself exited 0 and unloaded
cleanly.

Evidence SHA-256: `state` `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`,
`work.stdout` `514afcc52d6684db71652d3de58b5d049c7d4e00eafec08b430009ee2233485d`,
`work.stderr` `b41b839154848ecb928ff50d016a18c654457e548fcf16eb0df1586bd3acaefe`.
