/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#endif
#include <array>
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ActivationPacking.h"
#endif
#ifndef PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchImages.h"
#else
// Test-only NEW private root:do not masquerade as an old production image
// selection or grant it host-DMA/legacy registry capabilities.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerTerminal.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RootTerminal.h"
#endif
// Actual ROOT RV32 instructions;synthetic private peer proof fabric,NOT
// arithmetic/inference/native authority. Real peer math has separate tests.
struct DispatchMachine:BlockMachine {
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
	std::vector<uint32_t> originalInputs;
#endif
	struct PeerState {
		std::array<uint32_t,16> descriptor={}; unsigned requested=0,finished=0,operation=0,polls=0;
		bool launched=false;
	};
	std::vector<PeerState> peers=std::vector<PeerState>(119);
	uint32_t command[10]={},counts[8]={}; bool seen[10]={};
	unsigned active,kind,k,operation=1,packets=0,responses=0,descriptors=0,readbacks=0,doorbells=0,gathers=0;
	unsigned faultAt=0,corruptProof=UINT32_MAX,corruptWorker=0,badPcWorker=0,nonfiniteWorker=0;
	unsigned stopSlowWorker=0; // vary completion order independently of index
	unsigned stopSnapshotMode=0,pcSamples[119]={};
	unsigned corruptProgress=0;
	bool pending=false,unknown=false,badReadback=false,badCounter=false,mutateTasks=false,stuckResponseTag=false;
	DispatchMachine(const char* path,unsigned workers,unsigned type=12,unsigned inputs=5120)
		:BlockMachine(path,8,0,1,nullptr,true),active(workers),kind(type),k(inputs) {
#ifndef PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
		for (unsigned word=0;word<2048;word++) assert(PioneerQ38DispatchRoleWord(0,word)==(word<code.size()/4?Word(code,word*4):0));
#endif
		assert(Word(code,0x38)==0x0000006f && Word(code,0x3c)==0xffdff06f);
		for (unsigned c=0;c<8;c++) counts[c]=UINT32_MAX-15-c;
		for (unsigned w=0;w<119;w++) for (unsigned c=0;c<16;c++) memory[0x7d800+w*64+c*4]=c?0x12340000+c:0x16005;
		Configure(1);
	}
	static unsigned Route(unsigned worker) { const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14}; return columns[worker/10]|((worker%10+2)<<6); }
	static unsigned Bytes(unsigned type) { return type==8?576:type==12?2304:3392; }
	bool WorkCached() const {
#ifdef PIONEER_Q38_WORK_COHORT_CACHE
		return operation==1 && memory.at(0x74020)==1;
#else
		return false;
#endif
	}
	unsigned DescriptorEchoes() const { return WorkCached()?0:active; }
	virtual bool CompactOutputLayout() const {
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		return WorkCached();
#else
		return false;
#endif
	}
	void Configure(unsigned nonce,bool stop=false) {
		operation=stop?2:1; descriptors=readbacks=doorbells=gathers=0;
		const uint32_t header[]={0x314d3851,1,operation,stop?0:kind,stop?0:k,active,nonce,0,0,0,0,0,0,0,0,0};
		for (unsigned i=0;i<16;i++) memory[0x74000+i*4]=header[i];
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		memory[0x7401c]=stop?0:1;
#endif
		const unsigned routes[]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
		for (unsigned b=0;b<8;b++) memory[0x74040+b*4]=b<7?routes[b]:0;
		for (unsigned w=0;w<119;w++) {
			unsigned bank=w%7,groups=1+w%3;
			uint32_t part[]={stop?0:0x04000040+(w/7)*0x00800000,stop?0:bank,stop?0:groups,w<active?peers[w].finished+1:0,
				stop?0:routes[bank],0x16005,0,0};
			for (unsigned i=0;i<8;i++) memory[0x74080+w*32+i*4]=w<active?part[i]:0;
			peers[w].launched=false; peers[w].polls=0;
		}
		for (unsigned w=0;w<k/2;w++) memory[0xa0000+w*4]=0x40014001;
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		originalInputs.assign(k/2,0);
		for (unsigned i=0;i<k;i++) {
			unsigned state=(i*29+nonce*17)%257;
			uint16_t value=state==256?uint16_t(0xc000):PioneerInt8Operand(int(state)-128);
			originalInputs[i/2]|=uint32_t(value)<<((i%2)*16);
		}
		if (!stop) for (unsigned w=0;w<PioneerQ38PackedActivationWords(k);w++) {
			uint32_t packed; assert(PioneerQ38PackActivationWord(originalInputs.data(),k,w,packed)); memory[0xa0000+w*4]=packed;
		}
#endif
		unsigned blocks=k>>(kind==8?5:8),padded=(blocks+15)&~15U;
		for (unsigned w=0;w<padded;w++) memory[0xab000+w*4]=w<blocks?0x3c800000:0;
		pc=0x6000; regs[0]=0; // test-only fresh ROOT reset;peers persist
	}
	unsigned Worker(uint32_t route) const { for (unsigned w=1;w<=119;w++) if (Route(w)==route) return w; assert(false); return 0; }
	bool AllFinished() const { for (unsigned w=0;w<active;w++) if (!peers[w].launched || peers[w].requested!=peers[w].finished) return false; return true; }
	virtual uint32_t ExpectedOutput(unsigned w,unsigned row) const { return Bits(float(w*256+row+1)); }
	virtual std::array<uint32_t,64> Proof(unsigned worker) const {
		const auto& peer=peers[worker-1]; const auto& d=peer.descriptor;
		unsigned blocks=d[5]>>(d[1]==8?5:8),reads=d[7]*blocks+(d[5]*2+16383)/16384+1;
		unsigned flits=d[7]*blocks*d[2]/64+(d[5]*2+((blocks*4+63)&~63U))/64;
		std::array<uint32_t,64> proof={};
		for (unsigned c=0;c<8;c++) { proof[c]=UINT32_MAX-7-c; proof[8+c]=proof[c]+(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0); }
		proof[16]=d[7]*d[5]/16; proof[17]=d[12]; proof[18]=worker; proof[19]=0x32503851;
		const uint32_t cmd[]={d[6]+(d[7]*blocks-1)*d[2],0,d[8],0x90000,0,Route(worker),0,0,d[2],0};
		for (unsigned i=0;i<10;i++) proof[20+i]=cmd[i];
		proof[30]=memory.at(0x7d800+(worker-1)*64); proof[31]=Route(worker);
		for (unsigned i=0;i<16;i++) proof[32+i]=memory.at(0x7d800+(worker-1)*64+i*4);
		return proof;
	}
	virtual void Packet()
	{
		assert(pending && !unknown); unsigned worker=Worker(command[7]?command[5]:command[2]); auto& peer=peers[worker-1];
		assert(worker<=active && command[1]==0 && command[4]==0 && command[6]==0 && command[9]==0);
		bool write=command[7]==0x212; assert(write || !command[7]);
		assert((write?command[2]:command[5])==129 && command[8]>0 && command[8]<=1280);
		unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
		if (write) {
			if (remote==0x74000) {
				assert(bytes==64 && !doorbells); descriptors++;
				for (unsigned i=0;i<16;i++) peer.descriptor[i]=memory.at(local+i*4);
				const auto& d=peer.descriptor; const unsigned base=0x74080+(worker-1)*32;
				assert(d[10]==0x32503851 && d[11]==worker && d[12]==memory.at(base+12) && d[13]==operation
#ifdef PIONEER_Q38_WORK_SKIP_DOT_CHECK
					&& d[14]==unsigned(WorkCached())
#else
					&& !d[14]
#endif
					&& !d[15]);
				if (operation==1) assert(d[0]==0x31483851 && d[1]==kind && d[2]==Bytes(kind) && !d[3] && !d[4] && d[5]==k
					&& d[6]==memory.at(base) && d[7]==memory.at(base+8) && d[8]==memory.at(base+16) && d[9]==1);
			} else {
				assert(remote==0x74100 && bytes==4 && descriptors==active && readbacks==DescriptorEchoes() && !peer.launched);
				assert(memory.at(local)==peer.descriptor[12] && memory.at(local)==peer.finished+1); doorbells++;
				peer.requested=memory.at(local); peer.operation=operation; peer.launched=true;
				if (mutateTasks) memory[0x74080+(active-1)*32+28]=1;
			}
		} else if (remote==0x74000) {
			assert(bytes==64 && !doorbells); readbacks++;
			for (unsigned i=0;i<16;i++) memory[local+i*4]=peer.descriptor[i];
		} else if (remote==0x74140) {
			assert(bytes==64);
			unsigned delay=operation==2 && worker==stopSlowWorker?10:1+worker%3;
			if (peer.launched && ++peer.polls>delay) peer.finished=peer.requested;
			for (unsigned i=0;i<16;i++) memory[local+i*4]=0;
			memory[local]=peer.requested; memory[local+4]=peer.finished; memory[local+12]=0x32503851;
			memory[local+16]=peer.launched?(peer.finished==peer.requested?(peer.operation==2?2:0):1):0;
			if (corruptProgress && peer.launched && peer.finished==peer.requested) memory[local+20]=corruptProgress;
		} else if (remote==0xffb13140) {
			assert(bytes==4);
			bool next=stopSnapshotMode==1 || (stopSnapshotMode==2 && (pcSamples[worker-1]++&1));
			memory[local]=worker==badPcWorker?0xdeadc0dc:peer.operation==2 && peer.launched?
				(next?TT_Q38_PEER_STOP_FETCH:TT_Q38_PEER_STOP):0x7204c;
		} else if (remote==0xffb121b0) { assert(bytes==4); memory[local]=0x46800; }
		else {
			assert(remote==0x83f00 && operation==1 && AllFinished() && bytes==256+peer.descriptor[7]*64); gathers++;
			auto proof=Proof(worker); if (worker==corruptWorker && corruptProof<64) proof[corruptProof]^=1;
			for (unsigned i=0;i<64;i++) memory[local+i*4]=proof[i];
			for (unsigned row=0;row<peer.descriptor[7]*16;row++) memory[local+256+row*4]=(worker==nonfiniteWorker && !row)?0x7f800000:ExpectedOutput(worker-1,row);
		}
		counts[write?4:0]++; counts[write?5:1]+=(bytes+63)/64; counts[write?7:3]++; counts[2]++;
		responses++; pending=false;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0xffb20000,40)) { unsigned w=(a-0xffb20000)/4; seen[w]=true; return command[w]^(badReadback && w==2?1:0); }
		if (a==0xffb20040) { if (pending && packets!=faultAt) Packet(); return pending?1:0; }
		for (unsigned c=1;c<4;c++) if (a==0xffb20040+c*0x800) return 0;
		if (a>=0xffb20240 && a<0xffb202c0) return pending || (stuckResponseTag && responses)?1:0;
		const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
		for (unsigned c=0;c<8;c++) if (a==0xffb20000+offsets[c]) return counts[c==6?2:c]^(badCounter && responses && c==0?1:0);
		for (unsigned w=0;w<16;w++) if (a==0xffb20000+BlackholeDramSourceOffset(w)) return w?0x12340000+w:0x16005;
		if (Range(a,0x6e000,8192) || Range(a,0x74000,0x1000) || Range(a,0x75000,0x800)
			|| Range(a,0x76000,119*256) || Range(a,0x7d800,119*64) || Range(a,0x7f800,256)
			|| Range(a,0x80000,119*1024) || Range(a,0xa0000,k*2) || Range(a,0xab000,2176)) {
			assert(memory.count(a)); return memory.at(a);
		}
		return BlockMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		if (Range(a,0xa0000,k*2)) { assert(!pending && !unknown && !doorbells && !descriptors); memory[a]=value; return; }
