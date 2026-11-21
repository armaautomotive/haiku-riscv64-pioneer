# Qwen3.5 dataflow worker-start diagnostics

Tag0105 passed model authentication, upload, binding, and route proof, then
failed at the resident worker-ready barrier. Tag0106 adds per-core startup
diagnostics and bounded rollback for the exact firmware startup-fatal loop.
The diagnostic reports stage, core, mailbox state, failure, and phase. During
close, the driver validates runtime identity, the stable fatal mailbox, drained
queues, and NoC state before holding a startup-fatal core and restoring all
captured core state. Other failure states remain fail-closed and keep the
recovery latch.

Built with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121n
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010106` | `fde934a00df792bfdc67e7dd539d7720aacee210521ac95ea15ed47673ab2f03` |
| Haiku benchmark client | `09b18b00baeecc88d9226f5ebbd3303a1c6d1ff7f3ca80175077acdc457bd8f2` |
| Resident service text | `2e0e144040f9c29f9679d5300ff923847c75216b3bf0e577366be4e929e5f8c8` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Guarded trial wrapper | `f2e477818b25724209944556813ba6663894ce4ea96079e918f6b42c7bf1d04b` |
| Post-power-cycle recovery helper | `fe3eee0ad63cbf9ef1fd9eae49ba3f348cda9423386d0f0f455f0a74db2e5287` |

The service owner and PCI memory tests pass with AddressSanitizer and
UndefinedBehaviorSanitizer. The Haiku target compile passes for RV32IMF without
the RISC-V A extension or unresolved runtime helpers. The variant-60 driver and
standalone client build offline. These checks do not establish hardware
correctness or throughput.

## Hardware preflight

A strict host-key-pinned, read-only SSH check confirmed Haiku hrev99999, 64
CPUs, 137,034,936,320 bytes of RAM, and stable driver55 tag
`2026102100010012`, generation2, `quiesced0 cold0 active_owner0 open_count1
guard1`. The process list contains no 27B diagnostic or inference process.
Both model files were verified in the previous recovery checkpoint. Tag0106
has not been installed; no model load, prompt, generation, or decode rate has
been measured.

The guarded trial wrapper is a one-shot operation. It rechecks the prior
27B-process and stable-driver preflight, exact model sizes, candidate hashes,
CPU/RAM, then swaps driver60 and runs two warm generations. It accepts output
only with the expected token prefix and unload confirmation, and restores the
stable driver after the client closes cleanly. Any failure retains the recovery
lock and exact stable backup for evidence-led recovery.

