/* SPDX-License-Identifier: MIT */
/* Opt-in RAM-resource entry. Mode 4 alone permits the native kernel handoff. */
#ifdef PIONEER_KEXEC_PROBE
#include "pioneer_kexec.h"
#include "fdt.h"
extern "C" {
#include <libfdt.h>
}

bool PioneerKexecRoot(PioneerRootManifest& root)
{
	int length;
	int chosen = fdt_path_offset(gFdt, "/chosen");
	const unsigned char* data = (const unsigned char*)fdt_getprop(gFdt, chosen,
		"haiku,pioneer-root-v1", &length);
	return data && length == 80 && PioneerRootManifestRead(data, length, &root);
}

static unsigned int ReadUart(unsigned int offset)
{
	unsigned int value;
	uintptr_t satp;
	asm volatile("csrr %0, satp" : "=r"(satp));
	uintptr_t address = (satp ? 0xffffffc07fffe000ULL : 0x7040000000ULL) + offset;
	asm volatile("fence iorw, iorw; lwu %0, 0(%1); fence iorw, iorw"
		: "=r"(value) : "r"(address) : "memory");
	return value;
}

static void Halt() __attribute__((noreturn));
static void Halt()
{
	for (;;) asm volatile("nop" : : : "memory");
}

static void PutChar(char ch)
{
	if (ReadUart(12) & 0x80)
		Halt();
	for (unsigned int i = 0; i < 262144; i++) {
		if (ReadUart(20) & 0x20) {
			uintptr_t satp;
			asm volatile("csrr %0, satp" : "=r"(satp));
			uintptr_t address = satp ? 0xffffffc07fffe000ULL : 0x7040000000ULL;
			unsigned int value = (unsigned char)ch;
			asm volatile("fence iorw, iorw; sw %0, 0(%1); fence iorw, iorw"
				: : "r"(value), "r"(address) : "memory");
			return;
		}
	}
	Halt();
}

static void Put(const char* text)
{
	for (; *text; text++) {
		if (*text == '\n') PutChar('\r');
		PutChar(*text);
	}
}

void PioneerKexecWrite(const char* data, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (data[i] == '\n') PutChar('\r');
		PutChar(data[i]);
	}
}

void PioneerKexecStop(bool success, const char* reason)
{
	Put("HAIKU_KEXEC_LOADER v3 END status=");
	Put(success ? "PASS reason=" : "FAIL reason=");
	Put(reason);
	uintptr_t satp;
	asm volatile("csrr %0, satp" : "=r"(satp));
	Put(satp ? " (kernel handoff active)\n" : " (kernel not started)\n");
	Put("HALTED: relay recovery required\n");
	Halt();
}

static void Hex(const char* label, uint64_t value)
{
	Put(label);
	Put("0x");
	for (int bit = 60; bit >= 0; bit -= 4)
		PutChar("0123456789abcdef"[(value >> bit) & 15]);
	Put("\n");
}

static void Fail(const char* reason) __attribute__((noreturn));
static void Fail(const char* reason)
{
	Put("HAIKU_KEXEC_LOADER v3 END status=FAIL reason=");
	Put(reason);
	Put("\nHALTED: relay recovery required\n");
	Halt();
}

extern "C" void pioneer_kexec_trap(uint64_t cause, uint64_t pc, uint64_t value)
{
	Put("HAIKU_KEXEC_LOADER v3 TRAP\n");
	Hex("scause=", cause);
	Hex("sepc=", pc);
	Hex("stval=", value);
	Fail("supervisor_trap");
}

