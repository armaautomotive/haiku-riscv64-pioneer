# Core-verified startup and clean restore, stage s

The restricted tag `2026112300030010` candidate ran after full 4B and 27B
SHA-256 checks, and a fresh stable55 generation-2, process, lock and staged-file
hash preflight. It captured all 120 held workers, staged only worker zero,
and verified that the untouched workers matched their saved images.

With all TRISCs held, worker zero's B and NC startup shims copied their own
private RAM into L1 and each compared its copy against its local RAM before
publishing readiness. The host captured and rechecked both copies while the
shims waited, cancelled their handoff, observed their parked PCs, held them,
and verified that the L1 copies matched the retained host copies. No host
private-RAM read was issued. The handoff, reset-preservation setting and saved
fleet images were restored; mappings closed and the bank session ended cleanly.
See [serial phases](serial-phases.txt) and [postflight receipt](postflight.txt).

The wrapper hot-reloaded stable55 without reboot. Independent postflight
observed generation 4 idle (`cold0 active_owner0`), no model/diagnostic process
or exclusive lock, and expected metadata for both models. The full model hashes
matched before this trial; postflight metadata is not a new content hash.

This is a restricted one-worker startup/cancellation and saved-image restore
qualification, not a proof of concurrent-firmware exclusion, a complete
resident worker service, inference, or recovery from a released worker fault.
No model was loaded and no decode throughput was measured. Candidate driver
SHA-256 `2776db17e4ea8be9e1ddc96395904cc5e6a935eb0706abf800eae8a1bbd8e2fa`;
client SHA-256 `33083362025c9e7f6ea407a962ebbd6b37fad50ffda03abf22aa4dfcccc6385d`.
