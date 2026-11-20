# Qwen3.5-4B Work tag73: dispatcher phase profiling

Tag73 preserves the Tag72 inference path and adds three cumulative ROOT cycle
counters in the six previously reserved RootControl words. The counters split
each projection dispatch into command/doorbell plus ROOT matrix work, peer
completion/proof, and result gathering/final proof. The Work owner returns
these as data-only measurements. Work protocol version 7 validates their sum
against the measured dispatcher wall time. No NoC traffic or matrix behavior
was added for profiling.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-273-profile
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 55,556 | `f1bc5bffbb3d3fde7ea30d7369abe575e1d48775eefff831bc493aff8ea42cd0` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,444 | `bcf7c2a1dd48e4770f40421044542340810ea5db85d7c4b0e5faf1344e46978e` |
| Candidate driver55 | 251,088 | `2cd223ddf84bab6139803fa7de58d3beb459fdf1afbc172f8a7e30bd706a0582` |
| Work client | 304,848 | `3828c427a33b35f6b10fe7c209393b709e99bae9ef81bbf0d1d8832be356be22` |

The ROOT and peer links passed their image-size assertions. Strict pinned SSH
confirmed the earlier 27B diagnostic had exited, the original 27B file was
intact, and stable driver55 tag `2026102100010012` was idle at generation24.
Tag73 hot-loaded as driver64 generation25 without reboot, used all 120 Tensix
cores, unloaded cleanly, and restored stable driver55 idle at generation26.
The 27B model remained intact.

Authentication took `67.532080` s, vocabulary/tokenization took `12.812616`
s, and model load took `221.390849` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 182.234621 | 16 | 131.862021 | 0.121339 |
| 2 | 23 | 182.233046 | 16 | 131.869645 | 0.121332 |

Both outputs were exactly `A B C D E F G H I J K L M N O P`; each request
recorded 7,136 full-core waves and 14,128 projection waves. The 100 tok/s
target is not met.

Dispatcher phase counters are raw ROOT cycles; at the 1 GHz clock, one billion
cycles is one second. The phases sum to within about 0.006 s of the separately
measured `q35_dispatch_matrix()` wall time:

| Warm run | Validation/commands/ROOT matrix | Peer wait and physical proof | Result gather and final proof | Matrix dispatcher wall time |
| --- | ---: | ---: | ---: | ---: |
| 1 | 55.904471 s | 95.342690 s | 6.298996 s | 157.552167 s |
| 2 | 55.904520 s | 95.342702 s | 6.299009 s | 157.552252 s |

Peer completion and physical proof account for about 60.5% of the dispatcher
interval and are the next optimization target. These counters locate the wait
and proof boundary; they do not separate the underlying NoC transfers from
ROOT polling time.

The guarded wrapper reported `TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET` as
expected. It unloaded cleanly (`state=2 active=0 cold=0`), restored the stable
driver without reboot, removed the exclusive trial lock, and found no active
inference or diagnostic process. Evidence SHA-256: `state`
`507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`,
`work.stdout` `d7df0b3792e5600a522b5279d7171817b93fd6ce5233d3f3128d849d7dd771ea`,
`work.stderr` `0a328d72f7eb0cadf9ae165c63f3d7ecf8b7544ced4faa28d95bb1355a020095`.
