# Qwen3.5-4B Work tag53: capture the faulting NoC transaction

Tag `2026111800010053` adds a read-only terminal-fault snapshot after the
existing ROOT plus all-119-peer safe-idle proof. The poll response now carries
the ROOT's ten command words, eight counters and deltas from Work bootstrap,
plus worker119's stable status and two PC samples. This adds no NoC packet,
retry, reset, ownership change, or inference fallback. Work protocol version
is 3 so the client and driver agree on the extended poll structure.

Offline warnings-as-errors cross-build:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-253-final2
```

The standalone WorkOwner regression passed, including checks for the new
snapshot fields. Tag53 has not been installed on hardware. It should be used
for the next guarded worker119 fault trial, then the read-only receipt can
show whether worker119 observed the doorbell and how the ROOT NoC counters
advanced.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `fcecfd1e4089dae437fb406d023af105a2f3162a8cb950e2ed16f909d767c06a` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4f104ead3ca0e16159937a95313767c27ac9e2628e2c2cdda6fad0102d8d7c57` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Work client | `d71a5ed9bc550522e9987fd1d005b0685890a71c5e85014dbf1f4dc03c289d5d` |
