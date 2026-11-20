# Tag97 P100A hardware result

Tag97 passed the Tag96 sentinel timeout and completed all peer work for the
23-token prompt. It then faulted during the ROOT's final read-accounting proof
before publishing a prompt result. The run did not qualify; no valid output,
prompt-processing duration, or decode rate was produced.

## Preflight and recovery

The guarded preflight passed with the previous diagnostic and inference
processes absent, stable driver55 idle at generation10, and the pinned 4B and
preserved 27B model identities intact. The candidate installed as driver64
generation11 without reboot. After the fault, Work STOP and client unload
completed cleanly (`state=2 active=0 cold=0`), and the runner restored stable
driver55 idle at generation12. Postflight reported
`SAFE_FAULT_RECOVERY_VERIFIED`.

## Timings and failure

Authentication took `69.006291` s, vocabulary/tokenization took `15.215872` s
for 23 prompt tokens, and model loading took `226.172627` s for all 3,450 spans
(`2,740,498,432` bytes). Prompt work reached 22 committed steps but did not
complete the prompt.

The failure reported `dispatcher_stage=0x54000403` and
`dispatcher_detail=0x0001127d` (70,269 expected read requests). This checkpoint
comes after peer completion and output-proof gathering; the later ROOT counter
validation faulted before result publication. Worker119's snapshot had
matching completed sequence values (`0xda9`) at its wait PC. The Tag96
sentinel timeout is therefore past; the next blocker is exact ROOT read
accounting for the long status-poll interval. Do not weaken the proof or add a
constant offset without measuring the actual and expected counters.

## Captured evidence

Raw card files are in [`evidence/tag97-card`](evidence/tag97-card):

| File | SHA-256 |
| --- | --- |
| `work.stderr` | `c58b52ef24e420932658f096f16bb4a6c4efdb1de8f2cea07b64ea8d91ec677a` |
| `work.stdout` | `4f0597284320567acfe7a1732771b62158a4271df1379271052b1cfd07089bf0` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `507e1557e8544dfd3bc8ecba2bc12b14d22c681e8123d4faf7f6668b58573a8e` |

The guarded preflight and postflight outputs are saved beside the raw client
logs. The stable driver is currently generation12, idle, with no reboot.
