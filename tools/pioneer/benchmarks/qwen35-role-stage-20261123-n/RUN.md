# Held-role image staging and restoration, stage n

On 2026-11-23 the restricted tag `2026112300030005` driver qualified the
resident rewrite's reader and writer image owner on the Pioneer in Haiku.
Before card access, the wrapper verified that no model or diagnostic process
was running, stable driver55 generation 6 was idle, and the shared lock was
absent. Both model files had their expected ownership, mode and sizes.

The candidate prepared and inspected all 120 held cores, captured their boot
words, shim and extra-L1 image regions, staged and verified the fixed reader
and writer images, then restored and verified the captured regions. It closed
the image mappings and explicitly ended the bank session. The
[client receipt](qualify.stderr) and [filtered serial phases](serial-phases.txt)
record the completed stages. No core was released and no model was loaded or
run. The test took many minutes because its setup path checked and verified
individual writes; it is not a decode-speed measurement.

The wrapper hot-reloaded stable55 without a reboot at generation 8.
Independent postflight found `cold0 active_owner0`, no model or diagnostic
process and no exclusive lock. The 4B and 27B model files retained their
expected metadata; their content hashes were not recomputed. This establishes
the observed held-core image staging and clean restoration only. It does not
qualify concurrent-firmware exclusion, starting the role programs, inference,
or no-reboot recovery from a partial physical write.

Driver SHA-256:
`2940a8c1f306067fc3170da96f3a0ec02da9320286b980caf9fbf8d122f46b2e`.
Client SHA-256:
`ef76ebfa01d675dd43449f9aa8b7b128477f8962a3ecd093975af1c666d85e1d`.
