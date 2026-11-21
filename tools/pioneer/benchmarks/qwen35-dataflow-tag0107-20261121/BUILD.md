# Qwen3.5 dataflow rewrite tag0107

Tag0107 continues the Tenstorrent-style resident per-core rewrite. It waits
for startup results from all 120 workers before a failed installation enters
the stop audit. It captures the raw local route when the worker rejects phase
2 and prints the stop-audit stage and core if clean recovery fails. The
phase-2 recovery proof requires the fatal mailbox and queues to be stable, the
exact service image and core selection to match, the raw route to remain
unchanged and unmatched, the existing kernel NoC-source proof to hold, and all
NoC initiators and queues to be idle. Other failure states remain fail-closed.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0107-rebuild
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010107`, dataflow ABI 2 | `176aacf58aeb972aabbe05d5dca6f89f5af1a1f3648ba96d7c932abea085668` |
| Haiku dataflow client | `232a98fd41c77f3e559e5de983a156febb65ff483c16da5181dcc28f268d72cd` |
| Resident service text | `11a07f757d185198b462720a5761b6b4f6ac90d690e23cc2a1eff7801b0721c6` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Guarded two-warm trial wrapper | `ea3bfe5f38c17df00952fe641456794d286c28f02126f89c8f730e87c6c584c4` |
| Post-cycle recovery helper | `792abe6d73163fb3f0711c50b5cd453bddabe37cc06f619b2a471f3eff0e555c` |

The owner and PCI-memory tests pass with AddressSanitizer and
UndefinedBehaviorSanitizer. The RV32IMF service compile passes without A
atomics or runtime helpers. The offline package build passes. No hardware
trial has run with tag0107; this record does not establish model output,
throughput, or silicon recovery.
