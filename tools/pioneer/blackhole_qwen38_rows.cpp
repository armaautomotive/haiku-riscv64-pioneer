// OFFLINE-ONLY multi-output-row follow-on. NOT selected by any driver.
#define PIONEER_Q38_STREAM
/* SPDX-License-Identifier: MIT */
// OFFLINE full GGUF quantization-block primitive: sixteen output rows,
// K32(Q8_0) or K256(Q4_K/Q6_K). All2/16 matrix panels and affine accumulation
// stay on this ONE card core. No host intermediate/NoC/DRAM/DMA/PCI access.
// Not a full-K matrix, resident model, decoder, or performance result.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Panel.h"
#if defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) && (!defined(PIONEER_Q38_WORK_DIRECT_SRCB) || !defined(PIONEER_Q38_WORK_AFFINE_REPLAY) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_WORK_BATCH_RAW_INPUT))
#error Frontend barrier is a separate OFFLINE original-layout direct-SrcB peer,not a batch/format mix
#endif
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
#ifndef PIONEER_Q38_WORK_BATCH4_FETCH
#error Direct batch input requires the complete separate bounded-batch peer
#endif
#define PIONEER_Q38_RAW_BASE rawBase
#define PIONEER_Q38_RAW_ARGUMENT ,rawBase
#else
#define PIONEER_Q38_RAW_BASE 0x90000
#define PIONEER_Q38_RAW_ARGUMENT
#endif
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
#if !defined(PIONEER_Q38_WORK_DIRECT_SRCB) || !defined(PIONEER_Q38_WORK_DOT_ZERO_ELISION) || !defined(PIONEER_Q38_WORK_AFFINE_REPLAY) || !defined(PIONEER_Q38_PERSISTENT_ROWS) || defined(PIONEER_Q38_NATIVE_Q8) || defined(PIONEER_Q38_PREDECODED_Q4) || defined(PIONEER_Q38_Q4_GROUP32)
#error Batch4 is a separate OFFLINE original-layout Work peer experiment
#endif
#include "PioneerQ38Batch4Fetch.h"
#endif
#if defined(PIONEER_Q38_WORK_DIRECT_SRCB) && (!defined(PIONEER_Q38_SFPU_ACCUMULATOR) || !defined(PIONEER_Q38_INLINE_CONFIG) || !defined(PIONEER_Q38_WORK_ZERO_MINIMUM) || !defined(PIONEER_Q38_WORK_INPUT_SUM_CACHE) || !defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK) || !defined(PIONEER_Q38_VECTOR_FIXED_FORMAT) || !defined(PIONEER_Q38_SHARED_ROWS) || defined(PIONEER_Q38_BFLOAT_PANEL) || defined(PIONEER_Q38_NATIVE_Q8) || defined(PIONEER_Q38_PREDECODED_Q4) || defined(PIONEER_Q38_Q4_GROUP32) || defined(PIONEER_Q38_PREPARED_Q4_BLOCK) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE))
#error Separate offline direct-SrcB requires original-TC fixed integer Work/shared-input/cache roles
#endif
#ifdef PIONEER_Q38_MATRIX_PHASE_REUSE
#include "PioneerQ38MatrixPhases.h"
#endif
#ifdef PIONEER_Q38_Q4_TWO_PHASE
#if !defined(PIONEER_Q38_MATRIX_PHASE_REUSE) || !defined(PIONEER_Q38_VECTOR_FIXED_FORMAT) || defined(PIONEER_Q38_BFLOAT_PANEL)
#error Q4 two-phase candidate requires the fixed integer vector peer
#endif
#include "PioneerQ38Q4Phases.h"
#endif
#ifdef PIONEER_Q38_BFLOAT_PANEL
#include "PioneerQ38BFloatPanel.h"
#endif
#ifdef PIONEER_Q38_SFPU_ACCUMULATOR
#include "PioneerQ38SfpuAccumulator.h"
#endif
#ifdef PIONEER_Q38_CACHED_AFFINE
#include "PioneerQ38CachedAffine.h"
#endif
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
#if !defined(PIONEER_Q38_INLINE_CONFIG) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE)
#error Block-affine cache requires the separate ordinary-layout inline Work peer
#endif
#include "PioneerQ38BlockAffineDst.h"
#endif
#if defined(PIONEER_Q38_BLOCK_SCALE_REUSE) && !defined(PIONEER_Q38_BLOCK_AFFINE_DST)
#error Block scale reuse requires the separately qualified block metadata lifetime
#endif
#if defined(PIONEER_Q38_Q4_GROUP32) && (!defined(PIONEER_Q38_BLOCK_SCALE_REUSE) || !defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK))
#error OFFLINE group32 requires the complete separately qualified scale reuse Work chain
#endif
#ifdef PIONEER_Q38_PREDECODED_Q4
#if !defined(PIONEER_Q38_BLOCK_SCALE_REUSE) || defined(PIONEER_Q38_Q4_GROUP32) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE)
#error Predecoded-Q4 is a separate OFFLINE ordinary-rounding scale-cache variant
#endif
#include "PioneerQ38PredecodedQ4.h"
#include "PioneerQ38DirectQ4Unpack.h"
#endif
#if defined(PIONEER_Q38_PREDECODED_Q4_STICKY) && !defined(PIONEER_Q38_PREDECODED_Q4)
#error Sticky direct-Q4 is ONLY the separate OFFLINE predecoded-layout study
#endif
#ifdef PIONEER_Q38_NATIVE_Q8
#if !defined(PIONEER_Q38_BLOCK_SCALE_REUSE) || defined(PIONEER_Q38_Q4_GROUP32) || defined(PIONEER_Q38_PREDECODED_Q4)
#error Native SM7 Q8 is a separate OFFLINE ordinary-rounding scale-cache variant
#endif
#include "PioneerQ38DirectNativeQ8.h"
#endif
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
#if !defined(PIONEER_Q38_BLOCK_SCALE_REUSE) || defined(PIONEER_Q38_Q4_GROUP32) || defined(PIONEER_Q38_PREPARED_Q4_AFFINE) || defined(PIONEER_Q38_PREDECODED_Q4) || defined(PIONEER_Q38_NATIVE_Q8)
#error Prepared block metadata is a separate OFFLINE original-rounding Work-only layout
#endif
#include "PioneerQ38PreparedQ4.h"
#endif
#if defined(PIONEER_Q38_SFPU_INTEGER) && (!defined(PIONEER_Q38_SFPU_ACCUMULATOR) || !defined(PIONEER_Q38_VECTOR_FIXED_FORMAT) || defined(PIONEER_Q38_BFLOAT_PANEL))
#error Integer SFPU accumulator requires fixed integer operands, NEVER BF16
#endif
#if defined(PIONEER_Q38_PIPELINED_MATRIX) && (!defined(PIONEER_Q38_SFPU_INTEGER) || !defined(PIONEER_Q38_Q4_TWO_PHASE))
#error Queued matrix phases require the separately qualified exact integer/SFPU candidate
#endif
#if defined(PIONEER_Q38_STATIC_VECTOR) && !defined(PIONEER_Q38_PIPELINED_MATRIX)
#error Static vector emission requires the separate queued integer/SFPU candidate
#endif
#if defined(PIONEER_Q38_CACHED_AFFINE) && !defined(PIONEER_Q38_STATIC_VECTOR)
#error Cached affine candidate requires the separate static integer/SFPU variant
#endif
#if defined(PIONEER_Q38_REPLAY_OPERAND) && !defined(PIONEER_Q38_CACHED_AFFINE)
#error Operand replay requires the separate cached-affine integer/SFPU variant
#endif
#if defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK) && (!defined(PIONEER_Q38_REPLAY_OPERAND) || !defined(PIONEER_Q38_PERSISTENT_ROWS))
#error Work dot-check isolation requires the replayed persistent integer/SFPU peer
#endif
#ifdef PIONEER_Q38_QUEUED_MATRIX_ENTRY
#if !defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK) || !defined(PIONEER_Q38_PIPELINED_MATRIX)
#error Queued matrix entry requires the latest fixed integer/SFPU Work peer
#endif
#include "PioneerQ38QueuedMatrixEntry.h"
#endif
#if defined(PIONEER_Q38_WORK_INPUT_SUM_CACHE) && (!defined(PIONEER_Q38_QUEUED_MATRIX_ENTRY) || !defined(PIONEER_Q38_SFPU_INTEGER))
#error Work input sums require the separately qualified integer queued persistent peer
#endif
#if defined(PIONEER_Q38_WORK_ZERO_MINIMUM) && !defined(PIONEER_Q38_WORK_INPUT_SUM_CACHE)
#error Zero-minimum Work requires the separate input-sum-cache integer peer
#endif
#if defined(PIONEER_Q38_NATIVE_OPERAND_STORE) && !defined(PIONEER_Q38_WORK_ZERO_MINIMUM)
#error Native operand store requires the complete separately tested zero-minimum integer peer
#endif
#if defined(PIONEER_Q38_WORK_DEFER_AFFINE) && !defined(PIONEER_Q38_NATIVE_OPERAND_STORE)
#error Deferred affine requires the separate native operand integer peer
#endif
#if defined(PIONEER_Q38_INLINE_CONFIG) && !defined(PIONEER_Q38_WORK_DEFER_AFFINE)
#error Inline config emission requires the separately qualified deferred-affine peer
#endif
#ifdef PIONEER_Q38_PREPARED_Q4_AFFINE
#ifndef PIONEER_Q38_INLINE_CONFIG
#error Prepared-Q4 format requires the separate inline-config integer peer
#endif
#include "PioneerQ38PreparedQ4.h"
#endif
#ifdef PIONEER_Q38_VECTOR_PANEL
#include "PioneerQ38AffineOnly.h"
#include "PioneerQ38VectorDstInit.h"
#include "PioneerQ38VectorUnpack.h"
#ifdef PIONEER_Q38_VECTOR_FIXED_FORMAT
#include "PioneerQ38VectorFixedFormat.h"
#ifdef PIONEER_Q38_VECTOR_AFFINE
#error Fixed raw format and experimental FP32 metadata unpack are separate unqualified variants
#endif
#endif
#ifdef PIONEER_Q38_AFFINE_REUSE
#ifdef PIONEER_Q38_AFFINE_RESIDENT
#include "PioneerQ38AffineResident.h"
#ifdef PIONEER_Q38_VECTOR_AFFINE
#error Resident metadata is read-only between panels and cannot use the mutable FP32 staging experiment
#endif
#else
#include "PioneerQ38AffineReuse.h"
#endif
#endif
#ifdef PIONEER_Q38_VECTOR_AFFINE
#include "PioneerQ38VectorAffineData.h"
#endif
#endif
#ifdef PIONEER_Q38_FAST_PANEL
#include "PioneerQwen38FastPanel.h"
#endif
extern "C" void block_fault_park() __attribute__((noreturn));
static inline volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
static inline void Fence() { asm volatile("fence" ::: "memory"); }
#ifdef PIONEER_Q38_VECTOR_INLINE
// Separate OFFLINE candidate:constant Tensix instructions use the SAME
// rotate-left-two inline encoding as qualified native F32/Q8 programs.
// Dynamic fields retain the old instruction-buffer store. No approximation
// or replay;the strict actual-instruction test decodes BOTH issue paths.
static __attribute__((always_inline)) inline void Emit(uint32_t command)
{
	if (__builtin_constant_p(command)) asm volatile(".word %0" :: "i"((command<<2)|(command>>30)) : "memory");
	else *Words(0xffe40000)=command;
}
#else
static inline void Emit(uint32_t command) { *Words(0xffe40000)=command; }
#endif
static inline uint32_t Bits(float f) { uint32_t u; __builtin_memcpy(&u,&f,4); return u; }
static inline float Float(uint32_t u) { float f; __builtin_memcpy(&f,&u,4); return f; }
static void Drain()
{
	volatile uint32_t* ack=Words(0xffe000fc); *ack=0; (void)*ack; Fence();
	Emit(0xa2101816); Emit(0x4500017e);
	for (unsigned attempt=0;attempt<1000000;attempt++) if (*ack) { Fence(); return; }
	block_fault_park();
}
#if defined(PIONEER_Q38_VECTOR_AFFINE) || defined(PIONEER_Q38_AFFINE_REUSE)
__attribute__((noinline))
#endif
static void Config(unsigned index,uint32_t value)
{
	volatile uint32_t* target=Words(0xffef0000+index*4); *target=value;
	if (*target!=value) block_fault_park();
}
#ifdef PIONEER_Q38_Q4_TWO_PHASE
// Keep the candidate inside the unchanged 8KiB view. Only this once/job
// fixed startup sequence is compacted; the hot compute loop remains O2.
__attribute__((noinline,optimize("Os")))
#endif
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
__attribute__((section(".text.pioneer_q38_replay_job_setup")))
#endif
static void Setup()
{
	// Same source overlay,integer engine,FOUR fidelity phases,converted
	// signed32 readback and full drains as native-qualified local47.
	Emit(0xb2000000); Drain();
	for (unsigned i=1;i<68;i++) Emit(0xb2000000|(i<<16));
	Emit(0xb2020001); Emit(0xb2030001); Emit(0xb2050004); Drain();
#ifdef PIONEER_Q38_AFFINE_REUSE
	// SAME exact ordered writes/readbacks. Compact immutable tables keep
	// the separate persistent candidate inside the unchanged8KiB code view.
	static const uint8_t indices[]={0,1,2,3,6,7,220,20,24,28,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
#ifdef PIONEER_Q38_BFLOAT_PANEL
	// Separate format contract:forced BF16 A/B,FP32 natural readback,
	// fixed unsigned raw-A UNPACK,ordinary BF16 B UNPACK. Same25 writes.
	static const uint32_t values[]={0x42b5,0x203c8000,0,0,0,0,0,0,0xffff,0x100,0x100,0x100000,0x100,
		0x10001e,16,14,0x13,0x7070,0x400040,0x100010,0x100015,8,5,3,0x7091};
#else
	static const uint32_t values[]={0x4231,0xa0220000,0,0x10000,0,0,0,0,0xffff,0x100,0x100,0x100000,0x100,
		0x100011,16,1,3,0x7070,0x400040,0x100010,0x100011,8,1,3,0x7091};
#endif
	static_assert(sizeof(indices)==sizeof(values)/sizeof(values[0]),"exact unchanged configuration pairs");
	for (unsigned i=0;i<sizeof(indices);i++) Config(indices[i],values[i]);
#else
	Config(0,0x4231); Config(1,0xa0220000); Config(2,0); Config(3,0x10000);
	Config(6,0); Config(7,0); Config(220,0);
	Config(20,0); Config(24,0xffff); Config(28,0x100); Config(50,0x100);
	Config(56,0x100000); Config(59,0x100);
	Config(64,0x100011); Config(65,16); Config(72,1); Config(73,3); Config(76,0x7070);
	Config(84,0x400040); Config(86,0x100010);
	Config(112,0x100011); Config(113,8); Config(120,1); Config(121,3); Config(124,0x7091);
#endif
	Emit(0x5e23fc00); Emit(0x5e41fc00); Drain();
	for (unsigned w=0;w<4;w++) { Words(0x70700)[w]=0; Words(0x70910)[w]=0; }
	for (unsigned w=0;w<64;w++) Words(0x70920)[w]=0;
	Fence();
}
#ifdef PIONEER_Q38_VECTOR_PANEL
struct VectorPanelEmitter {
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	unsigned blockAffineRow=0;
	unsigned BlockAffineRow() const { return blockAffineRow; }
#endif
#ifdef PIONEER_Q38_SFPU_ACCUMULATOR
	__attribute__((always_inline)) inline
#endif
	void Op(uint32_t value) { Emit(value); }
#ifdef PIONEER_Q38_INLINE_CONFIG
	// Emit the SAME ordered volatile write+readback in the hot caller.
	// No register/value/readback/drain change,only RV call/address overhead.
	__attribute__((always_inline)) inline void Config(unsigned index,uint32_t value) {
		volatile uint32_t* target=Words(0xffef0000+index*4); *target=value;
		if (*target!=value) block_fault_park();
	}
#else
	void Config(unsigned index,uint32_t value) { ::Config(index,value); }
#endif
	void Drain() { ::Drain(); }
};
#endif
#if defined(PIONEER_Q38_BFLOAT_PANEL) || defined(PIONEER_Q38_Q4_TWO_PHASE)
static void Matrix(unsigned type
#ifdef PIONEER_Q38_Q4_GROUP32
	,bool continuation=false
#endif
)
#else
static void Matrix()
#endif
{
#ifdef PIONEER_Q38_QUEUED_MATRIX_ENTRY
	VectorPanelEmitter entry;
#ifdef PIONEER_Q38_Q4_GROUP32
	if (continuation) PioneerQ38QueuedMatrixContinue(entry); else
#endif
	PioneerQ38QueuedMatrixEntry(entry);
#else
	Emit(0x3700000f); // reset all RWCs and fidelity to zero per panel
#ifdef PIONEER_Q38_VECTOR_PANEL
	// Native59-qualified:clear BOTH physical halves of FP32 rows32..47.
	// The former UseDst32b partial clear left the running dot uncleared on
	// this card. Keep decoded/metadata rows live;never clear ALL here.
	Emit(0x10080004); Emit(0x10080005); Drain();
#else
	Emit(0x10180000); Drain(); // original scalar path unchanged
#endif
	Emit(0x5160000a); Emit(0x5460000f);
#ifndef PIONEER_Q38_VECTOR_PANEL
	Emit(0x420000c1);
#endif
	Emit(0x428000c1); Drain();
#endif
#ifdef PIONEER_Q38_MATRIX_PHASE_REUSE
	VectorPanelEmitter e;
#ifdef PIONEER_Q38_BFLOAT_PANEL
	// The bounded integer operands are EXACT in BF16. Q8 uses phases0/1;
	// Q4<=15 and Q6[-32,31] fit phase0 exactly. EACH MVMUL still drains.
	if (type==8) { e.Op(0x26004020); e.Drain(); }
	e.Op(0x26c04020); e.Drain();
#else
#ifdef PIONEER_Q38_Q4_TWO_PHASE
#ifdef PIONEER_Q38_PIPELINED_MATRIX
	PioneerQ38QueuedPhases(e,type);
#else
	PioneerQ38Q4Phases(e,type);
#endif
#else
	PioneerQ38MatrixPhases(e);
#endif
#endif
#else
	for (unsigned phase=0;phase<4;phase++) {
		Emit(0xb20b0000|phase); Drain();
		Emit(phase==3?0x26c00020:0x26000020); Drain();
	}
#endif
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
#ifdef PIONEER_Q38_SHARED_ROWS
// FUTURE PRIVATE kernel descriptor only. This image has NO native handler.
// Retarget ONLY an idle,read-only command to fixed root L1,self destination
// unchanged. Never touch upper address/control/doorbell fields after UNKNOWN.
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
__attribute__((noinline,section(".text.pioneer_q38_batch4_stage")))
#endif
static void ReadCommand(uint32_t address,uint32_t route,uint32_t destination,unsigned bytes)
{
	if (*Words(0xffb20040) || (*Words(0xffb20240)&255)) block_fault_park();
	const unsigned offsets[]={0,8,12,32};
	const uint32_t values[]={address,route,destination,bytes};
	for (unsigned w=0;w<4;w++) {
		*Words(0xffb20000+offsets[w])=values[w];
		if (*Words(0xffb20000+offsets[w])!=values[w]) block_fault_park();
	}
}
static void SharedInputs(unsigned inputs,unsigned blocks)
{
	// Root logical worker(1,2),fixed immutable vector/scales until EVERY peer
	// completes. <=16KiB messages,64B alignment,padded scale tail is zero.
	for (unsigned at=0;at<inputs*2;) {
		unsigned bytes=inputs*2-at; if (bytes>16384) bytes=16384;
		ReadCommand(0xa0000+at,1|(2<<6),0xa0000+at,bytes);
		FetchBlock(0xa0000+at); at+=bytes;
	}
#ifdef PIONEER_Q38_PERSISTENT_ROWS
	// Future root outputs occupy80000..9dc00. Keep ROOT scales outside that
	// range while preserving this peer's LOCAL94000 arithmetic layout.
	ReadCommand(0xab000,1|(2<<6),0x94000,(blocks*4+63)&~63U);
	FetchBlock(0xab000);
#else
	ReadCommand(0x94000,1|(2<<6),0x94000,(blocks*4+63)&~63U);
	FetchBlock(0x94000);
#endif
}
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
// Cold prefix,original saved window. No asynchronous fetch/counter reset.
// A failed command parks INSIDE FetchBlock;never copy/issue after UNKNOWN.
static __attribute__((noinline,optimize("Os"),section(".text.pioneer_q38_batch4_stage")))
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
uint32_t
#else
void
#endif
Batch4Stage(
 unsigned type,unsigned inputs,unsigned groups,unsigned group,unsigned block,uint32_t first,uint32_t route) {
 PioneerQ38Batch4FetchPlan p;
 if(!PioneerQ38Batch4Fetch(type,inputs,groups,group,block,first,p))block_fault_park();
 if(p.issue){ReadCommand(p.source,route,p.destination,p.bytes);FetchBlock(p.source);}
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
 // Only this private selector follows SAME known-complete FetchBlock.
 // Decoder/metadata and UNPACK use this same tile without a RISC copy.
 return p.copySource;
#else
 // Consume one original tile in original order. Scratch bytes are from
 // this SAME known-complete batch;no CPU/host conversion or new format.
 for(unsigned w=0;w<p.copyWords;w++)Words(0x90000)[w]=Words(p.copySource)[w];
 (void)Words(0x90000)[p.copyWords-1];Fence();
#endif
}
#endif
#endif
#endif
#ifdef PIONEER_Q38_SFPU_ACCUMULATOR
// Private fixed-frame caller ONLY:sealed loops p=0..last per fetched block,
// raw90000..90d40,metadata91000..91280,stack6e000..70000 are DISJOINT.
// This avoids generic pointer/lifetime bookkeeping for these static facts.
// ALL finite metadata checks and atomic16-row publication still apply.
static bool FixedAffine(unsigned type,unsigned bytes,unsigned panel,uint32_t (&base)[16]
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
 ,uint32_t rawBase
#endif
)
{
	const uint8_t* raw=reinterpret_cast<const uint8_t*>(PIONEER_Q38_RAW_BASE);
	auto* out=reinterpret_cast<PioneerQ38Panel*>(0x91000);
#ifdef PIONEER_Q38_CACHED_AFFINE
	if (type==8) return (panel&1) || PioneerQ38AffineOnly(8,raw,bytes,panel,out);
	if (type!=14) return false; // Q4 belongs ONLY to the prepared cached path
#else
	if (type!=14) return (panel&1) || PioneerQ38AffineOnly(type,raw,bytes,panel,out);
#endif
	if (!panel) for (unsigned row=0;row<16;row++) {
		uint16_t half=uint16_t(PioneerQ38PanelByte(raw,208*16+row)|(PioneerQ38PanelByte(raw,209*16+row)<<8));
		if (!PioneerQ38BinaryScale(half,1,base[row])) return false;
	}
	uint32_t stage[16];
	for (unsigned row=0;row<16;row++) {
		unsigned v=PioneerQ38PanelByte(raw,(192+panel)*16+row);
		stage[row]=Bits(Float(base[row])*float(int(v)-int((v&128)*2)));
		if ((stage[row]&0x7f800000)==0x7f800000) return false;
	}
	for (unsigned row=0;row<16;row++) { out->coefficient[row]=stage[row]; out->minimum[row]=0; }
	return true;
}
#endif
extern "C" unsigned block_main()
{
	const volatile uint32_t* job=Words(0x74000);
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
	const bool skipDotCheck=job[14]==1; // descriptor frozen/validated by persistent caller
#endif
	unsigned type=job[1],bytes=job[2],count=type==8?32:256;
#ifdef PIONEER_Q38_STREAM
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
	// Fixed supported quant blocks:keep division OUT of the RISC hot path.
	unsigned inputs=job[5],blocks=inputs>>(type==8?5:8),groups=job[7];
#else
	unsigned inputs=job[5],blocks=inputs/count,groups=job[7];
#endif
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
	// OFFLINE candidate ONLY. The SAME immutable input is reused by every
	// output group. Group0 computes the ORIGINAL exact signed integer sums;
	// later groups reuse them within THIS job only. NEVER persist a cache
	// hit across jobs. Checked/single-group jobs retain the original path.
	const bool cacheSums=skipDotCheck && groups>1
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		&& type==12
#endif
		;
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
	const bool zeroMinimum=skipDotCheck && type!=12;
#endif
	// PRIVATE peer scratch95000..96100,inside the existing saved/owned
	// 6e000..b0000 view. Disjoint from raw90000..90d40,metadata91000..91280,
	// scales94000..94880,results84000..84400,inputa0000..a8800 and code/stack.
	// Integer-only gate excludes the older BF16 cache in this same region.
	static_assert(0x95000+(17408/16)*4==0x96100,"bounded private sum cache");
#endif
	uint32_t first=job[6];
	if (job[0]!=
#ifdef PIONEER_Q38_SHARED_ROWS
		0x31483851
#else
#ifdef PIONEER_Q38_VECTOR_PANEL
		0x31563851 // separate OFFLINE vector descriptor,NOT rows/persistent ABI
#else
		0x31523851
#endif
#endif
		|| !PioneerQ38PanelTileBytes(type) || bytes!=
#ifdef PIONEER_Q38_PREPARED_Q4_AFFINE
		(type==12?PioneerQ38PreparedQ4::Bytes:PioneerQ38PanelTileBytes(type))
#else
		(
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
			skipDotCheck && type==12?PioneerQ38PreparedQ4::Bytes:
#endif
#ifdef PIONEER_Q38_PREDECODED_Q4
			skipDotCheck && type==12?PioneerQ38PredecodedQ4::Bytes:
#endif
			PioneerQ38PanelTileBytes(type))
#endif
		|| job[3] || job[4] || !groups || groups>16 || (inputs!=5120 && inputs!=6144 && inputs!=17408) || inputs%count
		|| first<0x04000000 || (first&31) || uint32_t(groups*blocks*bytes)>0xc4000000-first || first>=0xc4000000) return 0;
#ifdef PIONEER_Q38_SHARED_ROWS
	// Both fields are PRIVATE kernel-derived,not new caller capabilities.
	// Root uses0,peers1. Destination XY in command remains kernel-owned.
	if (job[9]>1) return 0;
	if (job[9]) SharedInputs(inputs,blocks);
	ReadCommand(first,job[8],0x90000,bytes);
#endif
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
#ifdef PIONEER_Q38_BFLOAT_PANEL
		// Peer-private95000..9d800 (max34816B),disjoint from raw block,
		// metadata,results,scales and original authenticated shared input.
		// Overwrite EVERY word each job before ANY matrix/cache read.
		uint16_t lo,hi;
		if (!PioneerQ38BFloatActivation(uint16_t(word),lo) || !PioneerQ38BFloatActivation(uint16_t(word>>16),hi)) return 0;
		Words(0x95000)[w]=uint32_t(lo)|(uint32_t(hi)<<16);
#else
		if (!PioneerInt8OperandValid(uint16_t(word)) || !PioneerInt8OperandValid(uint16_t(word>>16))) return 0;
#endif
	}
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
	// Separate OFFLINE Work experiment. UNPACK reads eight rows,although
	// only its first row contributes to the exported projection. The
	// following rows are validated inputs;zero EVERY possible tail byte
	// EACH JOB before any read. Max end a88e0 remains in the ORIGINAL
	// saved private6e000..b0000 extent. No new NoC/source/host address.
	const bool directSourceB=skipDotCheck;
	if(directSourceB) {
		for(unsigned word=0;word<56;word++)Words(inputBase)[inputs/2+word]=0;
		(void)Words(inputBase)[inputs/2+55];Fence();
	}
#endif
	Setup();
#ifdef PIONEER_Q38_VECTOR_PANEL
	// Match native59's known startup state BEFORE defining vector rows.
	Emit(0x10180000); Drain();
	VectorPanelEmitter initEmitter; PioneerQ38VectorDstInit(initEmitter,
#ifdef PIONEER_Q38_BFLOAT_PANEL
		true
#else
		false
#endif
	,
#if defined(PIONEER_Q38_VECTOR_AFFINE) || defined(PIONEER_Q38_SFPU_ACCUMULATOR)
		true
#else
		false
#endif
	,
#ifdef PIONEER_Q38_VECTOR_FIXED_FORMAT
		true
#else
		false
#endif
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
		,skipDotCheck && type==12
#endif
	);
#ifdef PIONEER_Q38_NATIVE_Q8
	if(skipDotCheck && type==8)PioneerQ38DirectNativeQ8Setup(initEmitter);
#endif
#endif
#ifdef PIONEER_Q38_MATRIX_PHASE_REUSE
	VectorPanelEmitter phasesEmitter;
#ifdef PIONEER_Q38_Q4_TWO_PHASE
	PioneerQ38Q4PhasesSetup(phasesEmitter,type);
#else
	PioneerQ38MatrixPhasesSetup(phasesEmitter);
#endif
#endif
	for (unsigned row=0;row<groups*16;row++) Words(0x84000)[row]=0;
	unsigned panels=count/16;
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
	// Choose between TWO exact,immutable code targets ONCE per job. No
	// caller address/function pointer or per-panel Work/type branch.
#ifdef PIONEER_Q38_WORK_DEFER_AFFINE
	auto accumulate=&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,false,false>;
	if (skipDotCheck) accumulate=type!=12
		?&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,true,true>
		:&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,false,true>;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	if (skipDotCheck && type==12) accumulate=&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,false,true,true>;
#endif
#else
	const auto accumulate=skipDotCheck && type!=12
		?&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,true>
		:&PioneerQ38SfpuAccumulator<VectorPanelEmitter,true,false>;
#endif
#endif
	for (unsigned group=0;group<groups;group++) {
	for (unsigned block=0;block<blocks;block++) {
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
	uint32_t rawBase=0x90000;
#endif
#ifdef PIONEER_Q38_STREAM
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
	if(skipDotCheck)
#ifdef PIONEER_Q38_WORK_BATCH_RAW_INPUT
	rawBase=
#endif
	Batch4Stage(type,inputs,groups,group,block,first,job[8]);else
#endif
	FetchBlock(first+(group*blocks+block)*bytes);
	float scale=Float(Words(0x94000)[block]);
#else
	float scale=Float(scaleBits);
#endif
#ifdef PIONEER_Q38_AFFINE_REUSE
#if defined(PIONEER_Q38_SFPU_ACCUMULATOR) && !defined(PIONEER_Q38_CACHED_AFFINE)
	uint32_t affineBase[16];
#else
#ifdef PIONEER_Q38_CACHED_AFFINE
#ifndef PIONEER_Q38_PREPARED_Q4_AFFINE
	PioneerQ38CachedAffine affine;
#endif
	uint32_t affineBase[16]; // preserve the original Q6 path
#else
#ifdef PIONEER_Q38_AFFINE_RESIDENT
	PioneerQ38AffineResident affine;
#else
	PioneerQ38AffineReuse affine;
#endif
#endif
#ifdef PIONEER_Q38_CACHED_AFFINE
#ifndef PIONEER_Q38_PREPARED_Q4_AFFINE
	if (type==12 &&
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
		!skipDotCheck &&
#endif
		!affine.Prepare(12,reinterpret_cast<const uint8_t*>(PIONEER_Q38_RAW_BASE),bytes
#ifdef PIONEER_Q38_PREDECODED_Q4
		,skipDotCheck
#endif
	)) block_fault_park();
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	if (type==12 && skipDotCheck) {
		VectorPanelEmitter blockEmitter; blockEmitter.Drain(); // prior deferred store completes BEFORE format change
#ifdef PIONEER_Q38_PREDECODED_Q4_STICKY
		PioneerQ38DirectQ4Restore(blockEmitter); // once per block BEFORE FP32 metadata loads
#endif
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
		if(!PioneerQ38PreparedQ4::Block(reinterpret_cast<const uint8_t*>(0x90000),bytes,reinterpret_cast<volatile uint32_t*>(0x97000),256))block_fault_park();
#else
		if (!affine.BlockQ4(reinterpret_cast<PioneerQ38Panel*>(
#ifdef PIONEER_Q38_PREDECODED_Q4
			0x92000 // NEW tile90000..91100 must NOT alias original temporary91000
#else
			0x91000
#endif
		),reinterpret_cast<volatile uint32_t*>(0x97000),256)) block_fault_park();
#endif
		Fence();
		if (!PioneerQ38BlockAffineDst(blockEmitter
#ifdef PIONEER_Q38_BLOCK_SCALE_REUSE
			,Bits(scale)
#endif
		)) block_fault_park();
#ifdef PIONEER_Q38_PREDECODED_Q4_STICKY
		PioneerQ38DirectQ4Setup(blockEmitter); // native SrcA remains selected for ALL16 panels
#endif
	}
#endif
#endif
#else
	if (!affine.Prepare(type,reinterpret_cast<const uint8_t*>(0x90000),bytes)) block_fault_park();
#endif
#endif
#endif
#ifdef PIONEER_Q38_Q4_GROUP32
	int precedingSum=0;
	const bool pairedQ4=skipDotCheck && type==12;
#endif
	for (unsigned p=0;p<panels;p++) {
#if !defined(PIONEER_Q38_SFPU_ACCUMULATOR) || defined(PIONEER_Q38_CACHED_AFFINE)
		auto* local=reinterpret_cast<PioneerQ38Panel*>(0x91000);
#endif
#ifdef PIONEER_Q38_AFFINE_REUSE
#if defined(PIONEER_Q38_SFPU_ACCUMULATOR) && !defined(PIONEER_Q38_CACHED_AFFINE)
		if (!FixedAffine(type,bytes,p,affineBase PIONEER_Q38_RAW_ARGUMENT)) block_fault_park();
#else
#ifdef PIONEER_Q38_CACHED_AFFINE
#ifdef PIONEER_Q38_PREPARED_Q4_AFFINE
		if (type==12) {
			if (!(p&1) && !PioneerQ38PreparedQ4::Panel(reinterpret_cast<const uint8_t*>(0x90000),bytes,p,local)) block_fault_park();
		} else if (!FixedAffine(type,bytes,p,affineBase PIONEER_Q38_RAW_ARGUMENT)) block_fault_park();
#else
		if (!(type==12?
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
			(skipDotCheck || affine.Panel(p,local))
#else
			affine.Panel(p,local)
#endif
			:FixedAffine(type,bytes,p,affineBase PIONEER_Q38_RAW_ARGUMENT))) block_fault_park();
#endif
#else
		if (!affine.Panel(p,local)) block_fault_park();
#endif
#endif
#else
		if (!
#ifdef PIONEER_Q38_VECTOR_PANEL
			PioneerQ38AffineOnly
#else
#ifdef PIONEER_Q38_FAST_PANEL
			PioneerQ38FastPanel
#else
			PioneerQ38UnpackPanel
#endif
#endif
			(type,reinterpret_cast<const uint8_t*>(0x90000),bytes,p,local)) block_fault_park();
#endif
#ifndef PIONEER_Q38_VECTOR_PANEL
		for (unsigned w=0;w<128;w++) Words(0x70710)[w]=local->operand[w];
#endif
		int sum=0;
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		if (zeroMinimum) {
			// Exact +0 minimum means no correction term and no input sum.
			// Preserve EVERY original SrcB operand and its ordering/fence.
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
			if(!directSourceB)
#endif
			for (unsigned w=0;w<8;w++) Words(0x70920)[w]=Words(inputBase)[block*(count/2)+p*8+w];
		} else
#endif
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
		if (cacheSums && group) {
			sum=int(Words(0x95000)[block*panels+p]);
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
			if(!directSourceB)
#endif
			for (unsigned w=0;w<8;w++) Words(0x70920)[w]=Words(inputBase)[block*(count/2)+p*8+w];
		} else {
#endif
		for (unsigned w=0;w<8;w++) {
			uint32_t word=Words(inputBase)[block*(count/2)+p*8+w];
#ifdef PIONEER_Q38_BFLOAT_PANEL
			Words(0x70920)[w]=Words(0x95000)[block*(count/2)+p*8+w];
#else
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
			if(!directSourceB)
#endif
			Words(0x70920)[w]=word;
#endif
			for (unsigned lane=0;lane<2;lane++) {
				uint16_t value=uint16_t(word>>(lane*16)); sum+=int(value&1023)*((value&0x8000)?-1:1);
			}
		}
#ifdef PIONEER_Q38_WORK_INPUT_SUM_CACHE
			if (cacheSums) Words(0x95000)[block*panels+p]=uint32_t(sum);
		}
#endif
#ifndef PIONEER_Q38_VECTOR_PANEL
		(void)Words(0x70710)[127];
#endif
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
		if(!directSourceB)
#endif
		(void)Words(0x70920)[7];
		Fence();
#ifdef PIONEER_Q38_VECTOR_PANEL
		VectorPanelEmitter emitter;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
		emitter.blockAffineRow=128+(p/2)*8;
#endif
#ifdef PIONEER_Q38_BFLOAT_PANEL
		if (!PioneerQ38BFloatPanel(emitter,type,p)) block_fault_park();
#else
#ifdef PIONEER_Q38_VECTOR_FIXED_FORMAT
#ifdef PIONEER_Q38_NATIVE_Q8
		if(skipDotCheck && type==8) {
			if(!PioneerQ38DirectNativeQ8(emitter,p))block_fault_park();
		} else
#endif
#ifdef PIONEER_Q38_PREDECODED_Q4
		if(skipDotCheck && type==12) {
			if(!PioneerQ38DirectQ4Unpack(emitter,p))block_fault_park();
		} else
#endif
		if (!PioneerQ38VectorFixedUnpack(emitter,type,p
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
			,skipDotCheck && type==12
#endif
			PIONEER_Q38_RAW_ARGUMENT
#ifdef PIONEER_Q38_WORK_FRONTEND_BARRIER
			,skipDotCheck
#endif
		)) block_fault_park();
#else
		if (!PioneerQ38VectorUnpack(emitter,type,p)) block_fault_park();
#endif
#endif
#endif
#ifdef PIONEER_Q38_WORK_DIRECT_SRCB
		// FixedUnpack's existing FINAL drain has committed ALL deferred
		// SFPU stores. Only now alter/read back the bounded SrcB base;
		// never change config while a previous affine store is pending.
		if(directSourceB)emitter.Config(124,(inputBase+block*count*2+p*32-16)/16);
#endif
#if defined(PIONEER_Q38_BFLOAT_PANEL) || defined(PIONEER_Q38_Q4_TWO_PHASE)
		Matrix(type
#ifdef PIONEER_Q38_Q4_GROUP32
			,pairedQ4 && (p&1)
#endif
		);
#else
		Matrix();
#endif
#if defined(PIONEER_Q38_PREDECODED_Q4) && !defined(PIONEER_Q38_PREDECODED_Q4_STICKY)
		if(skipDotCheck && type==12)PioneerQ38DirectQ4Restore(emitter);
#endif
#ifdef PIONEER_Q38_Q4_GROUP32
		// Every raw panel,source publication,matrix phase and real matrix
		// drain STILL executes. Only the affine update waits for BOTH exact
		// integer panels sharing this Q4_K coefficient/minimum. This changes
		// FP32 rounding intentionally;NEVER selected by a native driver yet.
		if (pairedQ4) {
			if (!(p&1)) { precedingSum=sum; continue; }
			sum+=precedingSum;
		}
#endif
#ifdef PIONEER_Q38_SFPU_ACCUMULATOR
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
		// Private descriptor14 comes ONLY from the immutable root's Work
		// selector, never user DATA. Fixed decoded integer ranges prove the
		// dot bound mathematically; no address, length or source proof uses
		// its numeric value. Checked/debug jobs KEEP every on-card read.
		// Work still has the real matrix drain and final finite output gate.
		if (!skipDotCheck)
#endif
		for (unsigned row=0;row<16;row++) {
#ifdef PIONEER_Q38_SFPU_INTEGER
			int dot=int(Words(0xffbd8800)[row]); if (dot<-262144 || dot>262144) block_fault_park();
#else
			uint32_t raw=Words(0xffbd8800)[row]; float dot=Float(raw);
			if ((raw&0x7f800000)==0x7f800000 || dot<-262144.0f || dot>262144.0f) block_fault_park();
#endif
		}
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		if (!accumulate(emitter,type,!block &&
#ifdef PIONEER_Q38_Q4_GROUP32
			(pairedQ4?p==1:!p),pairedQ4 ||
#else
			!p,
#endif
			type==14 || !(p&1),Bits(scale),sum)) block_fault_park();
#else
		if (!PioneerQ38SfpuAccumulator<VectorPanelEmitter,
#ifdef PIONEER_Q38_SFPU_INTEGER
			true
#else
			false
#endif
		>(emitter,type,!block && !p,type==14 || !(p&1),Bits(scale),sum)) block_fault_park();
#endif
#else
#ifdef PIONEER_Q38_VECTOR_AFFINE
		// Preserve the original ALL16 dot range checks before vector math.
		for (unsigned row=0;row<16;row++) {
			int dot=int(Words(0xffbd8800)[row]); if (dot<-262144 || dot>262144) block_fault_park();
		}
		if (!PioneerQ38VectorAffineData(emitter,group,Bits(scale),sum)) block_fault_park();
		for (unsigned row=0;row<16;row++) {
			uint32_t value=Words(0xffbd9b00)[row];
			if ((value&0x7f800000)==0x7f800000) block_fault_park();
			Words(0x84000)[group*16+row]=value;
		}
		Fence(); Config(3,0x10000);
#else
		for (unsigned row=0;row<16;row++) {
			static_assert(sizeof(int)==4,"fixed signed32 integer dot");
#ifdef PIONEER_Q38_BFLOAT_PANEL
			uint32_t raw=Words(0xffbd8800)[row]; float dot=Float(raw);
			if ((raw&0x7f800000)==0x7f800000 || dot<-262144.0f || dot>262144.0f) block_fault_park();
			if (!(raw&0x7fffffff)) dot=0; // canonical original integer zero
#else
			int dot=int(Words(0xffbd8800)[row]); if (dot<-262144 || dot>262144) block_fault_park();
#endif
			float product=(Float(local->coefficient[row])*scale)*float(dot);
			float correction=(Float(local->minimum[row])*scale)*float(sum);
			float value=Float(Words(0x84000)[group*16+row])+(product+correction);
			if ((Bits(value)&0x7f800000)==0x7f800000) block_fault_park();
			Words(0x84000)[group*16+row]=Bits(value);
		}
#endif
		Fence();
#endif
	}
	}
#ifdef PIONEER_Q38_SFPU_ACCUMULATOR
	// Bounded finite inputs imply |accumulator|<2^68,normal-or-zero
	// intermediates and NO overflow/underflow. ALL dot checks remain per
	// panel;publish only final finite results after the entire group.
#ifdef PIONEER_Q38_WORK_DEFER_AFFINE
	// Complete ALL deferred stores BEFORE config changes or ANY CPU Dst
	// export. Checked retains every original per-panel drain unchanged.
	if (skipDotCheck) Drain();
#endif
#ifdef PIONEER_Q38_SFPU_INTEGER
	Config(3,0); // final FP32 accumulator export, NOT integer dot conversion
#endif
	for (unsigned row=0;row<16;row++) {
		uint32_t value=Words(0xffbd9b00)[row];
		if ((value&0x7f800000)==0x7f800000) block_fault_park();
		Words(0x84000)[group*16+row]=value;
	}
	Fence();
#ifdef PIONEER_Q38_SFPU_INTEGER
	Config(3,0x10000); // next group's checked signed32 dot readback
#endif
#endif
	}
	return groups*blocks*panels;
}
