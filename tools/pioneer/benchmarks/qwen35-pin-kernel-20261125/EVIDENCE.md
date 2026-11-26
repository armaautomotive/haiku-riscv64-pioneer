# Offline Pioneer pin-service kernel candidate

## Retained lease with link-reset witness (2026-11-26)

The compile-only link-reset witness now accepts exactly one recovery gateway,
including the retained gateway, while rejecting qualification and unsafe
configurations. The actual extracted recovery orchestration passed simulated
link-reset, timeout/retry and failure checks without repeating the pulse. A
separate test exercised the actual retained gateway and actual link-reset
algorithm with a stubbed recovery operation and simulated lease and pinned
nodes: the lease and pins remained held during link access and failure stayed
parked without replay. The sanitizer suites passed. A Haiku/RISC-V fused-driver
compile with both witnesses also passed and retained its no-I/O init guard;
log `/private/tmp/q35-retained-link-fused-compile-20261126j.log`. These are
software simulations and a compile-only build, not a physical recovery proof.
The physical retirement proof still denies; nothing was installed or run on
the card.

## Tree-lookup cleanup and final candidates (2026-11-26)

The public `find_child_node` wrapper also released its final iteration cursor
after the recursive lookup had dropped the device-manager lock. If that was
the last reference, node destruction could remove it from its parent's child
list without the lock, racing registration or traversal. The wrapper now
holds the existing recursive lock through traversal and cursor cleanup. The
actual extracted methods on simulated nodes reproduced the unlocked release
with the saved code and pass with the fix, including nested lock use. This is
software lifetime coverage, not a physical-removal guarantee.

With both tree fixes present, the saved ordinary and spin-idle controls again
reproduced byte-for-byte. Two independent retained-pin Haiku/RISC-V builds
produced SHA-256 `0a9937f75c74daa5b3a94dfa6f89d70ebd84aa2a08718a92a7f3b5e5a0080b22`;
the non-retained pin-service build is
`fa92437ef4ae431a959d23febe99e353f50178199f0a62d99b48eac6d6843707`.
The final manifests are `atomic-tree-retained-manifest-20261126.json` and
`atomic-tree-control-manifest-20261126.json`. The earlier registration-only
candidate hashes below remain historical. Neither final candidate was
installed or booted; no card was accessed.
The final offline recovery suite, including all three device-manager and PCI
tests, exited zero; log `/private/tmp/q35-atomic-tree-reload-20261126.log`.
The isolated source-check regression also rejected removal of either lock.

## Atomic device-manager reservation registration (2026-11-26)

`pci_reserve_device` creates a fixed-attribute child node to exclude another
legacy driver. In the saved device-manager source, `register_node` called
`FindChild` before acquiring the manager lock, then created the child under
the lock. Two callers could therefore both observe no reservation and both
register one. The root-node existence check had the same check-before-lock
pattern. The generic registration method now takes the existing recursive
lock before both checks and holds it through registration.

The test extracts the actual method and drives it on simulated nodes with a
controlled 16-thread schedule. The saved baseline accepted all 16 duplicate
registrations; the fixed source accepted one and rejected 15. The fixed test
also checks concurrent root registration and calling while the recursive
manager lock is already held. This reproduces the software race; it does not
show that a competing process actually raced the Pioneer.

A second test composes the actual `pci_reserve_device` function with the
actual device-manager registration method and simulated nodes. Under the same
controlled schedule, the saved baseline returned success to all 16 PCI
claimants; the fixed method returned success to exactly one. This covers the
two-stage legacy-reservation path, while still excluding physical hardware.

The saved ordinary and spin-idle control kernels reproduced byte-for-byte.
The offline Haiku/RISC-V retained-pin candidate with the registration fix is
SHA-256 `2b75f646d3120b355bdffa6d918f0f55426514e59e93a4f58581c6d61b225e61`;
the separate non-retained pin-service candidate is
`363656db9c5311ade5e1f1139d5cf93cb882f165659d16b546367bd9247a0951`.
Their manifests are `atomic-register-retained-manifest-20261126.json` and
`atomic-register-control-manifest-20261126.json`. A second retained build
produced the same kernel hash. The builder's saved-source check also rejected
altered lock placement, missing markers or module registration, modified pin
code and unrelated device-manager or CPU changes. Neither candidate was installed or
booted, and no card was accessed. This change makes competing software
registration atomic but does not itself exclude raw MMIO, firmware, DMA,
physical removal, or establish no-reboot recovery.
The source-check regression is `tests/pioneer_qwen35_pin_kernel_source_test.py`
with the saved source mirror supplied as its argument.
The broad offline recovery suite exited zero. It began before the additional
PCI-reservation integration case and final source-check regression were added;
both were run separately on the final files.

