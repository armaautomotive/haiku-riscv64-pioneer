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

No tag0108 hardware run has started. Tag0107 failed before inference at the
raw worker-0 route `0x312` and then failed the fleet resume step; its candidate
currently reports `cold1 active_owner1`. Recover tag0107 to stable driver55
before staging or running this checkpoint. No valid output, warm-run timing,
or decode throughput exists yet.
