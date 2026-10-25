/* SPDX-License-Identifier: MIT */
// NEW OFFLINE op7 returning convolution service;NOT selected by any driver.
// Independent persistent-loop variant:old qualified peer source stays exact.
// The unchanged FFN envelope is NOT compatibility:all new sections need new
// immutable pins and joint native/ROOT qualification before any device use.
#include "PioneerQ38ResidentOperatorImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#ifdef PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE
#include "PioneerQ38ResidentGdnOperatorImageRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnServiceShape.h"
extern "C" void q38_resident_persistent_gdn(const uint32_t*,const uint32_t*,unsigned,unsigned);
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentConvServiceShape.h"
extern "C" void q38_resident_persistent_conv(const uint32_t*,const uint32_t*,unsigned,unsigned);
#define PIONEER_Q38_SFPU_ACCUMULATOR
#define PIONEER_Q38_SFPU_INTEGER
#define PIONEER_Q38_AFFINE_RESIDENT
#define PIONEER_Q38_MATRIX_PHASE_REUSE
#define PIONEER_Q38_VECTOR_FIXED_FORMAT
#define PIONEER_Q38_AFFINE_REUSE
#define PIONEER_Q38_VECTOR_PANEL
#define PIONEER_Q38_VECTOR_INLINE
/* SPDX-License-Identifier: MIT */
// OFFLINE persistent peer prerequisite. NO driver installs/launches it.
// Same exact mixed-quant math;new private descriptor/doorbell/status owners.
// Root must freeze inputs+descriptor until ALL participants finish,then
// verify physical PC/reset/source/command/counters through qualified NoC
// reads. This image alone is NOT a coordinator,graph or launch permission.
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
#if PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE != 1 || !defined(PIONEER_Q38_FFN_PERSISTENT) || PIONEER_Q38_FFN_PERSISTENT != 1 || !defined(PIONEER_Q38_PROJECTION_PERSISTENT) || PIONEER_Q38_PROJECTION_PERSISTENT != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
#error Exact distinct OFFLINE gated input image role,never native/CPU/Work
#endif
#if defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
#error Offline QOI1 peer never shares native owner/DEBUG roles
#endif
#endif
#define PIONEER_Q38_SHARED_ROWS
#define PIONEER_Q38_PERSISTENT_ROWS
#define block_main q38_compute_once
#include "blackhole_qwen38_rows.cpp"
#undef block_main
#ifdef PIONEER_Q38_FFN_PERSISTENT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentShape.h"
enum {Q38PersistentMarker=TT_Q38_FFN_PERSISTENT_MARKER};
extern "C" void q38_ffn_persistent_activation(const uint32_t*,const uint32_t*,unsigned,unsigned);
extern "C" void q38_ffn_persistent_matrix_begin(const uint32_t*,unsigned);
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
#if PIONEER_Q38_PROJECTION_PERSISTENT != 1
#error Exact separate projection persistent image role required
#endif
#endif
#else
enum {Q38PersistentMarker=0x32503851};
#endif
extern "C" unsigned block_peer_wait(unsigned previous);
extern "C" void block_peer_stopped() __attribute__((noreturn));
__attribute__((noinline,section(".text.pioneer_q38_resident_service")))
static unsigned Counter(unsigned c)
{
#ifdef PIONEER_Q38_VECTOR_PANEL
	static const unsigned offset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
#else
	const unsigned offset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
#endif
	return *Words(0xffb20000+offset[c]);
}
#if defined(PIONEER_Q38_WORK_LOCAL_Q4) || defined(PIONEER_Q38_WORK_BATCH_RAW_INPUT) || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_WORK_PEER_PHASE_PROFILE)
__attribute__((noinline,section(".text.pioneer_q38_batch4_stage")))
#endif
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
__attribute__((noinline,optimize("Os")))
#endif
__attribute__((noinline,section(".text.pioneer_q38_resident_service")))
static bool IdleAll()
{
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
 for(unsigned c=0;c<4;c++)if(*Words(0xffb20040+c*0x800))return false;
 for(unsigned c=0;c<32;c++)if(*Words(0xffb20240+c*4)&255)return false;
 return true;
#else
	bool okay=true;
	for (unsigned c=0;c<4;c++) okay &= !*Words(0xffb20040+c*0x800);
	for (unsigned c=0;c<32;c++) okay &= !(*Words(0xffb20240+c*4)&255);
	return okay;
#endif
}
static unsigned SourceOffset(unsigned word)
{
	if (!word) return 0x100;
	if (word<=6) return 0x118+(word-1)*4;
	if (word<=12) return 0x130+(word-7)*4;
	return word==13?0x150:word==14?0x154:0x16c;
}
#if defined(PIONEER_Q38_WORK_LOCAL_Q4) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) || defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE) || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_WORK_PEER_PHASE_PROFILE)
__attribute__((noinline,section(".text.pioneer_q38_batch4_stage")))
#endif
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
__attribute__((noinline,optimize("Os")))
#endif
__attribute__((noinline,section(".text.pioneer_q38_resident_service")))
static bool SourceMatches(const uint32_t (&source)[16])
{
#ifdef PIONEER_Q38_PROJECTION_PERSISTENT
 for(unsigned w=0;w<16;w++)if(*Words(0xffb20000+SourceOffset(w))!=source[w])return false;
 return true;
#else
	bool okay=true;
	for (unsigned w=0;w<16;w++) okay &= *Words(0xffb20000+SourceOffset(w))==source[w];
	return okay;
#endif
}
__attribute__((noinline,section(".text.pioneer_q38_resident_service")))
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
	if (boot[0]!=Q38PersistentMarker || !worker || worker>=120 || self!=Route(worker)
		|| !source || boot[4] || boot[5] || boot[6] || boot[7] || *Words(0x74100)
		|| !IdleAll() || *Words(0xffb20100)!=source || *Words(0xffb20014)!=self) block_fault_park();
	// Future kernel MUST prove all16 original words before this capture
	// and compare them again at END. No caller may supply/alter this seal.
	uint32_t sealedSource[16]; for (unsigned w=0;w<16;w++) sealedSource[w]=*Words(0xffb20000+SourceOffset(w));
	if (sealedSource[0]!=source || !SourceMatches(sealedSource)) block_fault_park();
	for (unsigned i=0;i<16;i++) Words(0x74140)[i]=0;
	Words(0x74140)[3]=Q38PersistentMarker; Fence();
	unsigned previous=0;
	for (;;) {
		unsigned sequence=block_peer_wait(previous);
		const volatile uint32_t* job=Words(0x74000);
		uint32_t snapshot[16]; for (unsigned i=0;i<16;i++) snapshot[i]=job[i];
		if (!sequence || sequence>=0x7fffffff || sequence!=previous+1 || snapshot[10]!=Q38PersistentMarker
			|| snapshot[11]!=worker || snapshot[12]!=sequence ||
#ifdef PIONEER_Q38_FFN_PERSISTENT
#ifdef PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE
            (snapshot[13]-1>4 && snapshot[13]!=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION && snapshot[13]!=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION)
#elif defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE)
            (snapshot[13]-1>4 && snapshot[13]!=TT_Q38_RESIDENT_CONV_SERVICE_OPERATION)
#elif defined(PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE)
            (snapshot[13]!=1 && snapshot[13]!=2 && snapshot[13]!=6)
#elif defined(PIONEER_Q38_PROJECTION_PERSISTENT)
            (snapshot[13]-1>4)
#else
			(snapshot[13]-1>3)
#endif
#else
			(snapshot[13]!=1 && snapshot[13]!=2)
#endif
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
			|| snapshot[14]>1 || (snapshot[14] && snapshot[13]!=1)
#else
			|| snapshot[14]
#endif
			|| snapshot[15] || !IdleAll() || *Words(0xffb20100)!=source
			|| !SourceMatches(sealedSource)
#ifndef PIONEER_Q38_FFN_PERSISTENT
			|| *Words(0xffb20014)!=self
#endif
			|| *Words(0x74100)!=sequence) block_fault_park();
		if (snapshot[13]==2) {
#ifdef PIONEER_Q38_FFN_PERSISTENT
			for(unsigned i=0;i<10;i++)if(snapshot[i])block_fault_park();
#endif
			Words(0x74140)[0]=sequence; Words(0x74140)[1]=sequence; Words(0x74140)[4]=2;
			Fence(); block_peer_stopped(); // final PC audit BEFORE any reset/restore
		}
#ifdef PIONEER_Q38_FFN_PERSISTENT
		if(snapshot[13]>=TT_Q38_FFN_PERSISTENT_ACTIVATION) {
#ifdef PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE
			if(snapshot[13]==TT_Q38_RESIDENT_GDN_SERVICE_OPERATION)q38_resident_persistent_gdn(snapshot,sealedSource,worker,self);
			else
#endif
			if(snapshot[13]==TT_Q38_RESIDENT_CONV_SERVICE_OPERATION)q38_resident_persistent_conv(snapshot,sealedSource,worker,self);
			else q38_ffn_persistent_activation(snapshot,sealedSource,worker,self);
			previous=sequence;continue; // preserve sequence/Tensix state;NO reset
		}
#endif
		if (snapshot[0]!=0x31483851 || snapshot[3] || snapshot[4] || snapshot[9]!=1
#ifdef PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE
            || snapshot[1]!=8 || snapshot[5]!=6144
#endif
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
#ifdef PIONEER_Q38_FFN_PERSISTENT
		q38_ffn_persistent_matrix_begin(sealedSource,self);
#endif
		uint32_t before[8]; for (unsigned c=0;c<8;c++) before[c]=Counter(c);
		Words(0x74140)[0]=sequence; Words(0x74140)[4]=1; Fence();
		unsigned panels=q38_compute_once();
		unsigned blocks=snapshot[5]>>(snapshot[1]==8?5:8),count=snapshot[7]*snapshot[5]/16;
		unsigned reads=snapshot[7]*
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
			(snapshot[14]?blocks>>(snapshot[1]==8?2:1):blocks)
#else
			blocks
#endif
			+(snapshot[5]*2+16383)/16384+1;
		unsigned flits=snapshot[7]*blocks*snapshot[2]/64+(snapshot[5]*2+((blocks*4+63)&~63U))/64;
		if (panels!=count || !IdleAll() || !SourceMatches(sealedSource) || *Words(0xffb20014)!=self) block_fault_park();
		uint32_t after[8];
		for (unsigned c=0;c<8;c++) {
			after[c]=Counter(c);
			if (uint32_t(after[c]-before[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) block_fault_park();
		}
		for (unsigned i=0;i<16;i++) if (snapshot[i]!=job[i]) block_fault_park();
		for (unsigned i=0;i<8;i++) if (boot[i]!=(i==0?unsigned(Q38PersistentMarker):i==1?worker:i==2?self:i==3?source:0)) block_fault_park();
		for (unsigned i=0;i<16;i++) Words(0x83f00)[i]=i<8?before[i]:after[i-8];
		Words(0x83f00)[16]=panels; Words(0x83f00)[17]=sequence; Words(0x83f00)[18]=worker;
		Words(0x83f00)[19]=Q38PersistentMarker;
		for (unsigned i=0;i<10;i++) Words(0x83f00)[20+i]=*Words(0xffb20000+i*4);
		Words(0x83f00)[30]=source; Words(0x83f00)[31]=self;
		for (unsigned i=32;i<48;i++) Words(0x83f00)[i]=sealedSource[i-32];
		for (unsigned i=48;i<64;i++) Words(0x83f00)[i]=0;
		Fence(); // proof+ALL outputs precede finished;root never mutates early
		Words(0x74140)[4]=0; Words(0x74140)[1]=sequence; Fence();
		previous=sequence;
	}
}
