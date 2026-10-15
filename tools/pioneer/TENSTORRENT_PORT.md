# Tenstorrent P100A: RISC-V Haiku bring-up

## Status

Source/feasibility investigation started 2026-09-15. On October 15,
TinyStories-260K generated eight verified tokens under Haiku with every
learned linear projection/classifier computed on the P100A: `Once upon a
time, there was a`. All CPU reference token choices matched; 8224 device
tiles completed/stopped/restored in 123.050 seconds. This is an experimental
fixed-shape hybrid runtime: the host handles norms, attention/KV, activations,
embedding lookup and partial sums. **Full Qwen3.8-27B inference is not working
yet.** Its real Q4_K/Q8_0/Q6_K projection samples now pass independently.
Haiku retains 64 CPUs and 127.62 GiB usable RAM. See the live records below.
The native Assistant now integrates this fixed-shape runtime as explicitly
labeled experimental story completion: a real GUI prompt `Once` produced
eight output tokens, ` upon a time, there was a little`, using 9,252 card
tiles across nine evaluated positions. The model manager installs both pinned
data files from Hugging Face; hardware selection commits the compatible
model/backend pair. Stop is batch-boundary-safe and never falls back to CPU.
An initial DRAM-route preflight refusal and an exception-path failure were
observed; explicit error returns replaced exception unwinding, without
relaxing device guards. This remains experimental, not full Qwen support.
See [Assistant details and validation](../../src/apps/assistant/README.md#p100a-integration-verification-on-pioneer-2026-10-15).
The next performance prerequisite now passes on the real card: the sole
compute tile's NoC0 can read a fixed 64-byte GDDR sample into its own L1,
with a poisoned destination, independent host comparison and complete
L1/NIU/TLB/PCI rollback. This is **not** a resident Qwen backend, a bandwidth
measurement, or a faster-than-CPU result. The Assistant's supported modes
have not changed. See the October 15 device-residency record below.
The next live gate also passed twice: TRISC0 fetched resident GDDR inputs
itself and computed two dense BF16 matrix fixtures with zero numerical
mismatches. No host NoC doorbell write or input write between fixtures was
allowed. This remains a fixed one-tile proof, not Qwen inference or a speedup.
The same card now also passes full on-card accumulation for fixed 16-output
GEMV fixtures with 1,024, 5,120 and 6,144 inputs. It no longer needs a host
request/reset or a CPU partial sum for each 16-element input block. Model
weights are not yet persistent across requests and Qwen chat remains CPU-only.
As of October 14,
Haiku enumerates the installed P100A through the SD-free RAM boot path,
its optional read-only diagnostic works, and a controlled temporary BAR
mapping reads the card's NoC identification register successfully. A fixed
read-only NoC window also reads ARC boot/error status and telemetry pointers.
ARC boot status is stable at `0x5`, and the sampled error-status word is zero.
Bounded, whitelisted telemetry reads now work too: firmware bundle
`18.5.0` (raw suffix `01`), ASIC about 46.6 C, fan 1,882 RPM, seven enabled
GDDR channels with training complete and no reported uncorrected errors.
Repeated samples show an advancing firmware heartbeat. This is a firmware
health snapshot, not a memory stress test or working accelerator runtime.
One stopped Tensix tile also passes a reversible 64-byte L1 pattern test,
including verification of the original bytes after restoration.
The tests restore the card and bridge configuration exactly. The initial
experiments never enabled DMA; a later fixed owned-page read now does so
only for the duration of the verified operation described below. There is
no general-purpose model compute runtime yet. See the
live-test records below. Samsung's installed OS package, card firmware,
and default Linux boot remain unchanged.

A first optional Haiku diagnostic add-on is now in
`src/add-ons/kernel/drivers/misc/tenstorrent_pci_diag.c`. It binds only PCI
`1e52:b140` and exposes one read-only, versioned snapshot through
`/dev/misc/tenstorrent_pci_diag`. The matching
`tools/pioneer/tenstorrent_haiku_diag.c` prints the snapshot. The kernel
add-on compiled cleanly with `-Wall -Wextra -Werror` and linked against the
existing kernel build. The CLI also compiled and linked against the existing
Haiku runtime. Both used `/Volumes/HaikuBuildLocal`'s RISC-V compiler and are
saved under ignored `generated.pioneer/tenstorrent/haiku-build/`. Jam was
missing for that check, so this was a direct compile/link rather than an
integrated Haiku build. The two artifacts are staged in a separate test BFS filesystem and a
separate full-disk image based on the existing 2026-09-08 `haiku-mmc.image`.
The full image retains its original boot partition; its test BFS partition
contains the add-on at
`system/non-packaged/add-ons/kernel/drivers/dev/tenstorrent_pci_diag` and the
CLI at `system/non-packaged/bin/tenstorrent_haiku_diag`. A read-only BFS
consistency check found no allocation errors across 249 nodes. The source
image and boot partition hashes were verified unchanged. This is a manually
assembled test image, not a fresh integrated build of current source. Its
300 MiB fresh-install filesystem must not overwrite the persistent 4 GiB SD.
Those manually assembled SD images were not deployed. The October 14 live
tests below instead deploy the optional add-on/CLI on Samsung while booting
through the private RAM handoff.
The snapshot reports identity, command/status and raw BAR configuration,
but deliberately does not size BARs, infer usable mappings, touch MMIO,
enable bus mastering, use DMA, or flash/reset the card. The legacy `pci_info`
arrays hold 32-bit words: valid 64-bit BARs use adjacent entries for the high
address and high size. Do not truncate to one element. Neither these cached
values nor raw BAR registers substitute for validated resource allocation
and host-address translation.

On October 12, the unchanged SD booted normally after the P100A was physically
removed. With the card installed, UEFI PCI resource allocation failed and
Haiku rejected the USB controller's zero BAR; an xHCI error-cleanup bug then
panicked. See [the SD/xHCI recovery record](XHCI_RECOVERY_20261012.md).
Jam has now been restored for the USB driver build. The cleanup fix is not
a PCI-allocation fix or evidence of a functioning accelerator. Keep the P100A
out while establishing the Haiku port baseline.

### October 15: first owned host-to-card read passes twice

A new, separate read-only preflight first verified PCIe Device Status on
both endpoint and root (`0000/0000`, including no Transactions Pending), and
the read/write HDMA channel 0 registers used by pinned 18.5.0 firmware. Three
samples showed both enables zero and both statuses `3` (stopped). These
registers are accessed through documented device-local NoC DBI window 62 at
`0xf800000000380000`, using the full 43-bit BAR0 TLB address field; BAR2's
1 MiB aperture is insufficient for these offsets. No PCIe-tile/HDMA register
is written. Source: pinned official `tt-system-firmware` 18.5.0
`lib/tenstorrent/bh_arc/pcie_dma.c` and `tt-isa-documentation`
`8a34e25e04d1ad7dd129ac9231e126b85923fac3`, PCI Express Tile address spaces.
This checks the firmware-used channel pair, not undocumented DMA channels.

The first real **host-to-card read** then passed twice on Haiku. Each run
allocated a fresh owned, wired non-cached 4 KiB page above 4 GiB, checked its
mapping/patterns and read exactly 64 bytes from page offset 128 into the
poisoned final 64 L1 bytes of stopped physical tile `(1,2)`. No user pointer,
arbitrary length, host-write operation or firmware DMA command is accepted.
NoC0 initiator 0 uses translated PCIe coordinates `(19,24)` (physical `(2,0)`),
the already-audited SII window 0 and 48-bit root inbound identity mapping.
All 120 enabled compute tiles remain held reset.

The dedicated policy briefly enables endpoint/root bus mastering (`0406`),
after rechecking the firmware engines, SII/inbound mapping, exact idle
initiator, routes and full host-page pattern. Both runs completed in one
poll with exact response/flit/accepted/sent deltas **1/1/1/1**, zero
destination mismatches and zero full-page host-guard mismatches. Completion
requires ordered command readback, all four command buffers idle, all read
and outgoing-write IDs idle, stable physical mapping and unchanged routes.
Bus mastering was disabled/read back before freeing the host page; BME-off,
L1, NIU, TLB and PCI rollback all passed. PCI commands ended `0000/0000`;
pages were released, none quarantined, and no cold recovery was required.
A read-only preflight between the runs again found no pending/error status
and both firmware channels disabled/stopped. Assistant team 2705 stayed
ready and idle with its existing model/conversation untouched. A subsequent
native 1,024-input resident GEMV regression also passed: zero mismatches,
64/1,088/64/64 counters, complete rollback and no cold-recovery requirement.

Timeout, a dropped doorbell, unexplained counter delta, changed route/map or
unverified BME-off/rollback latches cold recovery and quarantines the wired
system-team page. A delayed read must never see reused private memory. No
reset, counter clear, payload rewrite, retry or module reload is a recovery
mechanism. ASan/UBSan tests exercise every lost setup/rollback write, the
maximum 48-bit source address, counter wrap, accepted-but-incomplete reads,
bad data/flits, host-guard corruption, PCIe pending status, changed mappings,
permanent cleanup failures and quarantine ownership. The unchanged MMIO,
ARC, reset, L1, BRISC, DRAM and compute-route suites also pass. The RISC-V
driver and all 27 utilities build successfully.

Native artifacts/logs are in `/boot/home/pioneer-host-read-20261015/` with
the prior read-only driver saved as `tenstorrent_pci_probe.before`. SHA-256:

- Driver: `828dc285510a06860dfb4a32ddbb40da32281d8b27a20f092bad96875d6247c0`
- Host-read CLI: `93bbf2ec60df71661925bd09fc51dae5c884245eb8d42c23d40035438c2625df`

The preceding read-only audit's artifacts are separately preserved at
`/boot/home/pioneer-host-dma-audit-20261015/` (driver
`c41f1b5c4e79709cef13a3580fcf72c214477903d23d1519b442d40c040a7fbf`, CLI
`df8a0a8f62db8d68037751cbcb68eb63a469a1888b29efd955b8aec661ef0db7`).
These results establish a safe, coherent high-address read path for the
owned test page. They are **not** a bulk bandwidth measurement, a persistent
model allocator, full Qwen inference or a faster-than-CPU result. Next:
larger bounded transfers, persistent weight storage and the actual Qwen
graph/backend, followed by correctness and same-model CPU/GPU benchmarks.

### October 15: owned high-address host buffer prerequisite

The native CPU-only host-page diagnostic passed on the running Haiku system.
It allocated a dedicated contiguous, wired 4 KiB system-team area above 4 GiB
(33 physical address bits), validated its full mapping, configured the same
ordinary non-cacheable RAM type used by the port's NVMe queues **before any
payload write**, checked two full-page patterns, revalidated the mapping and
released the area exactly once. Allocation/map/cache/release statuses were
all zero, both patterns matched, and the Assistant remained ready and idle.
This test did not reserve PCI, map device registers, enable bus mastering,
issue a device transaction or establish DMA coherence.

`PioneerHostPage.h` separates owned-page preparation from the CPU-only
prepare/check/release wrapper so a future bounded device-read test can retain
the allocation until proven completion. The native owner can quarantine a
wired system-team area after uncertainty; destruction must not free a page
which a delayed device read could still access. No DMA caller exists at this
checkpoint. Host ASan/UBSan tests cover every lost pattern write, invalid and
overflowing addresses, changed mappings, allocation/cache failures and exact
cleanup ownership. The driver and all 25 cross-built utilities compile.

Native artifacts and the scoped log are in
`/boot/home/pioneer-host-page-20261015/`; the previous driver is preserved as
`tenstorrent_pci_probe.before`. SHA-256:

- Driver: `5317631f2792bb5549172f74065dbda714e5b9b2f9fa7efcf59c7afb6133f907`
- Host-page CLI: `bf92b31bd4e2267bd685f629ad4b34bf95a4c1c3b8f8467bab0282930eb2c2be`

The next transfer gate is a strictly bounded read **from** this owned host
memory, including PCIe pending-transaction/DMA-engine checks, destination
poisoning, complete counter proof and quarantine on uncertain completion.
Neither this prerequisite nor the resident GEMV fixtures are full Qwen
inference or a speedup over the measured Qwen CPU baseline.

### October 14: resume after SD-free baseline

The card-free baseline is now established: Linux → Haiku → Linux → Haiku
without either SD, using the RAM handoff and the tested RTL8125 MSI-only
override. Haiku SSH and Samsung root work; the initial loader still exposes
one CPU and about 3 GiB RAM. The user requested renewed P100A bring-up and
confirmed the card remains physically removed. Normal Haiku shutdown closed
services/SSH, followed by the known seven-second relay power-off hold at
about 18:13 UTC. Physical fans/LEDs-off and AC disconnection must be confirmed
before installing the card. Leave both SD cards out.

Synced the previously absent read-only diagnostic C/header and their sole
Jam target into the reviewed build mirror. An integrated Jam build of
`<driver>tenstorrent_pci_diag` passed; the private artifact/log are under
`generated.pioneer/tenstorrent/haiku-integrated-20261014/`. Nothing was
installed on Samsung, no accelerator driver was loaded, and no card firmware,
PCI configuration or DMA settings were changed by this preparation.

Next: reinstall in the previously used lower slot with the correct fully
seated native power cable while AC is disconnected; restore PSU power but
leave the host off for controlled startup. First cold-boot the unchanged
NVMe Linux, record identity/link/BAR assignments read-only, then assess a
Haiku enumeration test through the new boot path. Bypassing UEFI removes
that particular boot dependency, not the documented BAR4 aperture limit.
The firmware access/backup/compatibility gate below still applies; do not
flash the old experimental bundle or hide an enabled/unassigned BAR merely
to make a driver bind.

### October 14: installed P100A, native enumeration and reversible BAR4 test

The user reinstalled the card. A relay-controlled cold boot reached unchanged
NVMe Linux 6.1.31 with both SD cards out. Linux again found `0002:81:00.0`,
`1e52:b140`, subsystem `1e52:0043`, at 2.5 GT/s x8. All three BAR allocations
still failed (512 MiB, 1 MiB, 32 GiB); command was `0000`, no driver bound.
The old known-good RAM handoff still booted Haiku and SSH with the card in,
but `listdev` omitted it. Serial proved why: the ECAM driver unconditionally
skipped hardware root `0x80`, rather than checking this installed endpoint.

Added an opt-in ECAM gate, leaving the default skip unchanged. It requires
the exact full-width FDT bus range `80..bf`, a four-byte big-endian
`haiku,pioneer-root-probe = <0x1e52b140>` property on `/pcie@7062000000`,
controller address `0x7062000000`, and config aperture `0x4800000000/0x1000`.
Existing unavailable-link fill-pattern guards remain. The tag is a private
enumeration request, not authentication of a card and not permission to
enable endpoint decoding, bus mastering, or firmware writes. Host ASan/UBSan
tests and integrated `<pci>ecam` cross-build passed.

Only the existing `add-ons/kernel/busses/pci/ecam` entry was replaced in a
new native-only copy of Samsung's HPKG. Sorted package path sets matched,
the new module extracted to the expected hash, and the Samsung kernel stayed
`38d4a4245db6a8ec4e2baa44c7705cdcdd5d4e66a8d43da97f3157699449a65b`.
The original package remained unchanged. The fresh 300 MiB RAM BFS passed
check-only consistency and package round-trip verification. Initial default
maximum compression was slow and reordered the TOC; the successful final
build uses fast compression and compares path sets independently of order.

Private native experiment under `/mnt/ssd/pioneer-haiku-kexec-20261013/`:

- `p100a-root-probe-2/samsung-ram.bfs`:
  `f6292d07bc16f41bedca3d1b63edb2da2fa9961065da3b86e66e74b95196b611`
- Its private HPKG:
  `7a830442362ba511bdfc08f6d99e26df5813475861d3583a90cb62cbef1f3cea`
- Candidate ECAM:
  `23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d`
- `boot-p100a-root-probe-2.sh`, session
  `p100a-root-probe-2-20261014`, uses those exact pins, the existing loader,
  fresh Samsung sector manifest, UART keepalive, and normal `systemctl kexec`.
  The tracked baseline boot helper's original artifact pins were not changed.

At about 18:39 UTC, serial showed the opt-in marker, root link state
`0x5000ff07`, and endpoint vendor `0x1e52`. Haiku mounted Samsung and SSH
returned. `listdev` identifies Blackhole. The optional read-only diagnostic
was installed in the user non-packaged driver tree and ran successfully:
Haiku virtual PCI `03:00.0`, subsystem `1e52:0043`, command `0000`, status
`0010`, raw BAR words `0000000c 0 0000000c 0 0000000c 0`.
Haiku's virtual bus number is not Linux hardware bus `81`.

Added the separate optional `<driver>tenstorrent_pci_probe` and
`tools/pioneer/tenstorrent_haiku_bar4_probe.cpp`. It accepts only an explicit
root, read/write-open ioctl. It reserves the sole matching P100A during the
transaction, rejects any enabled command, assigned BAR, or unexpected cached
BAR size, temporarily writes BAR4's high address, then restores and verifies
all six BAR words. No arbitrary register/address request, MMIO, bridge
change, command-enable operation, DMA, reset, or firmware access exists.
Simulated refusal, repeated use, narrower address masks, concurrent-command
interruption, and restoration-failure tests passed under ASan/UBSan.
The module cross-built and the CLI compiled natively with warnings as errors.

Live discovery needed the canonical user `drivers/bin/tenstorrent_pci_probe`
plus `drivers/dev/misc/tenstorrent_pci_probe -> ../../bin/tenstorrent_pci_probe`;
adding a file only to the already-scanned category was not noticed. The first
attempt therefore made no PCI writes. The canonical-bin installation was
discovered, and the controlled test then passed: all BARs restored exactly,
command `0000 -> 0000`, sizes `0x20000000 / 0x100000 / 0x800000000`, temporary
BAR4 readback `fffffff8:0000000c`. A subsequent independent read-only ioctl
confirmed the original BAR values and command `0000`.

This establishes hardware address-bit retention, **not** a resource allocation
or safe CPU mapping. A possible next experiment is to keep unused BAR4 in a
verified unforwarded high PCI interval while assigning only BAR0/BAR2 inside
the recorded host window. Before any memory enable, validate the complete
BAR intervals, bridge windows, host translations, interrupt state, exclusive
ownership, and rollback. Do not merely hide BAR4 in software. There has been
no card MMIO/DMA or firmware write. Firmware flashing still requires a
verified backup/recovery plan and separate approval.

### October 14: first working Haiku card-register access

The next optional probe operation audits the actual device-manager ancestry,
exact FDT tag/register layout, two-device root topology, all five outbound
windows (one I/O plus four memory), and domain-aware translation of both ends
of the proposed BAR0/BAR2 intervals. `GetPioneerPciProfile()` and its userspace
tool are read-only. The initial recursive traversal hit the existing kernel
`get_next_child_node()` missing-break behavior; the audit uses the working
`find_child_node()` API instead, without changing the installed kernel.
The exact PCI interface name is `bus_managers/pci/driver_v1`. A first layout
check omitted the I/O range, refused safely, and was corrected to include all
five actual ranges. The final audit passed at stage 6:

- Hardware controller/config: `0x7062000000`, `0x4800000000/0x1000`.
- Haiku virtual card/root buses: 3/2, root buses `003f0100`.
- Initial root command `0000`, memory window `0000fff0`, prefetch window
  `0001fff1`, both upper prefetch words zero (disabled windows).
- BAR0 PCI and CPU interval: `0x4900000000..0x491fffffff` (512 MiB).
- BAR2 PCI and CPU interval: `0x4920000000..0x49200fffff` (1 MiB).

The explicit `TT_PCI_PROBE_PIONEER_NOC_ID` operation requires root and a
read/write-open descriptor. It reserves the card, repeats the live audit,
rejects enabled commands, assigned BARs, unexpected sizes, root-port BARs,
enabled option ROMs, active MSI/MSI-X, malformed capability lists, and a
non-D0 PM state. It assigns only BAR0/BAR2 inside the recorded window and
places the complete unused 32 GiB BAR4 at PCI
`0xfffffff800000000..0xffffffffffffffff`, outside every forwarded memory
range. This is a hardware BAR placement, **not** hiding a live BAR in kernel
metadata and **not** disabling the BAR in hardware. It is not a general
resource allocator or a permanent driver configuration.

The temporary bridge prefetch window is `20010001`, upper base/limit `49/49`.
Only command `0402` (memory + INTx disable) may be enabled on card and bridge;
bus mastering, I/O, firmware writes and resets are never enabled. A single
BAR0 page at CPU `0x491fd04000` is mapped kernel-read-only and explicitly
uncached. Exactly three reads use register offset `0x44`, followed by unmap,
decoder shutdown, complete BAR/window restoration, and configuration readback.
Disassembly caught `__sync_synchronize()` producing only `fence rw,rw`;
the final kernel implementation explicitly uses RISC-V `fence iorw,iorw`.
The offset/coordinates are also documented by the pinned Apache-2.0 UMD
`blackhole_implementation.hpp` / `blackhole_arc_apb.cpp`; no GPL driver
implementation was copied.

Host ASan/UBSan tests cover every dropped configuration write, map/unmap
failure, failed decoder shutdown, unsafe geometry, stale/active BARs,
interrupt/ROM/PM guards, malformed/cyclic capabilities, unstable register
reads, and restoration checks. They assert no DMA-enable operation and no
BAR/bridge-window rewrite while its decoder is active. These are simulated
failure tests, not injected hardware failures.

Before the first MMIO test, SSH stopped responding during banner exchange
and serial showed a userspace "Crashed program" dialog. No new MMIO module
had been installed or run at that point; the cause is not established.
Graceful SSH shutdown was unavailable. The last optional-driver installation
had been synced; a relay power-off hold and cold boot recovered normal Linux
with card command `0000`. The same private RAM package booted Haiku again
without either SD. Native compilation was removed from the hardware-test
dependency: `tools/pioneer/tenstorrent_cross_build_tools.sh` builds all four
diagnostic CLIs against the configured build's SDK/startup objects/libroot,
using a fresh output directory and no device access. Its three C++ binaries
reproduced the earlier cross-build hashes and ran on Haiku.

At about 19:31 UTC, the first hardware transaction returned
`18611002 / 18611002 / 18611002`, with complete PCI rollback. The original
validator wrongly required the entire word to equal 2 or 11; upper register
fields are nonzero. The corrected validator checks the six-bit X/Y coordinate
pair while preserving the full raw value, matching PCIe tile `(2,0)` here.
A regression case covers this real value and rejects a wrong Y coordinate.
At about 19:36 UTC the repeat returned **state 1, stage 5, restored 1** and
the same three words. All six BAR words and all three bridge-prefetch words
matched their originals, card/root commands were `0000/0000`, and an
independent read-only driver snapshot confirmed the card's original disabled
state and status `0010`. SSH remained usable. This establishes bounded
memory-mapped card-register access, not ARC/DRAM health, DMA, or compute.

NoC-ID-tested module at this checkpoint in user `drivers/bin/tenstorrent_pci_probe`, with
the existing `dev/misc` symlink:
`1ec5a894442283e47b31338628a563cff5b4699b21941f298e10424b9c1e60b5`.
The previous binaries are retained under
`/boot/home/pioneer-p100a-20261014/` for recovery. Mac-built tools and module
are privately staged in its `mmio-cross-1/` directory. The baseline tracked
boot helper and installed OS package were not modified. The last boot used
native session `p100a-mmio-20261014-1` with the previously verified RAM bundle.

Next: bounded ARC status/telemetry over a 2 MiB NoC window, with restoration
of the exact TLB configuration. The observed PCIe tile is X=2; the pinned
UMD explicitly routes ARC access over NoC on that tile. Do **not** assume
the direct BAR0 ARC/APB route (available at X=11) works here. No ARC access,
firmware update, DMA test, general memory mapping API, or accelerator runtime
has been implemented/tested yet.

### October 14: ARC firmware responds over a restored NoC window

Added an explicit `TT_PCI_PROBE_PIONEER_ARC_SCRATCH` operation and matching
`tenstorrent_haiku_arc_probe --read-arc-status-and-restore` CLI. It reuses the
tested temporary PCI transaction and NoC-ID validation. Only PCIe tile `(2,0)`
may enter this new path. The ARC target is NoC0 `(8,0)`, reached through the
last 2 MiB BAR0 window, index 201. Its 12-byte configuration at BAR0
`0x1fc0096c` is saved, temporarily configured, verified, and restored exactly.
The independently encoded words are `00000400 / 04004000 / 00000a40`:
ARC local base `0x80000000`, strict unicast-read ordering, static read VC/buddy.
The ARC data page at BAR0 `0x19230000` is kernel-read-only and uncached;
only the TLB control page is writable. There is no ARC data-write method.

The sample reads scratch registers 2, 4, 11, 12 and 13, then repeats register
2, for exactly six ARC reads. These cover boot status, error status, message
queue pointer and telemetry data/tag pointers. Pointers are reported but not
dereferenced. There are no ARC messages, ASIC/core resets, firmware writes,
SPI operations or DMA. The outer operation unmaps the target before shutting
down PCI decoding and verifies complete PCI rollback. A TLB restore failure
gets one bounded retry while decoding is still on; any remaining restore
failure requires cold recovery. Destructors only remove virtual mappings and
never attempt MMIO after decoding has been shut down. Partial map failures
are also cleaned up by the outer transaction.

The fixed layout and ARC status/scratch facts were checked against pinned
Apache-2.0 UMD sources. Host ASan/UBSan tests independently decode the 96-bit
TLB layout and cover dropped setup/restoration writes, unknown saved bits,
nonzero saved configurations, boot/error/telemetry conditions, and bounded
reads. The existing NoC-ID/PCI policy suite also covers failed callback and
partial-map cleanup. Integrated kernel cross-build passed, and disassembly
shows explicit `fence iorw,iorw` around device accesses. The cross-build script
now builds five userspace tools; the original four binary hashes stayed the
same. Their installed operations retain their earlier ABI/behavior.

The new optional module was staged with its previous version retained. A
NoC-ID hardware regression test passed before the new operation was called.
At about 19:51 UTC, the ARC operation returned **ARC state 1, TLB restored 1,
PCI state 1, PCI restored 1**. TLB words were initially all zero, became the
three expected words, then read back as all zero after restoration:

- ARC boot: `00000005 -> 00000005`.
- ARC error status 0: `00000000`.
- Message queue pointer: `100161c4`.
- Telemetry data/tags pointers: `1001902c / 10018f28`.
- Card/root command after: `0000 / 0000`; NoC ID: `18611002`.

The independent read-only PCI snapshot confirmed original BAR words, command
`0000`, and status `0010`. No card firmware was changed. This proves bounded
ARC register access and boot-ready status, **not** a complete card/DRAM health
test, firmware compatibility assessment, interrupt/DMA path or compute stack.
A second ARC run returned the same values, again with initially zero TLB
words and exact restoration. The independent live profile then passed stage
6, including original disabled root windows and both domain translations;
the separate PCI snapshot again showed the original disabled endpoint.

Optional module at this ARC-only checkpoint (superseded below):
`d17d56a3035927d47eed55f0f7161aa0af0dbde117c9f7259afd124d62e149a7`.
ARC CLI:
`4ebae8542bd87a54afc58a897235f3f498d0e7b1beefddcbd73a9fcc69a166bd`.
They are in the existing private `mmio-cross-1/` staging directory. The prior
module is retained as `tenstorrent_pci_probe.before-arc-1` there.

Next: validate the advertised telemetry-table layout and ARC CSM bounds
against the vendor sources before reading whitelisted firmware-version and
health values. Do not assume that a nonzero pointer authorizes an arbitrary
NoC read or a flash operation.

### October 14: native Haiku firmware telemetry and live heartbeat

The next explicit operation, `TT_PCI_PROBE_PIONEER_TELEMETRY`, first performs
the tested ARC status transaction, then reuses TLB 201 for a read-only CSM
window. The CSM NoC base is **`0x10000000` directly**, not that address plus
the ARC/APB XBAR offset. Its words are `00000080 / 04004000 / 00000a40`.
Only the first 128 KiB is mapped, despite the hardware CSM having a larger
aperture. Both the ARC and telemetry TLB configurations and the entire PCI
transaction are independently restored and verified.

The parser requires aligned, bounded tag/data pointers, at most 128 entries,
a supported table major version, non-overlapping table/data spans, valid
offsets and no duplicate requested tags. It repeats the header after sampling
and clears the output mask if the header changed. Values are not an atomic
snapshot: live telemetry can change between individual reads. It reads only
12 whitelisted public health/version values, never board/ASIC serial values
or arbitrary requested addresses. There are no data writes, firmware
messages, SPI operations, reset commands, DMA or bus-master enables.

At 20:10 UTC the real card returned telemetry state 1, ARC state 1, PCI state
1 and all three restored flags 1. Table version was `00000100`, with 63
entries and all 12 requested tags present. A repeat returned the same
structural values and restored flags:

| Public telemetry | First sample | Repeat / interpretation |
|---|---|---|
| Firmware bundle | `12050001` | 18.5.0, raw suffix `01` |
| CM firmware | `000e0000` | unchanged raw value |
| ASIC temperature | `002e8f7a` | `002eb04a`, about 46.56 to 46.69 C |
| Maximum GDDR temperature | `0000002e` | 46, unchanged |
| Fan | `0000075a` | 1,882 RPM |
| Enabled GDDR | `000000f7` | seven enabled channels, consistent with nominal 28 GiB; capacity not tested |
| GDDR status | `00005515` | training complete/no error on all seven enabled channels |
| GDDR uncorrected errors | `00000000` | none reported in these samples |
| Enabled Tensix columns | `00000fff` | raw mask; no compute test |
| Firmware heartbeat | `0000a44a` | `0000b6a6`, advancing |
| Board temperature / shutdown limit | both zero | sensor/limit availability unverified; do not interpret as a healthy 0 C reading or limit |

The GDDR interpretation follows the pinned UMD's per-channel two-bit training
fields (completion bit then error bit), with disabled channel 3 ignored.
This firmware predates 19.7's upper-half BIST reporting: the zero upper half
does **not** mean a failed or completed BIST. This did not run a DRAM test.

The pinned UMD explicitly sets Blackhole's minimum compatible bundle to
18.5.0 and has an 18.5-family feature map. Its `FirmwareBundleVersion`
comparison normalizes away the suffix, so `12050001` meets that numerical
minimum. This does not validate every current Metal/LLM release or prove the
entire software stack compatible. The official versioned
[Metal v0.62.2 installation matrix](https://docs.tenstorrent.com/tt-metal/v0.62.2/tt-metalium/installing.html)
also names Blackhole firmware 18.5.0. No firmware update is needed merely to
continue this bounded native transport investigation. Flashing still requires
a verified backup/recovery plan and separate approval; the old experimental
19.15 bundle remains unflashed.

The independent profile passed stage 6 after the repeat, with original
disabled root windows. The separate diagnostic confirmed card command
`0000`, status `0010` and all six original BAR words. All original TLB words
were zero before and after both samples. Haiku and SSH stayed available.

Host ASan/UBSan tests cover pointer/span/count/version validation, maximum
128-entry tables (144 bounded CSM reads), duplicate/out-of-range entries,
changing headers, missing required fields and dropped setup/restore writes.
All earlier policy/BAR/MMIO/ARC suites still pass. The integrated kernel
module and all six userspace tools cross-built successfully; the previous
five CLI hashes were unchanged.

Optional module at this telemetry checkpoint (superseded below):
`5ee9781e130f6e8ca701c4bb95adecef4ede61f2a2b6cbbecaaabec43a3e0456`.
Telemetry CLI:
`75701df2eb37c67c20516fa3dc6fce668a1848385d732980ac247f63944013c6`.
Both are installed/staged in the existing private `mmio-cross-1/` tree.
The prior module is retained as `tenstorrent_pci_probe.before-telemetry-1`.

Next: audit a bounded device-memory/compute-core smoke-test path, including
physical/harvested tile coordinates, reset state, read/write NoC ordering,
scratch ownership and rollback, before any data or core-control writes.
Persistent mappings, interrupts, non-coherent host DMA, native UMD/runtime
and LLM execution remain unimplemented and unvalidated. Both SD cards stay
out; Samsung's installed OS package/default Linux boot remain unchanged.

### October 14: stopped Tensix core and reversible L1 memory test

Added two explicit private operations, `TT_PCI_PROBE_PIONEER_TENSIX_RESET`
and `TT_PCI_PROBE_PIONEER_L1_SCRATCH`, plus matching userspace tools. They
retain all earlier profile, PCI, ARC and telemetry checks, require the measured
firmware tag `12050001`, and require enabled-column telemetry bit 0. The
vendor's physical harvesting-column order starts at x=1, and its Tensix grid
starts at y=2. The sole permitted tile is therefore `(1,2)`. No arbitrary
coordinate or address is accepted from userspace.

The initial conservative build refused NIU configuration `00006005` because
translation bit 14 was set; that run made no core-register reads. Source
review of UMD's `fill_tensix_noc0_translated_mapping()` established that the
first enabled physical column maps to translated x=1. Since bit 0 requires
physical x=1 itself enabled, this one tile remains `(1,2)` in either mode.
The revised guard permits that specific case, not arbitrary translated
coordinates. One immediate run still used the previous loaded module before
Haiku's asynchronous driver watcher reloaded it; a later run reached the
new code. The canonical-file hash alone does not prove an already-loaded
driver revision has changed.

The reset-state operation maps only the read-only CSR page, temporarily using
TLB 201 words `000007fd / 40840800 / 00000a40`: NoC0 `(1,2)`, local base
`ffa00000`. It reads `ffb121b0` three times, never writes reset controls, and
requires all BRISC/TRISC0/TRISC1/TRISC2/NCRISC reset bits (`00047800`) held.
All three hardware reads returned exactly `00047800`; the TLB and full PCI
state were restored, independently confirmed by the diagnostic.

The L1 operation first passes that same gate. It separately saves/configures
TLBs 200 and 201, using TLB 200 for the last 64 bytes (`17ffc0..17ffff`) of
that stopped tile's 1.5 MiB L1. Its data window is strict **bidirectional,
dynamic VC**, words `00000000 / 40840800 / 00000040`. Do not reuse the static
read VC for mixed reads/writes: the pinned UMD explicitly warns that this can
crash the host. TLB 201 remains the strict read-only reset window.

The only data-write API is sixteen fixed, aligned words. Original values stay
kernel-local, never in the ioctl or logs. The operation checks reset state
before patterns and after testing, writes/reads two independently
address-varied patterns, restores all sixteen original words, and reads them
back. A failed data restoration gets at most one retry before either TLB is
changed. Persistent restoration failure requires cold recovery; no destructor
ever touches device MMIO after PCI decoding is off. Normal traffic is 48 L1
writes / 64 L1 reads, with at most 64 writes / 80 reads on a restore retry.
There is no core start, instruction loading, firmware message, flash, reset
write, interrupt enable, DMA or bus mastering.

At about 20:27 UTC the real card passed both patterns with zero mismatches,
data/TLB/PCI restored `1/1/1`, and reset before/after `00047800/00047800`.
All six TLB words were zero both before and after. Card/root command ended
`0000/0000`; the independent snapshot confirmed all original BARs and status
`0010`. A second full L1 run passed identically. Subsequent telemetry still
reported about 46.69 C, 1,882 RPM, GDDR errors zero and an advancing heartbeat
`0000de7c`. The live root profile again passed stage 6. No panic was captured,
and SSH remained working.

Both new host suites pass ASan/UBSan, including independent 96-bit TLB decode,
wrong firmware/harvest state, translated/non-translated modes, changed or
running core state, setup/restore failures, every dropped pattern/restore
data write, and persistent restore failure. Fault-injection initial TLB words
must differ from trial words: dropping a write of the already-present zero
is not a configuration failure. All seven host suites and the integrated
kernel build pass. All eight CLIs cross-build; the earlier six hashes remain
unchanged.

Current optional kernel module:
`f0ab0ebac81ffd0f2b8ac81ad3dacfb90ecbd1f2e8728b3765543178776b101b`.
Core-state CLI:
`5b3cdeb3b3f5db9c93d3b8f29ba61fbeded11c8159549e53e075c92323ae1813`.
L1 CLI:
`0b4e526fa699c52deb800efe4902b9e5fcabc7c3dffb96a30723cc2ea0a02372`.
Staging remains `mmio-cross-1/`; the previous module is retained as
`tenstorrent_pci_probe.before-l1-1`. The intermediate core-only modules are
also retained. Only reviewed selected sources were synced to the build mirror.

This proves native host PIO access to a stopped compute tile's local memory,
not execution on its RISC/Tensix processors, a GDDR capacity test, performant
transfers or an LLM backend. Next inspect the vendor's minimal core startup,
instruction/cache requirements and compatible runtime before launching code.
The latest vendor Metal installation matrix names newer firmware than this
card has; the older versioned 18.5-compatible matrix and UMD's minimum do
not certify current Metal/LLM releases. No automatic firmware upgrade.

### Minimal execution audit: not yet a hardware execution test

Read only the relevant official Metal v0.62.2 sources in an ignored,
no-checkout, filtered clone. Annotated tag object
`278c18eddaee71ff6127f083a4a5f2934b688866` resolves to commit
`d279aa36bebe71367a521449310f6f53100e92be` (Apache-2.0). No upstream script,
build or initialization command ran. The clone is private generated source,
not an added dependency or a validated native Haiku runtime.

Blackhole's `bh_hal_tensix.cpp` sets BRISC's reset PC to zero and places a
generated JAL at L1 address zero. `hal.cpp`'s startup helper explicitly bounds
the firmware base below `0x7ffff`. `dev_mem_map.h` reserves low L1 locations,
including ARC power-throttling scratch at bytes 16..31. BRISC's normal
`device_setup()` also performs architecture-specific instruction-cache
invalidation and other setup. These are not covered by the tail-of-L1 data
test. A valid CPU-side configuration-register address alone does not establish
that a host NoC write to it is supported or safe.

Do not simply clear BRISC's reset bit after a successful data round-trip.
A separate execution transaction needs audited boot-word/code/mailbox
ownership, executable-image validation, instruction/cache behavior, correct
read/write NoC attributes, bounded completion polling, and verified core stop
**before** restoring any instructions or data. Failure to stop must prohibit
rewriting live code and require cold recovery. No such transaction is
implemented or deployed yet. No reset controls or instruction bytes were
written during this audit. The last hardware operation remains the successful
repeat L1/telemetry/profile/independent PCI verification above.

### Continued execution audit and offline image, October 14 local time

Pinned official ISA documentation at
`8a34e25e04d1ad7dd129ac9231e126b85923fac3` in a private no-checkout clone.
Its [Blackhole soft-reset contract](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/TensixTile/SoftReset.md)
specifies cache invalidation when leaving reset, BRISC entry at zero, and no
new instruction starts while reset is held. It does not promise immediate
abortion of every in-flight operation. Its
[NoC memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/TensixTile/BabyRISCV/README.md)
marks backend configuration inaccessible over NoC: do not attempt a host
write to the CPU-local cache-control register. Local RISC RAM is also
reinitialized on release unless configured otherwise. This is a documented
startup contract, not yet validation on this installed board/firmware.

Added an original, offline-only RV32I checksum/park image and linker bounds.
The sole reset word jumps to code at 0x70000 (under the prior startup bound).
It computes 1+...+32, writes 528 at 0x17ffc0, consumes a readback, fences,
publishes PION at 0x17ffc4, consumes that readback, fences and parks. It uses
no stack, local RAM, CSR, interrupt, NoC command, DMA or Tensix instruction.
The host test assembles/links the real RV32I image, independently decodes it,
allows only its fixed instruction subset, bounds every instruction fetch and
data access, checks both results and observes the terminal self-loop. The
boot section is exactly four bytes; code is at most 128 bytes. Host test
passes with ASan/UBSan; this models instruction/data bounds, not hardware
cache timing or reset quiescence.

No driver/ioctl references this image, no instructions have been installed,
and no reset bit has been written. Integration still requires initial-zero
completion checks, exact code/data preservation, fixed firmware/tile gates,
completion/stop verification, and a cold-recovery-only path on uncertain
stop. Never rewrite live instructions or treat a timeout as restored state.
The original kernel driver's operation set and installed binary are unchanged.

### October 14: first native P100A scalar-core execution

Implemented the separate explicit BRISC operation and a read-only loaded-
module metadata query. All earlier operation ABIs and eight CLI hashes stay
unchanged. The new policy uses dynamic bidirectional VC for both L1 and reset
access, starts only bit-11 BRISC on the already validated tile `(1,2)`, and
requires drained results plus two park-PC samples before stop verification
and code restoration. Original L1 bytes remain kernel-local. Unknown core
state or execution/stop uncertainty leaves the image in place and latches
cold-recovery-required; later operations are refused under the same lock.
Reloading a module is not cold recovery. Local BRISC RAM/register state may
reset; the L1/PCI/TLB restore flags do not promise arbitrary RISC context
restoration. No other RISC, DMA, firmware message or flash operation is used.

New optional module SHA:
`58ffcadcdc3e9ae0804b75ddda17ad55b06da97613a9d8b5b51a1b5b64107174`.
New explicit CLI SHA:
`d8c88121d0b9c194ba4b0c8f7ebdd1629e41e093e4584c85cfbdc7a6a8889c70`.
They are in `brisc-exec-1/`, with the exact previous trace driver retained as
`tenstorrent_pci_probe.before-brisc-1`. A first metadata check correctly saw
the old loaded module; a later check confirmed devfs had reloaded the new
one. All prior topology/telemetry/held-core/L1 checks passed before execution.

At about 21:01 local October 14 (04:01 UTC October 15), the first execution
PASSED: checksum 528, completion 50494f4e, both PC samples 00070048, reset
00047800 -> 00047000 -> 00047800. Started/completed/stopped and all three
L1/TLB/PCI restored flags were 1; card/root commands ended 0000/0000. The
repeat passed identically, using five polls versus one in the first run.
Independent health/core-state/profile checks and the host 64-CPU test passed
afterwards; no recovery or SD change was necessary. This proves bounded
scalar RISC code runs on the real P100A under Haiku. It is not Tensix matrix
compute, a GDDR memory audit, a native runtime port or accelerator LLM inference.

### October 14: first verified read-only GDDR route

Added a separate fixed bank-0/port-0 NIU identification transaction, not a
payload memory operation. Before issuing a NoC request, it reads the PCIe
NIU's translation configuration and proves translated `(18,12)` maps to
physical `(0,0)`. It requires the measured firmware `12050001`, GDDR mask
`f7`, status `5515`, zero reported uncorrected errors and the existing held-
core control result. Only the fixed source-configuration/target-ID registers
are readable through this new ioctl; no arbitrary addresses or input fields.

The initial live check restored all PCI/TLB state but refused its ID because
firmware 18.5 retains the physical/power-on `NOC_ID_LOGICAL` label at this
port, zero for `(0,0)`. This register is a writable software label, not the
routing lookup table. Policy v2 accepts either that observed physical label
or the prescribed translated label; the independent physical
`NOC_NODE_ID`, typed `NOC_ENDPOINT_ID`, source-table translation and repeated
sample checks remain required. No identity field or actual routing-table
check was removed. Host ASan/UBSan tests cover dropped configuration/restore
writes, wrong/unstable IDs, all-zero/all-one reads and source-table changes.

At 21:28 local October 14 (04:28 UTC October 15), loaded policy v2 and the
fixed route PASSED. Three samples were identical:
`NODE=1c611000 ENDPOINT=00080000 NIU=0000e005 LOGICAL=00000000`.
The source NIU was `6005` with unchanged translation words. TLB/PCI restore
flags were 1/1, commands ended 0000/0000, and cold-recovery flag was zero.
Only bank-0 identification was tested, not GDDR capacity or matrix compute.

Installed optional driver SHA:
`d999e495ca4fb73cb577f1558e3f5b515dd19c5700a283802ec455b1701b4bec`.
CLI SHA: `4f7b46c23bcbe5cb513ba25e755fe59a850a08cd7975c68f4c7f4ca0a963cbb4`.
Artifacts and the previous driver are retained under
`/boot/home/pioneer-p100a-20261014/dram-route-2/`; the earlier BRISC driver
also remains backed up under `dram-route-1/`. Metadata-only
`--check-fixed-dram-support` identifies loaded policy v2. Explicit
`--read-fixed-dram-route-and-restore` performs the bounded transaction.

Register facts were audited against official ISA documentation commit
`8a34e25e04d1ad7dd129ac9231e126b85923fac3`,
`BlackholeA0/NoC/{Coordinates,MemoryMap}.md` and
`BlackholeA0/PCIExpressTile/HostToDeviceTLBs.md`, together with the pinned
UMD coordinate manager. Preserve those sources' own license notices; this
policy is an independent implementation of register facts, not copied
documentation code. No DMA, firmware messages/flash or model inference ran.

### October 14: seven-bank GDDR payload samples passed twice

Added two separate operations: read-only identification of the seven enabled
bank/port-0 routes, and an explicit fixed reversible payload-sample test.
Firmware 18.5's live source tables map physical banks 0/1/2 to translated
`(18,12)/(18,15)/(18,18)` and banks 4/5/6/7 to
`(17,12)/(17,15)/(17,18)/(17,21)`. This differs from the newer pinned UMD's
mirror-bank ordering. The policy derives routes only within the eight
documented DRAM port-0 slots, proves each against the source tables, and
then independently verifies physical NODE_ID and typed bank ENDPOINT_ID.
Firmware retains physical labels on the west ports and translated labels
on the east ports; only the matching physical or matching translated label
is accepted, not arbitrary zero on every bank. The initial seven-bank v1
attempt safely refused bank 1's physical label, before any payload access.

The fixed payload footprint is five 64-byte samples per enabled bank at
local addresses `04000000`, `40000000`, `80000000`, `c0000000`, `fe000000`:
2,240 bytes total. All are above the pinned HAL's low reservations (including
its optional profiler reservation, 6,400,064 bytes total) and below
`MEM_DRAM_SIZE=ff000000`; the top 16 MiB is never used as payload. The
driver reserves the otherwise undecoded card, requires exact telemetry,
proves all 120 enabled Tensix tiles' five RISCs are held, and preserves the
original sample bytes kernel-locally. Each pattern is written to the whole
set before reading back, so aliased banks/high addresses cannot silently
pass independent write/read pairs. Before restoring bytes, it re-proves all
120 cores held and unchanged source tables. Unknown ownership or failed
restoration latches cold-recovery-required; cleanup does not blindly rewrite
payload. No arbitrary-address, firmware-message or DMA interface is exposed.

Host ASan/UBSan tests passed for all individual TLB-write drops, payload-write
drops, bank/address aliases, wrong identities, source changes, unknown core
ownership and failed payload/TLB restoration. Existing policy and fixed-image
tests passed; all ten pre-existing diagnostic executable hashes are unchanged.

At about 21:39 local October 14 (04:39 UTC October 15), the explicit sample
test PASSED twice: seven routes, 240 core checks (120 before and 120 before
restore), 35 saved/modified regions, two patterns, zero mismatches. Data/TLB/
PCI restored flags were 1/1/1, commands ended 0000/0000 and recovery flag was
zero. Subsequent telemetry reported firmware 18.5, GDDR f7/status5515,
uncorrected errors zero, ASIC 48.48 C, fan 1,882 RPM and advancing heartbeat.
Haiku still reported 64 CPUs and max memory 137,034,936,320 bytes.

Installed optional driver SHA:
`b3d9b69dca6a300d9a367e1fdbe8e47ed56d9fad5cbebc8c4326fc8905a19ee9`.
New CLI SHA:
`0035dffe29337164e54b27a75c90527addb02c889377c68ceb51fd5ea1c5e042`.
Artifacts and prior-driver backup are in
`/boot/home/pioneer-p100a-20261014/dram-samples-2/`.
`--check-fixed-dram-samples-support` must identify loaded policy v2;
`--read-enabled-dram-routes-and-restore` is payload-read/write-free;
`--test-fixed-dram-samples-and-restore` performs the explicit sample test.

This establishes BAR4-free reversible host access to distinct high-address
samples on all seven GDDR banks. It does NOT test every byte of nominal
28 GiB, provide a runtime allocator/DMA transport, execute Tensix matrix
operations or run an LLM on the accelerator. Kernel and boot defaults and
card firmware are unchanged; no SD change or reboot was needed.

### October 14: first fixed Tensix matrix trial timed out; cold recovery

Added an original 572-byte RV32I TRISC0 fixture and fixed optional execution
transaction. It uses the sole idle tile `(1,2)`, a reset jump at `6000`, code
at `70000`, two 512-byte result arrays at `17f800`, and three status words at
`17ffc0`. The 403-word L1 footprint is saved kernel-locally. The scalar RISC
initializes fixed inputs but does no matrix arithmetic: actual `MOVD2A/B`,
supported `UNPACR_NOP` handoffs and `MVMUL` instructions are queued. Two
8x16-by-16x16 constant fixtures should yield 128 FP32 values of 16.0 and
128 of 96.0. Manual TTSync waits for instruction completion before reading
the destination. No unsupported SETDVALID, NoC command, DMA or ARC message.

This explicitly reinitializes one unused tile's coprocessor context using
documented soft reset; it does not preserve prior Src/Dst or RISC local RAM.
It first proves all 120 enabled compute tiles held, checks source translation
and entry/FIFO/sync controls, bounds completion polling, and requires the
drained completion marker, two park-PC samples, known stop/reset state and
L1 restoration. Uncertain execution leaves image bytes intact and latches
cold recovery; cleanup never rewrites potentially live code.

Offline assembly/link/bounds and independent RV32I/MMIO decoding tests passed
under ASan/UBSan: 5,292 scalar steps, 11 manual drains, two reference matrix
results; disabling modeled matrix execution fails the expected-output check.
This is a software functional model, not a model of hardware timing. Policy
tests cover individual image/TLB/reset-write drops, timeout, incorrect result,
PC, source changes and failed restoration. The RISC-V driver/tools build
passed; all 11 prior CLI hashes remain unchanged. The kernel driver stack
frames inspected were bounded (ioctl 1,808 bytes; compute policy 1,760 bytes).

At 21:58 local October 14 (04:58 UTC October 15), the first live matrix trial
TIMED OUT after 2,000 polls: started/completed/stopped 1/0/1, phase 1,
fixtures 0, marker 0. Reset readbacks were `47800 -> 87ffff -> 46800 ->
47800`, but completion was not confirmed; no matrix result is claimed.
The L1 image was deliberately not restored. TLB restored 1, outer PCI state
5/restored 0, commands 0000/0000 and cold-recovery flag 1. No further card
operation or driver reload was attempted after the latch. Haiku remained
responsive with 64 CPUs and max memory 137,034,936,320 bytes. Requested
ordinary `sync; shutdown -s` over pinned SSH, then a relay cold cycle for
recovery. A shutdown request is not proof of a clean filesystem shutdown.

First-trial installed driver SHA:
`693b2a61d7772682ed12bc7983d33c78a0924572a83272fd8cd209768fc29b66`.
CLI SHA: `1898e05062a97bf91ede1d2968b272732091bb9071c865e66e7c2434f5b34fcf`.
Artifacts and the passing GDDR module backup are retained in
`/boot/home/pioneer-p100a-20261014/matmul-1/`. Metadata-only
`--check-fixed-matmul-support` identifies policy v1. Execution is only through
explicit `--run-fixed-tensix-matmul-and-reset`. The ordinary boot and firmware
are unchanged. Next: record timeout PC snapshots/checkpoints, locate the
Tensix startup/completion stall, and cold-recover before another attempt.

The first trial was followed by an ordinary shutdown request, a relay-off
hold, more than 90 seconds off, and a fresh ZSBL/Linux boot. Linux verified
64 online CPUs, an untainted kernel and disabled P100A PCI decoding. The
next preparation found a clean BFS journal; the dirty-journal profile
correctly refused it. The equivalent pinned clean-filesystem profile
`memory128-spin-idle6-nosd-control` then passed its input checks and prepared
the same private full-RAM kernel without changing the default Linux boot.

The revised candidate records each manual completion wait's return PC in
L1 and samples the TRISC0 PC periodically, including on timeout. The
official ISA's `Bits32_UNPACR_NOP_SETDVALID.svg` places mode 7 in bits 0..2,
not bits 8..10: corrected handoffs are `43000007` / `43800007`. The initial
candidate's `43000700` / `43800700` encodings were incorrect. This source
correction alone does not establish where that initial execution stalled.
The candidate image is now 588 bytes / 147 code words, with a 407-word
preserved footprint and metadata policy v3. The offline test passes 5,336
scalar steps and 11 manual drains; this remains a functional model, not
hardware timing proof. The rebuilt driver SHA is
`e90f98933706b2ec8297391bcc0bf0cd80e8b337577efaf0c91d46ccda284ab2`;
CLI SHA `400f34aecb42d56445c9e4ed90be95b5bcaf31851b31204639d93e482ccf5bc7`.
Neither successful matrix execution nor model inference is established by
these build/source checks.

At 22:12 local October 14 (05:12 UTC October 15), the revised live trial
reached a frontend PC snapshot `70110` (the input-fill loop), then subsequent
PC, L1 status and reset reads returned zero. It refused a stop write from
that unrecognized reset state: state 7, started/completed/stopped 1/0/0,
polls 2, no completed fixture. L1/TLB/PCI restored 0/1/0, commands 0000/0000,
cold recovery 1. The initial PC snapshot is speculative, not proof of
retirement or completion. Haiku remained responsive with all 64 CPUs/full
RAM. No further card operation or module reload followed; an ordinary
shutdown request and a relay-off hold began another cold recovery. The
unexpected readbacks are unresolved; no matrix result is claimed.

### October 14: scalar controls isolate the compute startup failure

Following another shutdown request and relay cold recovery, unchanged NVMe
Linux verified 64 CPUs, taint 0, kexec slot empty and P100A command `0000`.
The pinned clean-filesystem full-RAM Haiku profile then booted again.
The historical BRISC checksum control passed (528 / `50494f4e`, matching
park PCs `70048`, started/completed/stopped 1/1/1, L1/TLB/PCI restored 1/1/1,
cold recovery 0). This establishes a working scalar execution/transport
control after recovery; it does not vindicate the failed matrix image.

Added a separate scalar-only TRISC0 control. Its fixed image has the same
19 checksum instructions, but the jump at TRISC0's reset address `6000`
targets `70000`. Only the TRISC0 reset bit changes (`47800 -> 46800 ->
47800`); no coprocessor instruction, direct-Dst access, NoC command, DMA or
ARC message is issued. A redirected TRISC0 reset entry is rejected. The
loaded metadata is v2; new explicit operations are `5461` / `5462`.
The original BRISC operations and results layout are unchanged. Both reset
jumps are independently assembled/decoded and the shared scalar instruction
body is functionally checked offline. Fault tests exercise both core modes.

At 22:24 local October 14 (05:24 UTC October 15), the TRISC0 scalar control
PASSED on the real card under Haiku: checksum 528, completion `50494f4e`,
both PC snapshots `70048`, started/completed/stopped 1/1/1, one poll,
L1/TLB/PCI restored 1/1/1, commands 0000/0000, cold recovery 0.
Driver SHA `2f201f915a5cdbedcb5c8d7f153a352663ced6281ec9a2291fd3967ab046d2c8`;
CLI SHA `2d8a8d1b373baa1c7aedefb13f9d35dc5a0c8f9ba8d0178efa99d4eff500d0d3`.
Native backups/artifacts are under `/boot/home/pioneer-p100a-20261014/trisc-1/`.

The next independent control uses that identical scalar image, but explicitly
reinitializes the sole unused tile's backends/Auto-TTSync with `87ffff`
before releasing TRISC0, and repeats the broad reset before stopping.
This is a different explicit operation (`5463` / metadata `5464`, v3), not
a silent widening of the scalar-only operation. It does not preserve prior
Src/Dst/register context and still executes no coprocessor instruction.
The versioned CLI flags are `--check-reset-trisc-support` and
`--run-reset-trisc-checksum-and-restore`. The original scalar-only flags
remain available. Uncertain reset/execution still latches cold recovery.
Driver SHA `6469b2c4df1dbe09decfac7400fe76edd2e21f822eed7b1cdbebda135a6b7f65`;
CLI SHA `32bcc4c9348a230ebb4dc07d6858d50863c09457c929fcf28fe4773764bbd16a`.

At 22:28 local October 14 (05:28 UTC October 15), that broad-reset scalar
control also PASSED: started/completed/stopped 1/1/1, three polls, checksum
528, marker `50494f4e`, both PC snapshots `70048`, restoration 1/1/1,
commands 0000/0000 and cold recovery 0. Thus both scalar core launch and
the broad reset sequence have successful live controls. This narrows the
unresolved matrix failure to the additional configuration/coprocessor/Dst
path; it is not evidence that any matrix instruction has completed.

The configuration-only control uses two independently assembled, decoded
same-width substitutions in the fixed matrix image: jump from `700d4` to
`70200`, then publish protocol count 2 instead of looping over fixtures.
This executes SETC16 configuration, backend configuration writes/readbacks,
SETRWC and four manual completion drains, but no direct-Dst access,
MOVD2A/B, handoff or MVMUL. Its output footprint must remain zero; those
zeros are explicitly NOT matrix results. New explicit operations are
`5465` / metadata `5466` (v4), with result version 2. The matrix operation
is not changed into a configuration-only operation. CLI flags are
`--check-fixed-tensix-config-support` and `--run-fixed-tensix-config-and-reset`.

At 22:31 local October 14 (05:31 UTC October 15), configuration-only
PASSED on silicon: started/completed/stopped 1/1/1, two polls, final wait
return PC `7020c`, both park PCs `70224`, marker `4d415431`, protocol count
2, untouched-zero output, L1/TLB/PCI restored 1/1/1, commands 0000/0000,
cold recovery 0. The native artifacts/backups are in
`/boot/home/pioneer-p100a-20261014/config-1/`.
Driver SHA `bf3f740571e8de515992cd3ba36b461d7a8c0993bfe9ba021e3d14ae59e5e447`;
CLI SHA `eb7c1b6839878f7a5c31fea1bc936d61814699d82a51516a967831acfe210e21`.
This narrows the failing matrix path further; the RISC-to-Dst input access,
operand moves/handoffs and matrix execution remain unverified.

Also source-audited the official minimal `tenstorrent/tt-lab` runtime at
`e6baf562a72491ed9d594cddbdf42e0925b1f79e`. Its documented one-chip model
path is useful reference, not a native Haiku runtime: it assumes Linux/x86,
exclusive KMD ownership, mandatory pinned host DMA, and BAR4 DRAM windows.
Its source is retained in an ignored no-checkout audit clone; no upstream
build, firmware-loader or installation script was executed.

### October 14: SFPU-input alternative still loses target readbacks

Replaced direct RISC-to-Dst input stores with SFPLOADI/SFPSTORE fills: all
24 input rows are written on the vector unit, with an explicit completion
drain before output ZEROACC or MOVD2A/B. Explicitly clears the SFPU stack
address base as well. The 640-byte image has 160 code words, 420 preserved
words, park PC `70258` and final-wait return PC `70240`. Matrix/config-only
metadata versions are now 5/6, so old metadata checks refuse the changed
image. The independent decoder verifies all assembled bytes, both nonzero
matrix reference fixtures, 768 SFPU input writes, 256 actual output reads,
13 drains and NO direct RISC input store. This functional test takes 2,401
scalar steps; it still does not model hardware timing. Fault/rollback and
seven-bank GDDR policy regressions pass.

At 22:39 local October 14 (05:39 UTC October 15), the new config-only control
PASSED on silicon, all restorations 1/1/1 and recovery 0. After matching
matrix metadata, one SFPU-input matrix trial failed with the same early
unexpected zero reset/PC/L1 readbacks: state 7, polls 2,
started/completed/stopped 1/0/0, no fixture/marker, L1/TLB/PCI restored
0/1/0, commands 0000/0000 and cold recovery 1. No subsequent card operation
or module reload was attempted. Haiku was still responsive with all 64 CPUs
and full RAM; ordinary shutdown was requested and a relay cold recovery
started. Bypassing direct input stores did NOT resolve the failure.

Native artifacts/backups: `/boot/home/pioneer-p100a-20261014/sfpu-matmul-1/`.
Driver SHA `d9b1586a8609b7388ac84b41e2bf3c3bb987695df17f3cdc75c9c9f726d869d9`;
CLI SHA `248bb9c1d04b723411e30e2aa30f4de9b742f8181d90b1a45345a6f8acd7664c`.
Next audit: the normal ARC power/clock initialization used before compute,
against the installed 18.5.0 firmware, not speculative firmware flashing.

### October 14: read-only operating-state snapshot

Audited official firmware tag `v18.5.0`, commit
`80b056af360a58730756467d5cc8df8db789640d`, matching the reported bundle
family. Its normal `AICLK_GO_BUSY` handler requests the maximum through
the firmware DVFS arbiter; it is not a forced-voltage/frequency command.
The default ASIC state is already A0. No state/clock/flash command was sent
on the basis of this source audit.

Added a separate bounded read-only operating telemetry selector at `5467`,
result version 2. The legacy health selector and result layout are unchanged.
Its fixed whitelist reads clocks, voltage, power limits/input, thermal trip
count and firmware/heartbeat; no serial or ASIC identifiers. Bounds, table
validation and TLB rollback use the existing tested telemetry policy.
CLI flag: `--read-operating-telemetry-and-restore`.

After the latest cold recovery, the clean-filesystem full-RAM Haiku profile
booted again with 64 CPUs and max memory 137,034,936,320 bytes. At 22:47
local October 14 (05:47 UTC October 15), the operating snapshot PASSED:
AI clock 800 MHz, AXI 960 MHz, ARC 800 MHz, Vcore 742 mV, board power limit
300 W, input power 89 W, thermal trip count 0, bundle `12050001`,
CM `000e0000`, heartbeat `12b0`. AICLK maximum is not published by this
firmware. ARC/TLB/PCI restoration all passed and commands returned 0000/0000.
These readings make a normal busy-clock test worth investigating; they do
not establish missing clock activation as the cause of the compute failure.

Artifacts/backups: `/boot/home/pioneer-p100a-20261014/operating-1/`.
Driver SHA `d66b82ed9657cef9a0b1cd8eee50496f057916281d02b46d90921e15cdccf1f7`;
CLI SHA `0090f6c27a4781e44dcaa76d098ff92c78f891b23caeeb95e9f604e5c4e22dad`.
Next: a fixed ARC no-op round trip with queue ownership/range/timeout guards,
then assess normal runtime clock activation. No firmware image is modified.

### October 14: first verified ARC request/reply under Haiku

Implemented a fixed `GET_AICLK` (`34`) request with zero arguments, rather
than assuming a declared NOP has a handler. The matching 18.5.0 firmware
defines the clock query, its nonzero clock/mode reply, four queues of four
32-byte entries, double-wrapped pointers and an eight-word header. This
implementation uses application queue 3 only. It verifies an idle, stable
queue twice, non-overlapping descriptor/telemetry/queue regions, the inbound
MSI catcher's readiness before publication, all staged request words, and
the exact pointer/counter advancement before acknowledging the response.
It never rewinds queue counters or writes firmware-owned response bytes.
Saved host-owned request bytes stay kernel-local and are restored only after
proven idle ownership. Unknown publication/completion/acknowledgement latches
cold recovery; notification is an inbound ARC interrupt, not host MSI or DMA.

The initial 128 KiB guard correctly refused the live queue descriptor
`10029ea8/00000404/0/0` before any request write. PCI/TLB restoration passed,
recovery 0. The pinned firmware DTS explicitly documents 512 KiB of CSM at
`10000000`; the queue-only limit was therefore increased to the first
192 KiB, just enough for this queue, while legacy telemetry retains its
128 KiB bound. Tests now use the observed live placement and cover exact
upper bounds, all 64 request/response pointer combinations, counter wrap,
each dropped request/pointer/TLB write, malformed/overlapping descriptors,
bad response, timeout, changed ownership and failed restoration.

New explicit operations: `5468` clock query, `5469` metadata v2.
CLI: `--check-fixed-arc-query-support`, then
`--query-fixed-arc-clock-and-restore`. There is no arbitrary message API,
clock/voltage/reset command, host DMA, firmware flash or RISC start here.

At 23:00 local October 14 (06:00 UTC October 15), the clock query PASSED on
the P100A under Haiku: submitted/replied/acknowledged 1/1/1, three polls,
status 0, 800 MHz, mode 3 (normal/unforced DVFS), MSI-catcher status 1.
Application queue is `1002a208`; request/response pointers advanced 0/0 to
1/1 and the sequence counter 0 to 1. Request/TLB/PCI restored 1/1/1,
commands 0000/0000, cold recovery 0. This proves the firmware transport,
not matrix execution or model inference. No clock-changing message was sent.

Artifacts/backups: `/boot/home/pioneer-p100a-20261014/arc-query-2/`.
Driver SHA `dc84a37a2d7c0e3512b48ec38af22f843e7a4053d07590440f46929e87e7cc13`;
CLI SHA `2580224de840261ac7027c788d56303f83b4af8fcaabb2615b50a95798dd9313`.

### October 14: normal DVFS activation passed; clocked matrix still failed

The matching system firmware v18.5.0, commit
`80b056af360a58730756467d5cc8df8db789640d`, implements normal busy (`52`)
and idle (`54`) requests through its voltage-before-frequency DVFS arbiter.
Added a fixed clock guard, not arbitrary messages or forced clock/voltage.
It requires measured idle 800 MHz, normal mode 3 and ASIC temperature at
most 70 C, verifies busy above 800 and at most 1400 MHz, and verifies return
to the original idle clock. Unknown queue state prevents further commands.
Host tests cover all individual dropped writes, failed clock settling,
idle-restoration failure and no commands after uncertain transport/compute.

At 06:08 UTC October 15 (23:08 local October 14), the real P100A passed:
800 -> 1350 -> 800 MHz, mode 3 throughout, five messages, busy/idle replies
acknowledged, clock/request/TLB/PCI restored, cold recovery 0. Operations
`546a`/`546b` expose this one fixed cycle and its metadata. Artifacts:
`/boot/home/pioneer-p100a-20261014/arc-clock-1/`; driver SHA
`ff116ac09be48073f4e7df6d3aeca8257fecd3a5525dbd88cd6d2198fe7388cd`,
CLI SHA `c3aab27eacfec8f173812fdc8b4bc363cf8c26010aff0c4168afbb3595dd81b2`.

Added guarded matrix operation `546c`, metadata `546d` v7, wrapping the
unchanged 160-word SFPU-input fixture. A first invocation safely refused
before routes, clock messages or core start; every restoration passed.
The diagnostic invocation then passed the full baseline (seven routes,
120 held cores, expected firmware/harvesting/training/error masks), entered
1350 MHz normal mode, started TRISC0, but again lost trustworthy target
reset/PC/status readings on poll 2. Matrix state 7, complete/stop 0/0,
no output or completion marker. This **does not establish a clock fix**.
No idle request was issued after compute uncertainty; cold recovery was
latched. Three ARC messages preceded compute, queue bytes/TLB restored,
math L1/context not restored, aggregate PCI state 5, command words 0000/0000.
Requested normal Haiku shutdown, then relay power-off and cold recovery.

Artifacts: `/boot/home/pioneer-p100a-20261014/clocked-matmul-1/`;
driver SHA `006f5fa75f81a38ca815dcf1a8605681dae487ac978324a890e8748b736cc577`,
diagnostic CLI v2 SHA
`db1cc8c5cb53ac6b03b5031c279db62031d0725cba563bb2b1c6bd2f578a14e9`.
Neither matrix execution nor any model inference has passed. Next isolate
the first SFPU/data-path operations, retaining all cold-recovery gates.

### October 14: vector execution passes; operand preparation isolates failure

Added separately assembled fixed-prefix controls with unchanged 160-word
code/420-word L1 footprint and common drain/park/restore protocol. New
operations `546e`/`546f` (metadata v8) run two SFPLOADI instructions only;
`5470`/`5471` (v9) additionally execute the twelve SFPSTORE instructions
covering 24 rows of Dst, but no ZEROACC/MOVD2/matrix/RISC Dst reads.
The assembler-byte comparison, constrained functional decoder and kernel
policy/fault tests pass. These controls require their zero output footprint
to remain unchanged; count 2 is a protocol marker, not two matrix results.
An offline completion-marker-before-drain mistake was caught and fixed
before deployment. Neither test numerically reads back Dst on hardware.

At 06:24 UTC October 15, SFPU-load-only PASSED on P100A: started/completed/
stopped 1/1/1, two polls, expected final phase/marker and two parked PCs,
L1/TLB/PCI restored 1/1/1, recovery 0. Artifacts
`/boot/home/pioneer-p100a-20261014/sfpu-load-1/`; driver SHA
`658be222d062267a8106725d40cd2f3049de5403f160e76a631af31dd4d878aa`,
CLI SHA `26f0434df9355d73adfe293aa2297e57e7a70b4fc13641e1e507a6d5005a5461`.

At 06:27 UTC, SFPU-store-only PASSED with the same complete/stop/restore
checks, one poll, recovery 0. Artifacts
`/boot/home/pioneer-p100a-20261014/sfpu-store-1/`; driver SHA
`32a2fbda464205a83c3b690e0d8c77df2de925bb0b80dab9b8209a8ce5323f37`,
CLI SHA `109002f28743e7d2c045637f879d7e97759d5de54924eba6a38092f7a1d3829f`.

Added operand-only `5472`/`5473` (v10): same SFPU prefix, ZEROACC,
MOVD2A/B and UNPACR_NOP handoff, then drain/park without matrix or RISC
Dst reads. Also added matrix-exec-only `5474`/`5475` (v11), which issues
one MVMUL/drain but skips numerical readback. At 06:31 UTC, operand-only
FAILED: start/complete/stop 1/0/0, poll 2 target reset/PC/status all zero,
state 7, L1 not restored, TLB restored, aggregate PCI state 5, recovery 1.
Matrix-exec-only was therefore NOT run. Card/root commands remained 0000.
Requested normal shutdown and relay power-off; no further card commands.

Artifacts `/boot/home/pioneer-p100a-20261014/operand-1/`; driver SHA
`bd630ce8d779938af3b185d8e98e7fc9a5153b5bdfb4f57409cd1b71d026d237`,
CLI SHA `9073be2f688cd2663fd5a0a2f1ff9031a4afbf9854b0e52a8a83d810127e1747`.
The failure is now narrowed to ZEROACC, MOVD2A/B or operand handoff, not
necessarily MVMUL or RISC Dst reads. No numerical matrix or model success.

### October 14: ZEROACC/MOVD pass; handoff remains non-repeatable

After another default Linux cold recovery and clean full-memory/64-CPU
Haiku boot, repeated SFPU-store control passed. New fixed ZEROACC-only
(`5476`/`5477`, metadata v12) and MOVD-only (`5478`/`5479`, v13) controls
both PASSED at 06:38 UTC October 15: completion/stop 1/1, three/two polls,
expected phase/marker/park, L1/TLB/PCI restored, recovery 0. Both bypass
UNPACR_NOP handoff, MVMUL and RISC Dst reads. This narrows the failing
prefix to the handoff rather than SFPU, ZEROACC or MOVD2A/B individually.
Artifacts `/boot/home/pioneer-p100a-20261014/zeroacc-1/`; driver SHA
`ac0d9b7bcde0133cc985f7d93a6fe272578c505574bc6caa13a4746e4ab6967c`,
CLI SHA `e1f13d2dc3c74313416f20836897c729e0fd4d285448659d2d997dbc99ebba89`.

The ISA's pure-handoff `43000007`/`43800007` differs from the pinned
LLK's actually used dummy-valid helper: `SET_DVALID=1`, `UNP_ZEROSRC=1`
through its macro yields `43000101`/`43800101`. Added a separately selected
kernel-style variant using that clear-and-valid sequence BEFORE MOVD2
(otherwise the clear would erase inputs). It executes once, retaining
Matrix ownership through the second fixture. It is not silently substituted
into the existing diagnostic variants. All images retain their fixed
footprint and are independently assembled/decoded, including a numerical
model that fails if matrix execution is removed. New metadata v14 operand
(`547a/b`), v15 matrix-execution-only (`547c/d`), v16 full numeric (`547e/f`).

At 06:43 UTC, kernel-operand-only PASSED once: one poll, complete/stop and
all restoration checks passed, recovery 0. The immediately following
kernel-matrix-exec-only test TIMED OUT, 2000 polls, phase `70190`, two PCs
`70274`, still-running reset `46800`. That phase is the handoff drain,
BEFORE reaching MVMUL. Stop/reset succeeded but L1 was deliberately not
restored after uncertain completion; cold recovery 1. No numeric full
matrix test was run. Thus this is **not a proven repeatable handoff fix**.
Requested normal shutdown and relay cold recovery. Next test the numerical
variant first from a fresh cold state, without a preceding dummy-valid test.

Artifacts `/boot/home/pioneer-p100a-20261014/kernel-handoff-1/`; driver SHA
`716ca2cc741892ab2a0584d69d67508b57859868b0278aa6549e2e684aea752c`,
CLI SHA `6d1ecbff645d8a0c8293acd2263fae80e53dc285dce86859496d6e81d9b8c23b`.
No firmware, clock-gating, voltage-force or default-boot changes were made.

### October 14: matrix completes with wrong values; Dst numerical control passes

Following a timed (>60 s) relay off interval, default Linux cold boot and
clean 64-CPU/full-memory Haiku handoff, the v16 kernel-style numerical
matrix fixture ran FIRST at 06:50:45 UTC October 15. Both fixture markers,
drains, parked PCs and stop completed. L1/TLB/PCI restoration passed,
recovery 0, but all 256 numerical outputs mismatched: representative bits
`72741ef6/72741ef6`, not `41800000/42c00000`. Matrix state 6, aggregate
PCI state 2 (bad result, not failed restoration). This is execution progress,
NOT numerical matrix success. Same kernel-handoff-1 artifacts as above.

Added fixed SFPU-fill/RISC-Dst-read control `5480`/`5481`, metadata v17,
result v12. It bypasses ZEROACC, MOVD2, handoff and MVMUL, reading known
input rows 0..7 after SFPU fills (1.0 then 2.0). Independent assembly and
decoder agree; removing SFPU writes breaks the model's numerical check.
At 06:56 UTC this control PASSED on P100A: 256/256 output values correct,
representative bits `3f800000/40000000`, start/complete/stop 1/1/1, four
polls, L1/TLB/PCI restored, recovery 0. This numerically establishes the
SFPU-fill and RISC-Dst-read path, not matrix operands or arithmetic.

Artifacts `/boot/home/pioneer-p100a-20261014/dst-read-1/`; driver SHA
`9d1022bbe7f559537eb9fbd3150857034eb080953b68fc4d35ae3ebd37ccbb94`,
CLI SHA `b024b9ecb57c5344ff6b08420f903236a8716f81146e9063e4ce065ccf0467ae`.
Next numerically round-trip SrcA/SrcB separately to isolate MOVD2 formatting
from MVMUL. The pinned LLK has a Blackhole MOVD2B/B2D FP32 workaround
(issue 449); the generic ISA functional descriptions are not timing/silicon
proof. No speculative forced clock/voltage or firmware change is justified.

### October 15: source-register numerical controls and pure-valid alternative

Added SrcA/SrcB round-trip controls (`5482/3`, metadata v18; `5484/5`, v19),
using MOVA2D of eight rows / MOVB2D broadcast of one row, never MVMUL.
Both use the kernel-style clear-and-valid handoff first. At 07:03 UTC the
SrcA test timed out at phase `70190`, PCs `70274`, before the source check.
Stop succeeded, but completion was uncertain, so L1 was not restored and
cold recovery was latched. SrcB was NOT run. No numerical inference about
SrcA can be drawn from a timeout before reaching it. Normal shutdown and
relay power-off requested/completed; all card commands remained 0000.
Artifacts `/boot/home/pioneer-p100a-20261014/src-roundtrip-1/`; driver SHA
`b3c54d468a475046d4c4a46952b6ef193295f651dea5be9fe0f7eb5d04f18e73`,
CLI SHA `d5b8e3c197a1abfbf91057a0695510eb295e384b7f648626268d0082bc846362`.

The pinned LLK Blackhole `instructions/assembly.yaml` documents a separate
pure-valid form: Clr_to1_fmt_Ctrl bits 7:6 = 3, with UNP_NOP bits 1:0 = 2.
This yields `430000c2/438000c2`, distinct from both the ISA's low-mode-7
form and the clearing `43000101/43800101` helper. Added explicitly separate
fixed variants: SrcA `5486/7` metadata v20, SrcB `5488/9` v21, full matrix
`548a/b` v22. They change only these two handoff payloads in the kernel-style
prefix, with no register/overlay-write modes, clearing, DMA or ARC command.
All 17 assembled code variants and the reset-jump section match the
independently decoded constants and policy tests. This is a prepared
experiment, NOT yet a hardware result.

At 07:12 UTC October 15, the fresh-cold-boot pure-SrcA test TIMED OUT:
phase `701e8` (after MOVA2D), PCs `70274`, 2000 polls, running reset
`46800`, stop succeeded. Thus the NOP/format-3 form got past its handoff
drain but did not provide a readable Matrix-owned SrcA bank on this setup.
No SrcB or MVMUL variant was run. Cold recovery was latched, L1 deliberately
left untouched, normal shutdown and relay power-off completed. Artifacts
`/boot/home/pioneer-p100a-20261014/pure-handoff-1/`; driver SHA
`32de6fbb87ca6e751d1d360b7587d668ccb10b2ef6320ed665d32c56a232affe`,
CLI SHA `29f8a6b8dd3010c6826878e5eba28cac5df7f30a2b40eff4fb8eb332642c52ec`.
Next investigate the standard bounded L1-to-SrcA/B UNPACR path rather than
treating another guessed standalone handoff encoding as a fix.

### October 15: first repeated numerical matrix success via normal L1 unpack

Added a separate fixed TRISC0 program using normal uncompressed BF16
L1-to-SrcA/B UNPACR loads, FP32 arithmetic and RISC Dst readback. It issues
no device NoC requests, host DMA, ARC commands, packer or firmware updates.
The sole idle tile's compute context is deliberately reinitialized, not
preserved. Original L1 words remain kernel-private and are restored only
after proven completion/stop. Legacy images retain their original layout.

New image: 648 words covering code/private inputs, 908 total preserved
words (3,632 bytes), park `703bc`, final wait return `703a4`. Code ends at
`70404`; A header/payload occupies `70700..7090f`, B `70910..70a1f`, all
inside the existing 4 KiB mapped code page. No instructions execute from
input buffers. The snapshot now uses a bounded kernel heap allocation;
null allocation refuses before writes, avoiding kernel-stack growth.
Completion blocks ThCon until CFGEXU, both unpackers, FPU and SFPU are
idle, then polls a GPR63 SETDMAREG marker. No PCBuf completion read is used.

Configuration-only v26 passed first. Initial source tests completed and
restored but matched only one SrcA element / one broadcast SrcB column.
Our SETADCXY mask `f` cleared X1, the input end counter, after establishing
255/127 bounds. Y-only mask `a` preserves those bounds, as the runtime does.
The decoder now tracks both X channels and rejects clobbered bounds.
At 07:38 UTC all 256 SrcA and all 256 SrcB values then PASSED.

Matrix returned 19 and 115, consistent with accumulating onto the prior
SrcB test's 3, then onto 19. The one-row ZEROACC sequence did not invalidate
this FP32 block on the live card. Switching this L1-only path to the
runtime's all-Dst clear fixes it. A regression preloads Dst with 3 and
reproduces 19/115 if the clear is removed. The precise silicon cause of
the former one-row clear behavior is not established.

At **07:42:36 UTC October 15**, actual MVMUL PASSED all 256 outputs:
`41800000/42c00000` = 16.0/96.0, complete/stop 1/1, one poll, zero
mismatches, L1/TLB/PCI restored, recovery 0. An immediate WARM repeat also
PASSED, three polls, no driver reload or cold recovery. MVMUL releases/flips
both operand banks; source round-trip controls release them after readback.
Haiku still reports 64 CPUs and 137,034,936,320 bytes maximum RAM (127.62
GiB usable). Subsequent operating telemetry passed: idle 800 MHz, 742 mV,
input 89 W, thermal trips 0, advancing heartbeat, command words 0000/0000.

Passing operations: `548c/d` matrix/metadata v31, `548e/f` SrcA v32,
`5490/1` SrcB v33, `5492/3` configuration-only v34. CLI flags start
`--check-l1-unpack-...-support` / `--run-l1-unpack-...-and-reset`.
Artifacts `/boot/home/pioneer-p100a-20261014/l1-unpack-3/`; driver SHA
`e4a4f2d212f2de7b06043b9ed5da85c7c708933e85d080f93f0407076fb38a59`,
CLI SHA `875b4c43db93d4cc5ff2c53e3859d3ec2f4f3cc68b3f49fef8d9cd6ec223abd1`.
Earlier underfilled v23..26 artifacts: `l1-unpack-1/`, driver
`70974fb9f229fd181c2cc1a200f674dc9076b7c66212af65cd43de4b080846e2`, CLI
`e9627ecc3f1dfcdd47ec0180cf9229935724ec5d10c43b1644b96cd39cf3b547`.
Corrected-bounds/stale-accumulator v27..30: `l1-unpack-2/`, driver
`e1a4bf366587ddfe0e68229cb9b1340771e06917f99ddcaa4690da2e8e97b4ec`, CLI
`e5dd619aa86cba47cc8e1880b90561b397936bd4cf7c75ed5dfea5ad7a4ec4ef`.

All 21 code variants are independently assembled/decoded. Tests cover
rollback, recovery gates, null allocation, ADC bounds and stale Dst; removing
actual unpacking or MVMUL breaks the numerical reference. This does NOT yet
prove nonuniform layouts, a scalable runtime or model inference. Next test
nonuniform data before model-backend integration. Default Linux boot,
firmware and both SDs remain unchanged; no BAR4 enlargement/bus mastering.

### October 15: nonuniform, mixed-sign layouts verified

At **07:59 UTC**, the new SrcA dense readback, SrcB patterned broadcast,
identity multiplication and nonsymmetric dense multiplication all PASSED
on the live P100A. Each operation checked all 256 FP32 outputs against an
integer CPU oracle. The dense multiplication also passed an immediate
warm repeat. All runs completed/stopped, restored L1/TLB/PCI, and reported
recovery 0 and commands 0000/0000. Haiku retained all 64 CPUs and
137,034,936,320 maximum RAM bytes.

Input B rows contain distinct values in [-8,7], with a second independent
fixture `-B-1`; dense A is `(row+2*column)%5-2`. A separate identity A
establishes orientation. The independently assembled RV32 program only
copies bounded BF16 input tables; actual MVMUL generates the outputs.
The decoder independently calculates the reference and rejects a transposed
A layout or removed MVMUL. It also verifies all 25 program variants,
read/write whitelists and existing rollback/recovery gates.

New fixed operations 5494/5 identity v35, 5496/7 dense v36, 5498/9 SrcA v37,
549a/b SrcB v38 preserve the same 908-word footprint and completion PCs.
Native artifacts: `/boot/home/pioneer-p100a-20261014/nonuniform-1/`.
Driver SHA `8bfc79537c1f07be7ceaf388183589ec20547e838e8a4d570e391e77a25bf32e`;
CLI SHA `641cf396986aa3d573e32ff4f58acf278d16c540b9397fabbc5799d076bcaa1b`.
This proves these bounded numerical layouts, NOT model inference or a
scalable backend. Next add a bounded data-only input/output interface;
no user-controlled device addresses, code or DMA descriptors.

### October 15: bounded data-only BF16 interface and model bootstrap

Added root-only ioctl `549c` with metadata `549d` v39. Request is exactly
two 16x16 BF16 arrays (`A[k][column]`, `B[row][k]`); output is 16x16 FP32
`B @ A`. No caller-controlled addresses, code, dimensions or descriptors.
NaN/Inf and nonzero subnormals are rejected before hardware access.
The driver snapshots inputs on a bounded heap allocation, zeros results,
and never reads userspace during execution. Result state **10 EXECUTED**
is deliberately distinct from a numerical fixture PASS. Output is erased
if completion, stop or L1/TLB/PCI restoration fails. Cold-recovery gates
remain in force. Context on the single idle tile is reinitialized.

The separately assembled program adds four fidelity phases at `70480..704c3`,
retaining both source banks until the last MVMUL. Thread register 11 selects
the phase; each change and multiplication is explicitly drained. Code/data
and snapshot footprint remain 908 words. The decoder compares the complete
assembled image, emulates all four phase contributions, and verifies that
omitting low phases fails. Parameterized policy tests cover data-write loss,
invalid inputs and timeout/no-restore rules; the source lock audit now has
22 guarded handlers.

Live driver SHA `295c8c82f6a39a07cd7d74c8cb237d7602226c7de445951a06140c407d2ae0bb`,
artifact folder `/boot/home/pioneer-p100a-20261014/data-tile-1/`.
Nine diagonal/equal-exponent reduction controls passed **exactly**. The
full-mantissa mixed-exponent sweep is NOT bit-exact: an initial absolute
1e-5 target failed, and a normalized 1/4096 target missed one output in
trial 6. At **08:19 UTC**, all eight trials / 2048 outputs passed the explicit
BF16-level acceptance target `abs(error) <= 1e-6 + sum(abs(products))/1024`.
Measured maximum absolute error 0.00425720215, maximum normalized error
0.000244877592, elapsed 1.342 seconds. This is an empirical acceptance
test, not a universal hardware error bound or a diagnosis of the precise
mixed-exponent reduction rounding. Official MVMUL documentation explicitly
does not promise IEEE-exact reduction. The earlier strict failures remain
important evidence; do not describe this path as bit-exact FP32 GEMM.
Sweep CLI SHA `d47c088ca83b11ed84f7c5f60d43e21c8c11844acd28510bd84004fb8cd5e170`.

`tenstorrent_haiku_tinyllama.cpp` is a strict fixed-shape 260K TinyStories
bootstrap, not a Qwen loader. Every learned linear projection and classifier
uses actual P100A BF16 tile operations, without CPU fallback. Host software
does normalization, attention/KV, activations, embedding lookup and partial
tile sums. A separate full-FP32 model context compares every generated
token's 512 logits (cosine >=0.999, relative L2 <=0.02, exact argmax agreement).
The offline implementation matches the independently compiled pinned
karpathy/llama2.c reference; all eight BF16-emulated greedy tokens agree:
`Once upon a time, there was a`. Those are **offline** results, not yet a
claim about live P100A inference. A two-token live test is underway.

At **08:23 UTC**, the first LIVE token completed: id 403, ` Once`, identical
to the independent CPU argmax. Logit cosine 0.999997107, relative L2
0.002905042, maximum error 0.0623066. All 1028 real device tiles completed
and restored; elapsed 173.071 seconds. This is hybrid inference with every
learned linear projection on the P100A, not all-device execution. The next
token is still being tested. Native harness SHA
`a1de72855ce62f2bdf94985da281c7d44ea1a58de6f7f3fa593c10fc6585843d`,
artifacts `/boot/home/pioneer-p100a-20261014/tinyllama-1/`.

At **08:26 UTC**, the two-token LIVE run PASSED: ` Once upon` (ids 403,407),
both matching the independent full-FP32 CPU argmax. Second token cosine
0.999997918, relative L2 0.002064653, maximum logit error 0.0702281. All
2056 actual P100A tile transactions completed/stopped/restored; total
346.172 seconds. The P100A, not a CPU fallback, computed all learned linear
projections/classification; CPU still handles the other listed operations.
This establishes a very small working hybrid model, not the requested 27B
model, useful throughput, resident weights or all-device inference.

Pinned model author/revision: karpathy/tinyllamas
`0bd21da7698eaf29a0d7de3992de8a46ef624add`; raw model SHA
`b0a507e7ad0f626624f17112325e66691f9076d622e1d3274d103d00299f2696`,
tokenizer SHA `037cb335abb25d1fa9e8ecae30ed2a3a8ace9302862ebcdc05d51a6bbb10c312`.
`tenstorrent_fetch_tiny_bootstrap.sh` fetches only pinned plain C/license,
raw float checkpoint and tokenizer, verifies hashes, and executes nothing.
Do not download/execute the `.pt` pickle. The reference source remains
unmodified in ignored artifacts, with its original MIT license.

Reproduce offline checks:

```sh
sh tools/pioneer/tenstorrent_fetch_tiny_bootstrap.sh /absolute/fresh/artifacts
sh tools/pioneer/tests/run_pioneer_tinyllama_tests.sh /absolute/fresh/artifacts
sh tools/pioneer/tenstorrent_cross_build_tools.sh CONFIGURED_BUILD /absolute/fresh/tools
```

The diagnostic per-tile save/check/restore path is far too slow for 27B.
Qwen3.8-27B 4-bit, resident GDDR weights and a scalable batched backend remain
separate, unfinished milestones. No firmware/boot/SD changes or DMA enable.

### October 15: bounded batching and eight-token live inference

Added data-only batch ioctl `549e`, metadata `549f` v40. It accepts 1..16
fixed-shape inputs, privately snapshots/validates all before hardware access,
and holds the same lock/PCI reservation throughout. Original L1 is saved
once/restored once. Every job is independently started, drained, parked and
stopped; only the 256 input-table words and three progress words change
between jobs. Executable code and entry are untouched. A failed batch
erases all public outputs. A timeout after job one still forbids L1 restore
and cold-latches the driver; dropped second-job data before start safely
restores the original snapshot. Offline tests cover all 16 ordered results,
invalid final input, count bounds, mid-batch timeout and dropped data.
The lock audit now has 23 guarded handlers. Single-job/legacy tests pass.

At **08:30 UTC**, eight real matrices passed in one transaction, 2048 checked
outputs, same numerical error as the single-job test, elapsed 0.189 s. An
immediate warm repeat passed in 0.191 s, recovery 0, full restoration.

At **08:33 UTC**, the live batched pretrained-model run PASSED all eight
tokens: ` Once upon a time, there was a` (ids 403,407,261,378,432,383,286,261).
All CPU argmaxes matched. Across tokens cosine >=0.999989242, relative L2
<=0.005256786, max logit absolute error <=0.110325. All **8224 real device
tiles** completed/stopped/restored, total **123.050 seconds**, about 15.38
seconds/token vs 173.09 before batching (11.25x faster). The first two tokens'
logits were identical to the unbatched live results. Host attention/norm/KV
and partial sums remain; no CPU fallback for learned projections.
Post-run health: 64 CPUs, 137,034,936,320 maximum RAM bytes, card idle 800 MHz,
742 mV, 90 W input, zero thermal trips, advancing heartbeat, PCI commands
0000/0000. No cold recovery, boot changes, firmware flash, SD changes or DMA.

Artifacts `/boot/home/pioneer-p100a-20261014/batch-1/`: driver SHA
`c0cdb75a7f2d4a90638be45660e99121b02deb0152656fc96643071ff3c32a77`,
matrix CLI `9dee6fe46afbc29b851030dd617cb27fa2dfb7cf95fd0dcd9ec9cd4e9c32b925`,
model harness `3f7571fdfd2098b4d5ac5d001a3453f8d45fecc38a40e03099ba9545713fb968`.
This is a working tiny hybrid language model, NOT Qwen3.8-27B. Resident
quantized weights, device-side transfers and a scalable model backend remain
necessary for that target. Default boot remains Linux and both SDs stay out.

### October 15: requested Qwen GGUF preflight and mixed-format projections

Pinned primary model candidate: [ggml-org/Qwen3.8-27B-GGUF](https://huggingface.co/ggml-org/Qwen3.8-27B-GGUF),
revision `71bc7b627595dc8a91039addd9c791ae548d6747`, file
`Qwen3.8-27B-Q4_K_M.gguf`, **18,973,870,528 bytes** (17.67 GiB).
Published full LFS SHA-256:
`c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747`.
Only a 16 MiB header range and three small tensor ranges were downloaded,
NOT the complete model. Range hashes do not verify that full-file checksum.
HTTP responses were bounded 206 ranges from that immutable revision.

The new read-only `tenstorrent_gguf_preflight.py` parses GGUF v3 with bounded
lengths/counts, validates tensor formats/alignment/non-overlap/full-file
bounds and never evaluates chat templates or executes a model. Tests reject
every header truncation, duplicate keys, bad alignment/types/dimensions and
out-of-file tensors. Actual header ends at byte 10,994,112 and describes
851 tensors: **193 Q4_K, 288 Q8_0, 17 Q6_K and 353 F32**. It is not uniform
4-bit. Architecture is `qwen35`, 64 layers, width 5120, FFN 17408, recurrent
inner width 6144, full-attention interval 4. The TinyStories harness cannot
load this graph; convolution/Gated DeltaNet and gated-attention execution,
tokenization and memory planning remain substantial unfinished work.

Independent original host block decoders now cover Q4_K (144 bytes/256
weights), Q8_0 (34/32), Q6_K (210/256) and IEEE FP16 scales. Offline fixtures
test all finite FP16 encodings, 16384 Q4_K, 8192 Q8_0 and 65536 Q6_K values,
including signed scales/quants and rejection of nonfinite scales.
No existing llama.cpp files were changed. Native probe uses bounded host
block decode/BF16 conversion, P100A matrix multiplication and host partial
sums. It compares both decoded-FP32 and BF16 CPU references for 16 real
weight rows with a synthetic activation vector; this is NOT complete model
inference or device-side quantized storage/dequantization.

At **08:41..08:44 UTC**, actual Qwen weights PASSED:

| Tensor sample | Shape tested | CPU FP32 cosine | Relative L2 | Device tiles / transactions | Time |
|---|---|---|---|---|---|
| Q4_K `blk.0.ffn_gate.weight` rows 0..15 | 5120 → 16 | 0.999997570 | 0.002212184 | 320 / 20 | 4.337 s |
| Q8_0 `blk.0.attn_gate.weight` rows 0..15 | 5120 → 16 | 0.999998024 | 0.002027083 | 320 / 20 | 4.330 s |
| Q6_K `blk.3.attn_output.weight` rows 0..15 | 6144 → 16 | 0.999998654 | 0.001672537 | 384 / 24 | 5.231 s |

All outputs satisfied the separate BF16 numerical budget; every transaction
completed/stopped/restored without recovery. Driver remains batching SHA
`c0cdb75a7f2d4a90638be45660e99121b02deb0152656fc96643071ff3c32a77`.
Latest native projection CLI SHA
`ed5b2b1e2006a05bccfff851f21e552a34ba25f38b1f5f3ddb86e8d2f9149e3e`,
folder `/boot/home/pioneer-p100a-20261014/qwen-mixed-1/`.
The earlier Q4-only CLI was
`5719b881425a9f9ecb7f38b33587e49f0772197952a9ce60fcf4fee9bab1ad3e`
in `qwen-q4-1/`.

`tenstorrent_fetch_qwen_preflight.sh` reproduces bounded header and tensor
ranges, verifies exact lengths, Content-Range and local range hashes. It
does not execute code, connect to the card, fetch pickle files, or download
the complete 19 GB model. Run the inspector with declared full size
18973870528. Do not treat the header range as a runnable GGUF file.

Remaining scale gate: resident quantized weights and device-side transfers
must replace host PIO weight streaming, then implement/validate the complete
64-layer hybrid graph. A model-sized BF16 expansion would exceed 28 GB and
must not be used as the residency plan. No SD, default boot, firmware or DMA
changes were needed for any of these results.

Download the other pinned official sources with:

```sh
sh tools/pioneer/tenstorrent_fetch.sh
```

Default location is ignored `generated.pioneer/tenstorrent/`. Existing trees
are never overwritten. No upstream build/install scripts are executed.

| Component | Official source | Investigated revision | License notice |
|---|---|---|---|
| Kernel driver | https://github.com/tenstorrent/tt-kmd | 22cca4cb33455d962cf8340a8f5b896dae696fd7 | GPL-2.0-only; ioctl header has Linux-syscall-note |
| User-mode driver | https://github.com/tenstorrent/tt-umd | 5c9ea40f8e29f84c32dde2646f93e19e1d13dd9a | Apache-2.0 |

Keep original notices and provenance. Do not relabel copied GPL driver code
as MIT. Packaging/upstream acceptance needs an explicit licensing review.
These pins are investigation snapshots, not a validated mutually compatible
release pair or a firmware recommendation.

### October 15: device-residency prerequisite, not Qwen acceleration

The existing tile interface cannot become a practical Qwen accelerator just
by removing reference comparisons or enabling its UI entry. It streams tiny
BF16 weight tiles over host PIO, executes on one tile, and repeatedly starts,
drains, stops and restores it. Qwen needs persistent device weights and much
larger, parallel device-dispatched operations. Qwen remains CPU-only in the
Assistant; TinyStories remains explicitly experimental, not a Qwen fallback.

A fresh CPU comparison baseline used the existing native llama.cpp
`c060ca974c77`, installed Qwen3-0.6B-Q8_0 (SHA-256 `9465e63a...`), 32 CPU
threads, batch/microbatch 128, GPU layers 0, normal warmup and three repeats:
prompt processing 128 tokens `32.219 +/- 0.303` tokens/s; generation 64 tokens
`11.695 +/- 0.091` tokens/s. See the unmodified native
[JSONL results](benchmarks/haiku-qwen3-0.6b-q8-cpu-residency-baseline-20261015.jsonl).
Prompt and generation are separate llama-bench cases, not an Assistant
end-to-end latency result. A preceding one-repeat prompt32/generation16
check gave `33.888/12.015` tokens/s. Benchmarks ran in a separate process,
without switching the open Assistant away from TinyStories. Do not compare
these numbers to the much smaller TinyStories model as an acceleration ratio.

Two new root-only, fixed-address operations were implemented and deployed:

- `tenstorrent_haiku_compute_noc --read-compute-noc-and-restore` first
  verifies all seven GDDR routes and all 120 enabled tiles held reset. It
  samples physical `(1,2)` NoC0's identity, translation configuration, four
  command controls and sixteen outstanding-response counters three times.
  Only temporary BAR/TLB controls are written. The live samples agreed:
  node `10611081`, endpoint `00010001`, NIU configuration `00006005`, all
  commands/counters zero, request FIFO disabled. TLB/PCI rollback passed.
- `tenstorrent_haiku_resident_read --read-fixed-gddr-to-l1-and-restore`
  repeats the ownership/route gates, requires unused initiator 0 and no NIU
  interrupt enables, then reads bank 0 at 64 MiB through the host window for
  comparison. Those bytes stay kernel-local and GDDR is never written.
  It saves the final 64 bytes of `(1,2)`'s L1 and poisons them with the
  inverse of the expected bytes. It temporarily enables the NIU request
  FIFO and issues exactly one 64-byte read from GDDR to that L1 address.
  Both endpoints are on-card; bus mastering/host DMA stays disabled.

Completion requires command-control readback before the first counter read,
exactly one additional read response, data flit, command acceptance and read
request sent, all initiators idle, all outstanding/outgoing ID counters zero,
stable source routes and unchanged stopped-core state. A command accepted
without its completed response is not success. The destination must then
match the independently read GDDR bytes. Only proven completion/ownership
permits restoring L1, initiator fields and the original FIFO setting. A
timeout or uncertain route/completion leaves payload/control state untouched
and latches cold recovery; never retry, clear counters or reload the driver
to bypass it. Event counters naturally advance; this does not roll them back.

The first live transfer and the final stricter-flit-check build both PASSED:
issued/completed `1/1`, one poll, zero mismatches, counter deltas `1/1/1/1`,
L1/NIU/TLB/PCI restored `1/1/1/1`, card/root commands `0000/0000`, cold flag
zero. No core was released or reset-written. Metadata queries also confirm
the old data-tile/batch ABI remains available. Neither test transfers model
weights, proves all-bank bandwidth, nor executes Qwen on the device.

The source policy tests use ASan/UBSan and inject every individual TLB,
initiator, FIFO, doorbell and L1 write loss, timeouts, accepted-but-incomplete
commands, incorrect flit counts, data corruption, source changes, active
cores, enabled interrupts, counter wrapping and permanent L1 rollback
failure. They verify no request retry, counter clear or NIU/L1 rewrite after
uncertainty. All thirteen PCI/ARC/core/L1/DRAM/compute policy suites pass;
the existing independent TinyStories/reference and Assistant-adapter tests
pass too. The optional driver builds through Jam and all twenty-one tools
cross-build with warnings as errors.

Final installed driver SHA-256:
`4e128ef4857f6e311ce0c784a6020632e711a860bb08c27892ba48bb43d68790`.
The source files were synced to `/Volumes/HaikuBuildLocal/haiku` before the
build. Verified native backups/evidence are in
`/boot/home/pioneer-compute-noc-20261015/` and
`/boot/home/pioneer-resident-read-20261015/`; copied logs are ignored under
`generated.pioneer/tenstorrent/resident-read-20261015/`. No Assistant binary,
conversation, settings, model, boot medium, kernel, firmware or CPU/RAM
configuration changed during these tests. The original dirty llama.cpp
submodule was left untouched.

Register semantics were audited against official
[NoC memory-map documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/NoC/MemoryMap.md)
and [completion-counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/NoC/Counters.md).
The implementation is original fixed-policy code, not an upstream Linux
runtime transplanted onto Haiku.

Remaining acceptance gates for a real Qwen P100A mode:

1. Own bounded GDDR allocations, upload resident weights once, verify their
   contents and lifetimes, and safely release them on cancel/exit. Preserve
   quantized storage for the requested 27B model; full BF16 expansion does
   not fit its 28 GB card.
2. Build persistent, larger matrix-vector kernels using device-side
   GDDR-to-L1 transfers; validate first on one tile, then distribute work
   across the enabled tiles. Do not use a host ioctl/reset per tiny tile.
3. Integrate a real Qwen inference backend: begin with installed ordinary
   Qwen3-0.6B and independent layer/logit references; the requested 27B
   `qwen35` hybrid graph and mixed quantization require additional support.
4. Validate generation, KV/context growth, deterministic comparisons,
   cancellation, repeated requests, model switching and recovery, with no
   silent CPU fallback.
5. Measure the same model/quantization, prompt and output lengths against
   the CPU using repeated warm runs, time to first token, prompt throughput,
   sustained generation and total latency. Include cold load separately.
   Only expose a practical faster-Qwen mode after this gate passes.

### October 15: compute-core-driven resident-input matrix proof

The fixed `TT_UNPACK_RESIDENT` variant of `blackhole_tensix_unpack.S`
retains the independently verified full-fidelity BF16 matrix program and
its original completion/park addresses. L1 activation/weight tables are
initialized to zero, not usable fixtures. A bounded NoC reader is appended
outside the input buffers, within one fixed 4 KiB code/data page. Before
each of the two matrix fixtures, TRISC0 issues a read of the same 1,088-byte
resident GDDR block into its private L1 input area. The host cannot ring a
NoC doorbell through this operation, and performs no intervening data writes.

The reader checks an idle initiator, records the expected response count,
reads back its doorbell before polling counters, waits for that response
and zero outstanding requests, then fences before consuming externally
written L1. Its 65,536-iteration deadline parks with `NOCF`, never `MAT1`,
on uncertainty. It never retries a request or clears counters. The linker
and independent RV32I/FPU/NoC simulator verify exact assembled bytes,
executable bounds, both dense numerical outputs, delayed transfers,
corruption detection and missing/incomplete-response refusal.

The optional root-only `tenstorrent_haiku_resident_math` ioctl first runs
the existing firmware, seven-bank route, 120-held-tile and compute-source
checks. It saves/validates the fixed GDDR block and L1 footprint, uploads
one known dense BF16 fixture block, then installs/verifies the poisoned
program. The NIU's fixed command fields and request FIFO are configured and
verified before releasing only TRISC0. The host requires normal math drain,
two fixtures, `MAT1`, two park-PC samples, exact NoC counter deltas
`2/34/2/2` (responses/flits/accepted/read requests), all initiators idle and
all outstanding/outgoing counters zero before declaring completion.

Unlike the earlier no-NoC tile test, a timeout does **not** assert reset:
an outstanding request could still overwrite L1 after a RISC parks. Unknown
execution, route, completion or stop therefore forbids further reset,
NIU/control or payload writes and latches cold recovery. Only verified
completion and stop permit restoring original NIU fields/FIFO, L1, GDDR,
TLBs and PCI state. Src/Dst, local RISC state and coprocessor configuration
are explicitly initialized, not claimed preserved.

Two real Pioneer runs PASSED with math ABI 27, state 1, started/completed/
stopped `1/1/1`, three host polls, zero numerical mismatches, phase `703a4`,
two fixtures, marker `4d415431`, both PCs `703bc`, reset
`47800 -> 87ffff -> 46800 -> 47800`, NoC deltas `2/34/2/2`, all five
GDDR/NIU/L1/TLB/PCI restoration flags 1, commands `0000/0000`, cold flag zero.
No firmware, host DMA, model, Assistant setting or boot change was made.
The input block is resident across the two fixtures only: this is not yet
a model allocator, persistent command queue, multicore backend or benchmark.

ASan/UBSan policy tests inject every individual setup/restore write loss,
NoC timeout, wrong flit count, wrong park PC, numerical corruption, source
change, active cores/interrupts, allocation failure and permanent GDDR/L1
restore failures. They assert no host doorbell, repeated start, reset or
payload rewrite after uncertain execution. All previous image and thirteen
PCI/ARC/core/memory policy suites remain passing. Jam builds the optional
driver and all twenty-two utilities cross-build with warnings as errors.

Installed driver SHA-256:
`e3cbe60248d8ec940ed7f00dfa6357e619a7028e9efc849fc4b256537668c8a5`;
CLI SHA-256:
`a375d177bd5b0a6c0769f951f243fff0b037466258f249c936a5dd0ae3323613`.
Verified original driver and native evidence are in
`/boot/home/pioneer-resident-matrix-20261015/`; copied logs and assembled
artifacts are ignored under
`generated.pioneer/tenstorrent/resident-matrix-20261015/`. Larger resident
matrix-vector kernels, model residency, Qwen graph integration and the
same-model faster-than-CPU acceptance gate remain unfinished.

### October 15: larger resident GEMV with on-card partial sums

The fixed resident program now has a separate `TT_UNPACK_GEMV` variant:
exactly 16 outputs and K chosen from `{1024,5120,6144}`. It uses one launch,
initializes Dst once, then iterates over K/16 blocks. Each iteration's source
is the fixed GDDR base plus `block*1088`; the device issues its read, waits
for completion, unpacks BF16 and adds four fidelity phases into the retained
FP32 accumulator. Only after the full dot product completes are results
copied to L1. No host block dispatch, intervening input writes or CPU partial
sums are used. No arbitrary code, pointers, coordinates or shapes are accepted.

The original matrix proof and GEMV share a templated policy selected solely
by trusted kernel program types. The original operation keeps math ABI 27;
the new root-only GEMV operation has math ABI 28 and an explicit version/shape
request. It maps/saves at most 417,792 fixed GDDR bytes, one code/data page,
the fixed output/status words and NIU controls. The same cold-recovery,
stopped-core ownership, NoC completion, numerical comparison and exact
GDDR/NIU/L1/TLB/PCI rollback gates apply. A failed/uncertain completion never
triggers a reset, retry or memory rewrite.

The simulator independently decodes the assembled RV32I instructions,
checks executable/data address bounds and models NoC payload delivery and
the BF16 FPU. Every output matches an independent full-length integer dot
product for all three shapes, including unused zero output rows. Fault
tests detect stale Dst, clearing between blocks, corrupted weights, delayed
or missing responses at the first, middle and final block. The byte test
independently assembles every supported shape and compares every code word.
Shared policy tests retain exhaustive small-operation write-loss coverage;
larger GEMV tests sample upload/restore interiors and boundaries and cover
NoC/PC/source/rollback failures and invalid shapes.

All three live Pioneer tests PASSED:

| Inputs -> outputs | Device reads / response flits | Host polls | Entire reversible test |
|---|---|---|---|
| 1024 -> 16 | 64 / 1088 | 11 | 0.341552 s |
| 5120 -> 16 | 320 / 5440 | 9 | 0.983847 s |
| 6144 -> 16 | 384 / 6528 | 9 | 1.146131 s |

Each had state 1, ABI 28, started/completed/stopped `1/1/1`, zero mismatches,
phase `703a0`, matching tile count, marker `4d415431`, both park PCs `703b8`,
all five restoration flags 1, commands `0000/0000` and cold flag zero. The
wall times include all initial ownership checks, CPU PIO uploads/readbacks,
numerical checks, original-memory restoration and mapping cleanup. They are
**not kernel throughput, real Qwen projections, whole-model inference,
nor a CPU speed comparison**. Inputs are known synthetic integer/BF16
fixtures. Persistent model allocations and efficient host transfers remain
necessary before practical inference timing can be measured.

The optional driver builds through Jam, all twenty-three utilities
cross-build with warnings as errors, and original image/memory/compute
tests remain passing. Final installed driver SHA-256:
`0efb637754cb925facd830678d125908c2e4b2815cbb4a08df17e7695e5ae176`;
GEMV utility SHA-256:
`189fb50a269e7113b55d7e2613b9ad19fea7bc7275031fcd00845e2ccfc3b407`.
Native backup/logs are under `/boot/home/pioneer-resident-gemv-20261015/`;
ignored local evidence is under
`generated.pioneer/tenstorrent/resident-gemv-20261015/`. Driver source and
program definitions were synced to the build mirror. Assistant, models,
boot, firmware and host DMA state were not changed.

### October 15: read-only host-transfer address-path audit

Before enabling any host DMA, a new root-only read-only preflight checks the
compute source route `(19,24)` to the actually connected PCIe tile `(2,0)`,
all previous idle/firmware/route gates, the Cadence inbound no-BAR-match
mapping, and the card's NoC-to-host SII window configuration. A temporary
read-only BAR0 TLB uses the full 43-bit local-address field to reach fixed
device-local `fffffffff0000000 + 134..150`, not a host pointer. The native
driver maps both SII and root-AT pages read-only/uncached. There are no
SII/inbound/initiator/payload writes, bus mastering, core starts or DMA.

The live audit PASSED: connected PCIe `18611002`; all three samples had
root inbound `0000002f/00000000` (the existing 48-bit identity mapping), and
SII windows `00400000 00400200 00400100 00400300 00000000 00000200
00000100 00000300`, matching pinned firmware 18.5 defaults. In particular,
window 0 uses memory requests with iATU bypass, no address prefix, relaxed
ordering, no-snoop or DBI flag. TLB/PCI restoration flags were `1/1`,
commands `0000/0000`, cold flag zero. This proves the sampled configuration,
**not an actual DMA transfer or cache-coherent host-buffer lifetime**.

ASan/UBSan tests verify independent full-width address decoding, route
refusal, every individual TLB-write loss/rollback, wrong/changing inbound
or SII values, active cores, source changes and failed restoration. Jam and
all twenty-four cross-built utilities pass. Installed driver SHA-256:
`0d0ebffb6accc34621d5054f83b6cb40b51ca3cb5d835215ecda7a4dc4385148`;
host-path utility SHA-256:
`806def48eea75766a3eece25bcb88b869be5628c143cd3c402b2a6c28971d6dc`.
Native backup/log: `/boot/home/pioneer-host-path-20261015/`; ignored local
evidence: `generated.pioneer/tenstorrent/host-path-20261015/`. No firmware,
model, Assistant, boot or DMA state was changed. Next: dedicated owned,
pinned non-cached host buffers and a bounded device-read-only transfer.

Register provenance: pinned official system-firmware 18.5 `pcie.c` (SII
base, window data fields and default values), and
[PCI Express tile documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/PCIExpressTile/README.md).
This policy is original code; no GPL KMD implementation was copied.

## Findings from source

- P100A belongs to Blackhole. KMD `enumerate.h` identifies Blackhole as
  1e52:b140; this ID alone does not identify the precise board SKU.
- KMD `blackhole.c` maps portions of BAR0 and BAR2. BAR4 length determines
  the exposed count of 4-GiB TLB windows. Do not assume a particular BAR size
  or that a 32-bit `pci_info` field can describe the complete aperture.
- KMD `memory.c` uses coherent DMA allocation, pinned user pages, scatter/
  gather mappings and IOMMU handling. Blackhole's current DMA limit is 58 bits;
  that is not proof the Pioneer host can DMA to every physical RAM address.
- KMD `interrupt.c` requests one PCI interrupt with Linux IRQ allocation.
  Haiku has MSI/MSI-X interfaces, but the P100A path must be verified.
- UMD `tt-kmd-lib/src/tt_kmd_lib.c` uses Linux ioctls plus file-backed mmap
  offsets for device/TLB/DMA memory. Implement a Haiku transport; do not assume
  a Linux ioctl number or mmap offset works with Haiku's driver ABI.
- UMD `device/api/umd/device/driver_atomics.hpp` already includes RISC-V
  fences. This is a head start, not evidence that the whole runtime supports
  RISC-V Haiku. Audit discovery, mapping, dependencies and cache behavior.
- UMD's README describes IOMMU/hugepage requirements for host-device memory.
  An initial bounded DMA allocation is more realistic than promising general
  Linux hugepage/pinning compatibility on Haiku.

## Existing Haiku foundations and risks

`src/add-ons/kernel/busses/pci/ecam/ECAMPCIControllerFDT.cpp` already has
SG2042 PCIe translation and MSI support. Preserve this working infrastructure;
change it only when a captured P100A failure demonstrates a missing capability.

The current NVMe driver explicitly uses non-cacheable DMA bounce buffers on
SG2042 because ordinary cached payload memory is not coherent in this port.
This is a central accelerator risk: CPU fences alone do not flush/invalidate
non-coherent caches. Initially use small, explicitly managed DMA buffers with
validated bus addresses, ownership transitions and lifetime rules.

BAR addresses are PCI bus addresses, not automatically CPU physical addresses.
Validate translated ranges, 64-bit BAR pairing, full widths, mapping attributes
and bridge apertures before touching registers. Avoid destructive BAR sizing
probes against a live device. PCI enumeration is not accelerator readiness.

## Incremental implementation plan

1. **Inventory:** install with the machine fully powered down; capture Linux
   PCI identity/BDF, assigned BAR resources, link state and firmware/driver
   versions. Confirm Haiku enumerates the same endpoint. Do not flash firmware.
2. **Native diagnostic driver:** bind only the Blackhole device, expose a
   versioned read-only identity/resource ioctl, and fail closed on unsupported
   operations. No DMA, resets, arbitrary physical mapping or user register
   writes. Build this as an optional add-on, not a mandatory boot dependency.
3. **Controlled device access:** validate BAR mappings, port the minimum ARC
   firmware/message and TLB access required for a bounded identity/telemetry
   query. Register reads are not all side-effect-free; audit each access.
4. **DMA/interrupt smoke tests:** a small buffer round-trip with known patterns,
   guards, timeout, cleanup and repeated tests; then high RAM, multiple CPUs,
   cancellation and process-exit cases. Enable bus mastering only after DMA
   mappings and teardown are correct. No arbitrary user-page DMA initially.
5. **UMD transport:** substitute Haiku discovery/ioctls/areas and implement only
   verified operations. Unsupported Linux-only features return errors, not
   dummy success. Validate runtime/firmware compatibility before compute.
6. **Compute:** minimal supported runtime operation, then matrix multiply,
   then an LLM integration and performance tests. A PCI/kernel driver by itself
   does not make the existing CPU llama.cpp build use the card; it needs a
   compatible accelerator backend/runtime path too.

Hardware-dependent steps cannot be marked complete from compilation alone.
Preserve the working SD/Samsung CPU-only configuration as a recovery path.

## Fedora hardware inventory, 2026-09-21

With the P100A in the bottom PCIe slot and a native 12+4-pin PSU cable, the
first boot did not enumerate the card. After a clean shutdown and physical
reseat, Fedora 38 (Linux 6.1.31, riscv64) enumerated `0002:81:00.0` as
`1e52:b140`, class `1200` (processing accelerator). The root port is
`0002:80:00.0`. The link trained at PCIe 2.5 GT/s x8; this is below the
endpoint's advertised 32 GT/s x16 capability and is not a performance result.

No Tenstorrent module, device node, or `tt-smi` installation was present.
Linux failed to assign all three memory BARs: BAR0 512 MiB, BAR2 1 MiB,
and BAR4 32 GiB. It also failed to allocate the root-port's requested
48 GiB prefetchable bridge window. The root bus advertises a high
prefetchable window at `0x4900000000..0x4affffffff` (8 GiB), plus a
768 MiB lower prefetchable window. Its non-prefetchable windows cannot
accommodate the 32 GiB prefetchable BAR. `lspci` shows the bridge memory
windows disabled and the endpoint resources unassigned. The other Pioneer
root buses likewise advertise only 8 GiB high prefetchable windows in this
Fedora boot; moving slots alone is not an established fix.

The card's usable memory capacity is distinct from the size of
its BAR4 PCI address aperture. Do not treat the user's 28 GB memory figure
as a contradiction of the measured 32 GiB BAR. The captured allocation
failure is sufficient to stop driver/LLM installation attempts for now.
The SG2042 manual and Tenstorrent BAR4 discussion below narrow the possible
workaround; a larger device-tree window is not the solution.
Do not write live PCI configuration, flash firmware, or replace the working
boot setup as a diagnostic shortcut. A future aperture change needs a
recovery plan and separate validation. No Linux/Haiku PCI configuration,
driver, firmware, or storage was changed during this inventory.

## Linux boot check, 2026-10-04

With the Haiku SD removed, the Pioneer booted its SPI loader, then kexec'd
Fedora Linux 6.1.31 from NVMe. The serial log again shows the P100A at
`0002:81:00.0` (`1e52:b140`) requesting BAR0 512 MiB, BAR2 1 MiB and BAR4
32 GiB. The root port's high prefetchable aperture remains 8 GiB; Linux reports
no space for the 48 GiB bridge window and fails to assign all three endpoint
BARs. This confirms the same aperture problem on a fresh boot with the card
installed. Serial capture is saved in ignored
`generated.pioneer/tenstorrent/pioneer-linux-boot-2026-10-04.log`. The boot
used no SD, and no PCI settings, card firmware, or disks were modified.

### Read-only aperture feasibility check

The active device tree's `pcie@7062000000` ranges match the kernel source in
`arch/riscv/boot/dts/sophgo/mango-pcie-4rc.dtsi`: 768 MiB low prefetchable,
256 MiB non-prefetchable, 8 GiB high prefetchable at `0x4900000000`, and
4 GiB high non-prefetchable at `0x4b00000000`. `/proc/iomem` confirms the
adjacent `0x4c00000000..0x4fffffffff` address space belongs to the next
PCIe controller, while `0x4800000000..0x48ffffffff` contains this root's
configuration and smaller windows. Thus the 32-GiB-aligned interval
`0x4800000000..0x4fffffffff` cannot simply be assigned to BAR4.

The Sophgo Cadence host driver programs an outbound translation region for
each device-tree range. Its CPU-address fixup masks addresses with
`0xCFFFFFFFFF`; for example, the next 32-GiB-aligned candidate at
`0x5000000000` would become `0x4000000000` in the programmed translation
register. The controller's wider address decode and a non-overlapping host
aperture have **not** been established, so changing only the device-tree
window would be an unsafe experiment. This read-only check made no changes
to the Fedora boot or PCI configuration.

The local Tenstorrent KMD source derives its number of BAR4 TLB windows from
the exposed BAR length, but the UMD and compute stack still need validation
without BAR4. Even after BAR0/BAR2 allocation works, host-side TT-Metal
support on riscv64 remains a separate porting question.

### SG2042 physical limit and BAR4-free path

The [SG2042 Technical Reference Manual, Chapter 2, Table 1](https://github.com/milkv-pioneer/pioneer-files/blob/main/hardware/SG2042-TRM.pdf)
assigns only **16 GiB** of CPU physical address space to each PCIe link:
`0x4000000000..0x43ffffffff`, `0x4400000000..0x47ffffffff`,
`0x4800000000..0x4bffffffff`, and `0x4c00000000..0x4fffffffff`.
The P100A is on PCIe1 link 0 (`0x4800000000..0x4bffffffff`). Its 32 GiB
BAR4 cannot fit inside that link's documented physical aperture, regardless
of how the current 8 GiB device-tree subwindow is enlarged. Do **not** make
a 32 GiB device-tree `ranges` change or borrow space from the neighboring
link. The live `lspci -vv` capability list also shows no PCI Resizable BAR
capability; a standard BAR resize is not available in this observation.

There is, however, a credible BAR4-free path. A [Tenstorrent engineer's
August 2026 Linux PCI patch](https://lists.openwall.net/linux-kernel/2026/08/24/1853)
targets Blackhole device `1e52:b140`, exactly this P100A's PCI ID. It
proposes omitting BAR4 from Linux resource assignment when the host aperture
is too small, allowing BAR0 and BAR2 to fit. The author reports that this
made `tt-smi` and `tt-bh-linux` work on a 4 GiB-aperture RISC-V host.
That is evidence of basic card access without BAR4, **not** evidence that
TT-Metal or LLM inference works without it.

The patch is not safe to copy uncritically: [Linux PCI review](https://lkml.iu.edu/2608.3/06163.html)
points out that zeroing Linux's BAR4 resource metadata does not disable
the BAR in the device. Enabling memory decoding could leave a live,
unassigned BAR that decodes an unintended PCI address. Do not use this
unmodified PCI quirk on the Pioneer. The firmware-side sizing mechanism
below is preferable once the card can safely be reached for flashing.

On the live Fedora boot, read-only `setpci` shows PCI COMMAND `0x0000`
(memory decoding and bus mastering both off), while BAR4's low/high config
dwords are `0x0000000c`/`0x00000000` (an unassigned 64-bit BAR). This is
safe in the current state, but it reinforces the reviewer's concern:
turning on memory decoding after merely hiding BAR4 from Linux would leave
the hardware BAR pointed at bus address zero. No PCI config register was
written during this check.

### Firmware-side BAR4 resize: offline dry run

Tenstorrent's public [system-firmware source](https://github.com/tenstorrent/tt-system-firmware)
at `04f0df6ebfbd016d98189e8131b16af17b48638b` exposes
`pci0_property_table.pcie_bar4_size` in MiB. The P100A table defaults to
`32768`; `lib/tenstorrent/bh_arc/pcie.c` accepts power-of-two values and
explicitly handles `0` as “BAR4 Disabled.” The project's own
`scripts/update_bar4_size.py` edits that field in a firmware bundle and
states that a **cold reboot** is required for the change to take effect.
The `P100A-1` board in the bundle matches this card's `0x0043` subsystem
device family, but its exact installed firmware version remains unknown.

An offline dry run used the official `v19.15.0` firmware bundle, whose
downloaded SHA-256 matched the published
`c1a317f9658435a7f2e8ab1e18a9fe942cd36334d8b88f01b777c8de975b2aef`.
The official script changed only `P100A-1` PCI bus 0 to BAR4 size `4096`
MiB, and its post-write verification passed. `tt_fwbundle.py diff` reported
only the P100A-1 `cmfwcfg` entry changed, from CRC `8cc1b6a1` to
`34a5fa6f`. The resulting **unflashed, experimental** bundle is at
`generated.pioneer/tenstorrent/fw_pack-19.15.0-p100a-bar4-4096-test.fwbundle`
(ignored local artifact, not pushed to GitHub) with SHA-256
`6dda5a74404672a194bf472a2c816e75b043dda015307d6b6774c6e4df61d602`.
Do not copy this firmware bundle to the public project repository or flash
it without determining the card's installed firmware version, taking a
recoverable backup, verifying board compatibility, and establishing a way
to access the card with BAR0/BAR2 despite today's failed allocation.

Why 4 GiB: one BAR4 4-GiB TLB window plus BAR0 (512 MiB) and BAR2 (1 MiB)
should, in principle, fit the root's 8 GiB prefetchable window. Linux bridge
allocation and Tenstorrent userspace behavior with only one large window
have **not** been tested. Setting BAR4 to zero is another documented firmware
option for basic access, but its effect on TT-Metal/LLM use is even less
certain. No card firmware, kernel, boot configuration, or PCI register was
changed during this work.

Next gate: find a safe one-time access path (for example a compatible host)
and a firmware backup/recovery procedure, then select a matching firmware
release and test a 4 GiB BAR4 configuration under supervision. Do not
reboot or flash this Pioneer unattended: a failed card flash or boot test
could need physical recovery. First verify PCI resource assignment and
`tt-smi`; model inference is a later milestone.

## Official references

- P100A hardware: https://docs.tenstorrent.com/aibs/blackhole/installation.html
- Kernel driver: https://github.com/tenstorrent/tt-kmd
- User driver and host-memory requirements: https://github.com/tenstorrent/tt-umd
- Compute runtime: https://github.com/tenstorrent/tt-metal
