# Pioneer stable-driver recovery, 2026-11-22

The prior tag0120 dataflow process and 27B diagnostic had exited. Read-only
status still showed tag0120 generation3 `cold1 active_owner1`; the recovery
lock and stable55 backup were present. The installed tag0120 driver and
`restore.incoming` matched their recorded SHA-256 values. Both model files
were present at their recorded sizes.

After filesystem sync, one authorized seven-second front-panel OFF hold was
sent. Haiku and Linux SSH both timed out after more than three minutes. One ON
pulse booted Fedora from the existing NVMe. The saved Linux-to-Haiku helper
matched SHA-256 `8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff`.
Its pinned 64-CPU `memory128-spin-idle7-top-page-nosd-control` profile
prepared and booted normally, verifying its image, kernel bundle, kexec
executable, FDT and profile inputs. No SD card or relay channel 2 was used.

Fresh Haiku reported tag0120 generation1 `cold0 active_owner0`. The saved
post-power-cycle recovery script matched SHA-256
`08e3bd34d89a57076544bc6dc6e44b5e907de7fac7acb350353f7b04ad5f2a0b`
and restored the preserved stable driver without accelerator execution.
Independent status and process checks then reported:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY
LOCK_ABSENT
STABLE041_RESTORED
```

The installed stable-driver SHA-256 is
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
The 4B model is `2740937888` bytes and the preserved 27B model is
`18973870528` bytes. File sizes are not a fresh full-content hash. No model
run, throughput measurement, or in-place reset test was performed. The loaded
guard remains the old `B_KEEP_LOADED` image and does not implement the new
recovery API; activating that guard requires staging it and another fresh
Haiku kernel boot. The recovery prototype remains disabled and unqualified.
