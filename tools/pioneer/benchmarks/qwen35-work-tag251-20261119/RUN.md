# Qwen3.5-4B Work tag251 recovery experiment

Tag251 tested automatic driver recovery when the client closes its device
handle after a terminal ROOT fault without sending Work STOP or BANK END.
This close-time path failed and was removed from the next candidate.

The guarded trial began from stable041 generation8, `cold0 active_owner0`.
Authentication took `69.003763` s and prompt tokenization took `12.495517` s
for 18 tokens. The 4B model load completed and ROOT reached the known
worker119 doorbell fault before prefill, with zero commits:

```text
root_trace=0xd0008210
dispatcher_stage=0x58778210
dispatcher_detail=0x4
```

The client observed the first safe-idle proof and closed the device without
STOP/END. The close call did not return; SSH and ping became unreachable and
remained down for more than three minutes. No generated token, clean Work
unload, or decode rate was produced. The in-progress stdout and stderr were
open during the host stall and are preserved as untrusted raw files under
[`evidence`](evidence).

Recovery used one seven-second OFF action, waited 180 seconds, and sent one
0.75-second ON action after the host remained unreachable. The host booted
Linux under the saved Linux SSH pin. Its strict 64-CPU
`memory128-spin-idle7-top-page-nosd-control` preflight passed, then the pinned
Linux-to-Haiku kexec helper returned Haiku hrev99999 without changing the
default NVMe boot configuration. Candidate tag51 loaded at generation1 with
`cold0 active_owner0`. The hash-pinned recovery helper hot-reloaded stable041
and verified:

```text
P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0
  active_owner0 open_count1 guard1;CPU_METADATA_ONLY
STABLE041_RESTORED
```

The installed driver hash is the original stable041
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`. The
4B and 27B files remain intact at 2,740,937,888 and 18,973,870,528 bytes.
The shared lock is absent and no inference process remains.

Tag250's explicit STOP/END path already recovered the same fault and hot-
reloaded stable041 at generation8 without board reboot. That remains the
verified rebootless recovery path. A client that disappears before STOP/END
is still retained safely and may require cold recovery.

## Artifact hashes

| Evidence | SHA-256 |
| --- | --- |
| Postflight driver status | `19bad0c49c805fc05af3952c4f5fcdf7ad9c063833144baabfc7de495ec41895` |
| Postflight process list | `c141ab00a9997a6d1bcfe8272665b641dd3a26bc28d3c961dab730f295f8af34` |
| Postflight sysinfo | `2595aefff895713ed53ef5f2fe201ec0e2b568183924a63756e99e833247a51b` |
| Postflight model metadata | `dea51d41ef4fc0b6a1787d8e89d0ae84ff7a8bdac1ab3eb0d90bdf04f4a12fc0` |
| Installed driver hash record | `222952c207ef9fc310364849e52df42900ace3def3451b9e02650d9fb0d740bf` |
| Stable recovery state | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |
| Interrupted stdout, post-cycle raw | `0cca47e1a140ac54812608b3c4b1d0fb36fc020e4fb5f106110970b185a47def` |
| Interrupted stderr, post-cycle raw | `3d1ab5df246002409048ce2674fa631d8249b52428968d7d4dd026f128b1fda7` |
