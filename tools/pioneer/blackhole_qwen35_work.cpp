/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen35DeviceIO.h"
#include "PioneerQwen35RootImageLayout.h"

extern "C" void block_fault_park() __attribute__((noreturn));
extern "C" bool q35_token_core(PioneerQwen35::DeviceIO*,PioneerQwen35::RootScratch*,
	const PioneerQwen35::Request*);

// Only capability needed from the freestanding C runtime. Every caller
// passes aligned whole words. Reject a changed compiler contract on-card.
extern "C" void* memcpy(void* destination,const void* source,__SIZE_TYPE__ size)
{
	if ((reinterpret_cast<__UINTPTR_TYPE__>(destination)
		|reinterpret_cast<__UINTPTR_TYPE__>(source)|size)&3) block_fault_park();
	auto* to=static_cast<volatile uint32_t*>(destination);
	auto* from=static_cast<const volatile uint32_t*>(source);
	for (unsigned i=0;i<size/4;i++) to[i]=from[i];
	return destination;
}

extern "C" void* memset(void* destination,int value,__SIZE_TYPE__ size)
{
	if (value || ((reinterpret_cast<__UINTPTR_TYPE__>(destination)|size)&3)) block_fault_park();
	auto* to=static_cast<volatile uint32_t*>(destination);
	for (unsigned i=0;i<size/4;i++) to[i]=0;
	return destination;
}

extern "C" unsigned q35_work_main()
{
	using namespace PioneerQwen35;
	static_assert(RootScratchAddress+sizeof(RootScratch)==RootScratchEnd, "fixed L1 arena");
	const auto& control=*reinterpret_cast<const volatile RootControl*>(RootControlAddress);
	const auto& layout=*reinterpret_cast<const DeviceLayout*>(RootLayoutAddress);
	auto& status=*reinterpret_cast<volatile RequestProgress*>(RootProgressAddress);
	auto* tokens=reinterpret_cast<volatile uint32_t*>(RootOutputAddress);
	if (control.magic!=RootBootMagic || control.context!=layout.context || !control.requestId
		|| control.eos!=EndOfText || control.endTurn!=EndOfTurn) return 0;
	auto* progressWords=reinterpret_cast<const volatile uint32_t*>(RootProgressAddress);
	for (unsigned i=0;i<sizeof(RequestProgress)/4;i++)
		if (progressWords[i]) return 0;
	for (unsigned i=0;i<6;i++)
		if (control.reserved[i]) return 0;
	const Request request={reinterpret_cast<const uint32_t*>(RootPromptAddress),
		control.promptCount,control.outputLimit,control.context,control.eos,control.endTurn};
	auto* scratch=reinterpret_cast<RootScratch*>(RootScratchAddress);
	const Range rope={control.rope.bank,control.rope.address,control.rope.bytes};
	DeviceIO io(layout,rope,status,tokens);
	if (!io.Ready()) return 0;
	status.requestId=control.requestId;
	asm volatile("fence" ::: "memory");
	status.magic=RootProgressMagic;
	asm volatile("fence" ::: "memory");
	return q35_token_core(&io,scratch,&request)?status.generated:0;
}
