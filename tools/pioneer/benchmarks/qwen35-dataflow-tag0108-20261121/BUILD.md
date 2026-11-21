# Qwen3.5 dataflow rewrite tag0108

Tag0108 replaces firmware-side route decoding with a kernel-assigned logical
core identity in each mailbox. The host captures a stable, unique raw local
route for every selected core during preflight and checks that same route
before commands. This uses the existing audited 120-core mapping without
assuming that the BRISC local NoC register equals the host's DRAM route
encoding. The worker still rejects an invalid raw route and has no CPU
projection fallback.

Close diagnostics now identify the failing core and substage when restoring
or resuming the captured fleet. The mailbox protocol and client ABI are both
version 3.

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0108-rebuild2
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010108`, dataflow ABI 3 | `62d64043cf5f3e2110b96446839efdfe2c61a945c8d87cd21d9d9a52072de1dc` |
| Haiku dataflow client | `cbee1483b73fa8e08754fbd04d1f78376e725ae7dc6e078ad271fdc2a58a256a` |
| Resident service text | `605cebc2507f86ff3feae51f30edb7f4fbdba12aa376f9bc037fc9faeadfd29f` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |
| Service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Guarded two-warm trial wrapper | `34eb777d50f55d1f9353e3fff1e8c460975257091457baf30b922552c8f64e28` |
| Post-cycle recovery helper | `3505c44f32a02febef52e61d0d6cb5108b1003a9fb2ed60e4fed258839ab6e4b` |

The five focused owner, PCI-memory, mailbox, and argmax tests pass with
AddressSanitizer and UndefinedBehaviorSanitizer. The RV32IMF worker compile
passes without A-extension atomics or runtime helpers. The offline driver,
client, service image, reload guard, and default-control package build passes.
Both guarded scripts pass `bash -n`.

## Tag0108 silicon run

The guarded tag0108 trial passed its preflight from stable driver55 generation2
idle. The 4B and 27B files had their pinned exact sizes, no inference or 27B
diagnostic process was running, and Haiku exposed all 64 CPUs and full RAM.
The candidate installed as generation3 and accepted the complete 4B model
upload.

Authentication took `100.027643` s, vocabulary/prompt tokenization took
`13.277220` s for prompt IDs `760,6511,314,9338,369`, and model load/upload
took `334.815789` s for `2,740,498,432` bytes. The first generation ioctl
(`0xAF56`) returned `EINVAL`. No generation completed, so prompt-processing
time, decode time, valid output, warm-run rate, and tokens/s are unavailable.
The client's prefix/repeatability failure messages follow the rejected ioctl
and do not represent generated output.

Close returned `result=18` (`DataflowServiceResumeFailed`), diagnostic stage9
(fleet resume), core0, substage3. The client reported
`unload_clean=0 state=0 active=0 cold=0 service_closed=0`; the wrapper refused
driver restoration while tag0108 was generation3 `cold1 active_owner1`, and
retained its lock and stable backup. Raw output and error logs are in
[`evidence`](evidence/).

After verifying the client had exited, one authorized OFF/ON recovery cycle
and the hash-pinned SD-free Linux-to-Haiku handoff were used. The guarded
post-cycle helper quiesced tag0108 generation1 and restored stable driver55 at
generation2. The lock is absent; the installed stable driver and saved stable
image both hash to `041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`.
The 4B and 27B files remain at `2,740,937,888` and `18,973,870,528` bytes.
See [`RUN.md`](RUN.md) and [`evidence/post-recovery.txt`](evidence/post-recovery.txt).

The next rewrite checkpoint must identify why the generation request is
rejected and why resume phase3 fails for core0. The 100 decode tokens/s goal is
unmet. No assistant integration or legacy Work-path changes are part of this
rewrite checkpoint.
