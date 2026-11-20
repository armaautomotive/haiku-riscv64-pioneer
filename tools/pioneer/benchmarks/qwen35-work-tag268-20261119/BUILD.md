# Qwen3.5-4B Work tag68: bounded batch-fetch trial

Tag68 adds bounded reads of two complete original-layout Qwen3.5 weight tiles
for ROOT and peer projections. ROOT is built at `-O2`, peers at `-Os`; each
read stays within 8 KiB. The fixed peer text image is 7,952 bytes.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --device-opt-level 2 --peer-opt-level s --batch-fetch \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-268-root2-batch-final
```

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| ROOT image boot | — | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | 55,152 | `25d4209cf295a2e3a127c57279ca6babe7a36502afcf672fdfc099b18e198cce` |
| Peer image boot | — | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | 7,952 | `bc2a5b3fa4846a461a5377241d5efbca1589fbdd921e338b30840919d22db204` |
| Candidate driver68 | 250,976 | `ead16505c7199dd1e9c92c814871e735b5a6fe2ace8ae5ee5269b58a899f137a` |
| Work client | 304,736 | `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d` |

## Haiku result

Tag68 passed guarded preflight from stable driver55 generation14 and
hot-loaded as driver64 generation15 without reboot. Authentication took
`68.208760` s, tokenization `13.165097` s, and model load `221.612372` s for
3,450 spans (`2,740,498,432` bytes). Before prompt processing, the final router
counter audit faulted at `dispatcher_stage=0x54000410`, detail `0x2ce1`.
ROOT and worker119 command snapshots were captured and the prior per-job
proofs passed. The client unloaded cleanly; guarded recovery restored stable
driver55 at generation16 and removed the trial lock without reboot.

The fault records are `work.stdout`, `work.stderr`, `work.exit`, and `state` in
this directory. This attempt did not produce output or a decode rate.

Evidence SHA-256: `state` `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e`,
`work.exit` `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865`,
`work.stdout` `1dc899b2aec037314c5ce20dc39b36557699240619628287e3ad50f31c75e521`,
`work.stderr` `e91fe750a99881525cd466bd168d39937c3680ed03a642edbc5296236a3e27a0`.
