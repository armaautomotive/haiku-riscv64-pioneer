/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RESIDENT_CONTINUE_IMAGE
#error Continuation IMAGE has no existing native/CPU/Work/component role
#endif
// Private root dispatcher source. Selection requires a pinned kernel image.
// The kernel MUST authenticate/own ALL initialized bank extents,private
// tasks/bank routes/source seals and all119 peer images/sequence histories.
// No physical route,address or sequence here is a new userspace capability.
#ifdef PIONEER_Q38_RESIDENT_ADVANCE_IMAGE
#include "PioneerQ38ResidentAdvanceImageRole.h"
#if !defined(PIONEER_Q38_PROJECTION_MATRIX_ROOT) || PIONEER_Q38_PROJECTION_MATRIX_ROOT != 1 \
 || !defined(PIONEER_Q38_PROJECTION_NEXT_ROOT) || PIONEER_Q38_PROJECTION_NEXT_ROOT != 1
#error Advance matrix component needs its exact nonzero-layer projection grammar
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE
#include "PioneerQ38ResidentOutputRootImageRole.h"
#endif
typedef __UINT32_TYPE__ uint32_t;
#define PIONEER_ATTENTION_FREESTANDING
#ifdef PIONEER_Q38_FFN_GRAPH_ROOT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphShape.h"
enum {Q38DispatchPeerMarker=TT_Q38_FFN_PERSISTENT_MARKER};
#elif defined(PIONEER_Q38_PROJECTION_MATRIX_ROOT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMatrixShape.h"
enum {Q38DispatchPeerMarker=TT_Q38_FFN_PERSISTENT_MARKER};
#else
enum {Q38DispatchPeerMarker=0x32503851};
#endif
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
#if !defined(PIONEER_Q38_WORK_SENTINEL_POLL) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_WORK_COHORT_CACHE) || !defined(PIONEER_Q38_REUSE_ACTIVATIONS) || !defined(PIONEER_Q38_WORK_COMPACT_OUTPUT) || !defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK) || defined(PIONEER_Q38_OUTPUT_TRANSFER_ROOT) || defined(PIONEER_Q38_WORK_ROOT_COMMAND_REUSE) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_PEER_PHASE_ROOT)
#error NEW OFFLINE resident FFN root requires its exact private chain,not host DMA/cache experiment selectors
#endif
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
#if PIONEER_Q38_PROJECTION_MATRIX_ROOT != 1
#error Exact separate projection matrix role required
#endif
enum {TT_Q38_FFN_MATRIX_MARKER=TT_Q38_PROJECTION_MATRIX_MARKER,
 TT_Q38_FFN_MATRIX_FRAME=TT_Q38_PROJECTION_MATRIX_FRAME,TT_Q38_FFN_MATRIX_ROWS=TT_Q38_PROJECTION_MATRIX_ROWS,
 TT_Q38_FFN_MATRIX_RESIDENT_INPUT=TT_Q38_PROJECTION_MATRIX_INPUT};
#define PioneerQ38FfnMatrixShape PioneerQ38ProjectionMatrixShape
#define PioneerQ38FfnMatrixOrdered PioneerQ38ProjectionMatrixOrdered
#else
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixShape.h"
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CompactOutput.h"
#endif
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
#if !defined(PIONEER_Q38_WORK_SENTINEL_POLL) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || defined(PIONEER_Q38_WORK_ROOT_COMMAND_REUSE) || defined(PIONEER_Q38_WORK_BATCH4_FETCH)
#error Separate OFFLINE peer timing root ONLY,not command-reuse/batch/mixed modes
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerPhaseData.h"
#endif
#ifdef PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
#if !defined(PIONEER_Q38_WORK_SENTINEL_POLL) || defined(PIONEER_Q38_WORK_BATCH4_FETCH)
#error Separate OFFLINE command reuse requires original-layout sentinel root,not batch fetch
#endif
#include "PioneerQ38RootCommandReuse.h"
#define Q38_COMMAND_PARAMETER ,bool reuseCommand
#define Q38_COMMAND_WITH(value) ,value
#else
#define Q38_COMMAND_PARAMETER
#define Q38_COMMAND_WITH(value)
#endif
#if defined(PIONEER_Q38_WORK_BATCH4_FETCH) && (!defined(PIONEER_Q38_WORK_SENTINEL_POLL) || !defined(PIONEER_Q38_WORK_COHORT_CACHE) || !defined(PIONEER_Q38_WORK_COMPACT_OUTPUT))
#error Batch4 is a separate OFFLINE exact-counter Work coordinator experiment
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerTerminal.h"
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
#include "PioneerQwen35PeerAudit.h"
#define Q35_PEER_AUDIT_PARAMETER ,bool auditPhysical=true
#define Q35_PEER_AUDIT_ARGUMENT(work,stopped) ,PioneerQ35PeerPhysicalAuditRequired(work,stopped)
#else
#define Q35_PEER_AUDIT_PARAMETER
#define Q35_PEER_AUDIT_ARGUMENT(work,stopped)
#endif
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
#include "PioneerQwen35Projection.h"
#include "PioneerQwen35RootImageLayout.h"
#ifdef PIONEER_Q35_WORK_BATCH_FETCH
#include "PioneerQwen35BatchFetch.h"
#endif
#ifdef PIONEER_Q35_WORK_BATCH_PREFETCH
#if !defined(PIONEER_Q35_WORK_BATCH_FETCH) || !defined(PIONEER_Q35_WORK_ROOT_IMAGE)
#error Double-buffered peer prefetch requires its matching Qwen35 Work ROOT proof
#endif
#include "PioneerQwen35BatchPrefetchPlan.h"
#endif
#if !defined(PIONEER_Q38_WORK_COHORT_CACHE) || defined(PIONEER_Q38_WORK_SKIP_DOT_CHECK) \
 || defined(PIONEER_Q38_REUSE_ACTIVATIONS) || defined(PIONEER_Q38_FFN_RESIDENT_MATRIX)
#error Qwen3.5 Work dispatcher requires the checked persistent-cohort path
#endif
extern "C" unsigned q35_work_matrix_band();
enum {Q38DispatchScaleCache=PioneerQwen35::RootScaleAddress,Q35RootProjectionResult=0x9e000,Q35RootProjectionJob=0x74f60};
#else
enum {Q38DispatchScaleCache=0xab000};
#endif
#ifdef PIONEER_Q38_WORK_SENTINEL_POLL
#if (!defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) && !defined(PIONEER_Q35_WORK_ROOT_IMAGE)) \
 || !defined(PIONEER_Q38_WORK_COHORT_CACHE) \
 || (!defined(PIONEER_Q38_WORK_COMPACT_OUTPUT) && !defined(PIONEER_Q35_WORK_ROOT_IMAGE))