## Persistent guard and reservation ordering (2026-11-26)

The earlier non-loadable retained gateway cleared the shared guard and
unreserved PCI before waiting for physical retirement proof. That left a
window in which the persistent guard could report clean and a competing
driver could claim the card while the retained controller lease remained.
The compile-only recovery path now keeps the guard ticket cold and the PCI
reservation held through the wait. After independent retirement proof and
successful controller-lease and pin retirement, it validates the original
ticket, attempts PCI unreserve once, and only then completes the guard. A
failure retains quarantine and never automatically repeats unreserve. If
context destruction publishes a new fault after guard completion, the cookie
remains quarantined and no successful recovery is reported.

The retained path requires the rewrite's PCI-reservation ledger and checks it
before each retry, before accepting post-reset readiness and before final
unreserve. An injected lost reservation leaves the gateway faulted, its
controller lease parked and its pins retained; restoring the test ledger does
not make it retry. This is a software ownership check, not proof against
physical removal or other writers. Builds without reservation tracking or
the retained recovery prerequisites are rejected.

Tests exercised the actual recovery sequence with simulated resources and the
actual persistent guard implementation with simulated PCI, including guard
validation failure, unreserve failure, guard-completion failure, a fault
between validation and completion, attempted replay, success and a destructor
fault. Ordinary and live-takeover recovery
controls passed. The compact Haiku/RISC-V driver linked with the changes and
still denies initialization. These tests establish software ordering only;
the hardware-retirement proof still always refuses, and no card was accessed.

A separate integrated simulation now follows the actual retained gateway,
binding, pin service, controller lease and deferred finalizer together. Its
reset operation, guard, PCI device and final physical proof are simulated;
the test asserts the lease and pins are retired while the guard is still cold
before unreserve and guard completion. It also exercises a pending retry on a
different simulated thread. The separate test above uses the actual guard.

## Compile-only retained recovery gateway (2026-11-26)

The retained recovery ioctl now uses a persistent gateway in the non-loadable
rewrite. It acquires the retained pin and controller modules, checks the
validated card/root profile, pins both nodes while the temporary profile pins
are held, and uses the binding for each retry. The normal recovery-owner
admission is checked before those resources are acquired. A pending operation must have
exactly one valid guard ticket, and subsequent calls must retain that same
reset ticket. An already-started reset cannot be adopted after a lease gap.
Before completing the guard, the gateway keeps the owner, PCI reservation,
pins and parked lease and returns an incomplete status until a separate
retirement proof is supplied.

The current driver proof function unconditionally denies retirement: physical
writer, firmware, DMA and device-removal exclusion are not established. The
actual gateway was tested with actual retained pin/controller code and fake
nodes, including a deliberately injected test-only proof, missing module,
changed topology, partial pin-acquisition rollback, stale generation and
ticket validation. The real proof function was separately tested to refuse.
The complete compact resident Haiku/RISC-V driver linked with this gateway and
retained its no-I/O `B_NOT_ALLOWED` initialization. Builds missing the scope,
combining both gateways, or using the isolated finish-test flag were rejected.
The ordinary loadable driver still has no qualified retained gateway; no
candidate was installed, booted, or used for physical recovery.

The broad offline reload suite exited zero after the gateway was added; its
source extraction preceded the final same-ticket and admission refactor.
Those changed gateway and ordinary/live recovery paths were rerun separately
under ASan/UBSan, and the full compact driver was rebuilt with the final code.

## Retained controller module and topology handoff (2026-11-26)

The non-loadable ECAM retained-lease module exports the controller's existing
begin/park/resume/finish methods. The controller still refuses initialization
in this configuration. The driver-side binding, adapters and retained scope
exercise the actual device-manager and ECAM module entry points in simulation, including
a pending call, a retry from another thread, parked config refusal and gated
release. An injected finish failure re-parks the lease and retains both pins.
The PCI profile now provides validated card and root-port nodes to a
callback while the synchronous pins remain held; a simulation acquires the
retained pins there and checks they survive callback return. Wrong FDT and a
replaced root port are refused before callback. These references do not exclude
physical removal or other hardware access.

