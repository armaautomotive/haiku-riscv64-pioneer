# One-core pre-stack startup, stage o

On 2026-11-23 the restricted tag `2026112300030006` driver ran after
preflight confirmed stable driver55 generation 8 idle, no model or diagnostic
process, and no exclusive lock. It captured and staged the fixed role images
across 120 held workers. On worker zero, the serial trace then reported both
B/NC pre-stack ready receipts, cancellation and the expected parked PCs. The
three TRISCs remained held; no model was loaded or run.

The next hold/RAM verification step failed before its success marker. The
original candidate did not distinguish a failed hold, a reset-preservation
readback failure, an inaccessible private-RAM copy or a mismatch. The owner
and snapshots were retained, and the candidate reported `cold1 active_owner1`.
See [qualification receipt](qualify.stderr), [serial phases](serial-phases.txt),
and [retained state](state.txt). No token throughput was measured.

The retained stage required the authorized front-panel power cycle because
no in-place recovery for this failure is qualified. After Linux booted, the
hash-pinned helper started Haiku without changing the boot disk. The preserved
stable driver was restored and independently reported driver55 generation 2
with `cold0 active_owner0`, no exclusive lock and expected 4B/27B file
metadata. Subsequent full [model checksums](model-sha256.txt) matched the
recorded 4B and 27B SHA-256 values.

A recovery script transferred while the old driver was cold had the correct
length but an incorrect SHA-256. Its contents included bytes also present in
the separately built driver43 control image. It was not executed; after the
power cycle a fresh script was transferred, matched its local SHA-256 and
successfully restored the stable driver. The cause of the earlier corruption
is unknown. A separate deterministic 1 MiB transfer subsequently matched its
SHA-256 on the restored host, which does not establish the earlier cause.
Preserve the corrupted copy for investigation and hash-check all future
transfers. The next candidate adds precise hold and RAM diagnostics;
this result does not establish physical RAM preservation, clean no-reboot
recovery, fleet startup, valid inference or performance.
