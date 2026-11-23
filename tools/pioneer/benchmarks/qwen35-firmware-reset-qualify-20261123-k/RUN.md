# Firmware-reset qualification, stage k

The restricted qualification on Haiku used driver tag `2026112300030002`
(SHA-256 `638b3a74449e0574ccfe11e0ba078632f1c7f19c3e61d0ef64326ab04a2b5c15`)
and client SHA-256 `50ef9ee8086c94a4149cabd889fefcd824e9955897df25bbe8dfdde5cae7986d`.
Before card access, the script verified that no model or diagnostic process
was running, stable driver55 was idle at generation 2, the exclusive lock
was held for this test, and both 4B and 27B files had their expected metadata.
Those metadata checks are not full-content hashes.

The empty-session firmware-reset qualification did **not** complete. It
reached firmware stage 5 and failed with `detail=0x44248006`; the host-DMA
audit observed root PCIe Device Status `0x0006`. Root AER Uncorrectable
Status was `0x00000000` before reset and `0x00000001` after reset. Both were
still clear immediately after the reset request was sent. Sampled root Link
Status remained `0x8081`; Root Error Status, Error Source ID and all four TLP
header words were zero. These samples do not identify the source, establish
that the link is safe, or determine whether the first endpoint poll played a
role. The client reported `reset_completed=0` because the complete recovery
and audit did not succeed. No model was loaded and no tokens or throughput
were measured. See [qualify.stderr](qualify.stderr) for the raw evidence.

After failure, CPU-only status reported driver60, generation 3, `cold1` and
`active_owner1`. The test retained its lock and owner. Following one authorized
OFF hold, a rest of more than three minutes, and one ON pulse, the Pioneer
booted Fedora from NVMe. Its hash-pinned helper handed off to Haiku, and the
stage-k restoration script returned the stable driver. Independent postflight
reported driver55 generation 2, `cold0 active_owner0`, no diagnostics and no
exclusive lock. Both model files retained the expected owner, mode, link
count and size; this is not a full-content hash. The installed stable driver
SHA-256 was `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
