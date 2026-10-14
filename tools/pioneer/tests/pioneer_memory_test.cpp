/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <libfdt.h>
#include "../../../src/system/boot/platform/riscv/pioneer_kexec.h"
#include "../../../src/system/boot/platform/riscv/pioneer_memory.h"

struct Stop { const char* reason; };
[[noreturn]] static void PioneerKexecStop(bool success, const char* reason)
{
	assert(!success); throw Stop{reason};
}
#include "pioneer_memory_parser.inc"

using uint64 = uint64_t;
using uint32 = uint32_t;
using addr_t = uint64_t;
struct addr_range { uint64_t start, size; };
static constexpr uint64_t KERNEL_TOP = UINT64_MAX;
static constexpr int B_OK = 0;
#define ASSERT_ALWAYS(value) assert(value)
#define PIONEER_KEXEC_PROBE 1
static struct {
	addr_range physical_memory_range[32];
	unsigned num_physical_memory_ranges;
	struct { addr_range physMap; } arch_args;
} gKernelArgs;
static const PioneerMemoryPlan* sPlan;
static const PioneerMemoryPlan* PioneerKexecMemoryPlan() { return sPlan; }
struct Pte {
	bool isRead, isWrite;
	uint64_t val = 6;
};
struct Mapping { uint64_t virt, phys, size; };
static std::vector<Mapping> mappings;
static void MapRange(uint64_t virt, uint64_t phys, uint64_t size, uint64_t flags,
	bool device, bool reserve)
{
	assert(flags == 6 && size && !(size & 4095) && !device && !reserve);
	mappings.push_back({virt, phys, size});
}
static int insert_virtual_allocated_range(uint64_t start, uint64_t size)
{
	assert(start == gKernelArgs.arch_args.physMap.start);
	assert(size == gKernelArgs.arch_args.physMap.size);
	assert(start + size == 0); // Physical map ends at the top of Sv39.
	return 0;
}
static void SetupPhysicalMemoryMap()
#include "pioneer_memory_mapping.inc"

using status_t = int;
static constexpr uint64_t B_PAGE_SIZE = 4096;
#define B_PRId32 "d"
static unsigned reservations;
static int reservationStatus;
static status_t vm_block_address_range(const char* name, void* start, uint64_t size)
{
	assert(!strcmp(name, "physical memory map"));
	assert((uintptr_t)start == gKernelArgs.arch_args.physMap.start);
	assert(size == gKernelArgs.arch_args.physMap.size - 4096);
	assert((uintptr_t)start + size == UINT64_MAX - 4095);
	reservations++;
	return reservationStatus;
}
[[noreturn]] static void panic(const char*, ...) { throw Stop{"reservation_failed"}; }
static void debug_early_boot_message(const char*) {}
static void ReserveMap()
{
	auto* args = &gKernelArgs;
#include "pioneer_memory_reservation.inc"
}

