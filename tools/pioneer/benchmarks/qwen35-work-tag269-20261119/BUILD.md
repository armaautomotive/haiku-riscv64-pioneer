# Qwen3.5-4B Work tag69: ROOT batch counter correction

Tag69 retains the two-tile bounded fetch from tag68 and adjusts the expected
ROOT read count to count batch requests. ROOT is built at `-O2`, peers at
`-Os`; the peer image remains 7,952 bytes.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s --batch-fetch \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-269-root2-batch
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | — | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 55,192 | `7a9c15e000fbdbf294f4288eb1340ac2378f5a29197f0a05a354f247b027587d` |
| Peer image boot | — | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,952 | `bc2a5b3fa4846a461a5377241d5efbca1589fbdd921e338b30840919d22db204` |
| Candidate driver69 | 250,976 | `61a8b684f774ad66a412e21f5988aa6d219e6d10662d0f82cf6503db97e1dae9` |
| Work client | 304,736 | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

## Haiku result

Tag69 passed preflight at stable driver55 generation16 and hot-loaded as
driver64 generation17. Authentication took `68.443985` s, tokenization
`13.147074` s, and model load `223.953868` s. It faulted before prefill at
`dispatcher_stage=0x54000403`, detail `0x2cb9`; the router snapshot showed
`0x2cc4` reads against the expected `0x2cb9`. Inspection found that the
expected ROOT count used batch requests while the accumulated ROOT read
counter still added original tiles. Tag70 changes both counts to the same
batch-request total.

The client unloaded cleanly, and guarded recovery restored stable driver55 at
generation18 without reboot. This run produced no output or decode rate. Raw
`work.stdout.raw`, `work.stderr.raw`, `work.exit`, and `state` are saved beside
this report.

Evidence SHA-256: `state` `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`,
`work.stdout.raw` `e2fb99db83005a3f64db7963738826b400bcdf684862c42238c1d253d582b033`,
`work.stderr.raw` `f3333d6efced6c7b5f882b1d07db02f1ccb3182ddae18000489facadaded0208`.
