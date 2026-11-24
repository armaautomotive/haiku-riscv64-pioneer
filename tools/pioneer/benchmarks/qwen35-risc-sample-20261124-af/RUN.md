# Fixed RISC reset-word observation

On 2026-11-24 UTC, restricted driver tag `2026112400030023` was run once after
the Haiku qualification build and the reload, control and recovery simulations
passed. The wrapper checked both complete model hashes, no 27B diagnostic or
other inference process, stable driver55 generation2 idle, the exclusive lock
absent, and the candidate, client, guard and stable-restoration hashes before
the driver swap. Independent full model hashing also matched before the run.

Held-fleet inspection prepared 120 cores. The fixed ARC reset-unit sample
returned all eight words as `ffffffff` in both passes, with a stable ARC boot
word and confirmed restoration of the temporary TLB route. The client ended
the empty session cleanly, and the wrapper restored stable driver55 at
generation4 without a reboot. Independent postflight found `cold0`,
`active_owner0`, no Q35 lock or inference process, and the expected stable
driver and guard hashes. See the receipts and `wrapper.log`. The tag is
consumed. The full post-run hashes of both model files also match; see `post-model-hashes.txt`.

This is an observation of the reset-unit baseline on this particular card.
It does not prove fleet ownership, authorize any reset writes, establish
no-reboot recovery, run matrix projections, produce model output, or measure
decode throughput. No reset or inference was issued by this qualification.
