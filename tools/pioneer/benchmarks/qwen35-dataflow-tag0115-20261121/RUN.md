# Qwen3.5 dataflow rewrite tag0115 hardware run

The guarded run completed authentication, vocabulary/tokenization, and the
full resident model load. Authentication took `99.897654` s; vocabulary load
and prompt tokenization took `13.042067` s; model load took `330.400460` s
and uploaded `2740498432` resident bytes. The first prompt embedding failed
on core0 after `0.611537` s with worker failure7 (`EmbeddingWeightRead`),
phase5. No valid output token was produced, so decode tokens/s is unavailable;
the second warm run was not started.

The bounded recovery wait exhausted all `65536` polls and reported
`recovery_audit_detail=260` (`0x104`): index4 in the NoC status scan, the first
queue-status register (NoC0 queue0 at offset `0x240`), remained busy. Tag0115
did not capture that register's value. The all-core audit therefore rejected
acknowledgement, CLOSE failed its core0 stop audit, and the wrapper refused
in-place restore while the candidate reported `cold1 active_owner1`. It kept
the exact stable55 backup and exclusive lock.

The candidate and client exited with code1. After collecting logs, the
authorized SD-free reboot handoff returned the card to a fresh tag0115
generation1; the guarded post-cycle helper restored stable55 generation2 and
cleared the lock. The 4B and 27B files remain intact. This is not a successful
recovery without reboot and is not a model performance result. Raw output,
audit details, and recovery snapshots are in `evidence/`.