#error Separate offline sentinel polling requires the complete exact Work/root-counter chain
#endif
#include "PioneerQ38PollSentinel.h"
#endif
#ifdef PIONEER_Q38_OUTPUT_TRANSFER_ROOT
#include "PioneerQ38OutputTransfer.h"
#endif
#if defined(PIONEER_Q38_WORK_SPARSE_EXPORT) && !defined(PIONEER_Q38_OUTPUT_TRANSFER_ROOT) && !defined(PIONEER_Q38_FFN_RESIDENT_MATRIX)
#error Sparse private exports require the Work bounded-output root
#endif
#if defined(PIONEER_Q38_WORK_COMPACT_OUTPUT) && !defined(PIONEER_Q38_WORK_SPARSE_EXPORT)
#error Compact output requires the exact sparse Work output root
#endif
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ActivationPacking.h"
#define Q38_ROOT_TABLE static
#else
#define Q38_ROOT_TABLE
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
#define Q38_ROOT_SMALL __attribute__((noinline,optimize("Os")))
#else
#define Q38_ROOT_SMALL
#endif
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
#ifdef PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT
enum {Q38DispatchSourceCache=TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW};
#else
enum {Q38DispatchSourceCache=0x7d800};
#endif
static void Fence() { asm volatile("fence" ::: "memory"); }
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
enum {Q38DispatchCheckpointAddress=0x17ffb0};
#else
enum {Q38DispatchCheckpointAddress=0x17ffc0};
#endif
// Diagnostic ONLY in the separately owned/saved runtime-status L1. Normal
// completion overwrites BOTH words with the exact fixed DONE/count proof.
// Fault park preserves them;NO extra NoC packet/host DMA/source authority.
static __attribute__((noinline)) void Checkpoint(unsigned stage,unsigned detail)
{
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	Words(0xe7068)[0]=0xd0000000U|(stage&0xffffU);
#endif
	Words(Q38DispatchCheckpointAddress)[0]=stage; Words(Q38DispatchCheckpointAddress)[1]=detail; Fence();
}
// A Work dispatcher rejection is returned to StageBand as a plain zero, so
// keep the rejecting source line and its compact context in the ROOT progress
// record as well as the existing L1 checkpoint. The owner prefers this
// failure-only receipt if the checkpoint aperture cannot be read back.
static unsigned DispatcherReject(unsigned sourceLine,unsigned detail,
	unsigned context0=0,unsigned context1=0)
{
	const unsigned stage=0x54f00000U|(sourceLine&0xffffU);
	Checkpoint(stage,detail);
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	Words(0xe7070)[0]=stage; Words(0xe7074)[0]=detail; Fence();
	if (context0 || context1) {
		Words(0xe7078)[0]=context0; Words(0xe707c)[0]=context1; Fence();
	}
#endif
	return 0;
}
extern "C" void block_fault_park() __attribute__((noreturn));
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
static __UINT64_TYPE__ Q35DispatchCycles()
{
	for (unsigned attempt=0;attempt<16;attempt++) {
		uint32_t high,low,confirm;
		asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
			: "=r"(high),"=r"(low),"=r"(confirm) :: "memory");
		if (high==confirm) return (static_cast<__UINT64_TYPE__>(high)<<32)|low;
	}
	block_fault_park();
}
struct Q35DispatchProfile {
	bool enabled;
	__UINT64_TYPE__ previous;
	volatile uint32_t* slots;
	explicit Q35DispatchProfile(bool active)
		: enabled(active),previous(0),slots(Words(PioneerQwen35::RootControlProfileAddress))
	{
		if (!enabled) return;
		previous=Q35DispatchCycles();
		if (!previous) block_fault_park();
	}
	void Mark(unsigned phase)
	{
		if (!enabled) return;
		if (phase>=3) block_fault_park();
		const __UINT64_TYPE__ now=Q35DispatchCycles();
		if (now<previous) block_fault_park();
		const __UINT64_TYPE__ before=(static_cast<__UINT64_TYPE__>(slots[phase*2+1])<<32)
			|slots[phase*2];
		const __UINT64_TYPE__ delta=now-previous;
		const __UINT64_TYPE__ total=before+delta;
		if (total<before) block_fault_park();
		slots[phase*2]=uint32_t(total);
		slots[phase*2+1]=uint32_t(total>>32);
		Fence();
		previous=now;
	}
};
#endif
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
#ifndef PIONEER_Q38_WORK_COMPACT_OUTPUT
#error Root cycle diagnostic requires the separate complete compact-output root
#endif
#include "PioneerQ38RootCycleProfile.h"
#endif
#ifdef PIONEER_Q38_OUTPUT_TRANSFER_ROOT
// Separately gated bounded-output variant. The kernel must own/seal the
// complete high-physical host mapping,
// keep it pending across BME, and quarantine ALL owners on uncertainty.
struct RootOutputIO {
	uint32_t ReadReg(unsigned at) { return *Words(0xffb20000+at); }
	void WriteReg(unsigned at,uint32_t value) { *Words(0xffb20000+at)=value; }
	uint32_t ReadL1(unsigned at) { return *Words(at); }
	void Fence() { ::Fence(); }
};
static __attribute__((noinline,section(".q38_output"))) bool OutputHeaderValid(const uint32_t* header)
{
	if (!header[8]) { for (unsigned i=9;i<14;i++) if (header[i]) return false; return true; }
	if ((header[13]&4095) || header[13]>PIONEER_Q38_OUTPUT_OWNER) return false;
	for (unsigned i=14;i<16;i++) if (header[i]) return false;
	return header[9]==PIONEER_Q38_OUTPUT_MAGIC && !(header[10]&0xfffff)
		&& header[11] && header[11]<=0xffff && header[12]==PIONEER_Q38_OUTPUT_OWNER;
}
static __attribute__((noinline,section(".q38_output"))) bool ReturnHostOutput(const uint32_t* header,const uint32_t* source
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
	,unsigned words
#endif
)
{
	uint32_t d[32]; for (unsigned i=0;i<32;i++) d[i]=0;
	d[0]=PIONEER_Q38_OUTPUT_MAGIC; d[1]=1; d[2]=header[5]; d[3]=header[6];
	d[4]=header[10]; d[5]=header[11]; d[6]=header[12];
	d[8]=header[13];
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
	d[7]=PioneerQ38CompactOutputBytes(header[5],words);
#else
	d[7]=header[5]*1024;
#endif
	for (unsigned i=0;i<16;i++) d[16+i]=source[i];
	RootOutputIO io; PioneerQ38OutputTransfer<RootOutputIO> transfer(io);
	if (!transfer.Run(d) || !transfer.Complete() || transfer.Unsafe()) return false;
	unsigned chunks=(d[7]+4095)/4096,flits=d[7]/64;
	Words(0x7f800)[6]+=chunks; Words(0x7f800)[7]+=chunks;
	Words(0x7f800)[8]+=flits; Words(0x7f800)[9]+=flits;
	Words(0x7f800)[49]=PIONEER_Q38_OUTPUT_MAGIC; Words(0x7f800)[50]=d[7]; Words(0x7f800)[51]=d[3];
	Words(0x7f800)[52]=Words(0x7f800)[53]=chunks;
	Words(0x7f800)[54]=Words(0x7f800)[55]=flits; Words(0x7f800)[56]=1;
	// No host physical word is placed in the publicly returned statistics.
	Fence(); return true;
}
#endif
static unsigned XY(unsigned worker)
{
	Q38_ROOT_TABLE const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	unsigned row=worker,column=0; while (row>=10) { row-=10; column++; }
	if (!worker || worker>=120 || column>=12) block_fault_park();
	return columns[column]|((row+2)<<6);
}
static unsigned Offset(unsigned c)
{
	Q38_ROOT_TABLE const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228}; return offsets[c];
}
static unsigned SourceOffset(unsigned w)
{
	if (!w) return 0x100;
	if (w<=6) return 0x118+(w-1)*4;
	if (w<=12) return 0x130+(w-7)*4;
	return w==13?0x150:w==14?0x154:0x16c;
}
static bool Idle()
{
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
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
static bool PacketIdle() { return !*Words(0xffb20040) && !(*Words(0xffb20240)&255); }
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
// Only copied AFTER shape validation,BEFORE the FIRST packet. These fixed
// private shadows have no command/response/output alias and are not changed
// during this helper call. A new native owner MUST save/prove their view.
static __attribute__((noinline)) bool ProjectionRootSealed(){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=Words(TT_Q38_PROJECTION_MATRIX_SHADOW)[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_PROJECTION_MATRIX_FRAME)[i]!=Words(TT_Q38_PROJECTION_MATRIX_SHADOW)[i])return false;
 for(unsigned i=0;i<119;i++)if(Words(TT_Q38_PROJECTION_MATRIX_ROWS)[i]!=Words(TT_Q38_PROJECTION_ROWS_SHADOW)[i])return false;
 return true;
}
#endif
#ifdef PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
struct RootCommandIO {
	uint32_t Read(unsigned word){return Words(0xffb20000)[word];}
	void Write(unsigned word,uint32_t value){Words(0xffb20000)[word]=value;}
};
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
struct RootInitiatorCommandIO {
	unsigned base;
	uint32_t Read(unsigned word){return Words(base)[word];}
	void Write(unsigned word,uint32_t value){Words(base)[word]=value;}
};

