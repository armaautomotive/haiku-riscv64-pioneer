# Qwen3.5-4B Work tag246: recover a parked peer fault

Tag `2026111800010046` extends Work STOP recovery to an explicit Q8PF peer
fault. It still refuses to reset an active, changing, or unknown peer. Before
restarting the fleet, the driver repeats a bounded proof of the terminal ROOT,
stable dispatch ledger, all 119 peer statuses and PCs, immutable peer code and
sources, and quiet NoC queues. A peer error is admissible only with the exact
fault marker, a valid return PC in the installed peer image, and two PC reads
at the known fault park.

After that proof, STOP holds ROOT and reboots all 119 peer processors through
the existing `PioneerQ38DispatchBootstrap` contract: it stages each sealed
boot record, zeroed command/status state, checks source and code, records NoC
counters, arms the fleet, and releases it. It waits for every peer to return
to its exact fresh WAIT state with unchanged counters. The existing owner STOP
then restores ROOT memory and the dispatcher closes all 120 cores through its
normal terminal proof. A failed or incomplete stage remains quarantined; it
cannot report a valid model result or trigger CPU inference.

## Offline build and checks

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-246c
```

The build completed. The Work owner regression compiled with
`-Wall -Wextra -Werror` and passed, including nonce bounds, active request,
Q8PF marker, in-image fault return PC, reason-`0x50` panel-count receipt, and
fresh WAIT-state checks. `git diff --check` and the driver build-script shell
syntax check passed. The generated driver reports a 1,088-byte
`RecoverWorkFault` stack frame.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `b1a1055b97d362672443b723400400d27cc0fc1a517b2ac73c6438d7bd742450` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |
| Trial wrapper | `d858c23f450a5e17dc619dc36a3ee4385f893c9f538d61db2f2507963ec97af5` |
| Power-cycle recovery helper | `8319ae1cf8bcbb4235b9ac91cab3ffd051b1e3f6e2e0b351958fb40521e15cfc` |

## Tag245 hardware result

The pinned 64-CPU Haiku profile passed preflight; tag244 recovery restored
stable041 at generation2. Tag245 loaded the 4B model in `225.714405` seconds,
after `101.414734` seconds authentication and `12.996782` seconds prompt
tokenization. It uploaded all 3,450 spans (`2,740,498,432` bytes), then ROOT
faulted before prefill and committed zero tokens at stage `0x54000102`, detail
`0x77`. Peer1 reported the Q8PF marker. Tag245 rejected that peer error and
left the candidate at generation3 `cold1 active_owner1`; no valid output or
decode rate was produced. The original logs and state snapshot are in
`evidence/`.

Tag246 has built offline. Its fault-restart path still needs the guarded
Pioneer hardware run; no rebootless recovery or valid model output is claimed
until that run passes. The 27B model and stable041 backup remain preserved.