#endif
		if (Range(a,0xffb20000,40)) { assert(!pending && !unknown); unsigned w=(a-0xffb20000)/4; command[w]=value; seen[w]=false; return; }
		if (a==0xffb20040) { assert(value==1 && !pending && !unknown); for (bool read:seen) assert(read); pending=true; packets++; return; }
		if (Range(a,0x6e000,8192)) { minStack=std::min(minStack,unsigned(a)); memory[a]=value; return; }
		if (Range(a,0x75000,0x800) || Range(a,0x7f800,256)) { assert(!pending && !unknown); memory[a]=value; return; }
		if (Range(a,0x76000,119*256) || Range(a,0x80000,119*1024)) { assert(AllFinished() && !pending && !unknown); memory[a]=value; return; }
		if (a==0x17ffc8 && value==0x464d3851) { unknown=true; l1[Index(a)]=value; return; }
		assert(!Range(a,0x74000,0x1000) && !Range(a,0x7d800,119*64) && !Range(a,0xa0000,k*2) && !Range(a,0xab000,2176));
		BlockMachine::Store(a,value);
	}
	unsigned StepLimit() const override { return 1000000000; }
	bool RunRoot() {
		while (pc!=TT_Q38_DISPATCH_PARK && pc!=TT_Q38_DISPATCH_FAULT_PARK) StepBlock();
		assert(minStack>=0x6e000);
		if (pc==TT_Q38_DISPATCH_FAULT_PARK) return false;
		assert(l1[Index(0x17ffc8)]==0x314d3851);
		if (l1[Index(0x17ffc4)]!=active) return false;
		assert(!pending && descriptors==active && readbacks==DescriptorEchoes() && doorbells==active && gathers==(operation==1?active:0));
		if (operation==1 && CompactOutputLayout()) {
			unsigned packed=0;
			for (unsigned w=0;w<active;w++) for (unsigned row=0;row<peers[w].descriptor[7]*16;row++) {
				assert(memory.at(0x80000+packed*4)==ExpectedOutput(w,row)); packed++;
			}
		} else if (operation==1) for (unsigned w=0;w<119;w++) for (unsigned row=0;row<256;row++) {
			unsigned groups=w<active?peers[w].descriptor[7]:0;
			assert(memory.at(0x80000+w*1024+row*4)==(row<groups*16?ExpectedOutput(w,row):0));
		}
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
		if (operation==1) for (unsigned w=0;w<k/2;w++) assert(memory.at(0xa0000+w*4)==originalInputs[w]);
#endif
		return true;
	}
};
#ifndef PIONEER_Q38_DISPATCH_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
	for (unsigned k:{5120U,6144U,17408U}) {
		DispatchMachine valid(argv[1],2,8,k); assert(valid.RunRoot());
		unsigned words=PioneerQ38PackedActivationWords(k);
		for (unsigned where:{0U,words/2,words-1}) for (unsigned high=27;high<32;high++) {
			DispatchMachine m(argv[1],2,8,k); m.memory[0xa0000+where*4]|=1U<<high;
			assert(!m.RunRoot() && !m.packets && !m.doorbells && !m.descriptors);
		}
		for (unsigned raw:{255U,511U}) {
			DispatchMachine m(argv[1],2,8,k); m.memory[0xa0000]=(m.memory[0xa0000]&~511U)|raw;
			assert(!m.RunRoot() && !m.packets && !m.doorbells);
		}
		if (k!=6144) {
			DispatchMachine m(argv[1],2,8,k); m.memory[0xa0000+(words-1)*4]|=1U<<18;
			assert(!m.RunRoot() && !m.packets);
		}
		DispatchMachine old(argv[1],2,8,k); old.memory[0x7401c]=0;
		assert(!old.RunRoot() && !old.packets);
	}
	puts("PASS compact ACTUAL RV32 expansion:all257 input states+signed zero,all3 input widths,backwards overlap,high-bit/tail/invalid-magnitude/legacy-header faults refuse BEFORE ANY NoC packet;unchanged8KiB bound");
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	for (unsigned k:{5120U,6144U,17408U}) for (unsigned type:{8U,12U,14U}) {
		DispatchMachine m(argv[1],2,type,k); assert(m.RunRoot()); m.Configure(2);
		m.memory[0x7401c]=2;
		for (unsigned w=0;w<k/2;w++) m.memory[0xa0000+w*4]=m.originalInputs[w];
		assert(m.RunRoot());
		for (unsigned lane:{0U,16U}) {
			DispatchMachine bad(argv[1],2,type,k); bad.memory[0x7401c]=2;
			for (unsigned w=0;w<k/2;w++) bad.memory[0xa0000+w*4]=bad.originalInputs[w];
			bad.memory[0xa0000]^=0x4000U<<lane;
			assert(!bad.RunRoot() && !bad.packets && !bad.doorbells);
		}
		for (unsigned mode:{0U,3U,UINT32_MAX}) {
			DispatchMachine bad(argv[1],2,type,k); bad.memory[0x7401c]=mode;
			assert(!bad.RunRoot() && !bad.packets);
		}
	}
	puts("PASS actual reuse root:all3 widths xQ8/Q4/Q6 retained original257-state inputs,second nonces,invalid operands in BOTH halfwords and unknown transport selectors refuse BEFORE NoC");
