# Tenstorrent P100A: RISC-V Haiku bring-up

## Status

Source/feasibility investigation started 2026-09-15. As of October 14,
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
The test restores the card and bridge configuration exactly; DMA has never
been enabled. There is no functioning compute driver/runtime yet. See the
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

Download pinned official sources with:

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
