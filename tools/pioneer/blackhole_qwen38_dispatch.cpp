/* SPDX-License-Identifier: MIT */
// Private root dispatcher source. Selection requires a pinned kernel image.
// The kernel MUST authenticate/own ALL initialized bank extents,private
// tasks/bank routes/source seals and all119 peer images/sequence histories.
// No physical route,address or sequence here is a new userspace capability.
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
#ifdef PIONEER_Q38_WORK_SENTINEL_POLL
#if !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_WORK_COHORT_CACHE) || !defined(PIONEER_Q38_WORK_COMPACT_OUTPUT)
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
// Diagnostic ONLY in the separately owned/saved runtime-status L1. Normal
// completion overwrites BOTH words with the exact fixed DONE/count proof.
// Fault park preserves them;NO extra NoC packet/host DMA/source authority.
static __attribute__((noinline)) void Checkpoint(unsigned stage,unsigned detail)
{
	Words(0x17ffc0)[0]=stage; Words(0x17ffc0)[1]=detail; Fence();
}
extern "C" void block_fault_park() __attribute__((noreturn));
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
	for (unsigned i=13;i<16;i++) if (header[i]) return false;
	if (!header[8]) { for (unsigned i=9;i<13;i++) if (header[i]) return false; return true; }
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
		|| ((local^remote)&31) || !PacketIdle()) block_fault_park();
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
	Fence(); *Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
	Checkpoint(0x54000102,worker);
#ifdef PIONEER_Q38_TRANSFER_DIAGNOSTIC
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
	// stage:58|worker[22:16]|write[15]|remote[13:4] at[14:5]|
	// cmd-busy[4]|counter-mismatch[3:0]. detail:LAST mismatching counter's
	// observed-minus-expected,mod2^32 (highest set bit identifies counter).
	// pending==0 means the original final PacketIdle check rejected.
	Checkpoint(0x58000000|(worker<<16)|(write?0x8000U:0U)|(((remote>>4)&1023U)<<5)|pending,delta);
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
static bool Peer(unsigned worker,unsigned sequence,bool stopped,bool previous Q38_COMMAND_PARAMETER)
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
	if (status[2] || status[3]!=Q38DispatchPeerMarker || status[0]>sequence || status[1]>sequence || status[4]>2) block_fault_park();
	for (unsigned i=
#ifdef PIONEER_Q38_PEER_PROGRESS
		6
#else
		5
#endif
		;i<16;i++) if (status[i]) block_fault_park();
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
		if (status[0]+1<sequence || status[1]+1<sequence) block_fault_park();
		if (status[0]!=sequence || status[1]!=sequence || status[4]!=(stopped?2U:0U)) return false;
	}
	Words(0x7f800)[11]++;
	Transfer(false,worker,0x75000,0xffb13140,4 Q38_COMMAND_WITH(reuseCommand)); unsigned first=*Words(0x75000);
	Transfer(false,worker,0x75000,0xffb13140,4 Q38_COMMAND_WITH(reuseCommand)); unsigned second=*Words(0x75000);
	Transfer(false,worker,0x75010,0xffb121b0,4 Q38_COMMAND_WITH(reuseCommand)); unsigned reset=*Words(0x75010);
	Checkpoint(0x54000204,(worker<<24)|(first&0xffffff));
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
 // Exact COMPLETE QFP3 image ranges,NOT a role grant from the shared boot
 // marker. A separate owner/catalog MUST pin all four new sections.
 const unsigned pc[]={first,second};
 for(unsigned value:pc)if(reset!=0x46800 || (value&3) || value==TT_Q38_PEER_FAULT_ENTRY || value==TT_Q38_PEER_FAULT_PARK
  || (!stopped && PioneerQ38PeerTerminalPc(value))
  || (!((value>=0x64000 && value<0x67fe0) || (value>=0x70b00 && value<0x71f80) || (value>=0x72000 && value<0x73fe4))))block_fault_park();
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
		unsigned sentinel=0;if(!PioneerQ38SelectPollSentinel(parts,workers,sentinel))return false;
		for(unsigned attempt=0;attempt<65536 && polls<budget;attempt++) {
			polls++;
			if(Peer(sentinel+1,parts[sentinel][3],false,false Q38_COMMAND_WITH(work))) {complete[sentinel]=1;finished=1;break;}
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
			if(Peer(w+1,parts[w][3],stopped,false Q38_COMMAND_WITH(work))){complete[w]=1;finished++;}
		}
		if(work && finished<workers)PeerPollBackoff(backoff);
	}
	return finished==workers;
}
#endif
static unsigned Tile(unsigned type) { return type==8?576:type==12?2304:type==14?3392:0; }
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
#ifdef PIONEER_Q38_PEER_PHASE_ROOT
	uint32_t peerMetrics[7];
