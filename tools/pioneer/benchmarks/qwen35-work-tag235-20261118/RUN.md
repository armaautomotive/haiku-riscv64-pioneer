# Qwen3.5-4B Work tag235 diagnostic run

Before launch, strict Haiku SSH verified that the earlier diagnostic had
exited, stable041 was idle at generation 2 (`cold0 active_owner0 guard1`), the
shared lock was absent, 64 CPUs and full RAM were visible, and both Qwen model
files matched their expected sizes. The guarded wrapper repeated the checks
immediately before swapping in tag235.

Model authentication took 98.691868 s, prompt tokenization took 13.515672 s
for 18 tokens, and model upload took 225.917047 s for 3,450 resident spans
(2,740,498,432 bytes). The run failed before prefill and produced no token or
decode-rate measurement. Its terminal record was
`state=3 commits=0 pc=0x40060 progress_magic=0x31503542 prefilled=0
root_trace=0xe500000e fault_marker=0x31463551
dispatcher_stage=0x54f002da dispatcher_detail=0x0000000a`.

The new receipt maps stage `0x54f002da` to source line 730 in
`blackhole_qwen38_dispatch.cpp`, the padded-scale validation. Detail 10 is the
first rejected scale block. The dispatcher returned zero, causing
`StageBand()` reason 14. This narrows the failure to a bad scale-cache value
at block 10, but tag235 did not capture the raw value, tensor type, or input
width. The follow-up should include those values in a failure-only receipt
before changing scale validation or clearing behavior.

The candidate ended `cold1 active_owner1`, so the trial wrapper stopped before
driver restore. The client had exited. Recovery used one controlled OFF and
ON cycle, then the pinned Linux-to-Haiku SD-free helper. The recovery script
quiesced tag235 and restored stable041 at generation 2
(`cold0 active_owner0 guard1`); the shared lock is absent. Haiku has 64 CPUs
and 137,034,936,320 bytes maximum RAM. Both the 4B and 27B model files remain
intact.

| Artifact | SHA-256 |
| --- | --- |
| Final `state` (`STABLE041_RESTORED`) | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `state.pre-recovery` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `5fac4e3f6f74038e03323413af63f01139e9a4a3796d81a53706886cac3d005f` |
| `work.stderr` | `bf188dad0d4e447df47e8f347574d4e771682ce35c7a6d0c8bdae41d476de52d` |

Build hashes are in [BUILD.md](BUILD.md). No valid warm output or decode
speed has been achieved.