// Read both processor PCs and the reset register concurrently through three
// independent NIU request initiators. The peer status is still read and
// validated first; these are the same fixed, read-only physical audits as
// Transfer(), with exact per-counter completion and all four initiators idle
// before returning.
static bool ReadPeerPhysicalTriplet(unsigned worker,uint32_t& first,uint32_t& second,
	uint32_t& reset)
{
	if (!worker || worker>=120 || !Idle()) return false;
	const unsigned remotes[3]={0xffb13140,0xffb13140,0xffb121b0};
	const unsigned locals[3]={0x75000,0x75004,0x75008};
	const unsigned counterOffsets[4]={0x208,0x20c,0x210,0x214};
	uint32_t expected[4];
	for (unsigned c=0;c<4;c++) expected[c]=Words(0xffb20000+counterOffsets[c])[0]+3;
	for (unsigned channel=0;channel<3;channel++) {
		const unsigned initiator=channel+1;
		const uint32_t command[]={remotes[channel],0,XY(worker),locals[channel],0,129,0,0,4,0};
		RootInitiatorCommandIO io={0xffb20000+initiator*0x800};
		if (!PioneerQ38ProgramRootCommand(io,command,true)) return false;
	}
	Fence();
	for (unsigned channel=0;channel<3;channel++)
		*Words(0xffb20040+(channel+1)*0x800)=1;
	Fence();
	bool done=false;
	for (unsigned attempt=0;attempt<65536;attempt++) {
		done=true;
		for (unsigned channel=0;channel<3;channel++)
			done &= !*Words(0xffb20040+(channel+1)*0x800);
		for (unsigned c=0;c<4;c++)
			done &= Words(0xffb20000+counterOffsets[c])[0]==expected[c];
		if (done && Idle()) break;
		done=false;
	}
	if (!done) return false;
	Words(0x7f800)[6]+=3;
	Words(0x7f800)[8]+=3;
	Fence();
	first=Words(locals[0])[0];
	second=Words(locals[1])[0];
	reset=Words(locals[2])[0];
	return true;
}
#endif
#endif
static void Transfer(bool write,unsigned worker,unsigned local,unsigned remote,unsigned bytes Q38_COMMAND_PARAMETER)
{
	// ONLY the exact fixed peer data/status/PC/reset windows below. Never
	// code/boot writes,peer resets,host DMA or a caller-supplied coordinate.
	bool permitted=write?(remote==0x74000 && bytes==64) || (remote==0x74100 && bytes==4)
		:(remote==0x74000 && bytes==64) || (remote==0x74140 && bytes==64)
		|| (remote==0xffb13140 && bytes==4) || (remote==0xffb121b0 && bytes==4)
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
		|| (remote==0x84500 && bytes==64) // one fixed private DATA endpoint,never user address
#endif
		|| (remote==0x83f00 && bytes>=320 && bytes<=1280 && !(bytes&63));
	if (!permitted || !worker || worker>=120 || local<0x75000 || local>0x75800-bytes
		|| ((local^remote)&31)) block_fault_park();
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	const unsigned diagnosticStage=0x58000000U|(worker<<16)|(write?0x8000U:0U)
		|(((remote>>4)&1023U)<<5);
	const unsigned commandBusy=*Words(0xffb20040);
	const unsigned protocolBusy=*Words(0xffb20240)&255U;
	if (commandBusy || protocolBusy) {
		// Distinguish a refused transfer admission from a timeout after issue.
		Checkpoint(0x5c000000U|(diagnosticStage&0x00ffffffU),
			(commandBusy?1U:0U)|(protocolBusy<<8));
		block_fault_park();
	}
#else
	if (!PacketIdle()) block_fault_park();
#endif
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
 if(!ProjectionRootSealed() || !Idle())block_fault_park();
 uint32_t expected[8];for(unsigned c=0;c<8;c++){
  unsigned delta=c==2 || c==6?1U:c==0 || c==3?unsigned(!write):c==1?(!write?(bytes+63)/64:0U):c==5?(write?(bytes+63)/64:0U):unsigned(write);
  expected[c]=*Words(0xffb20000+Offset(c))+delta;
  if(c==6 && expected[6]!=expected[2])block_fault_park();
 }
#else
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	static const unsigned offsets[2][4]={{0x208,0x20c,0x210,0x214},{0x204,0x220,0x210,0x228}};
	const unsigned* offset=offsets[write?1:0];
#else
	const unsigned offset[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
#endif
	uint32_t expected[4]; for (unsigned c=0;c<4;c++) expected[c]=*Words(0xffb20000+offset[c])+(c==1?(bytes+63)/64:1);
#endif
	uint32_t command[]={write?local:remote,0,write?129U:XY(worker),write?remote:local,0,write?XY(worker):129U,0,write?0x212U:0,bytes,0};
#ifdef PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
	if(reuseCommand){
		// Exact PacketIdle proof above;only private local writes and counter
		// reads intervened. NO other NIU command writer exists in this owner.
		RootCommandIO io;if(!PioneerQ38ProgramRootCommand(io,command,true))block_fault_park();
		Checkpoint(0x54000101,worker);
	}else {
#endif
	for (unsigned i=0;i<10;i++) Words(0xffb20000)[i]=command[i];
	Checkpoint(0x54000101,worker);
	for (unsigned i=0;i<10;i++) if (Words(0xffb20000)[i]!=command[i]) block_fault_park();
#ifdef PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
	}
#endif
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	// Preserve the exact pending transaction before posting its doorbell. A
	// synchronous frontend/NoC fault can bypass the bounded timeout receipt
	// below, so recovery still has the worker, direction, and remote window.
	Checkpoint(diagnosticStage,bytes);
#endif
	Fence(); *Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
	Checkpoint(0x54000102,worker);
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	Checkpoint(diagnosticStage|16U,bytes);
	// Separate diagnostic ONLY. Reuse the ORIGINAL command/counter reads;
	// do not issue another packet/read/retry after the fixed polling limit.
	unsigned pending=0,delta=0;
#endif
	for (unsigned attempt=0;attempt<65536;attempt++) {
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
		pending=*Words(0xffb20040)?16U:0U; delta=0;
		for (unsigned c=0;c<4;c++) {
			unsigned value=*Words(0xffb20000+offset[c]);
			if (value!=expected[c]) { pending|=1U<<c; delta=value-expected[c]; }
		}
		bool complete=!pending;
#else
		bool complete=!*Words(0xffb20040);
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  for(unsigned c=0;c<8;c++)complete &= *Words(0xffb20000+Offset(c))==expected[c];
#else
		for (unsigned c=0;c<4;c++) complete &= *Words(0xffb20000+offset[c])==expected[c];
#endif
#endif
		if (complete &&
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
   Idle()
#else
   PacketIdle()
#endif
  ) {
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
   if(!ProjectionRootSealed())block_fault_park();
#endif
			Words(0x7f800)[write?7:6]++; Words(0x7f800)[write?9:8]+=(bytes+63)/64; Fence(); return;
		}
	}
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	// stage:5b|worker[22:16]|write[15]|remote[13:4] at[14:5]|
	// cmd-busy[4]|counter-mismatch[3:0]. detail:LAST mismatching counter's
	// observed-minus-expected,mod2^32 (highest set bit identifies counter).
	// pending==0 means the original final PacketIdle check rejected.
	Checkpoint(0x5b000000|(worker<<16)|(write?0x8000U:0U)|(((remote>>4)&1023U)<<5)|pending,delta);
#endif
	block_fault_park(); // UNKNOWN:no later packet/reset/restore/retry
}
static bool WaitPc(unsigned pc) { return pc==0x7204c || pc==0x72050; }
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
#ifndef PIONEER_Q38_FFN_RESIDENT_MATRIX
// Fixed supported shapes avoid integer division/remainder in the hot RISC
// loop. Two packed words expand to three original words,backwards in place.
static __attribute__((noinline,optimize("Os"))) bool ExpandInputs(unsigned inputs)
{
	unsigned packed=PioneerQ38PackedActivationWords(inputs);
	if (!packed) return false;
	for (unsigned w=0;w<packed;w++) if (Words(0xa0000)[w]&0xf8000000U) return false;
	unsigned groups=packed/2;
	if (packed&1) {
		uint32_t tail=Words(0xa0000)[packed-1]; if (tail>>18) return false;
		Words(0xa0000)[inputs/2-1]=uint32_t(PioneerQ38ExpandActivation(tail))|(uint32_t(PioneerQ38ExpandActivation(tail>>9))<<16);
	}
	for (unsigned group=groups;group--;) {
		uint32_t first=Words(0xa0000)[group*2],second=Words(0xa0000)[group*2+1];
		Words(0xa0000)[group*3+2]=uint32_t(PioneerQ38ExpandActivation(second>>9))|(uint32_t(PioneerQ38ExpandActivation(second>>18))<<16);
		Words(0xa0000)[group*3+1]=uint32_t(PioneerQ38ExpandActivation(first>>18))|(uint32_t(PioneerQ38ExpandActivation(second))<<16);
		Words(0xa0000)[group*3]=uint32_t(PioneerQ38ExpandActivation(first))|(uint32_t(PioneerQ38ExpandActivation(first>>9))<<16);
	}
	Fence(); return true;
}
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
// Keep the transport selector out of the large dispatcher register live set.
// Both paths retain exactly the original operand and padded-scale checks.
static __attribute__((noinline,optimize("Os"))) bool PrepareInputs(unsigned mode,unsigned type,unsigned inputs)
{
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	if (mode!=TT_Q38_FFN_MATRIX_RESIDENT_INPUT) return false;
#else
	if (mode==1 && !ExpandInputs(inputs)) return false;
#endif
	for (unsigned w=0;w<inputs;w++) if (!PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w/2]>>((w&1)*16)))) return false;
	unsigned blocks=inputs>>(type==8?5:8),padded=(blocks+15)&~15U;
	for (unsigned b=0;b<padded;b++) {
		uint32_t bits=Words(0xab000)[b],magnitude=bits&0x7fffffff;
		if (b>=blocks?bits!=0:magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return false;
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  if(b<blocks && (bits==0x80000000U || (type==8 && (bits&0x80000000U))))return false;
#endif
	}
	return true;
}
#endif
#endif
static bool Peer(unsigned worker,unsigned sequence,bool stopped,bool previous Q38_COMMAND_PARAMETER
	Q35_PEER_AUDIT_PARAMETER)
{
	Transfer(false,worker,0x75000,0x74140,64 Q38_COMMAND_WITH(reuseCommand)); Words(0x7f800)[10]++;
	const volatile uint32_t* status=Words(0x75000);
#ifdef PIONEER_Q38_FAULT_RETURN
	// ALREADY transferred local status;normal status remains unchanged.
#ifdef PIONEER_Q38_PEER_PROGRESS
	Checkpoint(0x5a000000|(worker<<8)|((status[5]&255U)<<16)|(status[5]>=13 && status[5]<=14?(status[7]&255U):0U),status[5]>=9 && status[5]<=14?status[6]:status[3]);
#else
	Checkpoint(0x54000280|worker,status[3]);
#endif
#else
	Checkpoint(0x54000201,worker);
#endif
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	// Work-only field diagnostics: this is the first gate after the exact peer
	// status transfer. Preserve the failed field/value in the two owned L1
	// words so a fault can be localized without adding NoC traffic or retries.
	if (status[2]) {
		// Q35 Work's peer fault park stores its reason in status[6] and
		// call-site PC in status[5]. Pack the worker ID with that PC.
		if (status[6]==0x50) {
			// This gate preserves actual/expected panel counts in words7/8.
			// The stage carries worker and peer reason; detail packs both counts.
			Checkpoint(0x54000000U|((worker&0xffU)<<16)|((status[6]&0xffU)<<8)|1U,
				((status[7]&0xffffU)<<16)|(status[8]&0xffffU));
		} else {
			Checkpoint(0x54000300U|(status[6]&0xffU),((worker&0xffU)<<24)|(status[5]&0x00ffffffU));
		}
		block_fault_park();
	}
	if (status[3]!=Q38DispatchPeerMarker) { Checkpoint(0x54000212,status[3]); block_fault_park(); }
	if (status[0]>sequence) { Checkpoint(0x54000213,status[0]); block_fault_park(); }
	if (status[1]>sequence) { Checkpoint(0x54000214,status[1]); block_fault_park(); }
	if (status[4]>2) { Checkpoint(0x54000215,status[4]); block_fault_park(); }
#else
	if (status[2] || status[3]!=Q38DispatchPeerMarker || status[0]>sequence || status[1]>sequence || status[4]>2) block_fault_park();
#endif
	for (unsigned i=
#ifdef PIONEER_Q38_PEER_PROGRESS
		6
#else
		5
#endif
		;i<16;i++) if (status[i]) {
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
			Checkpoint(0x54000220U|i,status[i]);
#endif
			block_fault_park();
		}
#ifdef PIONEER_Q38_PEER_PROGRESS
	// Isolated diagnostic protocol ONLY. Bounded marker permitted ONLY
	// while the peer is running;ALL terminal/WAIT/proof fields stay exact.
	if (status[5]>12 || (status[4]!=1 && status[5])) block_fault_park();
	Checkpoint(0x59020000|(worker<<8)|(status[0]==sequence?1U:0U)|(status[1]==sequence?2U:0U)|(status[4]<<2),status[5]);
#else
	Checkpoint(previous?0x54000202:0x54000203,worker);
#endif
	if (previous) {
		if (status[0]!=sequence || status[1]!=sequence || status[4]) block_fault_park();
	} else {
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
		if (status[0]+1<sequence) { Checkpoint(0x54000231,status[0]); block_fault_park(); }
		if (status[1]+1<sequence) { Checkpoint(0x54000232,status[1]); block_fault_park(); }
		if (status[0]!=sequence || status[1]!=sequence || status[4]!=(stopped?2U:0U)) {
			Checkpoint(0x54000233,(status[0]&0xffU)|((status[1]&0xffU)<<8)|((status[4]&0xffU)<<16));
			return false;
		}
#else
		if (status[0]+1<sequence || status[1]+1<sequence) block_fault_park();
		if (status[0]!=sequence || status[1]!=sequence || status[4]!=(stopped?2U:0U)) return false;
#endif
	}
	// The pinned Qwen35 Work image keeps the peer image and its per-wave
	// sequence/source/counter proof immutable for the resident session. The
	// driver already audited every physical PC when admitting that session;
	// re-reading three physical registers per peer after every projection was
	// serializing the device-program barrier. Retain the full physical audit
	// for checked dispatches and, unconditionally, for the closing STOP.
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	if (!auditPhysical) {
		if (stopped) block_fault_park();
		return true;
	}
#endif
	// Word11 counts the three physical PC/reset reads per audited peer; the
	// final exact NoC counter proof uses it independently of status reads.
	Words(0x7f800)[11]++;
	unsigned first,second,reset;
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	if (!ReadPeerPhysicalTriplet(worker,first,second,reset)) block_fault_park();
#else
	Transfer(false,worker,0x75000,0xffb13140,4 Q38_COMMAND_WITH(reuseCommand)); first=*Words(0x75000);
	Transfer(false,worker,0x75000,0xffb13140,4 Q38_COMMAND_WITH(reuseCommand)); second=*Words(0x75000);
	Transfer(false,worker,0x75010,0xffb121b0,4 Q38_COMMAND_WITH(reuseCommand)); reset=*Words(0x75010);
#endif
	Checkpoint(0x54000204,(worker<<24)|(first&0xffffff));
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
 // Exact COMPLETE QFP3 image ranges,NOT a role grant from the shared boot
 // marker. A separate owner/catalog MUST pin all four new sections.
 const unsigned pc[]={first,second};
 for(unsigned value:pc)if(reset!=0x46800 || (value&3) || value==TT_Q38_PEER_FAULT_ENTRY || value==TT_Q38_PEER_FAULT_PARK
  || (!stopped && PioneerQ38PeerTerminalPc(value))
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE
  // ALL SIX exact returning-QOL1 peer sections authenticated by the new
  // builder. These ranges are not compatibility with the older owner.
  || (!((value>=0x64000 && value<0x67fe0) || (value>=0x68000 && value<0x6a83c)
   || (value>=0x70b00 && value<0x71ff0) || (value>=0x72000 && value<0x73f04)
   || (value>=0xb8000 && value<0xc0a44))))block_fault_park();
#else
  || (!((value>=0x64000 && value<0x67fe0) || (value>=0x70b00 && value<0x71f80) || (value>=0x72000 && value<0x73fe4))))block_fault_park();
