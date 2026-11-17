/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION 1
#include "../PioneerQ38HeadCsrBatch.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

namespace Batch=PioneerQ38HeadCsrBatch;
static constexpr uint32_t registers[24]={
	0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,
	0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c,
	0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228
};

struct FakeIO {
	enum Fault {None,NoFifo,ExistingQueue,WrongBaseline,LostGuard,Overflow,CommandReadback,
		DroppedRead,StuckQueue,LateSeal,RemoteMismatch};
	Fault fault;
	unsigned group,worker,issued=0,guards=0,seals=0,fences=0,emptyPolls=0,results=0;
	bool drained=false;
	uint32_t command[10]={},memory[64]={},counts[8]={};
	const uint32_t* values;
	FakeIO(unsigned g,unsigned w,Fault f,const uint32_t (&wanted)[8],const uint32_t (&start)[8])
		:fault(f),group(g),worker(w),values(wanted){
		for(unsigned i=0;i<8;i++)counts[i]=start[i];
		if(fault==WrongBaseline)counts[0]++;
	}
	bool Seal(){++seals;return fault!=LateSeal || seals==1;}
	bool Guard(){++guards;return fault!=LostGuard || guards<4;}
	uint32_t PeerRoute(unsigned logical){assert(logical==worker+1);return logical+17;}
	void Fence(){++fences;}
	void Drain(){
		assert(issued==8 && !drained);drained=true;
		for(unsigned i:{0U,1U,2U,3U,6U})counts[i]+=fault==DroppedRead?7:8;
	}
	uint32_t Read(uint32_t address){
		if(address>=Batch::Scratch && address<Batch::Scratch+sizeof(memory)){
			assert(drained);results++;return memory[(address-Batch::Scratch)/4];
		}
		assert(address>=Batch::Niu);
		const uint32_t off=address-Batch::Niu;
		if(off<40)return command[off/4]^(fault==CommandReadback && issued==2 && off==0?1U:0U);
		if(off==Batch::Control || off==0x840 || off==0x1040 || off==0x1840)return 0;
		if(off==Batch::Overflow)return fault==Overflow && issued>=2;
		if(off==0x100)return fault==NoFifo?0:0x10000;
		if(off==Batch::Available){
			if(!issued && fault==ExistingQueue)return 0x1010100f;
			if(issued==8 && !drained && fault!=StuckQueue && ++emptyPolls>=2)Drain();
			return 0x10101000U|(16-(drained?0U:issued));
		}
		if(off>=0x240 && off<0x2c0){assert(!issued || drained);return 0;}
		for(unsigned i=0;i<8;i++)if(off==registers[16+i]){
			assert(!issued || drained);return counts[i];
		}
		assert(false);return 0;
	}
	void Write(uint32_t address,uint32_t value){
		assert(!drained && address>=Batch::Niu);
		const uint32_t off=address-Batch::Niu;
		if(off<40){command[off/4]=value;return;}
		assert(off==Batch::Control && value==1 && issued<8 && fences==issued*2+1);
		const auto remote=Batch::Niu+registers[group*8+issued];
		const auto local=Batch::Scratch+issued*32+(remote&31);
		assert(command[0]==remote && command[1]==0 && command[2]==worker+18 && command[3]==local);
		assert(command[4]==0 && command[5]==129 && command[6]==0 && command[7]==0 && command[8]==4 && command[9]==0);
		memory[(local-Batch::Scratch)/4]=values[issued]^(fault==RemoteMismatch && issued==7?1U:0U);
		issued++;
	}
};

int main(){
	const uint32_t original[8]={UINT32_MAX-2,22,23,24,25,26,23,28};
	for(unsigned worker=0;worker<119;worker++)for(unsigned g=0;g<3;g++){
		uint32_t tracked[8],wanted[8];
		for(unsigned i=0;i<8;i++){
			assert(Batch::Offset(g,i)==registers[g*8+i]);
			tracked[i]=original[i];wanted[i]=0x90000000|g*32|i;
		}
		FakeIO io(g,worker,FakeIO::None,wanted,tracked);
		assert(Batch::Verify(io,worker,g,wanted,tracked));
		assert(io.drained && io.issued==8 && io.results==8 && io.guards==8 && io.seals==2 && io.fences==16);
		for(unsigned i=0;i<8;i++)assert(tracked[i]==original[i]+(((0x4fU>>i)&1U)*8));
	}
	const uint32_t wanted[8]={1,2,3,4,5,6,7,8};
	const unsigned prefix[]={0,0,0,3,2,2,8,8,8,8};
	for(unsigned f=FakeIO::NoFifo;f<=FakeIO::RemoteMismatch;f++){
		uint32_t tracked[8];for(unsigned i=0;i<8;i++)tracked[i]=original[i];
		FakeIO io(1,59,static_cast<FakeIO::Fault>(f),wanted,tracked);
		assert(!Batch::Verify(io,59,1,wanted,tracked) && io.issued==prefix[f-1]);
		for(unsigned i=0;i<8;i++)assert(tracked[i]==original[i]);
		if(f!=FakeIO::RemoteMismatch)assert(!io.results);
	}
	uint32_t tracked[8]={};FakeIO invalid(0,0,FakeIO::None,wanted,tracked);
	assert(!Batch::Verify(invalid,119,0,wanted,tracked) && !Batch::Verify(invalid,0,3,wanted,tracked));
	assert(!Batch::Verify(invalid,0,0,tracked,tracked) && !invalid.seals && !invalid.issued);
	puts("PASS head CSR batch: 357 exact worker/group combinations, 10 faults, bounded 4B reads, no frontier update after failure; simulated only");
}
