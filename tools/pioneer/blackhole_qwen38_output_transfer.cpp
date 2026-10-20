/* SPDX-License-Identifier: MIT */
// OFFLINE instruction-model prerequisite ONLY;NO driver/ABI selects it.
// A native caller would need the complete private host/root/BME owner.
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "PioneerQ38OutputTransfer.h"
extern "C" void block_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
struct OutputIO {
	uint32_t ReadReg(unsigned at) { return *Words(0xffb20000+at); }
	void WriteReg(unsigned at,uint32_t value) { *Words(0xffb20000+at)=value; }
	uint32_t ReadL1(unsigned at) { return *Words(at); }
	void Fence() { asm volatile("fence" ::: "memory"); }
};
extern "C" unsigned block_main()
{
	uint32_t d[32]; for (unsigned i=0;i<32;i++) d[i]=Words(0x74000)[i];
	if (!PioneerQ38OutputTransferShape(d)) return 0;
	OutputIO io; PioneerQ38OutputTransfer<OutputIO> transfer(io);
	if (!transfer.Run(d) || !transfer.Complete() || transfer.Unsafe()) block_fault_park();
	for (unsigned i=0;i<32;i++) if (Words(0x74000)[i]!=d[i]) block_fault_park();
	io.Fence(); return d[2];
}
