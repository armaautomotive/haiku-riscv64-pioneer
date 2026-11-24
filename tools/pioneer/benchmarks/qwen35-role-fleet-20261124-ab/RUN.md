# 120-pair resident role qualification

On 2026-11-24 UTC, restricted driver tag `2026112300030019` was run once
after confirming stable driver55 generation6 idle, no exclusive lock or 27B
diagnostic, and the pinned 4B and 27B model hashes. The preflight and raw
wrapper, qualification, and serial phase logs are retained in this directory.

The fleet inspection prepared all 120 cores. All 120 reader/writer pairs
reached the pre-stack phase 3 checkpoint, and all startups completed; the
subsequent fleet service audit failed on core 0. The original binary did not
record which audit predicate failed. Stage 9 reported captured and staged
images, but no restoration or mapping closure. The wrapper returned 1,
reported `cold1 active_owner1`, and retained the exclusive lock and owner.
The evidence does not establish why the audit failed or that the fleet could
be stopped and restored without a reboot.

Recovery was initiated after saving the logs and confirming the staged
recovery script and stable driver backup hashes. A single authorized OFF
pulse was sent, the host remained unreachable after the shutdown interval,
and a single ON pulse was sent. Haiku booted and the staged recovery script restored stable55 generation2.
Independent postflight confirms `cold0 active_owner0`, the expected driver
and guard hashes, no lock or diagnostic process, and expected model metadata
(see `postflight.txt`). Both full post-run model hashes matched their pinned values
(see `post-model-hashes.txt`).
Tag0019 is consumed.

The TRISCs remained held. This was not model inference and measured no
decode throughput.
