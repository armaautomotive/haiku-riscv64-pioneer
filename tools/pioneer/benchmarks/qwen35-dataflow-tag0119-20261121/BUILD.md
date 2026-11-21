# Qwen3.5 dataflow rewrite tag0119

Tag0119 carries the first prompt-read failure farther through the resident
BRISC workers. On a NoC counter timeout, the worker records an 8-slot
completion-counter match mask, four-initiator busy mask, and 32-queue busy mask
in the existing 64-byte mailbox. A new read-only `QUERY_NOC` control action
reports the snapshot while keeping the 36-byte control request unchanged.
Dataflow ABI is 5. No extra NoC command, counter clear, core reset, or retry is
added to the error path.

The worker receipt cleanup now clears only the six declared `reserved[]`
words; it no longer indexes a seventh word over `logicalCore`. A regression
asserts that clearing a receipt preserves the selected logical core.

## Build outputs

Built offline with:

```sh
python3 tools/pioneer/build_qwen35_dataflow.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-dataflow-20261121-tag0119-build5
```

| Artifact | SHA-256 |
| --- | --- |
| Variant-60 driver, tag `2026112100010119`, dataflow ABI 5 | `51ad25d7a24305eca9e507c239aa779465af3ac704e149ee5a6cb33b0b9e6eb4` |
| Haiku dataflow client | `6e58418cd2d7fde5341f2fee49c0272ff0dbed15e25ef02296fe826c8dbe5ee1` |
| Resident service boot word | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Resident service text | `96a3f1c4a9ccfa679375fe0611c4dfeb7b72c888acff5ef200d65cce4105aeed` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Default-control driver | `5740484449bc285500813b847b6170e2bb292a8fc4d30eccb834a304698f0c0d` |

## Offline validation

- Full variant-60 driver, resident service image, and Haiku client build pass.
- Conservative kernel stack use is `15712/18432` bytes.
- Service-owner, owner, and embedding-worker ASan/UBSan tests pass; the
  runtime-recovery test passes.
- The service-owner fixture verifies NoC snapshot propagation, and the worker
  fixture verifies counter/queue fields. Receipt clearing preserves
  `logicalCore`.
- Build and trial-wrapper shell syntax checks pass.

The NoC counter names and read completion events follow the official
[Blackhole NIU counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md).
Tag0119 is prepared for one guarded, two-token diagnostic. It has not been
installed on the card; the two-warm throughput benchmark remains gated on a
valid output and clean unload. See [`RUN.md`](RUN.md).