#endif
#else
	if (reset!=0x46800 || first<0x72000 || first>=0x74000 || second<0x72000 || second>=0x74000 || ((first|second)&3)) block_fault_park();
#endif
	return stopped?PioneerQ38PeerTerminalPc(first) && PioneerQ38PeerTerminalPc(second):WaitPc(first) && WaitPc(second);
}
#ifdef PIONEER_Q38_WORK_SENTINEL_POLL
static __attribute__((noinline,section(".q38_peer_poll"))) void PeerPollBackoff(unsigned count)
{
	// Bounded local instructions ONLY,no clock/counter/NoC writes or reads.
	// Scheduling delay never substitutes for ANY completion/physical proof.
	for(unsigned i=0;i<count;i++)asm volatile("nop" ::: "memory");
}
static __attribute__((noinline,section(".q38_peer_poll"))) bool WaitPeers(const uint32_t (*parts)[8],unsigned workers,bool work,bool stopped)
{
	volatile uint32_t complete[119];for(unsigned w=0;w<119;w++)complete[w]=0;
	unsigned finished=0,polls=0,backoff=64;
	const unsigned budget=workers*65536; // SAME maximum status polls as original
	if(work) {
		unsigned sentinel=0,maxGroups=16;
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
		if(Words(0x74000)[7])maxGroups=PioneerQwen35::Vocabulary/16;
#endif
		if(!PioneerQ38SelectPollSentinel(parts,workers,sentinel,maxGroups))return false;
		const unsigned sentinelBudget=PioneerQ38PollSentinelBudget(parts[sentinel][2]);
		if(!sentinelBudget)return false;
		for(unsigned attempt=0;attempt<sentinelBudget && polls<budget;attempt++) {
			polls++;
			if(Peer(sentinel+1,parts[sentinel][3],false,false Q38_COMMAND_WITH(work)
				Q35_PEER_AUDIT_ARGUMENT(work,stopped))) {complete[sentinel]=1;finished=1;break;}
			PeerPollBackoff(backoff);backoff=PioneerQ38PollBackoffNext(backoff);
			if(!backoff)return false;
		}
		if(!finished)return false;
	}
	// A sentinel can finish BEFORE others. Every remaining worker STILL
	// passes its ORIGINAL current status,TWO physical PC samples,and reset.
	for(unsigned round=0;round<65536 && finished<workers;round++) {
		for(unsigned w=0;w<workers;w++)if(!complete[w]) {
			if(polls==budget)return false;
			polls++;
			if(Peer(w+1,parts[w][3],stopped,false Q38_COMMAND_WITH(work)
				Q35_PEER_AUDIT_ARGUMENT(work,stopped))){complete[w]=1;finished++;}
		}
		if(work && finished<workers)PeerPollBackoff(backoff);
	}
	return finished==workers;
}
#endif
static unsigned Tile(unsigned type)
{
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	return PioneerQwen35Projection::TileBytes(type);
#else
	return type==8?576:type==12?2304:type==14?3392:0;
#endif
}
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
static bool Q35RootMatrixIdleDiagnostic()
{
	for (unsigned c=0;c<4;c++) {
		unsigned value=*Words(0xffb20040+c*0x800);
		if (value) {
			Checkpoint(0x5e000010,(c<<24)|(value&0x00ffffff));
			return false;
		}
	}
	for (unsigned c=0;c<32;c++) {
		unsigned value=*Words(0xffb20240+c*4)&255U;
		if (value) {
			Checkpoint(0x5e000011,(c<<24)|value);
			return false;
		}
	}
	return true;
}
#endif
static bool Q35RootMatrix(const volatile uint32_t* job,unsigned type,unsigned inputs)
{
	unsigned groups=job[7],blocks=inputs>>(type==8?5:8),tiles=groups*blocks;
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	Checkpoint(0x5e000001,tiles);
	if (!Q35RootMatrixIdleDiagnostic()) return false;
	if (!tiles || groups>16 || job[6]<0x04000000 || job[6]>=0xc4000000
		|| (job[6]&63) || job[6]>0xc4000000-tiles*Tile(type) || job[8]>0xfff) {
		Checkpoint(0x5e000002,(groups<<24)|(blocks<<16)|(type<<8)|((job[6]&63)?1U:0U));
		return false;
	}
#else
	if (!Idle() || !tiles || groups>16 || job[6]<0x04000000 || job[6]>=0xc4000000
		|| (job[6]&63) || job[6]>0xc4000000-tiles*Tile(type) || job[8]>0xfff)
		return false;
#endif
	volatile uint32_t* command=Words(0xffb20000);
	uint32_t expected[10]={job[6],0,job[8],0x90000,0,129,0,0,Tile(type),0};
	for (unsigned i=0;i<10;i++) command[i]=expected[i];
	Fence();
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	for (unsigned i=0;i<10;i++) if (command[i]!=expected[i]) {
		Checkpoint(0x5e000004,(i<<24)|(command[i]&0x00ffffff));
		return false;
	}
	Checkpoint(0x5e000003,tiles);
	unsigned matrixBands=q35_work_matrix_band();
	Checkpoint(0x5e000006,matrixBands);
	if (matrixBands!=groups*(inputs/16)) {
		Checkpoint(0x5e000007,matrixBands);
		return false;
	}
#else
	for (unsigned i=0;i<10;i++) if (command[i]!=expected[i]) return false;
	if (q35_work_matrix_band()!=groups*(inputs/16)) return false;
#endif
	expected[0]=job[6]+(tiles-1)*Tile(type);
#ifdef PIONEER_Q35_WORK_BATCH_FETCH
	const unsigned fetchBatch=PioneerQwen35FetchBatch(type);
	if (!fetchBatch) return false;
	const unsigned ordinal=PioneerQwen35FetchOrdinal(type,blocks-1);
	const unsigned firstBlock=ordinal*fetchBatch,batchBlocks=blocks-firstBlock<fetchBatch?blocks-firstBlock:fetchBatch;
	expected[0]=job[6]+((groups-1)*blocks+firstBlock)*Tile(type);
	expected[3]=0x98000;
	expected[8]=batchBlocks*Tile(type);
#endif
	for (unsigned i=0;i<10;i++) if (command[i]!=expected[i]) {
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
		Checkpoint(0x5e000008,(i<<24)|(command[i]&0x00ffffff));
#endif
		return false;
	}
	for (unsigned row=0;row<groups*16;row++) Words(Q35RootProjectionResult)[row]=Words(0x84000)[row];
	Fence();
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
	Checkpoint(0x5e000009,groups*16);
#endif
	return true;
}
#endif
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
static __attribute__((noinline,section(".q38_peer_phase"))) bool CapturePeerPhase(const uint32_t (&parts)[119][8],unsigned workers,unsigned type,unsigned inputs,uint32_t (&out)[7]){
	unsigned selected=0;if(!PioneerQ38SelectPollSentinel(parts,workers,selected))return false;
	// Caller has ALREADY proved ALL current peer status/2physicalPC/reset/
	// immutable source/counter/final commands and copied EVERY output.
	Transfer(false,selected+1,0x75000,0x84500,64);
	uint32_t data[16];for(unsigned w=0;w<16;w++)data[w]=Words(0x75000)[w];
	return PioneerQ38PeerPhaseReadback(data,selected+1,parts[selected][3],type,inputs,parts[selected][2],out);
}
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
static Q38_ROOT_SMALL bool HeaderValid(const uint32_t* header)
{
	unsigned operation=header[2],type=header[3],inputs=header[4],workers=header[5],nonce=header[6];
	if (header[0]!=
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
		TT_Q38_FFN_MATRIX_MARKER
#else
		0x314d3851
#endif
		|| header[1]!=1 || (operation!=1 && operation!=2) || !workers || workers>119
		|| !nonce || nonce>=0x7fffffff || (operation==2 && (type || inputs || workers!=119))
		|| (operation==1 && (!Tile(type) || (inputs!=5120 && inputs!=6144 && inputs!=17408)))) return false;
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	if (operation==2?(header[7] || header[8]):(header[7]!=TT_Q38_FFN_MATRIX_RESIDENT_INPUT || header[8]!=1)) return false;
#else
	if (operation==2?header[7]:header[7]-1U>1U) return false;
#endif
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	if (operation==2?header[8]:header[8]>1) return false;
#ifdef PIONEER_Q38_OUTPUT_TRANSFER_ROOT
	if (!OutputHeaderValid(header)) return false;
#else
	for (unsigned i=9;i<16;i++) if (header[i]) return false;
#endif
#else
	for (unsigned i=8;i<16;i++) if (header[i]) return false;
#endif
	return true;
}
static Q38_ROOT_SMALL bool PartsValid(const uint32_t parts[119][8],const uint32_t* banks,const uint32_t* header)
{
	unsigned operation=header[2],type=header[3],blocks=operation==1?header[4]>>(type==8?5:8):0;
	for (unsigned bank=0;bank<7;bank++) {
		if (!banks[bank] || banks[bank]>0xfff) return false;
		for (unsigned other=0;other<bank;other++) if (banks[bank]==banks[other]) return false;
	}
	if (banks[7]) return false;
	for (unsigned w=0;w<119;w++) {
		const uint32_t* p=parts[w];
		if (w>=header[5]) { for (unsigned value:parts[w]) if (value) return false; continue; }
		if (!p[3] || p[3]>header[6] || !p[5] || p[5]!=Words(Q38DispatchSourceCache)[w*16] || p[6] || p[7]) return false;
		if (operation==2) { if (p[0] || p[1] || p[2] || p[4]) return false; continue; }
		unsigned bytes=p[2]*blocks*Tile(type);
		if (p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || !p[2] || p[2]>16 || p[1]>=7
			|| bytes>0xc4000000-p[0] || p[4]!=banks[p[1]]) return false;
		for (unsigned other=0;other<w;other++) if (p[1]==parts[other][1]
			&& p[0]<parts[other][0]+parts[other][2]*blocks*Tile(type) && parts[other][0]<p[0]+bytes) return false;
	}
	return true;
}
#endif
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
#include "PioneerQ38ProjectionMatrixRoot.h"
#else
#include "PioneerQ38FfnMatrixRoot.h"
#endif
#define Q38_MAIN_SECTION __attribute__((section(".q38_main")))
#else
#define Q38_MAIN_SECTION
#endif
extern "C" Q38_MAIN_SECTION unsigned block_main()
{
	Checkpoint(0x54000001,0);
	volatile uint32_t* job=Words(0x74000); uint32_t header[16];
	for (unsigned i=0;i<16;i++) header[i]=job[i];
	unsigned operation=header[2],type=header[3],inputs=header[4],workers=header[5],nonce=header[6];
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	const bool work=operation==1 && header[8]==1;
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	Q35DispatchProfile q35Profile(work);
#endif
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
	uint32_t peerMetrics[7];
#endif
#endif
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
	PioneerQ38RootCycleProfile cycles(work);
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	if (!HeaderValid(header)) return DispatcherReject(__LINE__,0);
#else
	if (header[0]!=0x314d3851 || header[1]!=1 || (operation!=1 && operation!=2) || !workers || workers>119
		|| !nonce || nonce>=0x7fffffff || (operation==2 && (type || inputs || workers!=119))
		|| (operation==1 && (!Tile(type) ||
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
			!PioneerQwen35Projection::Supports(type,inputs)))) return DispatcherReject(__LINE__,workers);
#else
			(inputs!=5120 && inputs!=6144 && inputs!=17408)))) return DispatcherReject(__LINE__,workers);
