/* SPDX-License-Identifier: MIT */
// OFFLINE actual linked dispatcher instructions. Synthetic already-sealed
// peers isolate transport/layout/proof behavior;NOT a math or inference oracle.
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPairedQkvPlan.h"
#include <fstream>
static unsigned Symbol(const char* root,const char* wanted)
{
	std::ifstream file(std::string(root)+"/program.symbols");
	std::string address,type,name;
	while (file>>address>>type>>name) if (name==wanted) return unsigned(std::stoul(address,nullptr,16));
	assert(false); return 0;
}
struct QkvDispatch:CoordinatorMachine {
	unsigned entry,fault,gathers=0,readerCount=0,lastWorker=0,lastPrefix=0;
	unsigned badProof=0,badRestore=0,nonfiniteWorker=0;
	unsigned delayWorker=0,delayedReads=0;
	bool readers[119]={};
	static uint32_t Value(unsigned tensor,unsigned row) { return 0x3e000000+tensor*0x100000+row; }
	QkvDispatch(const char* root):CoordinatorMachine(root,true,true),entry(Symbol(root,"decoder_matrix_main")),fault(Symbol(root,"coordinator_fault_park")) {
		operation=12; k=1024; n=4096; limit=workers=119; nonce=1;
		pc=entry; regs[1]=0x6000; regs[2]=0x70000;
		for (unsigned i=0;i<256;i++) memory[0x140000+i*4]=0;
		memory[0x140000]=0x31445243; memory[0x140004]=12; memory[0x140008]=119;
		memory[0x14000c]=1024; memory[0x140010]=4096; memory[0x140018]=1;
		memory[0x140014]=0x04000000; memory[0x140028]=0x05000000; memory[0x14002c]=0x06000000;
		for (unsigned i=0;i<119;i++) memory[0x140100+i*4]=1;
		for (unsigned a=0x80000;a<0x83fc0;a+=4) memory[a]=0;
		for (unsigned i=0;i<32;i++) memory[0x80800+i*4]=Bits(1.f);
		for (unsigned a=0x83fc0;a<0x8c000;a+=4) memory[a]=0xabcdef01;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x80000,0xc000)) return memory.at(a);
		return CoordinatorMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x83fc0,0x8c000-0x83fc0)) {
			if (badRestore && lastWorker==1 && a==lastPrefix+(badRestore-1)*4) value^=1;
			memory[a]=value; return;
		}
		CoordinatorMachine::Store(a,value);
	}
	void Submit() override {
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		bool write=command[7]==0x212; assert(write || !command[7]);
		unsigned xy=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(!command[1] && !command[4] && !command[6] && !command[9] && bytes && bytes<=16384
			&& !((local^remote)&31) && (write?command[2]:command[5])==129);
		unsigned worker=1; while (worker<120 && xy!=(PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6))) worker++;
		assert(worker<120); PioneerPairedQkvSpan span; assert(PioneerPairedQkvWorker(worker,span));
		if (write) {
			assert(remote==0x71300 && local==0x147140 && bytes==4 && memory.at(local)==1 && !readers[worker-1] && !gathers);
			assert(worker==++issued);
		} else if (remote==0x71300) {
			assert(issued==119 && !gathers && local==0x147000 && bytes==16 && !readers[worker-1]);
			for (unsigned i=0;i<4;i++) memory[local+i*4]=i<2?1:i==3?0x43524431:0;
			if (worker==delayWorker && delayedReads<2) { memory[local+4]=0; delayedReads++; }
			else { readers[worker-1]=true; readerCount++; }
		} else if (remote==0x17ffc8) {
			assert(worker==delayWorker && delayedReads && !gathers && local==0x147088 && bytes==4);
			memory[local]=0;
		} else {
			assert(remote==0x83fc0 && readerCount==119 && worker==119-gathers);
			for (bool done:readers) assert(done);
			unsigned destination=PioneerPairedQkvDestination(span);
			assert(destination && local+64==destination && bytes==span.rows*4+64 && destination+span.rows*4<=0x89000);
			gathers++; lastWorker=worker; lastPrefix=local;
			unsigned tiles=64*(span.rows/16),reads=tiles/32;
			uint32_t proof[16]={0,0,0,0,reads+1,reads*144+256,reads+1,reads+1,0x703ec,tiles,0x4d415431,1,0,0,0,0};
			for (unsigned i=0;i<16;i++) memory[local+i*4]=proof[i]^(badProof==i+1?1:0);
			for (unsigned i=0;i<span.rows;i++) memory[destination+i*4]=Value(span.tensor,span.first+i);
			if (worker==nonfiniteWorker) memory[destination+(span.rows-1)*4]=0x7fc00000;
		}
		unsigned at=write?4:0; counts[at]++; counts[at+1]+=(bytes+63)/64; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	bool Run() {
		while (pc!=0x6000 && pc!=fault) Step();
		return pc==0x6000 && regs[10]==119 && !pending;
	}
	void Verify() {
		assert(issued==119 && readerCount==119 && gathers==119 && transfers==357+2*delayedReads);
		for (unsigned t=0;t<3;t++) {
			unsigned base=t==0?0x87000:t==1?0x84000:0x86000,rows=t?1024:2048;
			for (unsigned row=0;row<rows;row++) assert(memory.at(base+row*4)==Value(t,row));
		}
		bool touched[(0x8c000-0x83fc0)/4]={};
		for (unsigned w=1;w<120;w++) { PioneerPairedQkvSpan span; assert(PioneerPairedQkvWorker(w,span)); unsigned a=PioneerPairedQkvDestination(span);
			for (unsigned i=0;i<span.rows*4+64;i+=4) touched[(a-64+i-0x83fc0)/4]=true; }
		for (unsigned i=0;i<sizeof(touched)/sizeof(touched[0]);i++) if (!touched[i]) assert(memory.at(0x83fc0+i*4)==0xabcdef01);
		assert(memory.at(0x143010)==119 && memory.at(0x143014)==4096 && memory.at(0x143018)==1 && memory.at(0x14301c)==12);
		unsigned read=238+2*delayedReads,rf=119+119+4096/16+2*delayedReads,write=119;
		assert(memory.at(0x143000)==read && memory.at(0x143004)==rf && memory.at(0x143008)==write && memory.at(0x14300c)==write);
		unsigned delta[]={read,rf,read+write,read,write,write,read+write,write};
		for (unsigned i=0;i<8;i++) assert(uint32_t(memory.at(0x143040+i*4)-memory.at(0x143020+i*4))==delta[i]);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2); compactImage=true;
	{ QkvDispatch m(argv[1]); assert(m.Run()); m.Verify(); printf("PASS OFFLINE actual QKV dispatcher instructions,steps%u\n",m.steps); }
	// Q may end exactly at the bank limit:validate Q2048,NOT aggregate4096.
	{ QkvDispatch m(argv[1]); m.memory[0x140014]=0x82400000-2048*1152; assert(m.Run()); m.Verify(); }
	for (unsigned worker:{1U,60U,61U,90U,91U,119U}) { QkvDispatch m(argv[1]); m.delayWorker=worker;
		assert(m.Run() && m.delayedReads==2); m.Verify(); }
	for (unsigned word=1;word<=16;word++) {
		QkvDispatch proof(argv[1]); proof.badProof=word; assert(!proof.Run() && proof.gathers==1);
		QkvDispatch restore(argv[1]); restore.badRestore=word; assert(!restore.Run() && restore.gathers==119);
	}
	for (unsigned a:{0x140014U,0x140028U,0x14002cU}) for (unsigned bad:{0U,0x04000001U,0x823ff000U,0x82400000U,UINT32_MAX}) {
		QkvDispatch m(argv[1]); m.memory[a]=bad; assert(!m.Run() && !m.transfers && !m.issued);
	}
	for (unsigned i=12;i<64;i++) { QkvDispatch m(argv[1]); m.memory[0x140000+i*4]=1; assert(!m.Run() && !m.transfers); }
	for (unsigned a:{0x140000U,0x140008U,0x14000cU,0x140010U,0x140018U}) {
		QkvDispatch m(argv[1]); m.memory[a]=0; assert(!m.Run() && !m.transfers);
	}
	// Operation0 is the existing held-PC diagnostic,NOT an invalid opcode.
	{ QkvDispatch m(argv[1]); m.memory[0x140004]=13; assert(!m.Run() && !m.transfers); }
	for (unsigned a:{0x14001cU,0x140020U,0x140024U}) { QkvDispatch m(argv[1]); m.memory[a]=1; assert(!m.Run() && !m.transfers); }
	for (unsigned worker=1;worker<=119;worker++) for (unsigned sequence:{0U,0x7fffffffU}) {
		QkvDispatch m(argv[1]); m.memory[0x140100+(worker-1)*4]=sequence; assert(!m.Run() && !m.transfers);
	}
	for (unsigned word=1;word<=10;word++) { QkvDispatch m(argv[1]);
		for (auto& value:m.command) value=0xdeadbeef;
		m.dropControl=word; assert(!m.Run() && !m.transfers && !m.issued);
	}
	for (unsigned offset:{0x40U,0x840U,0x1040U,0x1840U}) {
		QkvDispatch m(argv[1]); m.busyAddress=0xffb20000+offset; assert(!m.Run() && !m.transfers);
	}
	for (unsigned id=0;id<32;id++) { QkvDispatch m(argv[1]); m.busyAddress=0xffb20240+id*4; assert(!m.Run() && !m.transfers); }
	for (unsigned worker:{1U,61U,91U}) { QkvDispatch m(argv[1]); m.nonfiniteWorker=worker; assert(!m.Run() && m.gathers==120-worker); }
	for (unsigned command:{1U,119U,120U,238U,239U,357U}) for (bool counter:{false,true}) {
		QkvDispatch m(argv[1]); if (counter) m.badCounter=command; else m.failTransfer=command;
		assert(!m.Run() && m.transfers==command);
	}
	puts("PASS OFFLINE actual QKV dispatcher ALL119 routes/10 command-word lost stores/reader-first/reverse gather/ALL4096 output bits/K-prefix restore/guard words/wrapped counters/ALL16 proof+restore faults/ALL3 extent+reserved+shape refusals/ALL119 sequence+ALL4 initiator+ALL32 ID preflight faults/Q-V-K nonfinite refusal;synthetic sealed peers,NOT math/native inference or performance");
}
