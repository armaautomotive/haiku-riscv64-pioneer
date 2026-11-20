# Qwen3.5-4B Work tag64: recover normal terminal completion

Tag `2026111800010064` adds a separate recovery proof for a ROOT that has
reached its normal completion park but whose completion result or fleet-idle
validation did not pass. The driver checks the installed ROOT image and then
repeats the full ROOT/peer sequence, source, PC, and NoC-idle proof before it
restarts any core. A missing completion marker is recoverable for cleanup only;
it does not qualify as valid model output. STOP repeats the proof while the
card session remains open.

The tag263 diagnostic run exposed a validation bug: matrix diagnostic
checkpoints used progress words reserved for completion fields and full-core
and projection wave counts, so a successful projection produced invalid
completion telemetry. The checkpoint writer now uses its dedicated ROOT
checkpoint area. Tag64 is built without optional diagnostic writes; the
per-run wave counters remain available for result validation and measurement.

The host owner regression passes, including a missing-marker terminal park
whose first proof is rejected and whose fresh STOP proof succeeds. The
warnings-as-errors cross-build also passes.

Build command:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-264c
```

| Artifact | SHA-256 |
| --- | --- |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `6437ec4681562b8a5e09618a92e3ca7957faa348b327d6ee708efa81f781e5c8` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `ee3b913d14551b642d64db709247c6f33b4b71dc35b557be8ef15e413b889a03` |
| Candidate driver64 | `60d5e0b17d3aacf1970f015633e2fcbae612c156872b87b74b2a679456c31c2f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default43 control | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Work client | `6184f2aa1ff825172efd6ef89c1751ad07167d71a6fc0d66a990bfab16fac654` |

## Haiku trials

### Initial plain-text prompt

On November 19, tag64 passed preflight from stable041 generation 2 with the
prior diagnostic exited, no reload lock, 64 Haiku CPUs, full expected RAM, and
both model files present. The verified 4B model SHA-256 was rechecked by the
client. Tag64 was hot-loaded at generation 3 without reboot; Work support
reported no host inference. Authentication took 98.898154 s, vocabulary and
tokenization 13.264045 s, and model upload/load 224.602204 s for 3,450 spans
(2,740,498,432 bytes). Both runs decoded 32 tokens in 358.35 s (0.0893
tokens/s) after 189.63 s prompt processing. The output started a `<think>`
reasoning block and was truncated before the requested answer. It did not
qualify as valid output. Evidence is in `evidence/plain-prompt-failed/`.

The client unloaded cleanly (`state=2 active=0 cold=0`) and restored stable041
at generation 4 without reboot. That attempt exposed Haiku `awk` line
continuation incompatibilities in the trial wrapper; the corrected validator
was then exercised against the captured records.

### ChatML-corrected two-run comparison

The client now parses Qwen ChatML control tokens and receives an explicit
empty `<think>` block. The vocabulary-only check mapped them to the model's
special IDs. With the same tag64 driver, preflight passed from stable041
generation 4, the candidate loaded at generation 5, and Work reported host
inference disabled. Authentication took 67.558536 s, vocabulary/tokenization
13.015887 s, and model load 222.119649 s.

Both warm runs returned exactly `A B C D E F G H I J K L M N O P`. Each
processed 23 prompt tokens in 243.113 s and decoded 16 tokens in 173.317 s,
or **0.092316 decode tokens/s**. Each recorded 7,136 full-core waves and
14,128 projection waves, covering the ROOT plus all 119 Tensix peers. This
validates output and all-core execution, but the 100 tokens/s target remains
unmet by more than three orders of magnitude.

The client unloaded cleanly (`unload_clean=1 state=2 active=0 cold=0`). The
wrapper restored stable041 at generation 6, verified no active owner, and
removed the lock without reboot. Read-only postflight reported
`TWO_WARM_OUTPUTS_VALID_RATE_BELOW_TARGET`. Exact records and independent
hashes are in `evidence/chatml-16tokens/`. Driver SHA-256 is unchanged at
`60d5e0b17d3aacf1970f015633e2fcbae612c156872b87b74b2a679456c31c2f`; the
ChatML client is `02fd8274f05dfca37ca0a79b6952f6d0dbb6596320c9925ea32f06fb682a8a6d`.

The normal-completion cleanup and hot restore were exercised; the separate
missing-marker completion recovery branch was not forced. The next work is
profiling and reducing per-token projection/control overhead before another
speed claim.
