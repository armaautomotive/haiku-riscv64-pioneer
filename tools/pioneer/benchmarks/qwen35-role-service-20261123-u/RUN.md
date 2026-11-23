# Resident reader/writer idle-stop qualification, stage u

The restricted tag `2026112300030012` candidate ran once after both full model
SHA-256 checks and a fresh preflight found the stable driver idle, the prior
27B diagnostic absent and no exclusive lock. The wrapper repeated both model
hashes before swapping drivers. The candidate captured the images of all 120
held workers, staged worker zero's reader and writer code, and audited the
untouched fleet. Its role code matched the rewrite's actual linked resident
images; the separately guarded startup shims diverted a successful idle stop to
the exact stackless local-RAM restore code qualified in stage t.

With all TRISCs held, both roles copied and verified their own private RAM.
After the host checked both copies, it released the roles into their actual
resident service loops, observed both startup receipts and running code PCs,
and requested an idle mailbox stop. It observed both stop receipts, both
local-RAM restore receipts and the two restore-park PCs, then held B/NC and
verified the retained L1 copies. The original mailbox, handoff and reset
preservation were restored. The owner restored and rechecked the saved fleet
images, closed mappings and ended the bank session cleanly. See the
[serial phases](serial-phases.txt), [wrapper output](wrapper-output.txt),
[preflight hashes](pre-model-hashes.txt) and [independent postflight](postflight.txt).

The wrapper returned success and hot-reloaded stable55 without reboot. The
independent postflight observed generation 8 idle (`cold0 active_owner0`), the
expected stable driver hash, no model or 27B diagnostic process, no exclusive
lock and expected metadata for both model files. Both full model hashes matched
before and after the run; see the [postflight hashes](post-model-hashes.txt).

This is a one-worker idle lifecycle qualification. It did not send projection
work, exercise the two-read NoC pipeline, release a TRISC, run model inference,
prove firmware exclusion or demonstrate recovery from a real fault. No decode
speed was measured. The tested candidate driver SHA-256 was
`eb035341f505dc5c406e0e45946fd87b379b857ba58b5ba6fb3b2b1e503fb08a` and client
SHA-256 was `30b493d051eda2cb72a3a80ad937c2d4ad02109c8e1e8a2dbedfdc844e617944`.
The full offline suite, guard tests, wrapper/recovery tests and reproducible
cross-build passed before this run.
