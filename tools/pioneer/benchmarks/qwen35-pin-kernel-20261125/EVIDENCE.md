# Offline Pioneer pin-service kernel candidate

## Spin-idle profile correction (2026-11-25)

The earlier `0aca1f85...` candidate below uses the ordinary kernel and is not
the recorded 64-CPU spin-idle boot profile. Do not use it for that profile.
The saved NVMe handoff records a spin-idle RAM bundle; the running kernel's
4,840 exported symbol tuples all match the saved spin-idle kernel, compared
with 577 for the ordinary kernel. This strongly identifies the layout but is
not a hash of the running image; see `kernel-symbol-match.json`.

The revised builder reproduces both ordinary and spin-idle control kernels
byte for byte, checks the spin-idle code, and builds the pin-service kernel
from the spin-idle profile. Independent builds produced SHA-256
`6b43356be4b4f5974c555b06599b89d8008825fcbc26b40e8eb57b5edfde1372`;
see `spin-manifest.json`. A private local RAM filesystem was built from the
preserved base package and the original root-opt-in ECAM driver; its package
path list, replaced-file hashes, filesystem check, and package roundtrip
passed. Its image SHA-256 is
`9e569f8c23ae3726b0be8747f93b7d16c511f4a02f91df23e00bd3703c037b96`.
This only prepares a candidate: it has not been installed or booted, and it
does not establish physical recovery or a working inference path. Repeat the
27B process and idle-driver checks before any card access.

Two independent runs of `build_qwen35_pin_kernel.py` reproduced the saved
control kernel byte for byte and produced the same candidate kernel hash. The
manifest records source, pin-header, control, candidate and Jam-action hashes.
The candidate has a relocation from the built-in module table to the pin
service. It has not been installed, booted, or used on a card.

The saved kernel and its link objects are from the local
`/Volumes/HaikuBuildLocal/generated.riscv64` build. Jam came from Haiku
buildtools revision `33d46bf5c300bd34cd07c596431e4545c754ac03`. The build
compares the two changed kernel source files against the saved source mirror,
relinks the control using the original saved objects, and refuses to continue
unless the control matches the pinned SHA-256. The candidate recompiles only
those two translation units and retains the experimental pin flag. The
linker reported its existing orphan-section warnings; both links succeeded.

On the Pioneer, read-only inspection found `/boot/system/kernel_riscv64` hash
`38d4a4245db6a8ec4e2baa44c7705cdcdd5d4e66a8d43da97f3157699449a65b`.
That file has an ELF writable-segment offset of `0x194000`. `listimage 1`
reported the running kernel text at `0xffffffc000080000` and data at
`0xffffffc000215000`, a difference of `0x195000`, which matches the saved
control kernel's writable-segment offset. The matching offset is not proof
of the running kernel's hash. The actual boot artifact and rollback path
must be identified before a candidate boot. No reset, driver swap or model
inference was attempted; the idle/27B process check must be repeated
immediately before any card operation.

## Card-independent service probe and package-content audit

The actual probe driver was cross-built and its success and failure paths were
tested with simulated modules under ASan/UBSan. On the running baseline Haiku
kernel, the separate trial observed `B_ENTRY_NOT_FOUND` for the experimental
service, as expected, with zero callbacks. It removed the temporary driver,
device registration and staged files and confirmed the image was no longer
listed. The candidate has not been booted, so its service is not qualified.
The probe does not enumerate PCI or access the card.

Full local extraction of the base and private candidate packages found 5,857
entries in each. File contents, symlink targets and POSIX modes differed only
for the intentionally replaced kernel and PCI ECAM file. This comparison does
not verify Haiku-specific package attributes or prove equivalence with the
recorded Linux-side private package.

## Separate candidate boot helper

A separate helper can be generated only from the exact recorded Linux recovery
helper; it pins the private candidate BFS image and adds an explicit 64-CPU,
128-GiB no-SD profile while preserving the original control profile. Offline
argument and profile tests passed, as did the original helper regression. The
generated helper and bundle have not been uploaded, kexec-loaded or booted.
The recorded Linux-side private package and rollback route still require a
current comparison and verification before a candidate boot.

## Recorded private-package candidate

The authorized reboot reached Linux and verified the preserved control BFS
and pinned recovery helper hashes. The preserved private HPKG was retrieved
read-only and matched its recorded SHA-256 `7a830442362ba511bdfc08f6d99e26df5813475861d3583a90cb62cbef1f3cea`.
The original candidate extraction differed from this package only in the
kernel, across 5,857 entries compared by content, symlink target and POSIX
mode. This does not compare Haiku package attributes. The builder was changed
to start from that exact private package and replace only the kernel. The
new BFS SHA-256 is `3bc902182084edd40b8b98032a57e2f86f44b362be144117e08d0373763d30bf`;
the generated helper SHA-256 is `d39f631eba34876f0ba1b064ebec215147962203f376c206783eea08dbf5d8fd`.
These supersede the earlier candidate BFS and helper. The new candidate has
not been uploaded, kexec-loaded or booted.
