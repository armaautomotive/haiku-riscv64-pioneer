// OFFLINE-ONLY fullK streamed follow-on. NOT selected by any driver.
#define PIONEER_Q38_STREAM
/* SPDX-License-Identifier: MIT */
// OFFLINE full GGUF quantization-block primitive: sixteen output rows,
// K32(Q8_0) or K256(Q4_K/Q6_K). All2/16 matrix panels and affine accumulation
// stay on this ONE card core. No host intermediate/NoC/DRAM/DMA/PCI access.
// Not a full-K matrix, resident model, decoder, or performance result.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Panel.h"
#ifdef PIONEER_Q38_FAST_PANEL
#include "PioneerQwen38FastPanel.h"
#endif
extern "C" void block_fault_park() __attribute__((noreturn));
static inline volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
static inline void Fence() { asm volatile("fence" ::: "memory"); }
static inline void Emit(uint32_t command) { *Words(0xffe40000)=command; }
static inline uint32_t Bits(float f) { uint32_t u; __builtin_memcpy(&u,&f,4); return u; }
static inline float Float(uint32_t u) { float f; __builtin_memcpy(&f,&u,4); return f; }
static void Drain()
{
	volatile uint32_t* ack=Words(0xffe000fc); *ack=0; (void)*ack; Fence();
	Emit(0xa2101816); Emit(0x4500017e);
	for (unsigned attempt=0;attempt<1000000;attempt++) if (*ack) { Fence(); return; }
	block_fault_park();
}
static void Config(unsigned index,uint32_t value)
{
	volatile uint32_t* target=Words(0xffef0000+index*4); *target=value;
	if (*target!=value) block_fault_park();
}
static void Setup()
{
	// Same source overlay,integer engine,FOUR fidelity phases,converted
	// signed32 readback and full drains as native-qualified local47.
	Emit(0xb2000000); Drain();
	for (unsigned i=1;i<68;i++) Emit(0xb2000000|(i<<16));
	Emit(0xb2020001); Emit(0xb2030001); Emit(0xb2050004); Drain();
	Config(0,0x4231); Config(1,0xa0220000); Config(2,0); Config(3,0x10000);
	Config(6,0); Config(7,0); Config(220,0);
	Config(20,0); Config(24,0xffff); Config(28,0x100); Config(50,0x100);
	Config(56,0x100000); Config(59,0x100);
	Config(64,0x100011); Config(65,16); Config(72,1); Config(73,3); Config(76,0x7070);
	Config(84,0x400040); Config(86,0x100010);
	Config(112,0x100011); Config(113,8); Config(120,1); Config(121,3); Config(124,0x7091);
	Emit(0x5e23fc00); Emit(0x5e41fc00); Drain();
	for (unsigned w=0;w<4;w++) { Words(0x70700)[w]=0; Words(0x70910)[w]=0; }
	for (unsigned w=0;w<64;w++) Words(0x70920)[w]=0;
	Fence();
}
static void Matrix()
{
	Emit(0x3700000f); // reset all RWCs and fidelity to zero per panel
	Emit(0x10180000); Drain(); // clear Dst,NOT an integer sum across unequal scales
	Emit(0x5160000a); Emit(0x5460000f); Emit(0x420000c1); Emit(0x428000c1); Drain();
	for (unsigned phase=0;phase<4;phase++) {
		Emit(0xb20b0000|phase); Drain();
		Emit(phase==3?0x26c00020:0x26000020); Drain();
	}
}
#ifdef PIONEER_Q38_STREAM
// OFFLINE follow-on ONLY. No installed driver selects this image. A future
// kernel owner MUST bind the complete initialized extent and configure ALL
// other NoC command fields from its private bank authority before launch.
// NoC uncertainty parks immediately;never retry/clear a counter or command.
static void FetchBlock(uint32_t address)
{
	if (*Words(0xffb20040) || (*Words(0xffb20240)&255)) block_fault_park();
	*Words(0xffb20000)=address;
	if (*Words(0xffb20000)!=address) block_fault_park();
	uint32_t expected=*Words(0xffb20208)+1;
	*Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		if (!*Words(0xffb20040) && *Words(0xffb20208)==expected && !(*Words(0xffb20240)&255)) { Fence(); return; }
	}
	block_fault_park();
}
#endif
extern "C" unsigned block_main()
{
	const volatile uint32_t* job=Words(0x74000);
	unsigned type=job[1],bytes=job[2],count=type==8?32:256;
#ifdef PIONEER_Q38_STREAM
	unsigned inputs=job[5],blocks=inputs/count;
	uint32_t first=job[6];
	if (job[0]!=0x31533851 || !PioneerQ38PanelTileBytes(type) || bytes!=PioneerQ38PanelTileBytes(type)
		|| job[3] || job[4] || (inputs!=5120 && inputs!=6144 && inputs!=17408) || inputs%count
		|| first<0x04000000 || (first&31) || uint32_t(blocks*bytes)>0xc4000000-first || first>=0xc4000000) return 0;
	for (unsigned b=0;b<blocks;b++) {
		uint32_t magnitude=Words(0x94000)[b]&0x7fffffff;
		if (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return 0;
	}
	const uint32_t inputBase=0xa0000;
#else
	unsigned inputs=count,blocks=1;
	const uint32_t inputBase=0x80000;
	uint32_t scaleBits=job[4],magnitude=scaleBits&0x7fffffff;
	if (job[0]!=0x31423851 || !PioneerQ38PanelTileBytes(type) || bytes!=PioneerQ38PanelTileBytes(type)
		|| job[3] || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000))) return 0;