#endif
#endif
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
	PioneerQ38RootCycleProfile cycles(work);
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	if (!HeaderValid(header)) return 0;
#else
	if (header[0]!=0x314d3851 || header[1]!=1 || (operation!=1 && operation!=2) || !workers || workers>119
		|| !nonce || nonce>=0x7fffffff || (operation==2 && (type || inputs || workers!=119))
		|| (operation==1 && (!Tile(type) || (inputs!=5120 && inputs!=6144 && inputs!=17408)))) return 0;
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
	if (header[7]!=(operation==1?1U:0U)) return 0;
	for (unsigned i=8;i<16;i++) if (header[i]) return 0;
#else
	for (unsigned i=7;i<16;i++) if (header[i]) return 0;
#endif
#endif
	uint32_t parts[119][8],banks[8];
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	uint32_t ffn[64],firstRows[119];
#endif
	unsigned blocks=operation==1?inputs>>(type==8?5:8):0;
	for (unsigned bank=0;bank<8;bank++) banks[bank]=Words(0x74040)[bank];
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	for (unsigned w=0;w<119;w++) for (unsigned i=0;i<8;i++) parts[w][i]=Words(0x74080)[w*8+i];
	if (!PartsValid(parts,banks,header)) return 0;
#ifdef PIONEER_Q38_FFN_RESIDENT_MATRIX
	if (operation==1) {
		for(unsigned i=0;i<64;i++)ffn[i]=Words(TT_Q38_FFN_MATRIX_FRAME)[i];
		for(unsigned i=0;i<119;i++)firstRows[i]=Words(TT_Q38_FFN_MATRIX_ROWS)[i];
		if(!PioneerQ38FfnMatrixShape(ffn,header,banks,parts,firstRows))return 0;
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  for(unsigned i=0;i<64;i++)Words(TT_Q38_PROJECTION_MATRIX_SHADOW)[i]=ffn[i];
  for(unsigned i=0;i<119;i++)Words(TT_Q38_PROJECTION_ROWS_SHADOW)[i]=firstRows[i];
  Fence();
#endif
	}
#endif
#else
	for (unsigned bank=0;bank<7;bank++) {
		if (!banks[bank] || banks[bank]>0xfff) return 0;
		for (unsigned other=0;other<bank;other++) if (banks[bank]==banks[other]) return 0;
	}
	if (banks[7]) return 0;
	for (unsigned w=0;w<119;w++) {
		for (unsigned i=0;i<8;i++) parts[w][i]=Words(0x74080)[w*8+i];
		const uint32_t* p=parts[w];
		if (w>=workers) { for (unsigned value:parts[w]) if (value) return 0; continue; }
		if (!p[3] || p[3]>nonce || !p[5] || p[5]!=Words(Q38DispatchSourceCache)[w*16] || p[6] || p[7]) return 0;
		if (operation==2) { if (p[0] || p[1] || p[2] || p[4]) return 0; continue; }
		unsigned bytes=p[2]*blocks*Tile(type);
		if (p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || !p[2] || p[2]>16 || p[1]>=7
			|| bytes>0xc4000000-p[0] || p[4]!=banks[p[1]]) return 0;
		for (unsigned other=0;other<w;other++) if (p[1]==parts[other][1]
			&& p[0]<parts[other][0]+parts[other][2]*blocks*Tile(type) && parts[other][0]<p[0]+bytes) return 0;
	}
