# Qwen3.5 dataflow tag0110 hardware run

## Outcome

The guarded run installed tag0110 and uploaded the full 4B model, then failed
on the first prompt token in the embedding operation. It produced no valid
model output and no decode-rate measurement. The attempted close could not
audit the resident service, so the guard retained the candidate and stable
backup. Recovery required a board power cycle followed by the pinned,
SD-free Linux-to-Haiku kexec handoff. The recovery helper restored stable
driver55 at generation2, cleared the trial lock, and confirmed that both
model files remained present at their recorded sizes.

The 120-core Tensix service was selected; the client has no CPU inference or
Work fallback. This is a failed hardware validation, not a throughput result.

## Preflight and model transfer

The guarded preflight observed stable driver55, tag `2026102100010012`,
generation2, `quiesced0 cold0 active_owner0`; the earlier 27B diagnostic and
inference client had exited. The full 64-CPU, 137034936320-byte RAM Haiku
profile was active. The guard verified the Qwen3.5 GGUF as 2740937888 bytes
and the preserved Qwen3.8-27B GGUF as 18973870528 bytes before installing the
candidate.

Authentication took `99.320494` s. Vocabulary and prompt tokenization took
`12.779983` s and produced prompt IDs `760,6511,314,9338,369`. Resident model
load/upload took `326.685735` s for `2740498432` bytes.

## Generation and close

The first generation request reached the decoder. It returned
`result=31` (`DataflowServiceGenerationFailed`), `stage=4`,
`state=2 recovery=1`, `output=0`, `prompt_us=609649`, and `decode_us=0`.
The request-stage encoding identifies position0 and
`DataflowDecoderEmbeddingRunFailed`; it does not identify the lower-level
worker failure yet. The reported 0.609649 s is partial failed prompt time,
not a completed prompt-processing measurement.

No output tokens were produced, so the expected `11751,13` prefix could not
be checked. There were no valid warm runs, decode time, or tokens/s. The
wrapper correctly rejected unload as clean: close returned
`DataflowServiceStopAuditFailed` (`result=15`), diagnostic stage7/core0 with
state1, and the candidate remained tag0110 generation3
`quiesced0 cold1 active_owner1 open_count1`. The client and prior 27B
diagnostic had exited before recovery. The post-run process list showed only
system/SSH processes.

## Recovery and current card state

Following the saved board policy, one OFF hold was followed by a three-minute
shutdown wait and one ON pulse. Linux returned with the pinned SSH identity;
the validated hash-pinned `pioneer_linux_boot_haiku_smp-20261115-codex.sh`
helper used the `memory128-spin-idle7-top-page-nosd-control` profile to
return to Haiku without writing the SD card or changing default boot storage.
The tag0110 recovery helper verified the model files, quiesced candidate
generation1, restored stable driver55 tag `2026102100010012` generation2,
and removed the recovery lock. Independent postflight confirmed stable55
`quiesced0 cold0 active_owner0`, all 64 CPUs, 137034936320-byte RAM, no
inference/diagnostic process, and the unchanged 4B/27B file sizes. Stable
driver SHA-256 is
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.

The card is idle on stable driver55. The 27B setup is preserved. No further
benchmark run is scheduled until the offline rewrite adds lower-level
embedding failure diagnostics and a safe same-owner recovery/close path.

## Evidence

Raw client output, stderr, exit status, pre-recovery state, and independent
post-recovery snapshot are in `evidence/`.

| File | SHA-256 |
| --- | --- |
| `work.stdout` | `3f0887079ea7be33206d9c7a78e0e876a05b1e9b8a7533221d16895e25d790e4` |
| `work.stderr` | `9671392f26c01cdca017564636da92d69e792acb94d01b960478738fd05a42f7` |
| `work.exit` | `4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865` |
| `state` | `fbd1473a9abb5073bbd33c4b496a571ea742253449bef0484ab678f9efd1c9c4` |
| `status.before-recovery.txt` | `67e5a980e0b0e3b8e343ef3583db84757ee1907ab39ccc12cb132df7596af2f2` |
| `post-recovery.txt` | `5f57209ee021d856fafcc1778c3f024682d575eac4481868dec1298f95a00244` |

The offline build and artifact hashes are recorded in
[`BUILD.md`](BUILD.md). The target remains two valid warm outputs at or above
100 decode tokens/s, with model load, prompt, and decode times reported
separately and a clean unload.
