/* SPDX-License-Identifier: MIT */
// OFFLINE persistent peer prerequisite. NO driver installs/launches it.
// Same exact mixed-quant math;new private descriptor/doorbell/status owners.
// Root must freeze inputs+descriptor until ALL participants finish,then
// verify physical PC/reset/source/command/counters through qualified NoC
// reads. This image alone is NOT a coordinator,graph or launch permission.
#define PIONEER_Q38_SHARED_ROWS
#define PIONEER_Q38_PERSISTENT_ROWS
#define block_main q38_compute_once
#include "blackhole_qwen38_rows.cpp"
#undef block_main
extern "C" unsigned block_peer_wait(unsigned previous);
extern "C" void block_peer_stopped() __attribute__((noreturn));
static unsigned Counter(unsigned c)
{
#ifdef PIONEER_Q38_VECTOR_PANEL
	static const unsigned offset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
#else
	const unsigned offset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
#endif
	return *Words(0xffb20000+offset[c]);
}
static bool IdleAll()
{
	bool okay=true;
	for (unsigned c=0;c<4;c++) okay &= !*Words(0xffb20040+c*0x800);
	for (unsigned c=0;c<32;c++) okay &= !(*Words(0xffb20240+c*4)&255);
	return okay;
}
static unsigned SourceOffset(unsigned word)
{
	if (!word) return 0x100;
	if (word<=6) return 0x118+(word-1)*4;
	if (word<=12) return 0x130+(word-7)*4;
	return word==13?0x150:word==14?0x154:0x16c;
}
static bool SourceMatches(const uint32_t (&source)[16])
{
	bool okay=true;
	for (unsigned w=0;w<16;w++) okay &= *Words(0xffb20000+SourceOffset(w))==source[w];
	return okay;
}
static unsigned Route(unsigned worker)
{
#ifdef PIONEER_Q38_VECTOR_PANEL
	static const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
#else
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
#endif
	unsigned column=0,row=worker;
	while (row>=10) { row-=10; column++; }
	if (worker>=120 || column>=12) block_fault_park();
	return columns[column]|((row+2)<<6);
}
extern "C" void q38_persistent_main()
{
	const volatile uint32_t* boot=Words(0x74040);
	unsigned worker=boot[1],self=boot[2],source=boot[3];
	if (boot[0]!=0x32503851 || !worker || worker>=120 || self!=Route(worker)
		|| !source || boot[4] || boot[5] || boot[6] || boot[7] || *Words(0x74100)
		|| !IdleAll() || *Words(0xffb20100)!=source || *Words(0xffb20014)!=self) block_fault_park();
	// Future kernel MUST prove all16 original words before this capture
	// and compare them again at END. No caller may supply/alter this seal.
	uint32_t sealedSource[16]; for (unsigned w=0;w<16;w++) sealedSource[w]=*Words(0xffb20000+SourceOffset(w));
	if (sealedSource[0]!=source || !SourceMatches(sealedSource)) block_fault_park();
	for (unsigned i=0;i<16;i++) Words(0x74140)[i]=0;
	Words(0x74140)[3]=0x32503851; Fence();
	unsigned previous=0;
	for (;;) {
		unsigned sequence=block_peer_wait(previous);
		const volatile uint32_t* job=Words(0x74000);
		uint32_t snapshot[16]; for (unsigned i=0;i<16;i++) snapshot[i]=job[i];
		if (!sequence || sequence>=0x7fffffff || sequence!=previous+1 || snapshot[10]!=0x32503851
			|| snapshot[11]!=worker || snapshot[12]!=sequence || (snapshot[13]!=1 && snapshot[13]!=2)
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
			|| snapshot[14]>1 || (snapshot[14] && snapshot[13]!=1)
#else
			|| snapshot[14]
#endif
			|| snapshot[15] || !IdleAll() || *Words(0xffb20100)!=source
			|| !SourceMatches(sealedSource) || *Words(0xffb20014)!=self || *Words(0x74100)!=sequence) block_fault_park();
		if (snapshot[13]==2) {
			Words(0x74140)[0]=sequence; Words(0x74140)[1]=sequence; Words(0x74140)[4]=2;
			Fence(); block_peer_stopped(); // final PC audit BEFORE any reset/restore
		}
		if (snapshot[0]!=0x31483851 || snapshot[3] || snapshot[4] || snapshot[9]!=1
			|| !PioneerQ38PanelTileBytes(snapshot[1]) || snapshot[2]!=
#ifdef PIONEER_Q38_PREPARED_Q4_AFFINE
			(snapshot[1]==12?PioneerQ38PreparedQ4::Bytes:PioneerQ38PanelTileBytes(snapshot[1]))
#else
			(
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
				snapshot[14] && snapshot[1]==12?PioneerQ38PreparedQ4::Bytes:
#endif
#ifdef PIONEER_Q38_PREDECODED_Q4
				snapshot[14] && snapshot[1]==12?PioneerQ38PredecodedQ4::Bytes:
#endif
				PioneerQ38PanelTileBytes(snapshot[1]))
#endif
			|| (snapshot[5]!=5120 && snapshot[5]!=6144 && snapshot[5]!=17408)
			|| !snapshot[7] || snapshot[7]>16) block_fault_park();
		uint32_t before[8]; for (unsigned c=0;c<8;c++) before[c]=Counter(c);
		Words(0x74140)[0]=sequence; Words(0x74140)[4]=1; Fence();
		unsigned panels=q38_compute_once();
		unsigned blocks=snapshot[5]>>(snapshot[1]==8?5:8),count=snapshot[7]*snapshot[5]/16;
		unsigned reads=snapshot[7]*blocks+(snapshot[5]*2+16383)/16384+1;
		unsigned flits=snapshot[7]*blocks*snapshot[2]/64+(snapshot[5]*2+((blocks*4+63)&~63U))/64;
		if (panels!=count || !IdleAll() || !SourceMatches(sealedSource) || *Words(0xffb20014)!=self) block_fault_park();
		uint32_t after[8];
		for (unsigned c=0;c<8;c++) {
			after[c]=Counter(c);
			if (uint32_t(after[c]-before[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) block_fault_park();
		}
		for (unsigned i=0;i<16;i++) if (snapshot[i]!=job[i]) block_fault_park();
		for (unsigned i=0;i<8;i++) if (boot[i]!=(i==0?0x32503851:i==1?worker:i==2?self:i==3?source:0)) block_fault_park();
		for (unsigned i=0;i<16;i++) Words(0x83f00)[i]=i<8?before[i]:after[i-8];
		Words(0x83f00)[16]=panels; Words(0x83f00)[17]=sequence; Words(0x83f00)[18]=worker;
		Words(0x83f00)[19]=0x32503851;
		for (unsigned i=0;i<10;i++) Words(0x83f00)[20+i]=*Words(0xffb20000+i*4);
		Words(0x83f00)[30]=source; Words(0x83f00)[31]=self;
		for (unsigned i=32;i<48;i++) Words(0x83f00)[i]=sealedSource[i-32];
		for (unsigned i=48;i<64;i++) Words(0x83f00)[i]=0;
		Fence(); // proof+ALL outputs precede finished;root never mutates early
		Words(0x74140)[4]=0; Words(0x74140)[1]=sequence; Fence();
		previous=sequence;
	}
}