#endif
	for (unsigned w=0;w<inputs/2;w++) {
		uint32_t word=Words(inputBase)[w];
		if (!PioneerInt8OperandValid(uint16_t(word)) || !PioneerInt8OperandValid(uint16_t(word>>16))) return 0;
	}
	Setup();
	for (unsigned row=0;row<16;row++) Words(0x84000)[row]=0;
	unsigned panels=count/16;
	for (unsigned block=0;block<blocks;block++) {
#ifdef PIONEER_Q38_STREAM
	FetchBlock(first+block*bytes);
	float scale=Float(Words(0x94000)[block]);
#else
	float scale=Float(scaleBits);
#endif
	for (unsigned p=0;p<panels;p++) {
		auto* local=reinterpret_cast<PioneerQ38Panel*>(0x91000);
		if (!
#ifdef PIONEER_Q38_FAST_PANEL
			PioneerQ38FastPanel
#else
			PioneerQ38UnpackPanel
#endif
			(type,reinterpret_cast<const uint8_t*>(0x90000),bytes,p,local)) block_fault_park();
		for (unsigned w=0;w<128;w++) Words(0x70710)[w]=local->operand[w];
		int sum=0;
		for (unsigned w=0;w<8;w++) {
			uint32_t word=Words(inputBase)[block*(count/2)+p*8+w]; Words(0x70920)[w]=word;
			for (unsigned lane=0;lane<2;lane++) {
				uint16_t value=uint16_t(word>>(lane*16)); sum+=int(value&1023)*((value&0x8000)?-1:1);
			}
		}
		(void)Words(0x70710)[127]; (void)Words(0x70920)[7]; Fence();
		Matrix();
		for (unsigned row=0;row<16;row++) {
			static_assert(sizeof(int)==4,"fixed signed32 integer dot");
			int dot=int(Words(0xffbd8800)[row]); if (dot<-262144 || dot>262144) block_fault_park();
			float product=(Float(local->coefficient[row])*scale)*float(dot);
			float correction=(Float(local->minimum[row])*scale)*float(sum);
			float value=Float(Words(0x84000)[row])+(product+correction);
			if ((Bits(value)&0x7f800000)==0x7f800000) block_fault_park();
			Words(0x84000)[row]=Bits(value);
		}
		Fence();
	}
	}
	return blocks*panels;
}
