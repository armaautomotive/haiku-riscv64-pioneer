# Shared-role firmware preflight, 2026-11-22 UTC

Before the firmware query, strict pinned Haiku SSH process inspection showed no
27B or 4B diagnostic. The installed stable driver SHA-256 was
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
Immediately before the query, a fresh process check and absent-lock check
passed and the guard returned:

```text
P100A_RELOAD driver55 tag2026102100010012 generation20 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
```

The existing telemetry client (SHA-256
`0090f6c27a4781e44dcaa76d098ff92c78f891b23caeeb95e9f604e5c4e22dad`)
ran the operating-telemetry selector and exited zero. Relevant output:

```text
Telemetry state 1 (1 OK), TLB restored 1; ARC state 1 restored 1; PCI state 1 restored 1
Table version 00000100 entries 63 mask 00000f7f tags 10018f28 data 1001902c
CSM TLB word 0 before 00000000 trial 00000080 after 00000000
CSM TLB word 1 before 00000000 trial 04004000 after 00000000
CSM TLB word 2 before 00000000 trial 00000a40 after 00000000
Tag 14 AI clock (MHz)           : 00000320 800 MHz
Tag 15 AXI clock (MHz)          : 000003c0 960 MHz
Tag 16 ARC clock (MHz)          : 00000320 800 MHz
Tag 6 Vcore (mV)               : 000002e7
Tag 53 Board power limit (W)    : 0000012c
Tag 54 Input power (W)          : 0000005b
Tag 60 Thermal trip count       : 00000000
Tag 63 AI clock limit max       : not published
Tag 28 Firmware bundle          : 12050001 18.5.0 (suffix 0x01)
Tag 29 CM firmware              : 000e0000
Tag 32 Firmware heartbeat       : 0006db13
Tag 10 Thermal shutdown limit   : 00000000
Card/root commands after 0000/0000; no serial IDs, firmware commands/writes or DMA.
```

An independent subsequent guard query again showed driver55 generation20,
`quiesced0 cold0 active_owner0 open_count1 guard1`; the client and prior
diagnostics were absent from the process list. These are reported version
numbers, not a cryptographic measurement of the firmware image. No model was
loaded and no inference or throughput measurement was performed.

The official `v18.5.0` source tag resolves to commit
[`80b056af360a58730756467d5cc8df8db789640d`](https://github.com/tenstorrent/tt-system-firmware/tree/80b056af360a58730756467d5cc8df8db789640d).
Its initialization sets the L2CPU tile reset field while leaving the RISC
reset field at its documented default, and it exposes Tensix reset and
reinitialization commands. The observed versions do not establish that this
source exactly matches every installed image or exclude later firmware or
external control operations.
