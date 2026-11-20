# Qwen3.5-4B Work tag74: sentinel peer polling

Tag74 enables the existing bounded late-worker sentinel poll for the Qwen3.5
Work dispatcher. It waits on a peer assigned the largest group, then checks
every remaining peer with the same current status, two physical PC reads,
reset proof, and completion conditions. The sentinel is only a scheduling
hint; it does not stand in for any peer's proof. ROOT phase counters from
Tag73 remain enabled. Work protocol version 8 is unchanged in size.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-274-sentinel
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | 4 | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 56,040 | `c81ebcbc02b992a49014c3802a269874b91e2766c9424cf007ddcc6381b12a77` |
| Peer image boot | 4 | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,444 | `bcf7c2a1dd48e4770f40421044542340810ea5db85d7c4b0e5faf1344e46978e` |
| Candidate driver55 | 251,088 | `afcb6f39c1035e4699738fb7d96a0961090b7bcea3df315d025ecd55537a7d50` |
| Work client | 304,848 | `1d320998c3a5ba255f35c0d004656499f09c759575b03fa47da58d974057f797` |

The linker map confirms `WaitPeers` and `PeerPollBackoff` are inside the loaded
ROOT text image. The ROOT and peer links passed their image-size assertions.
Strict pinned SSH confirmed the earlier 27B diagnostic had exited, the 27B
file was intact, and stable driver55 tag `2026102100010012` was idle at
generation26. Tag74 hot-loaded as driver64 generation27 without reboot, used
all 120 Tensix cores, unloaded cleanly, and restored stable driver55 idle at
generation28. The 27B model remained intact.

Authentication took `67.547995` s, vocabulary/tokenization took `12.889718`
s, and model load took `222.551040` s for 3,450 resident spans
(`2,740,498,432` bytes). Prompt and decode timings are separate:

| Warm run | Prompt tokens | Prompt seconds | Decode tokens | Decode seconds | Decode tokens/s |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 23 | 183.843630 | 16 | 132.928275 | 0.120366 |
| 2 | 23 | 183.844920 | 16 | 132.926940 | 0.120367 |

Both outputs were exactly `A B C D E F G H I J K L M N O P`; each run
recorded 7,136 full-core waves and 14,128 projection waves. The 100 tok/s
target is not met.

At the 1 GHz ROOT clock, dispatcher cycle counts convert to seconds by dividing
by one billion:

| Warm run | Validation/commands/ROOT matrix | Peer wait and physical proof | Result gather and final proof | Matrix dispatcher wall time |
| --- | ---: | ---: | ---: | ---: |
| 1 | 55.899866 s | 97.582419 s | 6.295906 s | 159.784153 s |
| 2 | 55.899856 s | 97.582027 s | 6.295909 s | 159.783754 s |

Compared with Tag73, the sentinel candidate was slightly slower: decode fell
from about `0.12134` to `0.12037` tok/s, and peer wait/proof rose from about
`95.343` to `97.582` s. The sentinel preserves per-peer proof but does not
improve this workload, so it is not a speedup candidate.

The wrapper reported `TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET` as expected.
It unloaded cleanly (`state=2 active=0 cold=0`), restored the stable driver
without reboot, removed the exclusive trial lock, and found no active
inference or diagnostic process. Evidence SHA-256: `state`
`507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `9a271f2a916b0b6ee6cecb2426f0b3206ef074578be55d9bc94f6f3fe3ab86aa`,
`work.stdout` `26821133a7f2f0f9313bc0c042c1a5129c939aacf4d49cb0858a4dd998717143`,
`work.stderr` `0105f8afdc9649d4127d0229cf80344b1373dc9cab3b6d38efcce359b3a9730d`.
