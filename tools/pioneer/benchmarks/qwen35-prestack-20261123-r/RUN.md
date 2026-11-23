# One-core pre-stack startup, stage r

Tag `2026112300030009` ran after stable55 generation-2 idle, process, lock and
staged-file hash checks. It captured the held fleet and staged one worker. B
and NC both announced readiness, accepted cancellation, parked, and were held
again with the reset-preservation value verified.

The first B private-RAM read was issued and returned to the driver, but the
immediately following authority check failed (`step=5 issued=1`). The returned
word was withheld and never compared. Later diagnostic samples found the
mapping bound and BAR pinned while the route and read authority checks failed.
This does not prove why those checks failed or whether the read caused the
failure. The session retained its owner cold. See [serial phases](serial-phases.txt)
and [qualification receipt](qualify.txt). No inference or throughput was
measured.

Before recovery, the pre-staged stable55 image and recovery script matched
their pinned SHA-256 hashes. The authorized single OFF/ON power cycle and
pinned Linux-to-Haiku path returned to Haiku, and the verified pre-staged
recovery script restored stable55 generation 2. Independent [postflight](postflight.txt)
showed the driver idle, no diagnostic process or lock, and expected model
metadata. The subsequent full [model hashes](model-sha256.txt) matched both pinned model artifacts.
