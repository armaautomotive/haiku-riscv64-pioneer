/* SPDX-License-Identifier: MIT */
// Separate immutable image. Reuse the qualified SFPU arithmetic unchanged.
#define attention_main attention_tile_main
#include "blackhole_attention.cpp"
#undef attention_main

static bool NocIdle()
{
	volatile uint32_t* noc=Words(0xffb20000);
	bool idle=true;
	for (unsigned i=0;i<4;i++) idle &= noc[(i*0x800+0x40)/4]==0;
	for (unsigned i=0;i<32;i++) idle &= !(noc[(0x240+i*4)/4]&255);
	return idle;
}

static void Transfer(bool write,uint32_t local,uint32_t remote,unsigned bytes)
{
	volatile uint32_t* noc=Words(0xffb20000);
	if (!NocIdle()) attention_fault_park();
	const unsigned offsets[4]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	uint32_t expected[4];
	for (unsigned i=0;i<4;i++) expected[i]=noc[offsets[i]/4]+(i==1?bytes/64:1);
	volatile uint32_t command[10];
	for (unsigned i=0;i<10;i++) command[i]=0;
	command[0]=write?local:remote; command[2]=write?129U:786U;
	command[3]=write?remote:local; command[5]=write?786U:129U;
	command[7]=write?0x212U:0; command[8]=bytes;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) attention_fault_park();
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
		if (complete && NocIdle()) { Fence(); return; }
	}
	attention_fault_park(); // No retry, reset or output publication.
}

#ifndef PIONEER_RESIDENT_ATTENTION_NAME
#define PIONEER_RESIDENT_ATTENTION_NAME attention_main
#endif
extern "C" unsigned PIONEER_RESIDENT_ATTENTION_NAME()
{
	volatile uint32_t* descriptor=Words(0x74000);
	unsigned tokens=descriptor[1],layer=descriptor[2];
	uint32_t base=descriptor[3];
	// Kernel-derived private bank0 tail; never a caller-supplied address.
	if (descriptor[0]!=0x3152564b || !tokens || tokens>256 || layer>=28
		|| (base&4095) || base<0x04001000 || base>0x82400000) return 0;
	for (unsigned i=0;i<2048;i++)
		if ((Words(0x140000)[i]&0x7fffffff)>0x477fe000) return 0;
	for (unsigned i=2048;i<3072;i++) {
		uint32_t value=Words(0x140000)[i];
		if ((value&0x7c00)==0x7c00 || (value&0x7c000000)==0x7c000000) return 0;
	}
	for (unsigned i=0;i<2048;i++) Words(0x144000)[i]=0x7fc00000;
	Fence();
	for (unsigned i=0;i<2048;i++) if (Words(0x144000)[i]!=0x7fc00000) attention_fault_park();
	// The reused tile function still validates every fetched prefix before
	// arithmetic. Its ordinary descriptor is private, backed-up data too.
	descriptor[0]=0x314e5441; descriptor[2]=1; descriptor[3]=0; Fence();
	for (unsigned group=0;group<8;group++) {
		uint32_t kv=base+layer*0x100000+group*0x20000;
		Transfer(true,0x142000+group*512,kv+(tokens-1)*256,256);
		Transfer(true,0x142100+group*512,kv+0x10000+(tokens-1)*256,256);
		for (unsigned offset=0;offset<tokens*256;offset+=16384) {
			unsigned bytes=tokens*256-offset;
			if (bytes>16384) bytes=16384;
			Transfer(false,0x92000+offset,kv+offset,bytes);
			Transfer(false,0xa2000+offset,kv+0x10000+offset,bytes);
		}
		for (unsigned i=0;i<64;i++)
			if (Words(0x92000)[(tokens-1)*64+i]!=Words(0x142000)[group*128+i]
				|| Words(0xa2000)[(tokens-1)*64+i]!=Words(0x142100)[group*128+i]) attention_fault_park();
		for (unsigned i=0;i<256;i++) Words(0x90000)[i]=Words(0x140000)[group*256+i];
		Fence();
		if (attention_tile_main()!=tokens) attention_fault_park();
		for (unsigned i=0;i<256;i++) {
			uint32_t value=Words(0xb2000)[i];
			if ((value&0x7f800000)==0x7f800000) attention_fault_park();
			Words(0x144000)[group*256+i]=value;
		}
		Fence();
	}
	return tokens;
}
