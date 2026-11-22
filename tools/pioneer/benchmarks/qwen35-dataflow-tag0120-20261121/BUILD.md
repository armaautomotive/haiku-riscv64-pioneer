# Qwen3.5 dataflow rewrite tag0120

Tag0120 corrects the interpretation of the tag0119 NoC snapshot and preserves
the failed read request identity for diagnostics. The worker mailbox remains
64 bytes; the control ioctl remains 36 bytes; dataflow ABI is 5. A new
read-only `QUERY_READ_REQUEST` action returns the remote address, target route,
and transfer size without issuing another NoC request.

On a timed-out embedding read, the six existing reserved mailbox words hold:

| Word | Contents |
| --- | --- |
| `reserved[0]` | Remote address |
| `reserved[1]` | NoC stage/index in bits 0..15; route in bits 20..31 |
| `reserved[2]` | Observed completion-counter value |
| `reserved[3]` | Expected completion-counter value |
| `reserved[4]` | Counter-match and initiator masks in bits 0..11; byte count in bits 12..31 |
| `reserved[5]` | 32 outstanding transaction-ID slots |

The 32 slots are the 16 response-marked request-ID counters followed by the 16
write-outgoing-ID counters at NoC offsets `0x240..0x2bc`; they are not command
queues. See the official [Blackhole NIU counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md).

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0120-build8
```

The driver embeds runtime tag `2026112100010120`. The earlier build4 image
was compiled with tag0119 and is superseded; do not stage that driver.

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010120`, dataflow ABI 5 | `153748972239101302603e71f4ba3a3cd665fb6e63d51a7cb4094df91c538d7c` |
| Haiku dataflow client | `73cd7e9b47711866d67bd50eb8cb0ad358138760d2bb3d5cda54cfdf8802a60b` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `2025572629309596d727bbc3aec55b458f643930fbe0f1766633e168c20c4dcd` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, service image, and client build pass.
- Conservative kernel stack use is `15712/18432` bytes.
- The target worker compile passes for RV32IMF with no A-extension atomics or unresolved runtime helpers.
- Embedding-worker and service-owner ASan/UBSan fixtures pass, including request address/route/size propagation and fixed-mailbox packing.
- The runtime-recovery fixture passes; it leaves an outstanding NoC request untouched rather than clearing or retrying it.
- The tag0120 trial and post-boot stable-driver recovery scripts pass Bash syntax validation. The trial script pins the driver and client hashes and accepts only a one-run diagnostic.

## Hardware outcome

Tag0120 was installed and run after the tag0119 recovery helper restored stable
driver55 at generation2 and idle was verified. The exact one-run timings and
failure are in [`RUN.md`](RUN.md). It failed on the first core0 embedding
read, produced no output, and left tag0120 generation3 cold-latched with an
active owner. The candidate, recovery lock, and stable55 backup remain
retained.

The existing recovery script is for use after a fresh Haiku boot; it does not
provide in-place ASIC recovery. The verified host handoff is Fedora Linux to
Haiku through kexec from the existing NVMe. No SD-card workflow is used.

The next rewrite checkpoint needs a guarded Blackhole ASIC reset, post-reset
firmware readiness and hardware reinitialization, then proof that all 120
cores and NoC transactions are idle before it clears the cold latch. This
recovery path is not implemented or hardware-validated yet. No valid output,
decode rate, two-warm run, or clean unload has been established.
