/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define PIONEER_KEXEC_PROBE 1
#define ELFCLASS64 2
#define EM_RISCV 243
#define B_PRIu32 "u"
using uint32 = uint32_t;

struct preloaded_image { unsigned elf_class = 2; preloaded_image* next = nullptr; };
struct preloaded_elf64_image : preloaded_image {
	struct { unsigned e_machine = EM_RISCV; uint64_t e_entry = 0xffffffc002000000; } elf_header;
};
template<typename T> struct FakePointer {
	T* value = nullptr;
	operator T*() const { return value; }
	T* operator->() const { return value; }
	T* Pointer() const { return value; }
};
static struct {
	FakePointer<preloaded_image> kernel_image;
	preloaded_image* preloaded_images = nullptr;
} gKernelArgs;
static bool active;
static bool normalReached;
static bool PioneerKexecPreloadActive() { return active; }
struct Stop { bool success; const char* reason; };
[[noreturn]] static void PioneerKexecStop(bool success, const char* reason)
{
	throw Stop{success, reason};
}
static void mock_dprintf(const char*, ...) {}
#define dprintf mock_dprintf
static void invoke_start_kernel()
#include "pioneer_preload_guard.inc"
#undef dprintf

static void expect_stop(bool success, const char* reason)
{
	normalReached = false;
	try {
		invoke_start_kernel();
		assert(!"active preload unexpectedly returned");
	} catch (const Stop& stop) {
		assert(stop.success == success && !std::strcmp(stop.reason, reason));
	}
	assert(!normalReached);
}

int main()
{
	active = false;
	normalReached = false;
	invoke_start_kernel();
	assert(normalReached);
	active = true;
	expect_stop(false, "missing_ELF64_kernel");
	preloaded_elf64_image kernel;
	gKernelArgs.kernel_image.value = &kernel;
	kernel.elf_class = 1;
	expect_stop(false, "missing_ELF64_kernel");
	kernel.elf_class = 2;
	kernel.elf_header.e_machine = 62;
	expect_stop(false, "wrong_kernel_architecture");
	kernel.elf_header.e_machine = EM_RISCV;
	expect_stop(false, "no_boot_images");
	preloaded_image module;
	gKernelArgs.preloaded_images = &module;
	expect_stop(true, "kernel_and_boot_images_preloaded");
	module.next = &module;
	expect_stop(false, "module_list_bounds");
	std::puts("production preload guard: PASS (active mode never reaches kernel path)");
}
