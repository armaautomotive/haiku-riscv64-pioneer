# Qwen3.5-4B Work tag239 hardware run

Tag239 (`2026111800010039`) passed strict preflight from stable041 generation2
idle. The prior inference client had exited, the shared lock was absent, Haiku
reported all 64 CPUs and full RAM, and both model files were intact.

The guarded run authenticated in 99.032105 s, tokenized the 18-token prompt in
12.739196 s, and loaded/uploaded 3,450 resident spans (2,740,498,432 bytes) in
228.730062 s. It failed before prefill and before producing any token:

```text
state=3 commits=0 pc=0x40064 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002e6 dispatcher_detail=0xa
dispatcher_context0=0xbf61fd40 dispatcher_context1=0xd000a00
scale_before_clear=0 scale_after_clear=0 scale_after_stage_input=0
scale_at_stageband_entry=0 scale_after_initial_validation=0
scale_after_fragments=0 scale_after_state_validation=0
scale_before_batch_object=0 scale_after_batch_object=0
scale_after_batch_build=0 scale_after_batch_fields=0 scale_after_root_job=0
scale_after_worker_table=0 scale_before_dispatch=0 scale_before_validation=0
```

The client printed zero for all diagnostic words because `Poll()` only read
the evidence when the parked PC equaled `WorkFaultPc` (`0x40060`). This fault
reported `0x40064`. The explicit completion marker was `RootFault`
(`0x31463551`); the owner clears and verifies that word for each request before
starting, and the fault park writes it. Therefore tag239's printed diagnostic
words were not valid L1 snapshots. The reliable status is failure before
prefill/token, with no valid decode rate. The underlying slot-overwrite point
remains undetermined.

The client exited with status1 without a clean Work unload, leaving the
candidate driver cold/owned. The guarded recovery used one authorized OFF,
waited 180 seconds, then one ON, and returned through the pinned SD-free
Linux-to-Haiku recovery path. Stable041 was restored at generation2 idle; the
shared lock was absent, no inference process remained, and the 4B and 27B model
files remained at 2,740,937,888 and 18,973,870,528 bytes respectively. The
stage directories are `/boot/home/pioneer-qwen35-work-239.D7vM2` and the prior
tag238 `/boot/home/pioneer-qwen35-work-238.Z6nR3`.

Recovery is complete. The next code change makes the request-cleared
`RootFault` marker sufficient to capture evidence even when the PC is one
instruction beyond `WorkFaultPc`; the owner regression also simulates
`WorkFaultPc+4`.

## Artifact hashes

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55, tag `2026111800010039` | `02fc553c6ae377d6af7630c4611b1512d3f7873e920d809b84b9f27d0ef42dd2` |
| Work client | `dd3e91d51f9cc2f08002494cc55ed478b2272d2e42ec65f450e64377e58bfe2a` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `1ea760acf8db762f0ef7a20cfd755000e529a590e00a56a1285f732f60e40cf6` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `8f5d63ac984ada5568a7d7c04390a3cf115a664b68679281fe0b4df4a5170f83` |
| Recovery helper | `4f2a28f80c36991eb66dd54bc173ff6267c2935c0c88739e3f0d59ff12c709fb` |
| Pre-recovery state / failure state | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| Work exit | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| Work stdout | `dd70cb610dbb019f863b7793bece9ee50e252b0920cc0b36266e9be62545fa08` |
| Work stderr | `08b40508782c254684be08510aaf22a5db2990f96aa0fc516dbf1b8014be757c` |
| Stable041 restore image | `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403` |
| Final recovered state | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |

The local run artifacts are stored beside this file. No valid model output,
warm-run measurement, decode tokens/s value, or successful Work unload exists
yet.