#endif
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
	if (header[7]!=(operation==1?1U:0U)) return DispatcherReject(__LINE__,header[7]);
	for (unsigned i=8;i<16;i++) if (header[i]) return DispatcherReject(__LINE__,i);
#else
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	if (header[7]>1 || (operation==1?header[8]!=1:header[8])
		|| (header[7] && (operation!=1 || type!=14 || inputs!=PioneerQwen35::Hidden)))
		return DispatcherReject(__LINE__,header[7]?header[7]:header[8]);
	for (unsigned i=9;i<16;i++) if (header[i]) return DispatcherReject(__LINE__,i);
#elif defined(PIONEER_Q38_WORK_COHORT_CACHE)
	if (operation==1?header[7]!=0 || header[8]>1:header[7] || header[8]) return DispatcherReject(__LINE__,header[7]?header[7]:header[8]);
	for (unsigned i=9;i<16;i++) if (header[i]) return DispatcherReject(__LINE__,i);
#else
	for (unsigned i=7;i<16;i++) if (header[i]) return DispatcherReject(__LINE__,i);
#endif
#endif
#endif
	uint32_t parts[119][8],banks[8];
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	uint32_t ffn[64],firstRows[119];
#endif
	unsigned blocks=operation==1?inputs>>(type==8?5:8):0;
	unsigned q35RootTiles=0,q35RootFlits=0;
	for (unsigned bank=0;bank<8;bank++) banks[bank]=Words(0x74040)[bank];
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	for (unsigned w=0;w<119;w++) for (unsigned i=0;i<8;i++) parts[w][i]=Words(0x74080)[w*8+i];
	if (!PartsValid(parts,banks,header)) return DispatcherReject(__LINE__,0);
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	if (operation==1) {
		for(unsigned i=0;i<64;i++)ffn[i]=Words(TT_Q38_FFN_MATRIX_FRAME)[i];
		for(unsigned i=0;i<119;i++)firstRows[i]=Words(TT_Q38_FFN_MATRIX_ROWS)[i];
			if(!PioneerQ38FfnMatrixShape(ffn,header,banks,parts,firstRows))return DispatcherReject(__LINE__,0);
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  for(unsigned i=0;i<64;i++)Words(TT_Q38_PROJECTION_MATRIX_SHADOW)[i]=ffn[i];
  for(unsigned i=0;i<119;i++)Words(TT_Q38_PROJECTION_ROWS_SHADOW)[i]=firstRows[i];
  Fence();
#endif
	}