#endif
	for (unsigned workers:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		DispatchMachine m(argv[1],workers,type,type==12?17408:5120); assert(m.RunRoot());
		m.Configure(2); assert(m.RunRoot());
		printf("OFFLINE actual dispatcher %u peers type%u repeated:no early doorbell/gather,private proof/outputs PASS\n",workers,type);
	}
	{
		DispatchMachine m(argv[1],7); assert(m.RunRoot()); m.active=119; m.Configure(3,true); assert(m.RunRoot());
		unsigned packets=m.packets; m.pc=TT_Q38_DISPATCH_PARK_FETCH; m.StepBlock();
		assert(m.pc==TT_Q38_DISPATCH_PARK && m.packets==packets && !m.pending);
	}
	for (unsigned field=0;field<64;field++) {
		DispatchMachine m(argv[1],7); m.corruptWorker=7; m.corruptProof=field;
		assert(!m.RunRoot() && m.unknown);
	}
	for (unsigned worker:{1U,4U,7U}) {
		DispatchMachine m(argv[1],7); m.nonfiniteWorker=worker; assert(!m.RunRoot() && m.unknown);
		DispatchMachine pc(argv[1],7); pc.badPcWorker=worker; assert(!pc.RunRoot() && pc.unknown && !pc.gathers);
	}
	for (unsigned packet:{1U,20U,40U}) {
		DispatchMachine m(argv[1],7); m.faultAt=packet; assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC
		unsigned remote=m.command[7]?m.command[3]:m.command[0];
		unsigned worker=m.Worker(m.command[7]?m.command[5]:m.command[2]);
		unsigned stage=0x58000000|(worker<<16)|(m.command[7]?0x8000U:0U)|(((remote>>4)&1023U)<<5)|31U;
		assert(m.l1[m.Index(0x17ffc0)]==stage);
		assert(m.l1[m.Index(0x17ffc4)]==UINT32_MAX);
#endif
	}
	for (unsigned fault=0;fault<3;fault++) {
		DispatchMachine m(argv[1],7); m.badReadback=fault==0; m.badCounter=fault==1; m.mutateTasks=fault==2;
		assert(!m.RunRoot() && m.unknown);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC
	{
		DispatchMachine m(argv[1],2); m.badCounter=true;
		assert(!m.RunRoot() && m.unknown && !m.pending && m.packets==1 && !m.gathers);
		assert(m.l1[m.Index(0x17ffc0)]==0x58010281 && m.l1[m.Index(0x17ffc4)]==UINT32_MAX);
		DispatchMachine tag(argv[1],2); tag.stuckResponseTag=true;
		assert(!tag.RunRoot() && tag.unknown && !tag.pending && tag.packets==1 && !tag.gathers);
		assert(tag.l1[tag.Index(0x17ffc0)]==0x58010280 && !tag.l1[tag.Index(0x17ffc4)]);
		puts("PASS58 ORIGINAL polling values distinguish pending packet/counter mismatch/tag-not-idle;exact remote/worker/delta;NO additional packet or result gather");
	}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
	for (unsigned value:{1U,8U,9U}) {
		DispatchMachine m(argv[1],2); m.corruptProgress=value;
		assert(!m.RunRoot() && m.unknown && !m.gathers);
	}
#endif
	puts("PASS OFFLINE actual root dispatcher instructions:1/2/7/119 synthetic peers,repeat+ALL119 STOP,ALL descriptors before doors/ALL physical completions before gathers,64 proof faults,nonfinite/PC/NoC/immutable-task faults;NO real-peer/native/kernel/graph/inference/speed qualification");
}
#endif
