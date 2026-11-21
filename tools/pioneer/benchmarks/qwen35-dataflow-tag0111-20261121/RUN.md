# Qwen3.5 dataflow tag0111 hardware run

## Outcome

The guarded tag0111 trial panicked Haiku while the client was making the
generation ioctl. No valid model output was produced, no decode rate was
measured, and clean unload did not complete. I rebooted from KDL, returned to
Haiku through the pinned SD-free Linux-to-Haiku handoff, and ran the guarded
recovery helper. Independent postflight confirmed stable driver55 generation2
idle, no recovery lock, and both model files intact.

This is a failed hardware validation. The client has no CPU inference or Work
fallback; its 120-core resident Tensix service was the selected path.

## Preflight and progress

Before candidate install, the prior 27B diagnostic had exited. Stable driver55
tag `2026102100010012`, generation2, reported `quiesced0 cold0 active_owner0`;
the exclusive lock was absent. Haiku had 64 CPUs and the full
137034936320-byte RAM profile. The Qwen3.5-4B file measured 2740937888 bytes,
and the preserved Qwen3.8-27B file measured 18973870528 bytes.

Authentication took `99.830468` s. Prompt tokenization took `13.752066` s and
produced token IDs `760,6511,314,9338,369`. During model transfer the live
progress reached 3328 spans and 2657021952 of 2740498432 payload bytes. The
post-panic client files were damaged, so there is no authoritative completed
model-load time.

## Panic evidence and offline diagnosis

The candidate driver was tag `2026112100010111`, variant60. KDL recorded a
store page fault on thread `pioneer-q35-dataflow`. The saved user stack points
through the Qwen35 generation ioctl into the decoder sequence. The fault-time
stack pointer was `0xffffffc0445bf090`, outside the 20 KiB kernel-stack range
`0xffffffc0446b7000`–`0xffffffc0446bc000`.

The candidate's compiler `-fstack-usage` report identifies the overflow:

| Function | Stack frame |
| --- | ---: |
| `DataflowFfnSequence::Run` | 194800 bytes |
| `DataflowProgramBuilder::Build` | 75808 bytes |
| `DataflowRecurrentLayerSequence::Run` | 67584 bytes |
| `DataflowDenseLayerSequence::Run` | 66656 bytes |
| `DataflowDecoderSequence::Run` | 65424 bytes |
| `Qwen35DataflowProject` | 64560 bytes |

Several frames individually exceeded the entire kernel stack. This explains
the panic. The fault happened before any valid generated token or throughput
measurement; it is not evidence of slow Tensix execution.

Tag0112 moves the reusable projection descriptor and projection schedule to
heap-owned session/authority storage, removes the builder's large stack
candidate, and adds a build-time stack budget check. Tag0112 remains an offline
checkpoint until a new guarded hardware trial succeeds.

## Recovery and final card state

KDL offered no in-place driver recovery command, so I issued one KDL `reboot`
after saving the serial evidence. The board returned to its default NVMe Linux
boot. The pinned helper then used the `memory128-spin-idle7-top-page-nosd-control`
profile to kexec Haiku without writing the SD card or changing hardware
connections.

After reboot, the tag0111 recovery helper verified the client had exited,
quiesced fresh candidate generation1, restored the saved stable driver55,
cleared the exclusive lock, and reported generation2. Independent postflight
confirmed `quiesced0 cold0 active_owner0`, 64 CPUs, full RAM, no inference or
diagnostic process, and unchanged 4B/27B file sizes. The stable driver SHA-256
was `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.

## Evidence

| File | SHA-256 |
| --- | --- |
| `evidence/kernel-panic-serial.txt` | `e50655ff45312875acfbe30130147c76037e46c33b27dd5762724ee91071bcdf` |
| `evidence/kernel-debug-help.txt` | `eca8de47f75c203d14e0a5248eaf87d7e8b5af9cd6a5e829bebdfb38b2ccce8c` |
| `evidence/pre-recovery.txt` | `309261a0d8720acf26cbe8f342972273a4f7d2f1d1e73d4dc836a3f5614654c5` |
| `evidence/post-recovery.txt` | `b17a830864d4ae34065739f27ac8d6a07687bbfabc9514cb6e1d264584e7eb3c` |
| `evidence/work.stdout` (damaged) | `c45d49d24737fde25b8d65c5cc5cdb947525e8c3f40ccf8502e68c3fe5699927` |
| `evidence/work.stderr` (damaged) | `f35f66060c40a64e9a57d577d77b174723316258cb80e6fddd83ed18204eefd4` |

No completed load timing, prompt-processing timing, decode timing, generated
output, warm run, or tokens/sec result exists for tag0111.
