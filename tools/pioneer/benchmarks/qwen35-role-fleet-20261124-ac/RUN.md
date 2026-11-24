# 120-pair fleet audit with detailed PC logging

On 2026-11-24 UTC, restricted tag `2026112300030020` ran once after the
full rewrite and reload suites passed, both model hashes matched, and the
stable driver was verified idle with no lock or 27B diagnostic. The preflight,
wrapper, qualification, and serial phase logs are retained here.

The inspection prepared all 120 cores and all 120 reader/writer pairs started.
The fleet audit then failed on core 1, phase 4, with reader samples `0xb4968`
and `0x700cc` and writer samples `0xb7008` and `0xb703c`. The old audit accepted
only the main code slots. Disassembly of the exact staged reader image shows
`0x700cc` is an instruction in its own service shim loop, not the error park:
the observed range is `0x700b4` through `0x700d0`, with the error park at
`0x700e4`. This supports a false audit rejection, but does not prove all 120
pairs would have passed a corrected audit or stopped and restored successfully.

The wrapper exited 1 and reported `cold1 active_owner1`, with snapshots,
mappings and the exclusive lock retained. After capturing evidence and
verifying the staged recovery hashes, one authorized OFF pulse was sent;
after the shutdown interval and an SSH timeout, one ON pulse was sent. The verified staged recovery script restored stable55 generation2 after Haiku
booted. Independent postflight confirms `cold0 active_owner0`, matching driver
and guard hashes, no lock and expected model metadata (`postflight.txt`). Both full
post-run model hashes match their pinned values (`post-model-hashes.txt`).
Tag0020 is consumed.

TRISCs stayed held. This was not inference and measured no decode throughput.
