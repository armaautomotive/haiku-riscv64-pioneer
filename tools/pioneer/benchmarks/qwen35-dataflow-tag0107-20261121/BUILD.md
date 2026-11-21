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
| Variant-60 driver, tag `2026112100010107`, dataflow ABI 2 | `176aacf58aeb972aabbe05d5c3ca6f89f5af1a1f3648ba96d7c932abea085668` |
| Haiku dataflow client | `232a98fd41c77f3e559e5de983a156febb65ff483c16da5181dcc28f268d72cd` |
| Resident service text | `11a07f757d185198b462720a5761b6b4f6ac90d690e23cc2a1eff7801b0721c6` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Guarded two-warm trial wrapper | `ea3bfe5f38c17df00952fe641456794d286c28f02126f89c8f730e87c6c584c4` |
| Post-cycle recovery helper | `48912cd8f9a1a6fdad9917b3a3571296e69e91541a7f83ff4be72059c3e70bb8` |

The owner and PCI-memory tests pass with AddressSanitizer and
UndefinedBehaviorSanitizer. The RV32IMF service compile passes without A
atomics or runtime helpers. The offline package build passes.

## Hardware trial

On 2026-11-21, the guarded trial completed authentication, tokenization,
upload of the 2,740,498,432-byte model, and manifest binding. Installation
failed at worker 0 with `result=11`, startup diagnostic stage 2, phase 2,
raw local route `0x312`. The client then reported close result 18, which is
the service owner's `ResumeRestoredFleet` failure. No inference output or
timing was produced.

The wrapper refused rollback because the candidate status was
`driver60 tag2026112100010107 generation3 quiesced0 cold1 active_owner1
open_count1 guard1`. It retained the recovery lock and stable-driver backup.
The 27B file had passed the exact preflight check before the run; no command
loaded or modified it. This run does not establish clean no-reboot recovery,
valid output, or throughput. The board must be returned to a verified idle
stable-driver state before a later trial.
