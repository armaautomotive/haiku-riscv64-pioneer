/* SPDX-License-Identifier: MIT */
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#define CHECK(condition) do { if (!(condition)) { \
	std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
	std::exit(1); } } while (0)

using addr_t = uint64_t;
#define __HAIKU_ADDR_MAX UINT64_MAX
#include "layout.inc"
#include "classification.inc"

// Compile the unmodified production predicate at the native 64-bit width.
// Fabricated address-space-boundary test pointers are never dereferenced.
template<typename Address> struct Range {
	using addr_t = Address;
	using size_t = Address;
	static constexpr Address kMax = std::numeric_limits<Address>::max();
	static constexpr Address kUserTop = USER_TOP;
	static constexpr Address kUserBase = USER_BASE;
#include "range.inc"
	static bool Check(Address address, Address length)
	{
		return validate_memory_range(reinterpret_cast<const void*>(
			uintptr_t(address)), length);
	}
};

template<typename Address> static unsigned CheckRanges()
{
	using R = Range<Address>;
	const Address top = R::kMax;
	const Address userTop = R::kUserTop;
	unsigned checks = 0;
	auto check = [&](Address start, Address length, bool expected) {
		if (R::Check(start, length) != expected) {
			std::fprintf(stderr, "range%zu start=%llx length=%llx expected=%d\n",
				sizeof(Address) * 8, (unsigned long long)start,
				(unsigned long long)length, expected);
			CHECK(false);
		}
		checks++;
	};
	check(Address(top - 4095), 4096, true); // Exact Pioneer top-page regression.
	check(top, 1, true);
	check(top, 2, false);
	check(Address(top - 4095), 4097, false);
	check(0, 0, true);
	check(0, 1, true);
	check(top, 0, true);
	check(userTop, 0, true);
	check(Address(userTop + 1), 0, true);
	check(userTop, 1, true);
	check(userTop, 2, false);
	check(Address(userTop - 4095), 4096, true);
	check(Address(userTop - 4095), 4097, false);
	check(R::kUserBase, top, false); // True wrap, starting at a valid user address.
	check(R::kUserBase, Address(top - R::kUserBase + 1), false);
	check(Address(userTop + 1), Address(top - userTop), true);
	check(R::kUserBase, 0, true);
	check(R::kUserBase, 1, true);
	check(Address(R::kUserBase - 1), 2, false);
	check(0, Address(R::kUserBase + 1), false);
	// Exercise both sides of the top page, including zero and one-byte ranges.
	for (Address distance = 0; distance <= 8192; distance++) {
		check(Address(top - distance), 0, true);
		check(Address(top - distance), Address(distance + 1), true);
		check(Address(top - distance), Address(distance + 2), false);
	}
	return checks;
}

using phys_addr_t = uint64_t;
using phys_size_t = uint64_t;
using status_t = int32_t;
static constexpr status_t B_OK = 0;
static constexpr status_t B_BAD_ADDRESS = -1;
static constexpr size_t B_PAGE_SIZE = 4096;
static constexpr unsigned PAGE_SHIFT = 12;
static constexpr uint64_t kOffset = UINT64_C(0xffffffe012c0b000);
static constexpr uint64_t kLastPhysicalPage = UINT64_C(0x1fed3f4000);
static constexpr uint64_t kTopPage = UINT64_C(0xfffffffffffff000);
static_assert(kOffset + kLastPhysicalPage == kTopPage);
static std::array<uint8_t, B_PAGE_SIZE> sPage;
static unsigned sSetCalls, sCopyCalls;
static status_t sAccessStatus;

#include "range.inc"

// Endpoint shims model fault handling only: no actual top-of-VA host access.
static status_t arch_cpu_user_memset(void* pointer, char value, size_t length)
{
	sSetCalls++;
	CHECK(uintptr_t(pointer) == kTopPage && length == B_PAGE_SIZE);
	if (sAccessStatus == B_OK)
		sPage.fill(uint8_t(value));
	return sAccessStatus;
}
static status_t arch_cpu_user_memcpy(void* to, const void* from, size_t length)
{
	sCopyCalls++;
	if (sAccessStatus != B_OK)
		return sAccessStatus;
	CHECK(length == B_PAGE_SIZE);
	if (uintptr_t(from) == kTopPage)
		std::memcpy(to, sPage.data(), length);
	else {
		CHECK(uintptr_t(to) == kTopPage);
		std::memcpy(sPage.data(), from, length);
	}
	return B_OK;
}
#include "copy.inc"
#include "set.inc"

