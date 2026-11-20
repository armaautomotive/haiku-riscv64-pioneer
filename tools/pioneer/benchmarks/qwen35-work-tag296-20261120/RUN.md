# Tag96 P100A hardware result

Tag96 did not produce a valid model output. It reached prompt processing, then
the Work dispatcher exhausted its bounded peer-completion polls for worker119
during the vocabulary projection. The guarded fault path stopped the Work
owner, unloaded the client, and restored stable driver55 without rebooting.
This run is not a throughput qualification.

## Preflight and recovery

The guarded preflight confirmed that the earlier diagnostic and inference
processes had exited, stable driver55 was idle at generation8, and both the
Qwen3.5-4B and Qwen3.8-27B model identities were intact. Tag96 installed as
driver64 generation9. After the fault, the owner unloaded cleanly
(`state=2 active=0 cold=0`) and the wrapper restored stable driver55 at
generation10. Postflight reported `SAFE_FAULT_RECOVERY_VERIFIED`; the stable
driver is idle, with no reboot.

## Timings and failure

Authentication took `68.035040` s, vocabulary/tokenization took `12.983022` s
for 23 prompt tokens, and model loading took `227.979075` s for all 3,450 spans
(`2,740,498,432` bytes). Prompt processing began but did not complete, so no
prompt-processing duration, valid output, or decode rate was recorded.

The dispatcher fault was `stage=0x54000233 detail=0x0001a8a9`. That detail
records a transient worker119 state with low sequence bytes `0xa9` and `0xa8`
and active flag `1`; its job had a 130-group vocabulary shard. The later
post-fault snapshot showed the worker at its wait PC with matching completed
sequence values, consistent with the bounded sentinel poll ending just before
completion. The run still correctly failed closed because the root had not
collected completion proof from every peer.

## Captured evidence

Raw files are under [`evidence/tag96-card`](evidence/tag96-card):

| File | SHA-256 |
| --- | --- |
| `work.stderr` | `1231233773500dec7fe345688e4e31bef1f3ef233eea031f3db7c1e7449d4e72` |
| `work.stdout` | `92d5cf3bca90d771a12d65c9e3984e2966806e483c934a6e4ffe76ca2bce402a` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |

The exact postflight output is in [`postflight.txt`](evidence/tag96-card/postflight.txt).
