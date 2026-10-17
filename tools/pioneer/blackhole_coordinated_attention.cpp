/* SPDX-License-Identifier: MIT */
// OFFLINE prerequisite for eight concurrent fixed KV groups. No driver ABI
// loads this image. Reuses the qualified scalar/SFPU arithmetic unchanged.
#define PIONEER_ATTENTION_EXACT_HALF_PRODUCT 1
#define attention_main attention_tile_main
#include "blackhole_attention.cpp"
#undef attention_main
static bool GroupIdle()
{
	bool idle=true; volatile uint32_t* noc=Words(0xffb20000);
	for (unsigned i=0;i<4;i++) idle &= !noc[(i*0x800+0x40)/4];
	for (unsigned i=0;i<32;i++) idle &= !(noc[(0x240+i*4)/4]&255);
	return idle;
}
static unsigned GroupCounter(unsigned i)
{
	static const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
	return offsets[i];
}
static void GroupTransfer(bool write,uint32_t local,uint32_t remote,unsigned bytes,unsigned self)
{
	if (!GroupIdle() || (bytes!=256 && (bytes<512 || bytes>16384 || (bytes&255)))
		|| ((remote^local)&31)) attention_fault_park();
	volatile uint32_t* noc=Words(0xffb20000); volatile uint32_t command[10]; uint32_t expected[4];
	const unsigned offsets[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	for (unsigned i=0;i<4;i++) expected[i]=noc[offsets[i]/4]+(i==1?bytes/64:1);
	for (unsigned i=0;i<10;i++) command[i]=0;
	command[0]=write?local:remote; command[2]=write?self:786;
	command[3]=write?remote:local; command[5]=write?786:self;
	command[7]=write?0x212U:0; command[8]=bytes;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) attention_fault_park();
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
		if (complete && GroupIdle()) { Fence(); return; }
	}
	attention_fault_park();
}
extern "C" unsigned attention_main()
{
	volatile uint32_t* descriptor=Words(0x74000),*parameters=Words(0x70500),*proof=Words(0x90600);
	unsigned tokens=descriptor[1],layer=descriptor[2],group=descriptor[3];
	// Only peers1..8 on column1,rows3..10. No supplied routes/addresses:
	// the descriptor must match its fixed group-to-worker assignment.
	unsigned self=1+((group+3)<<6);
	if (descriptor[0]!=0x31475441 || !tokens || tokens>256 || layer>=28 || group>=8
		|| parameters[0] || parameters[1] || parameters[2] || parameters[3]!=1
		|| parameters[4]!=self || parameters[5] || parameters[6] || parameters[7]!=0x31475441 || !GroupIdle()) return 0;
	for (unsigned i=0;i<256;i++) if ((Words(0x90000)[i]&0x7fffffff)>0x477fe000) return 0;
	for (unsigned i=0;i<128;i++) { uint32_t v=Words(0x90400)[i];
		if ((v&0x7c00)==0x7c00 || (v&0x7c000000)==0x7c000000) return 0; }
	for (unsigned i=0;i<8;i++) proof[i]=*Words(0xffb20000+GroupCounter(i));
	for (unsigned i=8;i<24;i++) proof[i]=0;
	uint32_t kv=0x82400000+layer*0x100000+group*0x20000;
	GroupTransfer(true,0x90400,kv+(tokens-1)*256,256,self);
	GroupTransfer(true,0x90500,kv+0x10000+(tokens-1)*256,256,self);
	for (unsigned offset=0;offset<tokens*256;offset+=16384) {
		unsigned bytes=tokens*256-offset; if (bytes>16384) bytes=16384;
		GroupTransfer(false,0x92000+offset,kv+offset,bytes,self);
		GroupTransfer(false,0xa2000+offset,kv+0x10000+offset,bytes,self);
	}
	for (unsigned i=0;i<64;i++) if (Words(0x92000)[(tokens-1)*64+i]!=Words(0x90400)[i]
		|| Words(0xa2000)[(tokens-1)*64+i]!=Words(0x90500)[i]) attention_fault_park();
	descriptor[0]=0x314e5441; descriptor[2]=1; descriptor[3]=0; Fence();
	if (attention_tile_main()!=tokens || !GroupIdle()) attention_fault_park();
	for (unsigned i=0;i<8;i++) proof[8+i]=*Words(0xffb20000+GroupCounter(i));
	proof[16]=tokens; proof[17]=layer; proof[18]=group; proof[19]=0x31475441;
	Fence(); return tokens;
}
