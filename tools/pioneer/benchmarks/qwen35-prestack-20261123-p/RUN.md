# One-core pre-stack startup, stage p

The separately tagged `2026112300030007` candidate ran on 2026-11-23 after
full 4B and 27B SHA-256 checks and a fresh stable-driver, process and lock
preflight. It staged all 120 held workers and released only worker zero's B
and NC processors. Both startup shims published their ready and cancellation
receipts and reached the expected parked PCs. The driver then wrote the hold,
observed the held reset value, and verified the reset-preservation register.

The next operation, the first attempt to read B's private RAM, returned false
at offset zero. No RAM value or comparison result was obtained. The session
retained the owner as `cold1 active_owner1`; see the [serial phases](serial-phases.txt).
This identifies the failed operation, but does not yet establish whether its
range check, mapping, route, or authorization failed. No model inference or
decode throughput was measured.

The recovery script and stable driver backup were transferred and verified
before the test; both still matched their SHA-256 values after the failure.
After an authorized power cycle and the pinned Linux-to-Haiku path, the
stable driver was restored and independently reported generation 2 idle,
with expected model metadata and no exclusive lock. See the [client
receipt](qualify.txt) and [postflight](postflight.txt). The next candidate
instruments the actual read path and remains unqualified on hardware.
