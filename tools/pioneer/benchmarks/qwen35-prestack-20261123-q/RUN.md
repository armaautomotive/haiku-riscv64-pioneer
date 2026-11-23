# One-core pre-stack startup, stage q

The tag `2026112300030008` candidate ran after a fresh stable55 generation-2
idle, process, lock and staged-file hash preflight. It captured the held fleet,
staged one worker and checked the other workers' captured images. Worker zero's
B and NC shims reached ready, accepted cancellation and parked. The driver held
them again and verified reset preservation. The first B private-RAM read then
failed before any data comparison. The session retained its owner cold.

The new failure marker reports the private-RAM view bound and BAR pinned, but
post-failure route and read-authority samples were false. These are later
samples; they do not determine which check failed first, or whether the memory
read was actually issued. No RAM contents, inference or throughput were
reported. See [serial phases](serial-phases.txt) and [qualification
receipt](qualify.txt).

Before recovery, the pre-staged stable55 image and recovery script matched
their expected SHA-256 hashes. The authorized single OFF/ON power cycle and pinned Linux-to-Haiku helper
returned the machine to Haiku. The hash-verified, pre-staged recovery script
restored stable55 generation 2. Independent [postflight](postflight.txt)
showed the driver idle, no diagnostic process or exclusive lock, and expected
metadata for both models. Model content hashes were not repeated after this run.
