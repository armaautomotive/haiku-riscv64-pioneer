/* SPDX-License-Identifier: MIT */
// OFFLINE root dispatcher prerequisite. NO kernel/ABI selects this image.
// Future kernel MUST authenticate/own ALL initialized bank extents,private
// tasks/bank routes/source seals and all119 peer images/sequence histories.
// No physical route,address or sequence here is a new userspace capability.
typedef __UINT32_TYPE__ uint32_t;
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerTerminal.h"
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
static void Fence() { asm volatile("fence" ::: "memory"); }
// Diagnostic ONLY in the separately owned/saved runtime-status L1. Normal
// completion overwrites BOTH words with the exact fixed DONE/count proof.
// Fault park preserves them;NO extra NoC packet/host DMA/source authority.
static __attribute__((noinline)) void Checkpoint(unsigned stage,unsigned detail)
{
	Words(0x17ffc0)[0]=stage; Words(0x17ffc0)[1]=detail; Fence();
}
extern "C" void block_fault_park() __attribute__((noreturn));
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
	bool okay=true;
	for (unsigned c=0;c<4;c++) okay &= !*Words(0xffb20040+c*0x800);
	for (unsigned c=0;c<32;c++) okay &= !(*Words(0xffb20240+c*4)&255);
	return okay;
}
static bool PacketIdle() { return !*Words(0xffb20040) && !(*Words(0xffb20240)&255); }
static void Transfer(bool write,unsigned worker,unsigned local,unsigned remote,unsigned bytes)
{
	// ONLY the exact fixed peer data/status/PC/reset windows below. Never
	// code/boot writes,peer resets,host DMA or a caller-supplied coordinate.
	bool permitted=write?(remote==0x74000 && bytes==64) || (remote==0x74100 && bytes==4)
		:(remote==0x74000 && bytes==64) || (remote==0x74140 && bytes==64)
		|| (remote==0xffb13140 && bytes==4) || (remote==0xffb121b0 && bytes==4)
		|| (remote==0x83f00 && bytes>=320 && bytes<=1280 && !(bytes&63));
	if (!permitted || !worker || worker>=120 || local<0x75000 || local>0x75800-bytes
		|| ((local^remote)&31) || !PacketIdle()) block_fault_park();
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	static const unsigned offsets[2][4]={{0x208,0x20c,0x210,0x214},{0x204,0x220,0x210,0x228}};
	const unsigned* offset=offsets[write?1:0];
#else
	const unsigned offset[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
#endif
	uint32_t expected[4]; for (unsigned c=0;c<4;c++) expected[c]=*Words(0xffb20000+offset[c])+(c==1?(bytes+63)/64:1);
	uint32_t command[]={write?local:remote,0,write?129U:XY(worker),write?remote:local,0,write?XY(worker):129U,0,write?0x212U:0,bytes,0};
	for (unsigned i=0;i<10;i++) Words(0xffb20000)[i]=command[i];
	Checkpoint(0x54000101,worker);
	for (unsigned i=0;i<10;i++) if (Words(0xffb20000)[i]!=command[i]) block_fault_park();
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
		for (unsigned c=0;c<4;c++) complete &= *Words(0xffb20000+offset[c])==expected[c];
#endif
		if (complete && PacketIdle()) {
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
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
// Keep the transport selector out of the large dispatcher register live set.
// Both paths retain exactly the original operand and padded-scale checks.
static __attribute__((noinline,optimize("Os"))) bool PrepareInputs(unsigned mode,unsigned type,unsigned inputs)
{
	if (mode==1 && !ExpandInputs(inputs)) return false;
	for (unsigned w=0;w<inputs;w++) if (!PioneerInt8OperandValid(uint16_t(Words(0xa0000)[w/2]>>((w&1)*16)))) return false;
	unsigned blocks=inputs>>(type==8?5:8),padded=(blocks+15)&~15U;
	for (unsigned b=0;b<padded;b++) {
		uint32_t bits=Words(0xab000)[b],magnitude=bits&0x7fffffff;
		if (b>=blocks?bits!=0:magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return false;
	}
	return true;
}
#endif
#endif
static bool Peer(unsigned worker,unsigned sequence,bool stopped,bool previous)
{
	Transfer(false,worker,0x75000,0x74140,64); Words(0x7f800)[10]++;
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
	if (status[2] || status[3]!=0x32503851 || status[0]>sequence || status[1]>sequence || status[4]>2) block_fault_park();
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
	Transfer(false,worker,0x75000,0xffb13140,4); unsigned first=*Words(0x75000);
	Transfer(false,worker,0x75000,0xffb13140,4); unsigned second=*Words(0x75000);
	Transfer(false,worker,0x75010,0xffb121b0,4); unsigned reset=*Words(0x75010);
	Checkpoint(0x54000204,(worker<<24)|(first&0xffffff));
	if (reset!=0x46800 || first<0x72000 || first>=0x74000 || second<0x72000 || second>=0x74000 || ((first|second)&3)) block_fault_park();
	return stopped?PioneerQ38PeerTerminalPc(first) && PioneerQ38PeerTerminalPc(second):WaitPc(first) && WaitPc(second);
}
static unsigned Tile(unsigned type) { return type==8?576:type==12?2304:type==14?3392:0; }
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
static Q38_ROOT_SMALL bool HeaderValid(const uint32_t* header)
{
	unsigned operation=header[2],type=header[3],inputs=header[4],workers=header[5],nonce=header[6];
	if (header[0]!=0x314d3851 || header[1]!=1 || (operation!=1 && operation!=2) || !workers || workers>119
		|| !nonce || nonce>=0x7fffffff || (operation==2 && (type || inputs || workers!=119))
		|| (operation==1 && (!Tile(type) || (inputs!=5120 && inputs!=6144 && inputs!=17408)))) return false;
	if (operation==2?header[7]:header[7]-1U>1U) return false;
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	if (operation==2?header[8]:header[8]>1) return false;
	for (unsigned i=9;i<16;i++) if (header[i]) return false;
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
		if (!p[3] || p[3]>header[6] || !p[5] || p[5]!=Words(0x7d800)[w*16] || p[6] || p[7]) return false;
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
extern "C" unsigned block_main()
{
	Checkpoint(0x54000001,0);
	volatile uint32_t* job=Words(0x74000); uint32_t header[16];
	for (unsigned i=0;i<16;i++) header[i]=job[i];
	unsigned operation=header[2],type=header[3],inputs=header[4],workers=header[5],nonce=header[6];
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
	const bool work=operation==1 && header[8]==1;
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
	unsigned blocks=operation==1?inputs>>(type==8?5:8):0;
	for (unsigned bank=0;bank<8;bank++) banks[bank]=Words(0x74040)[bank];
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	for (unsigned w=0;w<119;w++) for (unsigned i=0;i<8;i++) parts[w][i]=Words(0x74080)[w*8+i];
	if (!PartsValid(parts,banks,header)) return 0;
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
		if (!p[3] || p[3]>nonce || !p[5] || p[5]!=Words(0x7d800)[w*16] || p[6] || p[7]) return 0;
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
		if (!PrepareInputs(header[7],type,inputs)) return 0;
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
	for (unsigned c=0;c<8;c++) Words(0x7f800)[16+c]=*Words(0xffb20000+Offset(c));
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
		for (unsigned attempt=0;attempt<65536 && !ready;attempt++) ready=Peer(w+1,parts[w][3]-1,false,true);
		if (!ready) block_fault_park();
	}
	for (unsigned w=0;w<workers;w++) {
		const uint32_t* p=parts[w]; unsigned worker=w+1;
		volatile uint32_t descriptor[16];
		for (unsigned i=0;i<16;i++) descriptor[i]=0; // explicit word stores,NO libc memset/byte writes
		descriptor[0]=operation==1?0x31483851U:0; descriptor[1]=operation==1?type:0; descriptor[2]=operation==1?Tile(type):0;
		descriptor[5]=operation==1?inputs:0; descriptor[6]=operation==1?p[0]:0; descriptor[7]=operation==1?p[2]:0;
		descriptor[8]=operation==1?p[4]:0; descriptor[9]=operation==1?1:0;
		descriptor[10]=0x32503851; descriptor[11]=worker; descriptor[12]=p[3]; descriptor[13]=operation;
		for (unsigned i=0;i<16;i++) Words(0x75000)[i]=descriptor[i];
		Fence();
		Transfer(true,worker,0x75000,0x74000,64);
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
		// Transfer returns only after exact ACK/counter completion. The peer
		// independently validates its immutable descriptor and publishes it
		// in the mandatory full final proof. No redundant descriptor echo.
		if (work) continue;
#endif
		Transfer(false,worker,0x75040,0x74000,64);
		for (unsigned i=0;i<16;i++) if (Words(0x75040)[i]!=descriptor[i]) block_fault_park();
	}
	for (unsigned w=0;w<workers;w++) {
		*Words(0x75000)=parts[w][3]; Fence(); Transfer(true,w+1,0x75000,0x74100,4);
	}
	volatile uint32_t complete[119]; for (unsigned w=0;w<119;w++) complete[w]=0;
	unsigned finished=0;
	for (unsigned round=0;round<65536 && finished<workers;round++) for (unsigned w=0;w<workers;w++)
		if (!complete[w] && Peer(w+1,parts[w][3],operation==2,false)) { complete[w]=1; finished++; }
	if (finished!=workers) block_fault_park();
	Checkpoint(0x54000300,workers);
	// Completion order is arbitrary. STOP has no ordered output gathers,
	// so explicitly finish with worker119's terminal physical audit. This
	// makes the final NIU command deterministic for the kernel's existing
	// exact proof;do not weaken that proof to accept an arbitrary command.
	if (operation==2) {
		bool terminal=Peer(workers,parts[workers-1][3],true,false);
		Checkpoint(0x54000301,workers);
		if (!terminal) block_fault_park();
	}
	// ALL readers finished BEFORE ANY proof/output gather or publication.
	for (unsigned i=0;i<119*64;i++) Words(0x76000)[i]=0;
	for (unsigned i=0;i<119*256;i++) Words(0x80000)[i]=0;
	if (operation==1) for (unsigned w=0;w<workers;w++) {
		const uint32_t* p=parts[w]; unsigned groups=p[2],reads=groups*blocks+(inputs*2+16383)/16384+1;
		unsigned flits=groups*blocks*Tile(type)/64+(inputs*2+((blocks*4+63)&~63U))/64;
		Transfer(false,w+1,0x75000,0x83f00,256+groups*64);
		const volatile uint32_t* proof=Words(0x75000);
		for (unsigned c=0;c<8;c++) if (uint32_t(proof[c+8]-proof[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) block_fault_park();
		volatile uint32_t command[10]; for (unsigned i=0;i<10;i++) command[i]=0;
		command[0]=p[0]+(groups*blocks-1)*Tile(type); command[2]=p[4]; command[3]=0x90000; command[5]=XY(w+1); command[8]=Tile(type);
		if (proof[16]!=groups*inputs/16 || proof[17]!=p[3] || proof[18]!=w+1 || proof[19]!=0x32503851
			|| proof[30]!=p[5] || proof[31]!=XY(w+1)) block_fault_park();
		for (unsigned i=0;i<10;i++) if (proof[20+i]!=command[i]) block_fault_park();
		for (unsigned i=0;i<16;i++) if (proof[32+i]!=Words(0x7d800)[w*16+i]) block_fault_park();
		for (unsigned i=48;i<64;i++) if (proof[i]) block_fault_park();
		for (unsigned row=0;row<groups*16;row++) if ((proof[64+row]&0x7f800000)==0x7f800000) block_fault_park();
		for (unsigned i=0;i<64;i++) Words(0x76000)[w*64+i]=proof[i];
		for (unsigned row=0;row<256;row++) Words(0x80000)[w*256+row]=row<groups*16?proof[64+row]:0;
	}
	for (unsigned w=0;w<119;w++) for (unsigned i=0;i<8;i++) if (parts[w][i]!=Words(0x74080)[w*8+i]) block_fault_park();
	Checkpoint(0x54000401,workers);
	for (unsigned bank=0;bank<8;bank++) if (banks[bank]!=Words(0x74040)[bank]) block_fault_park();
	for (unsigned i=0;i<16;i++) if (header[i]!=job[i] || source[i]!=*Words(0xffb20000+SourceOffset(i))) block_fault_park();
	Checkpoint(0x54000402,workers);
	if (!Idle()) block_fault_park();
	for (unsigned c=0;c<8;c++) Words(0x7f800)[24+c]=*Words(0xffb20000+Offset(c));
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
	if (Words(0x7f800)[6]!=reads || Words(0x7f800)[7]!=workers*2 || Words(0x7f800)[8]!=flits || Words(0x7f800)[9]!=workers*2) block_fault_park();
	for (unsigned c=0;c<8;c++) {
		Checkpoint(0x54000410+c,reads);
		unsigned expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+workers*2:workers*2;
		if (uint32_t(Words(0x7f800)[24+c]-Words(0x7f800)[16+c])!=expected) block_fault_park();
	}
	for (unsigned i=0;i<16;i++) Words(0x7f800)[32+i]=source[i];
	Fence(); return workers;
}