extern "C" void pioneer_kexec_start(uint64_t hart, const void* fdt,
	uint64_t satp, uint64_t status, uint64_t sie)
{
	/* No C++ constructors, heap, BSS state or emulator HTIF are used here. */
	if (satp || ((uintptr_t)fdt & 7) || !PioneerRamRange((uintptr_t)fdt, 40))
		Halt();
	uint32_t fdtSize = fdt_totalsize(fdt);
	if (fdtSize < 40 || fdtSize > 2 * 1024 * 1024
		|| !PioneerRamRange((uintptr_t)fdt, fdtSize)
		|| fdt_check_full(fdt, fdtSize) != 0
		|| (fdt_node_check_compatible(fdt, 0, "sophgo,sg2042") != 0
			&& fdt_node_check_compatible(fdt, 0, "sophgo,mango") != 0))
		Halt();
	int uart = fdt_path_offset(fdt, "/soc/serial@7040000000");
	int length;
	const unsigned char* reg = (const unsigned char*)fdt_getprop(fdt, uart,
		"reg", &length);
	if (!reg || length != 16 || PioneerReadBeCell(reg, 8) != 0x7040000000ULL
		|| PioneerReadBeCell(reg + 8, 8) != 0x1000)
		Halt();
	const unsigned char* shift = (const unsigned char*)fdt_getprop(fdt, uart,
		"reg-shift", &length);
	if (!shift || length != 4 || PioneerReadBeCell(shift, 4) != 2)
		Halt();
	const unsigned char* width = (const unsigned char*)fdt_getprop(fdt, uart,
		"reg-io-width", &length);
	if (!width || length != 4 || PioneerReadBeCell(width, 4) != 4)
		Halt();
	Put("HAIKU_KEXEC_LOADER v3 BEGIN (explicit RAM boot profile)\n");
	Hex("hart=", hart);
	Hex("incoming_satp=", satp);
	Hex("incoming_sstatus=", status);
	Hex("incoming_sie=", sie);
	Hex("fdt=", (uintptr_t)fdt);
	if (hart >= 64 || (status & 2) || sie)
		Fail("entry_contract");
	int chosen = fdt_path_offset(fdt, "/chosen");
	const unsigned char* mode = (const unsigned char*)fdt_getprop(fdt, chosen,
		"haiku,pioneer-kexec-probe", &length);
	if (!mode || length != 4 || !PioneerProbeModeValid(PioneerReadBeCell(mode, 4)))
		Fail("missing_explicit_probe_mode");
	uint64_t probeMode = PioneerReadBeCell(mode, 4);
	if (probeMode >= 3) {
		PioneerRootManifest rootInfo;
		const unsigned char* data = (const unsigned char*)fdt_getprop(fdt, chosen,
			"haiku,pioneer-root-v1", &length);
		if (!data || length != 80 || !PioneerRootManifestRead(data, length, &rootInfo))
			Fail("missing_or_invalid_root_manifest");
	}
	const unsigned char* cell = (const unsigned char*)fdt_getprop(fdt, chosen,
		"linux,initrd-start", &length);
	uint64_t start = PioneerReadBeCell(cell, length > 0 ? (size_t)length : 0);
	cell = (const unsigned char*)fdt_getprop(fdt, chosen, "linux,initrd-end", &length);
	uint64_t end = PioneerReadBeCell(cell, length > 0 ? (size_t)length : 0);
	if (end <= start || !PioneerRamRange(start, end - start)
		|| end - start < 1024 || end - start > 8ULL * 1024 * 1024 * 1024)
		Fail("initrd_bounds");
	Hex("initrd_start=", start);
	Hex("initrd_size=", end - start);
	const unsigned char* super = (const unsigned char*)(uintptr_t)(start + 512);
	PioneerBfsHeader info;
	if (!PioneerBfsHeaderCheck(super, 512, end - start, &info))
		Fail("bfs_header");
	Hex("bfs_block_size=", info.block_size);
	Hex("bfs_num_blocks=", info.num_blocks);
	Hex("bfs_root_offset=", info.root_offset);
	const unsigned char* root = (const unsigned char*)(uintptr_t)(start + info.root_offset);
	if (PioneerReadLe32(root) != 0x3bbe0ad9
		|| PioneerReadLe64(root + 4) != PioneerReadLe64(super + 116)
		|| (PioneerReadLe32(root + 20) & 0xf000) != 0x4000)
		Fail("bfs_root_inode");
	Put("bfs_root_inode=OK\n");
	if (probeMode >= 2)
		pioneer_kexec_preload(hart, fdt, start, end - start, probeMode);
	PioneerKexecStop(true, "RAM_header_root_only");
}
#endif
