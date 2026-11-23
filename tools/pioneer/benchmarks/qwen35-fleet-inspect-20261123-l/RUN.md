# Held-fleet inspection, stage l

On 2026-11-23 the restricted tag `2026112300030003` candidate ran an empty
bank session on the Pioneer in Haiku. Immediately before loading it, the
process scan found no model or diagnostic process, stable driver55 was idle
at generation 2, and the exclusive lock was absent. The candidate driver
SHA-256 was `ed99af51ccb5b6c63a6463aea98ff08588226a58afdec846985cf04efecc5844`.

The driver prepared all 120 dedicated worker leases and the ARC-reset and
held-fleet observations completed. The host-DMA inspection returned failure at
stage 3; its precise subcheck was not recorded. The result reported no failed
route restoration. The explicit bank end succeeded, and the wrapper
hot-reloaded stable55 at generation 4 with `cold0 active_owner0` and no
exclusive lock. The 4B and 27B files retained their expected ownership, mode
and sizes; their content hashes were not recomputed. The raw receipt is in
[`qualify.stderr`](qualify.stderr).

Source review after this result found that the host-DMA source check compared
the root NIU with its pre-lease configuration even though preparation sets
bit 16. This is a plausible cause, not a hardware-confirmed diagnosis. The
follow-up candidate checks the active lease and the full source route. No
worker program, model inference or throughput measurement occurred here.