#endif
#else
	for (unsigned bank=0;bank<7;bank++) {
		if (!banks[bank] || banks[bank]>0xfff) return DispatcherReject(__LINE__,bank);
		for (unsigned other=0;other<bank;other++) if (banks[bank]==banks[other]) return DispatcherReject(__LINE__,(bank<<16)|other);
	}
	if (banks[7]) return DispatcherReject(__LINE__,banks[7]);
	for (unsigned w=0;w<119;w++) {
		for (unsigned i=0;i<8;i++) parts[w][i]=Words(0x74080)[w*8+i];
		const uint32_t* p=parts[w];
		if (w>=workers) { for (unsigned i=0;i<8;i++) if (parts[w][i]) return DispatcherReject(__LINE__,(w<<8)|i); continue; }
		if (!p[3] || p[3]>nonce || !p[5] || p[5]!=Words(Q38DispatchSourceCache)[w*16]
			|| p[7] || (header[7]?(p[6]>=PioneerQwen35::Vocabulary || (p[6]&15)) : p[6]))
			return DispatcherReject(__LINE__,w);
		if (operation==2) { if (p[0] || p[1] || p[2] || p[4]) return DispatcherReject(__LINE__,w); continue; }
		unsigned bytes=p[2]*blocks*Tile(type);
		if (p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || !p[2]
			|| (header[7]?(p[2]>(PioneerQwen35::Vocabulary/16) || p[6]+p[2]*16>PioneerQwen35::Vocabulary)
				:p[2]>16) || p[1]>=7
			|| bytes>0xc4000000-p[0] || p[4]!=banks[p[1]]) return DispatcherReject(__LINE__,w);
		for (unsigned other=0;other<w;other++) if (p[1]==parts[other][1]
			&& p[0]<parts[other][0]+parts[other][2]*blocks*Tile(type) && parts[other][0]<p[0]+bytes) return DispatcherReject(__LINE__,(w<<16)|other);
	}
#endif
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	volatile uint32_t* q35RootJob=Words(Q35RootProjectionJob);
	if (operation==1 && q35RootJob[0]) {
		if (q35RootJob[0]!=PioneerQwen35Projection::Magic || q35RootJob[1]!=type
			|| q35RootJob[2]!=Tile(type) || q35RootJob[3] || q35RootJob[4]
			|| q35RootJob[5]!=inputs || !q35RootJob[7] || q35RootJob[7]>16
			|| q35RootJob[9]>=7 || q35RootJob[8]!=banks[q35RootJob[9]]) return DispatcherReject(__LINE__,q35RootJob[9]);
		unsigned bytes=q35RootJob[7]*blocks*Tile(type);
		if (q35RootJob[6]<0x04000000 || q35RootJob[6]>=0xc4000000 || (q35RootJob[6]&63)
			|| bytes>0xc4000000-q35RootJob[6]) return DispatcherReject(__LINE__,q35RootJob[7]);
		for (unsigned w=0;w<workers;w++) if (parts[w][1]==q35RootJob[9]
			&& q35RootJob[6]<parts[w][0]+parts[w][2]*blocks*Tile(type)
			&& parts[w][0]<q35RootJob[6]+bytes) return DispatcherReject(__LINE__,w);
		for (unsigned i=10;i<16;i++) if (q35RootJob[i]) return DispatcherReject(__LINE__,i);
		q35RootTiles=q35RootJob[7]*blocks;
		q35RootFlits=q35RootTiles*Tile(type)/64;
	} else {
		for (unsigned i=0;i<16;i++) if (q35RootJob[i]) return DispatcherReject(__LINE__,i);
	}
#endif
	if (operation==1) {
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
#ifndef PIONEER_Q38_FFN_RESIDENT_MATRIX
		if (!PrepareInputs(header[7],type,inputs)) return DispatcherReject(__LINE__,0);
#endif
#else
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		// Expand BACKWARDS inside the exclusively owned original input
		// region. Read BOTH source words before a destination store. Peers
		// have not received descriptors/doorbells and cannot read this job.
		if (!ExpandInputs(inputs)) return DispatcherReject(__LINE__,0); // original validation+final host bit checks remain
#endif
		for (unsigned w=0;w<inputs/2;w++) if (!PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w]))
			|| !PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w]>>16))) return DispatcherReject(__LINE__,w);
		unsigned padded=(blocks+15)&~15U;
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
		if (type==13 && blocks<padded) {
			Words(PioneerQwen35::RootScaleEvidenceAddress)[14]=Words(Q38DispatchScaleCache)[blocks];
			Fence();
		}
#endif
		for (unsigned b=0;b<padded;b++) {
			uint32_t bits=Words(Q38DispatchScaleCache)[b],magnitude=bits&0x7fffffff;
			if (b>=blocks?bits!=0:magnitude && (magnitude<0x2f800000 || magnitude>0x47800000))
				return DispatcherReject(__LINE__,b,bits,(type<<24)|(inputs&0x00ffffffU));
		}
#endif
	}
	Checkpoint(0x54000002,operation);
	if (!Idle()) block_fault_park();
	uint32_t source[16]; for (unsigned i=0;i<16;i++) source[i]=*Words(0xffb20000+SourceOffset(i));
	for (unsigned i=0;i<64;i++) Words(0x7f800)[i]=0;
	for (unsigned i=0;i<7;i++) Words(0x7f800)[i]=header[i];
	Words(0x7f800)[6]=0; // transfer counters,NOT header nonce (preserved at12)
	Words(0x7f800)[12]=nonce;
	for (unsigned c=0;c<8;c++) {
  Words(0x7f800)[16+c]=*Words(0xffb20000+Offset(c));
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  if(c==6 && Words(0x7f800)[18]!=Words(0x7f800)[22])block_fault_park();
#endif
 }
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	if(operation==1) {
		FfnMatrix::Fetch(ffn,firstRows);
		if(!PrepareInputs(TT_Q38_FFN_MATRIX_RESIDENT_INPUT,type,inputs))block_fault_park();
	}
