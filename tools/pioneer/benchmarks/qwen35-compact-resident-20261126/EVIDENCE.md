# Compact resident simulator image, 2026-11-26

The fused hybrid resident image previously used a third code region at
`0xd0000`, overlapping the old ROOT scratch. This simulator-only candidate
compiles the same selected sources with GCC `-Oz` and places all worker code
in the existing primary and dispatch regions. The primary text is 56,152
bytes at `0x72000` (limit `0x80000`); dispatch is 37,608 bytes at `0xa0400`
(limit `0xaa000`). The latter uses more than the older linker's `0x7000`
limit, but remains within the service owner's declared `0x9c00` maximum.
The existing PCI snapshot view describes `0x6e000..0xb0000`; those bounds
alone do not prove physical ownership, write exclusion or recovery.

The repeatable builder is `build_qwen35_compact_resident_ttsim.py`; it emits
explicit simulator-only markers before compiling and no driver header. Both
driver-header packagers reject the compact marker independently of the other
simulator-only markers. The simulator requires an explicit matching compact
flag, the fused hybrid markers and absence of an extra code image. Negative
tests reject marker/flag disagreement and an extra code image before loading
the simulator. A preexisting output directory is refused.

The build had no unresolved imports or forbidden RV32 atomics/divides, and
all 244 code symbols lay within the boot, text or dispatch ranges. The
existing conservative stack checks passed: the largest graph envelope was
7,648/8,192 bytes, including a 1,024-byte allowance. This is not a
whole-program stack proof. The rebuilt boot, text and dispatch are byte-for-
byte identical to the independently linked feasibility candidate. Build log:
`/private/tmp/q35-fused-compact-builder-final-20261126.log`.

The actual service owner was also exercised with a 37,608-byte synthetic
dispatch image in simulated IO. Both ordinary and shared-role configurations
staged and read back every word on 120 simulated workers, closed cleanly, and
retained the owner when a high-offset write or readback was faulty. Separate
ASan/UBSan tests of the actual PCI-memory adapter wrote both the first word
beyond the old `0x7000` limit and the last word of the declared dispatch
range, rejected writes past the limit and while running, and verified the
simulated snapshot restoration. These checks establish software bounds and
failure handling in fake memory only; no physical writer exclusion or device
restoration is implied.

The builder also offers `--mixed-optimization`, keeping the stream and graph
dispatch objects at their earlier `-O2` setting while the rest use `-Oz`.
That image linked within the same two declared ranges: 56,152-byte text and
38,988-byte dispatch. Its conservative graph stack envelope is
7,920/8,192 bytes including a 1,024-byte allowance, not a whole-program
proof. The service-owner fake-IO staging, readback, high-offset failure and
close checks also passed at this larger dispatch size in ordinary and
shared-role configurations. The mixed builder log is
`/private/tmp/q35-fused-compact-mixed-build-20261126.log`.

An authenticated four-layer simulation of the mixed image is active in
session `99286`, log
`/private/tmp/q35-fused-compact-mixed-four-layer-sim-20261126.log`. It has no
result yet. Retaining an earlier compiler setting is not evidence of faster
execution; both compact images remain non-packageable and hardware-unqualified.

An authenticated four-layer simulation using the byte-identical candidate is
active in session `28725`; its log is
`/private/tmp/q35-fused-compact-four-layer-sim-20261126.log`. No numerical
result is available yet. Other full-model simulations continue separately.
The compact image has not run on a P100A, may have different performance
from the prior build, and cannot be installed until model correctness,
physical role ownership and clean no-reboot recovery are established.

Image SHA-256: boot
`e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b`,
text `0cae7786d840092fecf0fa5a128b0a4a558da23d1d3ebea97caf52b68dc81d97`,
dispatch `eb7436a3b151dab2ef46fc2027d26e6cb195765701829d4fe15f631d4bca4f0f`.
