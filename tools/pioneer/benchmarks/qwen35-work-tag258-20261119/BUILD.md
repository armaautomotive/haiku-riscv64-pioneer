# Qwen3.5-4B Work tag58: retry safe recovery proofs

Tag `2026111800010058` carries forward tag57's ROOT-scale transfer fix and
repairs two Work recovery failures. The final safe-idle audit now uses the
peer nonce already accepted against the ROOT dispatch ledger instead of
requiring the older cached nonce. A failed proof before the first core reset
returns an explicit retryable result, keeps the Work owner and bank context
available, and lets the client repeat the read-only proof while holding its
driver session open. Failures after restart begins remain quarantined.

Offline warnings-as-errors cross-build:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-258b
```

The `PioneerQwen35::WorkOwner` recovery regression passed with `-Wall -Wextra
-Werror`, including a failed safe proof followed by successful retry.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `6694d4041ce3104a9902f17bd52c61a454edc46235fd70f12f3ef4d00c5605de` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4f104ead3ca0e16159937a95313767c27ac9e2628e2c2cdda6fad0102d8d7c57` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Work client | `6184f2aa1ff825172efd6ef89c1751ad07167d71a6fc0d66a990bfab16fac654` |
| Guarded tag58 trial wrapper | `baa0f339d93d790c24cc63486dfad482eeaed3e2ebc1876e81edc0bc0fe22f80` |

## Hardware trial: 2026-11-19

Tag57's retained owner could not be recovered in place. Following the saved
Pioneer procedure, the board was power-cycled once, booted Haiku, and returned
to stable041. Both model files were verified before the tag58 trial. Tag58
loaded at generation3 from stable041 generation2.

The model authenticated in `99.633210` s, vocabulary/tokenization took
`12.869909` s for the 18-token prompt, and all 3,450 resident spans
(`2,740,498,432` bytes) loaded in `223.299044` s. Before prefill, ROOT faulted
with zero committed tokens at `dispatcher_stage=0x58778210 detail=0x4`:
worker119's 4-byte write to remote doorbell `0x74100` remained pending. The
fault receipt includes the ROOT NoC state and worker119 status/job snapshots.
No valid output or decode rate was produced.

Tag58 completed explicit STOP and clean Work unload
(`state=2 active=0 cold=0`), then quiesced and closed the candidate and
hot-reloaded stable041 at generation4. The postflight independently reported
`SAFE_FAULT_RECOVERY_VERIFIED`; `cold0 active_owner0`, no trial lock, and both
model files remained intact. This hardware run verifies recovery from the
observed inference fault without another reboot. The new retryable proof
failure branch itself is covered by the offline WorkOwner regression; this
hardware run passed its safe-idle proof. Raw logs, exit status, state, and
postflight output are in [`evidence`](evidence).

Two valid warm outputs, prompt prefill, and the 100 decode tokens/s target
remain outstanding. The current board is back on stable041 and idle.