#endif
	// Verify ALL previous idle sequences/physical PCs/reset before staging
	// ANY doorbell. No host MMIO per peer is needed in this future image.
	for (unsigned w=0;w<workers;w++) {
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
		// Kernel owns this immutable cohort exclusively. Bootstrap proved
		// ALL119 WAIT PCs;each successful prior RUN proved CURRENT physical
		// WAIT PCs/reset/source/counters before publication. No peer writer
		// exists between jobs. Reuse that prior proof ONLY for WORK;keep the
		// full CURRENT completion/physical-PC audit below and ALL STOP audits.
		if (work) continue;
#endif
		bool ready=false;
		for (unsigned attempt=0;attempt<65536 && !ready;attempt++) ready=Peer(w+1,parts[w][3]-1,false,true Q38_COMMAND_WITH(false));
		if (!ready) block_fault_park();
	}
	for (unsigned w=0;w<workers;w++) {
		const uint32_t* p=parts[w]; unsigned worker=w+1;
		volatile uint32_t descriptor[16];
		for (unsigned i=0;i<16;i++) descriptor[i]=0; // explicit word stores,NO libc memset/byte writes
		descriptor[0]=operation==1?
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
			static_cast<uint32_t>(PioneerQwen35Projection::Magic)
#else
			0x31483851U
#endif
			:0; descriptor[1]=operation==1?type:0; descriptor[2]=operation==1?Tile(type):0;
		descriptor[5]=operation==1?inputs:0; descriptor[6]=operation==1?p[0]:0; descriptor[7]=operation==1?p[2]:0;
		descriptor[8]=operation==1?p[4]:0; descriptor[9]=operation==1?1:0;
		descriptor[3]=operation==1 && header[7]?p[6]:0;
		descriptor[10]=Q38DispatchPeerMarker; descriptor[11]=worker; descriptor[12]=p[3]; descriptor[13]=operation;
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
		descriptor[14]=work?1U:0U; // PRIVATE numeric-only mode; STOP and checked remain0
#endif
		descriptor[15]=operation==1?header[7]:0;
		for (unsigned i=0;i<16;i++) Words(0x75000)[i]=descriptor[i];
		Fence();
		Transfer(true,worker,0x75000,0x74000,64 Q38_COMMAND_WITH(work));
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
		// Transfer returns only after exact ACK/counter completion. The peer
		// independently validates its immutable descriptor and publishes it
		// in the mandatory full final proof. No redundant descriptor echo.
		if (work) continue;
#endif
		Transfer(false,worker,0x75040,0x74000,64 Q38_COMMAND_WITH(false));
		for (unsigned i=0;i<16;i++) if (Words(0x75040)[i]!=descriptor[i]) block_fault_park();
	}
	for (unsigned w=0;w<workers;w++) {
		*Words(0x75000)=parts[w][3]; Fence(); Transfer(true,w+1,0x75000,0x74100,4 Q38_COMMAND_WITH(work));
	}
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	Checkpoint(0x5d000001,workers); // all peer doorbells returned from Transfer
	if (q35RootTiles) {
		Checkpoint(0x5d000002,q35RootTiles);
		if (!Q35RootMatrix(q35RootJob,type,inputs)) block_fault_park();
		Checkpoint(0x5d000003,q35RootTiles);
	}
	Checkpoint(0x5d000004,workers); // root projection phase complete; peer completion polling follows
	if(work)q35Profile.Mark(0); // command/doorbell issue and ROOT matrix engine
#endif
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
	cycles.Mark(0); // validation/staging/descriptor+doorbell transfers
#endif
#ifdef PIONEER_Q38_WORK_SENTINEL_POLL
	if(!WaitPeers(parts,workers,work,operation==2))block_fault_park();
#else
	volatile uint32_t complete[119]; for (unsigned w=0;w<119;w++) complete[w]=0;
	unsigned finished=0;
	for (unsigned round=0;round<65536 && finished<workers;round++) for (unsigned w=0;w<workers;w++)
		if (!complete[w] && Peer(w+1,parts[w][3],operation==2,false Q38_COMMAND_WITH(false))) { complete[w]=1; finished++; }
	if (finished!=workers) block_fault_park();
#endif
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	Checkpoint(0x5d000005,workers); // all peers reported completion
#endif
	Checkpoint(0x54000300,workers);
	// Completion order is arbitrary. STOP has no ordered output gathers,
	// so explicitly finish with worker119's terminal physical audit. This
	// makes the final NIU command deterministic for the kernel's existing
	// exact proof;do not weaken that proof to accept an arbitrary command.
	if (operation==2) {
		bool terminal=Peer(workers,parts[workers-1][3],true,false Q38_COMMAND_WITH(false));
		Checkpoint(0x54000301,workers);
		if (!terminal) block_fault_park();
	}
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	if(work)q35Profile.Mark(1); // peer completion wait and physical terminal audit
#endif
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
	cycles.Mark(1); // worker wait+CURRENT physical terminal audits,not pure arithmetic
#endif
	// ALL readers finished BEFORE ANY proof/output gather or publication.
#ifdef PIONEER_Q38_WORK_SPARSE_EXPORT
	// WORK never returns the private peer-proof array to the host: EVERY
	// actual gathered proof is still checked below, before ANY output DMA.
	// Each active output slot is fully overwritten below, including its
	// zero tail. Clear only inactive output slots; their bytes stay exactly
	// the same as checked mode. STOP/checked retain the original full clear.
	if (!work) for (unsigned i=0;i<119*64;i++) Words(0x76000)[i]=0;
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
	// Work writes EVERY transported word below. Neither inactive workers
	// nor fixed per-worker tails are part of its compact transfer extent.
	if (!work) for (unsigned i=0;i<119*256;i++) Words(0x80000)[i]=0;
	uint32_t packedWords=0;
#else
	for (unsigned i=work?workers*256:0;i<119*256;i++) Words(0x80000)[i]=0;
#endif
#else
	for (unsigned i=0;i<119*64;i++) Words(0x76000)[i]=0;
	for (unsigned i=0;i<119*256;i++) Words(0x80000)[i]=0;
#endif
#ifdef PIONEER_Q35_WORK_PEER_EXEC_PROFILE
	unsigned peerComputeCriticalCycles=0;
