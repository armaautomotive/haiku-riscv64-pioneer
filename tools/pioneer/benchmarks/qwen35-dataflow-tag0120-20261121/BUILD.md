# Qwen3.5 dataflow rewrite tag0120

Tag0120 corrects the interpretation of the tag0119 NoC snapshot and preserves
the failed read request's identity for the next hardware diagnostic. The
worker mailbox remains exactly 64 bytes; the control ioctl remains 36 bytes
and dataflow ABI 5. A new read-only `QUERY_READ_REQUEST` action returns the
remote address, target route, and transfer size without issuing another NoC
request.

On a timed-out embedding read, the existing six reserved mailbox words now
hold:

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
queues. This follows the official
[Blackhole NIU counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md).

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0120-build4
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver | `754c3aec27cbfc74473c585e7b64d4441aced9f73eb7ec1eb9d955e31a047377` |
| Haiku dataflow client | `73cd7e9b47711866d67bd50eb8cb0ad358138760d2bb3d5cda54cfdf8802a60b` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `2025572629309596d727bbc3aec55b458f643930fbe0f1766633e168c20c4dcd` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, service image, and client build pass.
- Conservative kernel stack use is `15712/18432` bytes.
- The target worker compile passes for RV32IMF with no A-extension atomics or
  unresolved runtime helpers.
- Embedding-worker and service-owner ASan/UBSan fixtures pass, including
  request address/route/size propagation and fixed-mailbox packing.
- The runtime-recovery fixture passes; it leaves an outstanding NoC request
  untouched rather than clearing or retrying it.

The failing tag0119 run and unresolved hardware recovery are in
[`../qwen35-dataflow-tag0119-20261121/RUN.md`](../qwen35-dataflow-tag0119-20261121/RUN.md).
Tag0120 has not been staged on or run on the Pioneer.