static void* VirtFromPhys(phys_addr_t address)
{
	CHECK(address == kLastPhysicalPage);
	return reinterpret_cast<void*>(kOffset + address);
}
struct RISCV64VMPhysicalPageMapper {
	status_t MemsetPhysical(phys_addr_t, int, phys_size_t);
};
#define TRACE(...) do {} while (0)
#include "mapper.inc"
#undef TRACE
static status_t vm_memset_physical(phys_addr_t address, int value,
	phys_size_t length)
{
	return RISCV64VMPhysicalPageMapper().MemsetPhysical(address, value, length);
}
struct vm_page { uint64_t physical_page_number; };
struct ClearFailure {};
[[maybe_unused, noreturn]] static void panic(const char*, ...)
{
	throw ClearFailure{};
}
#define B_PRIxPHYSADDR "llx"
#define B_PRId32 "d"
#include "clear.inc"

int main()
{
	unsigned checks = CheckRanges<uint64_t>();
	sPage.fill(0xa5);
	vm_page page{kLastPhysicalPage >> PAGE_SHIFT};
	clear_page(&page); // Actual allocator -> actual RISC-V mapper -> user_memset.
	CHECK(sSetCalls == 1);
	for (uint8_t value : sPage)
		CHECK(value == 0);

	std::array<uint8_t, B_PAGE_SIZE> bytes;
	bytes.fill(0x6b);
	auto* top = reinterpret_cast<void*>(kTopPage);
	CHECK(user_memcpy(top, bytes.data(), bytes.size()) == B_OK);
	bytes.fill(0);
	CHECK(user_memcpy(bytes.data(), top, bytes.size()) == B_OK);
	for (uint8_t value : bytes)
		CHECK(value == 0x6b);
	CHECK(sCopyCalls == 2);
	// Wrapping and boundary-crossing ranges must fail BEFORE endpoint access.
	CHECK(user_memset(top, 0, B_PAGE_SIZE + 1) == B_BAD_ADDRESS);
	CHECK(user_memcpy(top, bytes.data(), B_PAGE_SIZE + 1) == B_BAD_ADDRESS);
	CHECK(user_memcpy(bytes.data(), top, B_PAGE_SIZE + 1) == B_BAD_ADDRESS);
	auto* userEnd = reinterpret_cast<void*>(uintptr_t(USER_TOP));
	CHECK(user_memset(userEnd, 0, 2) == B_BAD_ADDRESS);
	CHECK(user_memcpy(userEnd, bytes.data(), 2) == B_BAD_ADDRESS);
	CHECK(user_memcpy(bytes.data(), userEnd, 2) == B_BAD_ADDRESS);
	CHECK(sSetCalls == 1 && sCopyCalls == 2);

	sAccessStatus = B_BAD_ADDRESS;
	sPage.fill(0x7d);
	CHECK(user_memset(top, 0, B_PAGE_SIZE) == B_BAD_ADDRESS);
	CHECK(user_memcpy(top, bytes.data(), bytes.size()) == B_BAD_ADDRESS);
	CHECK(user_memcpy(bytes.data(), top, bytes.size()) == B_BAD_ADDRESS);
	bool panicked = false;
	try { clear_page(&page); } catch (const ClearFailure&) { panicked = true; }
	CHECK(panicked); // A failed clear must not silently publish uninitialized RAM.
	for (uint8_t value : sPage)
		CHECK(value == 0x7d);
	CHECK(sSetCalls == 3 && sCopyCalls == 4);
	std::printf("PASS %u actual 64-bit range checks; actual top physical page "
		"clear/copy path; overflow/boundary/fault refusal; failed clear panics. "
		"OFFLINE simulated endpoints only.\n", checks);
}
