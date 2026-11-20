# Qwen3.5-4B Work tag72: phase profiling

Tag72 preserves the Tag71 Q5 operand-store change and adds low-overhead ROOT
cycle counters for activation packing, projection setup/result handling, and
matrix dispatch. Counters are accumulated in units of 256 ROOT cycles. The
existing all-core and projection-wave counters remain in place. Work protocol
version 6 reports the added measurements.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-272-profile
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 55,012 | `368f6ebca65026fddb68fff14753c76a7b9aadba22d8f708bedd36576f7175b9` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,444 | `bcf7c2a1dd48e4770f40421044542340810ea5db85d7c4b0e5faf1344e46978e` |
| Candidate driver55 | 251,088 | `2bf85dec57c6ea29774d69b179c23344b91f1c6a343c9a4110e7e74b73765e89` |
| Work client | 304,848 | `95184fad49892b8300c9a493fdee92e5dbe65a4c6b05fb06427af48dd8372609` |

The ROOT and peer links passed their image-size assertions. The strict pinned
SSH preflight confirmed no Qwen3.8 diagnostic or inference process, stable
driver55 tag `2026102100010012` idle at generation22, and the original 27B
model still present at its expected size and permissions. The guarded trial
hot-loaded tag72 as driver64 generation23 without reboot, ran all 120 Tensix
cores, unloaded the model, and restored stable driver55 idle at generation24.
The 27B model remained intact.

Authentication took `67.544492` s, vocabulary/tokenization took `13.594684`
s, and model load took `223.028165` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 182.230910 | 16 | 131.864087 | 0.121337 |
| 2 | 23 | 182.234546 | 16 | 131.865508 | 0.121336 |

Both outputs were exactly `A B C D E F G H I J K L M N O P`; each request
recorded 7,136 full-core waves and 14,128 projection waves. The 100 tok/s
target is not met.

The phase counters count 256 ROOT cycles per unit. With the ROOT at 1 GHz,
that is 0.256 microseconds per unit. The measured activation-pack,
projection-setup/result, and matrix-dispatch intervals were:

| Warm run | Activation pack | Projection setup/result | `q35_dispatch_matrix()` wall time |
| --- | ---: | ---: | ---: |
| 1 | 5.352713 s | 1.513998 s | 157.548382 s |
| 2 | 5.352689 s | 1.513957 s | 157.548246 s |

The matrix-dispatch interval includes the synchronous dispatch and wait for
the Tensix work, so it identifies that combined path as the next optimization
target; it does not isolate host-side dispatch overhead from matrix execution.

The trial wrapper returned `TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET` as
expected. It unloaded cleanly (`state=2 active=0 cold=0`), restored the stable
driver without reboot, removed the exclusive trial lock, and found no active
inference or diagnostic process. Evidence SHA-256: `state`
`507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`,
`work.stdout` `1a1cca1ff1074170fd47b4314dad4ba76ed8ec8c64991f09ab941ea6e243d094`,
`work.stderr` `008c2dbeb82b16b55e30a12075dd3dfa76976491240cf75b4913061501920493`.
