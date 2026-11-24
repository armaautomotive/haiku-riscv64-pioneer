# Traced firmware-reset qualification

The restricted empty-session firmware-reset driver used unique tag
`2026112400030022`, driver SHA-256
`19d22e32d4665ffc1dd9ac367284e09daa7505e3b3b12e073c74f3437e5c6d45`,
and client SHA-256
`11a6b6406ab4368336455d9390d03a6913002c037bb077122367f748c5f7604e`.
Before card access the one-shot procedure checked both full model hashes,
processes, stable driver55 generation4, exclusive lock and the staged stable
backup. No model upload or inference was performed.

The test failed at firmware stage5 with `detail=0x44248006` and retained the
owner. CPU-only status after the test was driver60 generation5, `cold1
active_owner1`; the client exited and the stage remained locked. Samples 0
and 6 show root Device Status and uncorrectable AER clear before reset and
immediately after the firmware reset trigger. Sample 7 is taken after the
initial 100 ms wait, before the first endpoint vendor or marker poll, and
already shows Device Status `0x0006` and AER Uncorrectable `0x00000001`.
Samples 8 and 9, taken after the first vendor and marker reads, retain those
values. Thus the error arose between the trigger observation and the first
endpoint poll; it cannot be attributed solely to those two endpoint reads.
The observations do not identify the root cause or prove recovery. Link
Status remained `0x8081`; no AER acknowledgement or status clearing was
attempted. See `qualify.stderr` for raw samples.

A single authorized seven-second OFF press was completed at approximately
12:37:17 UTC. A restart and stable-driver restoration are in progress; see
the current session notes for their outcome. This is not a projection,
model-output, no-reboot recovery or throughput qualification. The tag is
consumed and must not be installed or run again.

After the authorized OFF/ON cycle, Fedora booted from NVMe. Its pinned Linux
handoff helper (SHA-256
`8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff`)
prepared and booted the identity-based 64-CPU Haiku profile. The optional
journal-view profile rejected the Samsung disk's changed Linux device name;
the normal profile resolved the pinned Samsung identity and completed its
read-only check. The staged restore script verified fresh boot and restored
stable55 generation2. Independent CPU-only status was `cold0 active_owner0`;
the exclusive lock was absent and no inference or diagnostic remained.
The installed stable driver and guard hashes matched their pinned values.
Both post-recovery full model hashes matched their pinned values; see
`post-model-hashes.txt`.

For interpretation, Linux's [PCI register definitions](https://github.com/torvalds/linux/blob/master/include/uapi/linux/pci_regs.h)
label uncorrectable-status bit 0 as undefined; they identify Data Link
Protocol Error with a different bit. The observed `0x00000001` alone cannot
be named as a specific PCIe failure. Continue investigation without clearing
or ignoring the error.
