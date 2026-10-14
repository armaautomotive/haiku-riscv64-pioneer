/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using addr_t = uint64_t;
using uint64 = uint64_t;
using uint32 = uint32_t;
#define B_PRIu32 "u"
#define B_PRIx64 "llx"
enum { SBI_SUCCESS = 0, SBI_HART_STATE_STARTED = 0, SBI_HART_STATE_STOPPED = 1, kPlatformSbi = 1 };
struct sbiret { long error; long value; };
static uint64_t sPioneerHart = 3;
static uint64_t mode;
static unsigned queries;
static int badHart = -1;
static bool mapped;
static bool entered;
static struct KernelArgs {
	uint32 num_cpus = 1;
	struct { uint32 machine_platform; uint32 hartIds[1] = {3}; uint32 plicContexts[1] = {7}; } arch_args;
	struct { addr_t start = 0xffffffc000800000ULL; addr_t size = 0x5000; } cpu_kstack[1];
} gKernelArgs;
static uint64 gTotalMem = 0xbde00000;
struct preloaded_elf64_image { struct { addr_t e_entry = 0xffffffc00010a018ULL; } elf_header; };
struct Stop { bool success; const char* reason; };
struct Enter {};
[[noreturn]] static void PioneerKexecStop(bool success, const char* reason) { throw Stop{success, reason}; }
static uint64 PioneerKexecMode() { return mode; }
static sbiret sbi_hart_get_status(uint32 hart)
{
	queries++;
	if (int(hart) == badHart) return {-1, 0};
	return {SBI_SUCCESS, hart == sPioneerHart ? SBI_HART_STATE_STARTED : SBI_HART_STATE_STOPPED};
}
static void fdt_init_pioneer(uint64_t hart) { assert(hart == sPioneerHart); }
static void smp_init_other_cpus() {}
static void fdt_set_kernel_args() {}
static void convert_kernel_args() {}
static void mmu_init_for_kernel(uint64& satp) { satp = 0x8000000000003b32ULL; }
static void mmu_validate_pioneer_handoff(addr_t entry) { assert(entry == 0xffffffc00010a018ULL); mapped = true; }
static void mock_dprintf(const char*, ...) {}
static void arch_enter_kernel(uint64 satp, KernelArgs* args, addr_t entry, addr_t stack)
{
	assert(mode == 4 && queries == 64 && mapped);
	assert(satp == 0x8000000000003b32ULL && args == &gKernelArgs);
	assert(entry == 0xffffffc00010a018ULL && stack == 0xffffffc000805000ULL);
	entered = true;
	throw Enter{};
}
#define dprintf mock_dprintf
static void invoke_handoff(preloaded_elf64_image* image)
#include "pioneer_handoff.inc"
#undef dprintf

static void expect_stop(uint64 requested, bool success, const char* reason)
{
	mode = requested; queries = 0; mapped = entered = false;
	preloaded_elf64_image image;
	try { invoke_handoff(&image); assert(false); }
	catch (const Stop& stop) { assert(stop.success == success && !std::strcmp(stop.reason, reason)); }
	assert(!entered);
}
int main()
{
	expect_stop(3, true, "kernel_handoff_prepared_SATP_not_switched");
	assert(queries == 64 && mapped);
	expect_stop(2, false, "kernel_execution_not_explicitly_requested");
	badHart = 12;
	expect_stop(4, false, "unexpected_HSM_state_before_handoff");
	assert(!mapped);
	badHart = -1;
	mode = 4; queries = 0; mapped = entered = false;
	preloaded_elf64_image image;
	try { invoke_handoff(&image); assert(false); } catch (const Enter&) {}
	assert(entered);
	std::puts("production handoff: PASS (mode 3 halts; mode 4 alone enters after HSM/mapping gates)");
}
