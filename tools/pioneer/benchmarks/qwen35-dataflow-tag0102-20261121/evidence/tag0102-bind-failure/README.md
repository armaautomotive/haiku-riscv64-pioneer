# Tag0102 first hardware attempt

The guarded candidate was installed from stable driver55 generation 14 as
variant60 tag `2026112100010102`, generation 15. Authentication took
68.232158 seconds and vocabulary setup/tokenization took 12.463031 seconds.
Prompt IDs were `760,6511,314,9338,369`. The uploader transferred the full
2,740,498,432-byte model. The following model-bind ioctl failed before
authority binding or service installation:

```text
P100A_Q35_DATAFLOW ioctl=0000af47 error=I/O error
P100A_Q35_DATAFLOW model setup failed bytes=2740498432 authority_bound=0 install_attempted=0
P100A_Q35_DATAFLOW cleanup=1 fd_closed=1
P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1
```

`0xaf47` is `B_DEVICE_OP_CODES_END` (9999) plus `0x8838`, the
`TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND` operation. In the driver, the
`B_IO_ERROR` return is the dispatch/fleet preflight branch after manifest
validation; it can mean dispatch preparation, fleet route proof, or selecting
the root runtime core. This attempt did not identify the narrower substage.

The client exited 1, produced no generated token or prompt/decode timing, and
released its bank cleanly. The guarded wrapper restored stable driver55 at
generation 16, with `quiesced0 cold0 active_owner0 open_count1`; no reboot was
used. The 27B model file was checked in place before the trial.

| File | SHA-256 |
| --- | --- |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `work.stdout` | `46f95c69a2463aefafa393bafdddafcf8ee10d012a5baa0d1fc53368fb06152d` |
| `work.stderr` | `a889a6e37bab27dc0eddbe508fe2b0f79de8749826822ff7c33e69d7eb1dea49` |