static void CheckPlan(const PioneerMemoryPlan& plan)
{
	for (unsigned i = 0; i < plan.count; i++) {
		const auto& r = plan.ranges[i];
		assert(r.size && !((r.start | r.size) & 4095));
		assert(PioneerMemoryContains(plan, r.start, r.size));
		assert(!PioneerMemoryContains(plan, r.start + r.size - 1, 2));
		if (i) assert(plan.ranges[i - 1].start + plan.ranges[i - 1].size < r.start);
	}
}
static void PutLimit(void* fdt, unsigned gib)
{
	assert(fdt_setprop_u32(fdt, fdt_path_offset(fdt, "/chosen"),
		"haiku,pioneer-memory-gib", gib) == 0);
}
static void ExpectStop(const void* fdt, const char* reason)
{
	PioneerMemoryPlan plan{};
	try { fdt_pioneer_memory_plan(fdt, 0x1fed400000, 0x12c00000, plan); assert(false); }
	catch (const Stop& stop) { assert(!strcmp(reason, stop.reason)); }
}
int main(int argc, char** argv)
{
	PioneerMemoryPlan plan{};
	assert(!PioneerMemoryPlanInit(plan, 0) && !PioneerMemoryPlanInit(plan, 3));
	assert(!PioneerMemoryPlanInit(plan, 127) && !PioneerMemoryPlanInit(plan, UINT64_MAX));
	assert(PioneerMemoryPlanInit(plan, 128));
	assert(!PioneerMemoryContains(plan, 0, 1));
	assert(!PioneerMemoryContains(plan, UINT64_MAX, 2));
	assert(!PioneerMemoryExclude(plan, 0, 0));
	assert(!PioneerMemoryExclude(plan, UINT64_MAX - 1, 2));
	assert(!PioneerMemoryExclude(plan, UINT64_MAX - 4095, 1));
	assert(PioneerMemoryExclude(plan, 0, 0x2200000)); // Already excluded firmware.
	assert(PioneerMemoryExclude(plan, 0x40000001, 4096));
	assert(!PioneerMemoryContains(plan, 0x40000000, 1));
	assert(!PioneerMemoryContains(plan, 0x40001000, 1));
	assert(PioneerMemoryContains(plan, 0x40002000, 1));
	assert(PioneerMemoryExclude(plan, 0x40000000, 0x10000000)); // Overlapping exclusion.
	assert(PioneerMemoryExclude(plan, 0x1fed3f5000, 0x12c0b000)); // FDT/initrd.
	CheckPlan(plan);
	assert(PioneerMemoryContains(plan, 0xc0000000, 0x40000000)); // Actual FDT RAM, not a PC hole.
	assert(!PioneerMemoryContains(plan, 0x1fed400000, 1));
	for (unsigned i = 0; i < plan.count; i++)
		gKernelArgs.physical_memory_range[i] = {plan.ranges[i].start, plan.ranges[i].size};
	gKernelArgs.num_physical_memory_ranges = plan.count;
	sPlan = &plan;
	SetupPhysicalMemoryMap();
	ReserveMap(); assert(reservations == 1);
	reservationStatus = -1;
	try { ReserveMap(); assert(false); }
	catch (const Stop& stop) { assert(!strcmp(stop.reason, "reservation_failed")); }
	reservationStatus = 0;
	assert(mappings.size() == plan.count);
	for (unsigned i = 0; i < plan.count; i++) {
		const auto& m = mappings[i]; const auto& r = plan.ranges[i];
		assert(m.phys == r.start && m.size == r.size);
		assert(m.virt - m.phys == mappings[0].virt - mappings[0].phys);
	}
	gKernelArgs.arch_args.physMap.size = UINT64_C(0xbde00000);
	ReserveMap(); assert(reservations == 2); // Default small map not changed.
	sPlan = nullptr; mappings.clear();
	gKernelArgs.num_physical_memory_ranges = 1;
	gKernelArgs.physical_memory_range[0] = {0x2200000, 0xbde00000};
	SetupPhysicalMemoryMap();
	assert(mappings.size() == 1 && mappings[0].phys == 0x2200000
		&& mappings[0].size == 0xbde00000);
	// A full fixed table refuses another split instead of writing past it.
	assert(PioneerMemoryPlanInit(plan, 8));
	for (unsigned i = 0; i < 31; i++)
		assert(PioneerMemoryExclude(plan, 0x3000000 + i * 8192, 4096));
	assert(plan.count == 32);
	assert(!PioneerMemoryExclude(plan, 0x4000000, 4096));
	CheckPlan(plan);
	assert(PioneerMemoryExclude(plan, 0, UINT64_C(0x2000000000)) && !plan.count);
	assert(argc == 2);
	FILE* file = fopen(argv[1], "rb"); assert(file);
	assert(fseek(file, 0, SEEK_END) == 0);
	long size = ftell(file); assert(size > 0 && size < 2 * 1024 * 1024);
	assert(fseek(file, 0, SEEK_SET) == 0);
	std::vector<unsigned char> original(size), fdt(size + 8192);
	assert(fread(original.data(), 1, size, file) == (size_t)size && fclose(file) == 0);
	auto reset = [&] { assert(fdt_open_into(original.data(), fdt.data(), fdt.size()) == 0); };
	reset();
	assert(!fdt_pioneer_memory_plan(fdt.data(), 0x1fed400000, 0x12c00000, plan));
	for (unsigned gib : {8, 16, 32, 64, 128}) {
		reset(); PutLimit(fdt.data(), gib);
		assert(fdt_pioneer_memory_plan(fdt.data(), 0x1fed400000, 0x12c00000, plan));
		CheckPlan(plan);
		assert(plan.ranges[0].start == 0x2200000);
		assert(PioneerMemoryContains(plan, 0x100000000, 4096));
		assert(!PioneerMemoryContains(plan, uint64_t(gib) << 30, 1));
		assert(!PioneerMemoryContains(plan, 0x1fed400000, 4096));
		assert(!PioneerMemoryContains(plan, 0x40000000, 4096)); // Actual saved memreserve.
	}
	reset(); PutLimit(fdt.data(), 127); ExpectStop(fdt.data(), "invalid_requested_memory_limit");
	reset(); PutLimit(fdt.data(), 128);
	assert(fdt_del_node(fdt.data(), fdt_path_offset(fdt.data(), "/memory@3")) == 0);
	ExpectStop(fdt.data(), "incomplete_128GiB_memory_layout");
	reset(); PutLimit(fdt.data(), 128);
	assert(fdt_setprop_string(fdt.data(), fdt_path_offset(fdt.data(), "/memory@2"), "status", "disabled") == 0);
	ExpectStop(fdt.data(), "disabled_memory_bank");
	reset(); PutLimit(fdt.data(), 128);
	assert(fdt_setprop_u32(fdt.data(), fdt_path_offset(fdt.data(), "/memory@1"), "reg", 0) == 0);
	ExpectStop(fdt.data(), "invalid_memory_bank_reg");
	reset(); PutLimit(fdt.data(), 128);
	assert(fdt_add_subnode(fdt.data(), fdt_path_offset(fdt.data(), "/reserved-memory"), "unknown") >= 0);
	ExpectStop(fdt.data(), "unknown_dynamic_memory_reservation");
	puts("PASS: production Pioneer FDT memory parser, exclusions, fixed bounds and sparse linear mapping; default unchanged");
}