#endif
	if (operation==1) {
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
#ifndef PIONEER_Q38_FFN_RESIDENT_MATRIX
		if (!PrepareInputs(header[7],type,inputs)) return 0;
#endif
#else
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		// Expand BACKWARDS inside the exclusively owned original input
		// region. Read BOTH source words before a destination store. Peers
		// have not received descriptors/doorbells and cannot read this job.
		if (!ExpandInputs(inputs)) return 0; // original validation+final host bit checks remain
#endif
		for (unsigned w=0;w<inputs/2;w++) if (!PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w]))
			|| !PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w]>>16))) return 0;
		unsigned padded=(blocks+15)&~15U;
		for (unsigned b=0;b<padded;b++) {
			uint32_t bits=Words(0xab000)[b],magnitude=bits&0x7fffffff;
			if (b>=blocks?bits!=0:magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return 0;
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
		descriptor[0]=operation==1?0x31483851U:0; descriptor[1]=operation==1?type:0; descriptor[2]=operation==1?Tile(type):0;
		descriptor[5]=operation==1?inputs:0; descriptor[6]=operation==1?p[0]:0; descriptor[7]=operation==1?p[2]:0;
		descriptor[8]=operation==1?p[4]:0; descriptor[9]=operation==1?1:0;
		descriptor[10]=Q38DispatchPeerMarker; descriptor[11]=worker; descriptor[12]=p[3]; descriptor[13]=operation;
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
		descriptor[14]=work?1U:0U; // PRIVATE numeric-only mode; STOP and checked remain0
#endif
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
	if (operation==1) for (unsigned w=0;w<workers;w++) {
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
		const uint32_t* p=parts[w]; unsigned groups=p[2],weightReads=groups*blocks;
		if(work)weightReads>>=type==8?2:1; // bounded <=8KiB DRAM batches
		unsigned reads=weightReads+(inputs*2+16383)/16384+1;
#else
		const uint32_t* p=parts[w]; unsigned groups=p[2],reads=groups*blocks+(inputs*2+16383)/16384+1;
#endif
		unsigned flits=groups*blocks*Tile(type)/64+(inputs*2+((blocks*4+63)&~63U))/64;
		Transfer(false,w+1,0x75000,0x83f00,256+groups*64 Q38_COMMAND_WITH(work));
		const volatile uint32_t* proof=Words(0x75000);
#ifdef PIONEER_Q38_PROJECTION_MATRIX_ROOT
  if(proof[2]!=proof[6] || proof[10]!=proof[14])block_fault_park();
#endif
		for (unsigned c=0;c<8;c++) if (uint32_t(proof[c+8]-proof[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) block_fault_park();
		volatile uint32_t command[10]; for (unsigned i=0;i<10;i++) command[i]=0;
		command[0]=p[0]+(groups*blocks-1)*Tile(type); command[2]=p[4]; command[3]=0x90000; command[5]=XY(w+1); command[8]=Tile(type);
#ifdef PIONEER_Q38_WORK_BATCH4_FETCH
		if(work){unsigned batch=type==8?4:2;command[0]-=(batch-1)*Tile(type);command[3]=0x98000;command[8]*=batch;}
#endif
		if (proof[16]!=groups*inputs/16 || proof[17]!=p[3] || proof[18]!=w+1 || proof[19]!=Q38DispatchPeerMarker
			|| proof[30]!=p[5] || proof[31]!=XY(w+1)) block_fault_park();
		for (unsigned i=0;i<10;i++) if (proof[20+i]!=command[i]) block_fault_park();
		for (unsigned i=0;i<16;i++) if (proof[32+i]!=Words(Q38DispatchSourceCache)[w*16+i]) block_fault_park();
		for (unsigned i=48;i<64;i++) if (proof[i]) block_fault_park();
		for (unsigned row=0;row<groups*16;row++) if ((proof[64+row]&0x7f800000)==0x7f800000) block_fault_park();
#ifdef PIONEER_Q38_WORK_SPARSE_EXPORT
		if (!work)
#endif
		for (unsigned i=0;i<64;i++) Words(0x76000)[w*64+i]=proof[i];
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		if (work) {
			unsigned begin=packedWords;
			if (!PioneerQ38CompactOutputAppend(groups,packedWords)) block_fault_park();
			for (unsigned row=0;row<groups*16;row++) Words(0x80000)[begin+row]=proof[64+row];
		} else
#endif
		for (unsigned row=0;row<256;row++) Words(0x80000)[w*256+row]=row<groups*16?proof[64+row]:0;
	}
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
	Fence(); return workers;
}
