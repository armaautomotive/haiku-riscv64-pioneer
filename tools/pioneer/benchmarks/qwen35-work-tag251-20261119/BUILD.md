# Qwen3.5-4B Work tag251: close-time recovery after a terminal fault

Historical failed experiment. Do not rerun the close-on-fault wrapper: on
hardware, `close()` did not return and Haiku became unreachable. See [`RUN.md`](RUN.md).

Tag `2026111800010051` extends the tag250 transfer diagnostic with a driver
close path for an explicit terminal ROOT fault. If the Work client closes its
device handle before sending STOP and BANK END, the driver now repeats the
full ROOT and 119-peer safe-idle proof and runs the existing bounded recovery
and teardown. An active run, a latched cold-recovery state, or any failed
proof still retains the owner without reset or cleanup.

The regression client has an opt-in `--close-on-fault` mode. It exits after
the terminal fault by closing the device handle, deliberately skipping Work
STOP and BANK END. The trial then requires the candidate driver to report
`cold0 active_owner0`, hot-reloads stable041, and verifies both Qwen model
files remain intact. This instrumented candidate is for recovery diagnosis,
not performance measurement.

Offline cross-build passed with warnings as errors:

```sh
python3 -B tools/pioneer/build_qwen35_work.py --transfer-diagnostic \
  /Volumes/HaikuBuildLocal/generated.riscv64 \
  /private/tmp/pioneer-q35-251-close-recovery-v2
```

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `20c866a2f51939fed831315bbd535d60394706fcafc1dafe0e3a08ccb8cc5100` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client with close-on-fault mode | `1821fffa15de65b0a36da064ff6decb99f461a9ded8908fa47cbbc56dbb8c6d7` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `4f104ead3ca0e16159937a95313767c27ac9e2628e2c2cdda6fad0102d8d7c57` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `25ee4dafbc7979e581951adb7e457aa4bf1a4130d1d850f238534a6bc32b3864` |
| Guarded trial wrapper | `ea5331927224484c5377f26bd5b31f3adce699b39c1c68ed7f6786d31c526eba` |
| Post-power-cycle stable041 recovery helper | `99d0e1f01be63edcb4cd80b83a2b8e77d49425edc1d3042e78f0f8be15c39e0d` |

The standalone `PioneerQwen35WorkOwner` regression test and the warnings-as-
errors cross-build passed. The guarded hardware trial failed: after the
terminal ROOT fault, synchronous device close did not return and Haiku became
unreachable. Recovery required one authorized power cycle and the pinned
SD-free Linux-to-Haiku kexec path. Stable041 was restored at generation2.
Details and raw post-cycle evidence are in [`RUN.md`](RUN.md).

This showed that fleet recovery must stay in explicit Work STOP and BANK END,
where the client can receive the proof result. Tag52 reverts the synchronous
close-time card I/O; an unknown client exit still retains the owner safely.
