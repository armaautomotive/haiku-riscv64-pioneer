# Qwen3.5-4B Work tag75: verified command-register reuse

Tag75 keeps Tag74's bounded sentinel scheduling and enables the existing
ROOT command-register reuse path for Work transfers. Before omitting any
register write, it reads every command field and compares it to the exact
expected value. A mismatch is written and read back; the existing idle,
counter, source, completion, and physical-peer proofs remain in place. The
ROOT phase counters remain enabled. Work protocol version 9 is unchanged in
size.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-275-cmdreuse
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 56,112 | `18929d20d17783429d6afc5bfcf39c20e6a9f612b4f296ab7c8bb71e6eb0a3ff` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,444 | `bcf7c2a1dd48e4770f40421044542340810ea5db85d7c4b0e5faf1344e46978e` |
| Candidate driver55 | 251,088 | `3fa9eb47624df67f614fbebb6f3a2b0654b57589edfde0e5a3a8fe1b417bb9f0` |
| Work client | 304,848 | `09f3f083c53f36cb6201c420090cf28e9f6e6f82f24fd202f822015f7b9d199f` |

The ROOT and peer links passed their image-size assertions. The guarded
P100A trial started from stable driver55 generation28. It hot-loaded candidate
driver64 generation29 without a reboot, then unloaded the model and restored
stable driver55 idle at generation30. The 27B model remained intact.

Authentication took `67.542853` s, vocabulary/tokenization took `12.817104`
s, and model load took `221.532290` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 183.551546 | 16 | 132.729030 | 0.120546 |
| 2 | 23 | 183.554992 | 16 | 132.725214 | 0.120550 |

Both outputs were exactly `A B C D E F G H I J K L M N O P`; each run
recorded 7,136 full-core waves and 14,128 projection waves. The 100 tok/s
target is not met.

At the 1 GHz ROOT clock, command issuance and ROOT matrix work took about
`55.8001` s per request, peer completion and physical proof took `97.4422` s,
and result/final proof took `6.2346` s. The dispatcher total was about
`159.483` s. Reusing command registers with exact read-before-skip checks did
not remove the peer wait/proof bottleneck; the small rate difference from
Tag74 is not a meaningful speedup.

The wrapper reported `TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET`. The client
reported a clean unload (`state=2 active=0 cold=0`), the stable driver was
idle at generation30, and no reboot was needed. Evidence SHA-256:

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa` |
| `work.stdout` | `ef8ac289acc1522fc71a492aabcc65b2dc9fe40bf44636c52ccc615376ce9469` |
| `work.stderr` | `0bef6793effa090f6a3be6e19d08b32657c53e25e579d9f04eafc330fb9e2f77` |