`build_sg2042_recovery_witness.sh` built and linked control, refusing witness,
gateway, fixed-probe and non-loadable retained modes against Haiku/RISC-V;
it checked module presence, invalid flag combinations and the retained
controller's initialization refusal. The retained binary SHA-256 is
`c490ccddeb5ed571197d457e5033b60614c899e75008dd8da04010f2a7fb9299`.
The recorded ECAM baseline also reproduced byte-for-byte in the qualification
builder. The compact resident driver and profile linked with the actual
binding and retained their no-I/O initialization guard. Nothing was installed
or booted. The compile-only retained mode now defers owner destruction and
cold-latch clearing until the gateway has retired the controller lease and
pins. Both the real recovery sequence and the finalizer passed focused
simulation tests, including refusal before retirement and an incomplete status
returned to the caller. The recovery ioctl in the non-loadable configuration
now uses the retained scope as described
above; physical ownership and release proofs remain to be established before
hardware qualification.

The offline reload suite exited zero. Its source extraction preceded the final
incomplete-status change and some fixture edits; the changed recovery sequence,
profile/pin handoff and binding were rerun separately under ASan/UBSan, and the
final driver was linked again against the actual compact resident image.

The later reload suite for the guard-ordering change also exited zero; the
final guard-race, reservation-ledger and gateway cases were run separately
afterward, and the guarded driver was rebuilt.

## Retained lifetime service (2026-11-26)

An opt-in built-in device-manager module retains initialized nodes and their
ancestors across calls. Monotonic IDs, owner/session identity and a running
flag reject stale handles and concurrent release. A node observed removed is
sticky-invalidated; references remain held until an authorized release.
`run_pioneer_retained_pin_service_tests.sh` tests the actual implementation
and actual `InitDriver`/`UninitDriver` logic against simulated nodes under
ASan/UBSan, including removal during the callback and cross-thread use.
The prior retained controller-scope simulation also passed.

`build_qwen35_pin_kernel.py --retained-pin-service` reproduced both saved
ordinary and spin-idle controls byte-for-byte, then linked an offline Haiku
RISC-V kernel and checked its built-in module registration. Its candidate
SHA-256 is `37ab8e6543d849a1b9d5ee37d3533d0202b4a0507db91ea715296ae17110e64f`;
the corresponding manifest is at `/private/tmp/q35-retained-pin-kernel-20261126/manifest.json`.
The builder also linked its existing non-retained configuration and checked
the retained module was absent. Neither candidate was installed or booted.

The service is a software lifetime reference, not physical-device ownership
or removal exclusion. The newer compile-only gateway consumes it as described
above, but it does not establish DMA, MMIO or firmware exclusion, safe PCI reset, no-reboot recovery,
model correctness or throughput. Repeat the 27B/idle preflight immediately
before any accelerator access.

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

## Live candidate boot and card-independent probe

The generated helper prepared session `q35pin_20261125a` and checked the
pinned inputs before a normal Linux shutdown kexec. Haiku started with 64 host
CPUs. Its read-only inventory matched all 4,841 candidate exported symbol
tuples; the old spin-idle control matched 373. This is layout evidence, not
a full running binary hash; see `candidate-symbol-match.json`.

The card-independent probe returned:

```text
PIN_PROBE version=1 pin=0 manager=0 invalid=-2147483643 root=0 callback=-2147483636 count=1
PASS probe present and cleanup; no accelerator access
```

The test checked lookup, invalid-argument rejection, a root-node callback and
cleanup; it did not test card or root-port pinning, concurrent removal, PCI,
firmware, DMA or recovery. The probe stage was removed. Afterward the stable
driver CPU-only status was `driver55`, generation 1, `cold0 active_owner0`;
the 4B and 27B model files retained their recorded sizes, not freshly hashed.
No card operation, model output, decode rate or no-reboot recovery was tested.

## Live card and root-port software pin check

The production recovery profile and temporary probe use the same nested
card/root-port pinning helper. After the stable driver and process checks, the
probe inspected the driver names and cookies while both references were held:

```text
PCI_PIN_PROBE version=1 manager=0 pin=0 card=0 port=0 callbacks=1 drivers=1
PASS pinned card/root-port metadata and cleanup; no PCI config or accelerator IO
```

The trial checked idle before and after, removed its temporary registration,
driver, exclusive lock and staged files, and the two model files had their
recorded sizes. This proves a bounded software-reference operation only, not
device presence throughout hot removal, PCI reset, physical writer exclusion,
no-reboot recovery, inference or throughput. The focused shared-profile
simulation and Haiku cross-compiles passed. The first full reload suite overlapped a
test edit; the final-tree rerun completed with exit 0 (`/private/tmp/q35-pinned-nodes-final-reload-20261125.log`).

The full fused/tree/link-reset resident witness cross-build also passed, with
load refusal checks and driver SHA-256
`e9c3ed638cad6dd088235cace286c947066c35f3718817c42a722b5deca769e1`.
It is not a loadable inference driver.