#endif
	if (operation==1) for (unsigned w=0;w<workers;w++) {
#ifdef PIONEER_Q35_WORK_BATCH_FETCH
		const uint32_t* p=parts[w]; unsigned groups=p[2],weightReads=groups*blocks;
		if(work)weightReads=groups*PioneerQwen35FetchCount(type,blocks); // bounded <=8KiB Qwen35 batches
		unsigned reads=weightReads+(inputs*2+16383)/16384+1;
#elif defined(PIONEER_Q38_WORK_BATCH4_FETCH)
		const uint32_t* p=parts[w]; unsigned groups=p[2],weightReads=groups*blocks;
		if(work)weightReads>>=type==8?2:1; // bounded <=8KiB DRAM batches
		unsigned reads=weightReads+(inputs*2+16383)/16384+1;
#else
		const uint32_t* p=parts[w]; unsigned groups=p[2],reads=groups*blocks+(inputs*2+16383)/16384+1;
#endif
		unsigned flits=groups*blocks*Tile(type)/64+(inputs*2+((blocks*4+63)&~63U))/64;
		const unsigned proofBytes=header[7]?320U:256U+groups*64U;
		Transfer(false,w+1,0x75000,0x83f00,proofBytes Q38_COMMAND_WITH(work));
		const volatile uint32_t* proof=Words(0x75000);
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  if(proof[2]!=proof[6] || proof[10]!=proof[14])block_fault_park();
#endif
		for (unsigned c=0;c<8;c++) if (uint32_t(proof[c+8]-proof[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) block_fault_park();
		volatile uint32_t command[10]; for (unsigned i=0;i<10;i++) command[i]=0;
		command[0]=p[0]+(groups*blocks-1)*Tile(type); command[2]=p[4]; command[3]=0x90000; command[5]=XY(w+1); command[8]=Tile(type);
#ifdef PIONEER_Q35_WORK_BATCH_FETCH
		if(work){unsigned batch=PioneerQwen35FetchBatch(type),ordinal=PioneerQwen35FetchOrdinal(type,blocks-1),firstBlock=ordinal*batch;
			unsigned batchBlocks=blocks-firstBlock<batch?blocks-firstBlock:batch;
			command[0]=p[0]+((groups-1)*blocks+firstBlock)*Tile(type);command[3]=0x98000;command[8]=batchBlocks*Tile(type);}
#ifdef PIONEER_Q35_WORK_BATCH_PREFETCH
		if(work)command[3]=PioneerQwen35BatchPrefetchBuffer(PioneerQwen35FetchOrdinal(type,blocks-1));
#endif
#elif defined(PIONEER_Q38_WORK_BATCH4_FETCH)
		if(work){unsigned batch=type==8?4:2;command[0]-=(batch-1)*Tile(type);command[3]=0x98000;command[8]*=batch;}
#endif
		if (proof[16]!=groups*inputs/16 || proof[17]!=p[3] || proof[18]!=w+1 || proof[19]!=Q38DispatchPeerMarker
			|| proof[30]!=p[5] || proof[31]!=XY(w+1)) block_fault_park();
		for (unsigned i=0;i<10;i++) if (proof[20+i]!=command[i]) block_fault_park();
		for (unsigned i=0;i<16;i++) if (proof[32+i]!=Words(Q38DispatchSourceCache)[w*16+i]) block_fault_park();
		for (unsigned i=48;i<64;i++) {
#ifdef PIONEER_Q35_WORK_PEER_EXEC_PROFILE
			if (i!=48 && proof[i]) block_fault_park();
#else
			if (proof[i]) block_fault_park();
#endif
		}
#ifdef PIONEER_Q35_WORK_PEER_EXEC_PROFILE
		if (proof[48]>peerComputeCriticalCycles) peerComputeCriticalCycles=proof[48];
#endif
		if (header[7]) {
			const unsigned logicalRow=proof[65];
			if ((proof[64]&0x7f800000)==0x7f800000 || logicalRow<p[6]
				|| logicalRow>=p[6]+groups*16) block_fault_park();
		} else for (unsigned row=0;row<groups*16;row++)
			if ((proof[64+row]&0x7f800000)==0x7f800000) block_fault_park();
#if defined(PIONEER_Q38_WORK_SPARSE_EXPORT) && !defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE) && !defined(PIONEER_Q38_RESIDENT_ADVANCE_IMAGE) && !defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE)
		if (!work)
#endif
		// Joined advance needs these ACTUAL verified receipts for its
		// independent per-worker BEFORE/current-counter chain. Retain them
		// in private L1 only: no host export or extra NoC transaction.
		for (unsigned i=0;i<64;i++) Words(0x76000)[w*64+i]=proof[i];
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		if (work) {
			unsigned begin=packedWords;
			if (!PioneerQ38CompactOutputAppend(groups,packedWords)) block_fault_park();
			for (unsigned row=0;row<groups*16;row++) Words(0x80000)[begin+row]=proof[64+row];
		} else
#endif
		if (header[7]) {
			for (unsigned row=0;row<256;row++) Words(0x80000)[w*256+row]=row<2?proof[64+row]:0;
		} else for (unsigned row=0;row<256;row++)
			Words(0x80000)[w*256+row]=row<groups*16?proof[64+row]:0;
	}
#ifdef PIONEER_Q35_WORK_PEER_EXEC_PROFILE
	// Advisory only: record the max peer mcycle delta after every gathered
	// proof passed. Overflow wraps telemetry but never changes Work validity.
	if (work && Words(PioneerQwen35::RootProgressAddress)[2]
		&& Words(PioneerQwen35::RootProgressAddress)[3]) {
		volatile uint32_t* profile=Words(PioneerQwen35::RootPeerExecProfileAddress);
		const uint32_t prior=profile[0], next=prior+peerComputeCriticalCycles;
		profile[0]=next;
		profile[1]+=next<prior;
		profile[2]++;
		Fence();
	}
#endif
	for (unsigned w=0;w<119;w++) for (unsigned i=0;i<8;i++) if (parts[w][i]!=Words(0x74080)[w*8+i]) block_fault_park();
	Checkpoint(0x54000401,workers);
	for (unsigned bank=0;bank<8;bank++) if (banks[bank]!=Words(0x74040)[bank]) block_fault_park();
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
	if(work&&!CapturePeerPhase(parts,workers,type,inputs,peerMetrics))block_fault_park();
#endif
#ifdef PIONEER_Q38_OUTPUT_TRANSFER_ROOT
	// EVERY peer's CURRENT terminal PC/source/counters and every gathered
	// output have already passed. Never DMA while a peer may still write.
	if (work && !ReturnHostOutput(header,source
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		,packedWords
#endif
		)) block_fault_park();
#endif
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	// All119/active participant proofs and every gathered result passed
	// above. No resident output while ANY peer may still write.
	if(work) {
		for(unsigned i=0;i<16;i++)if(header[i]!=job[i] || source[i]!=*Words(0xffb20000+SourceOffset(i)))block_fault_park();
		FfnMatrix::Scatter(ffn,firstRows,parts,packedWords);
	}
#endif
	for (unsigned i=0;i<16;i++) if (header[i]!=job[i] || source[i]!=*Words(0xffb20000+SourceOffset(i))) block_fault_park();
	Checkpoint(0x54000402,workers);
	if (!Idle()) block_fault_park();
	for (unsigned c=0;c<8;c++) {
  Words(0x7f800)[24+c]=*Words(0xffb20000+Offset(c));
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  if(c==6 && Words(0x7f800)[26]!=Words(0x7f800)[30])block_fault_park();
#endif
 }
	unsigned groups=0; for (unsigned w=0;w<workers;w++) groups+=parts[w][2];
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	Words(0x7f800)[48]=work;
	unsigned reads=Words(0x7f800)[10]+3*Words(0x7f800)[11]+workers*(operation==1 && !work?2:1);
	unsigned flits=Words(0x7f800)[10]+3*Words(0x7f800)[11]+(work?0:workers)+(operation==1?4*workers+groups:0);
#else
	unsigned reads=Words(0x7f800)[10]+3*Words(0x7f800)[11]+workers*(operation==1?2:1);
	unsigned flits=Words(0x7f800)[10]+3*Words(0x7f800)[11]+workers+(operation==1?4*workers+groups:0);
#endif
#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
#ifdef PIONEER_Q35_WORK_BATCH_FETCH
	const unsigned q35Blocks=inputs>>(type==8?5:8);
	const unsigned q35RootReads=q35RootJob[7]*PioneerQwen35FetchCount(type,q35Blocks);
	Words(0x7f800)[6]+=q35RootReads; reads+=q35RootReads;
#else
	Words(0x7f800)[6]+=q35RootTiles; reads+=q35RootTiles;
#endif
	Words(0x7f800)[8]+=q35RootFlits; flits+=q35RootFlits;
#endif
	Checkpoint(0x54000403,reads);
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
	if(work){reads++;flits++;} // exact additional64B read,response+oneflit;NEVER unaccounted
#endif
#if defined(PIONEER_Q38_OUTPUT_TRANSFER_ROOT) || defined(PIONEER_Q38_FFN_RESIDENT_MATRIX)
	unsigned writes=workers*2,writeFlits=writes;
	if (work) {
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
		unsigned outputPackets=PioneerQ38FfnMatrixOrdered(parts,firstRows,workers)?(ffn[12]+16383)/16384:workers;
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  unsigned inputReads=PioneerQ38ProjectionMatrixInputReads(ffn),inputFlits=PioneerQ38ProjectionMatrixInputFlits(ffn);
  reads+=inputReads;flits+=inputFlits;
  writes+=outputPackets;writeFlits+=ffn[12]/64;
  if(Words(0x7f800)[50]!=inputReads || Words(0x7f800)[51]!=inputFlits
   || Words(0x7f800)[52]!=outputPackets || Words(0x7f800)[53]!=ffn[12]/64)block_fault_park();
#else
		reads+=PioneerQ38FfnMatrixInputReads(ffn[5]);flits+=PioneerQ38FfnMatrixInputFlits(ffn[5]);
		writes+=outputPackets;writeFlits+=ffn[12]/64;
		if(Words(0x7f800)[50]!=PioneerQ38FfnMatrixInputReads(ffn[5]) || Words(0x7f800)[51]!=PioneerQ38FfnMatrixInputFlits(ffn[5])
			|| Words(0x7f800)[52]!=outputPackets || Words(0x7f800)[53]!=ffn[12]/64)block_fault_park();
#endif
#else
		unsigned bytes=
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
			PioneerQ38CompactOutputBytes(workers,packedWords),
#else
			workers*1024,
#endif
			chunks=(bytes+4095)/4096;
		reads+=chunks; flits+=bytes/64; writes+=chunks; writeFlits+=bytes/64;
#endif
	}
	if (Words(0x7f800)[6]!=reads || Words(0x7f800)[7]!=writes || Words(0x7f800)[8]!=flits || Words(0x7f800)[9]!=writeFlits) block_fault_park();
#else
	if (Words(0x7f800)[6]!=reads || Words(0x7f800)[7]!=workers*2 || Words(0x7f800)[8]!=flits || Words(0x7f800)[9]!=workers*2) block_fault_park();
#endif
	for (unsigned c=0;c<8;c++) {
		Checkpoint(0x54000410+c,reads);
#if defined(PIONEER_Q38_OUTPUT_TRANSFER_ROOT) || defined(PIONEER_Q38_FFN_RESIDENT_MATRIX)
		unsigned expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+writes:c==5?writeFlits:writes;
#else
		unsigned expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+workers*2:workers*2;
#endif
		if (uint32_t(Words(0x7f800)[24+c]-Words(0x7f800)[16+c])!=expected) block_fault_park();
	}
	for (unsigned i=0;i<16;i++) Words(0x7f800)[32+i]=source[i];
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
	cycles.Mark(2); // proof/output gather+DMA+final source/counter proof
	cycles.Publish(); // bounded existing statistics padding,Work ONLY
#endif
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
	if(work)for(unsigned w=0;w<7;w++)Words(0x7f800)[57+w]=peerMetrics[w]; // DISTINCT DATA marker,not root-cycle semantics
#endif
	#ifdef PIONEER_Q35_WORK_ROOT_IMAGE
	if(work)q35Profile.Mark(2); // gathered output, counter/source proofs, and final idle check
	#endif
	Fence(); return workers;
}
