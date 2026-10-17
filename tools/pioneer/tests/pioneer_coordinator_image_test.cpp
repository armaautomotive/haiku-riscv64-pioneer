/* SPDX-License-Identifier: MIT */
// Actual linked coordinator and worker RISC bytes + the exact math/SFPU
// interpreter. No C++ source execution, device API or hardware access.
#define PIONEER_COORDINATED_MACHINE_ONLY
#include "pioneer_coordinated_worker_test.cpp"
#include <map>
#include <memory>
#include <string>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCoordinatorImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFastCoordinatorImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCoordinatorReadbackImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFusedCoordinatorImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCompactCoordinatorImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerCoordinatedFleet.h"
static bool fastImage=false;
static bool readbackImage=false;
static bool fusedImage=false;
static bool compactImage=false;
struct CoordinatorMachine:Machine {
	std::vector<uint8_t> code,workerImage;
	std::map<uint32_t,uint32_t> memory;
	std::vector<std::unique_ptr<CoordinatedMachine>> peer;
	uint32_t command[10]={},counts[8]={};
	unsigned operation=0,k=0,n=0,limit=0,workers=0,trial=0;
	unsigned transfers=0,issued=0,controlWrites=0,dropControl=0,failTransfer=0,badCounter=0;
	int dropDescriptor=-1,corruptProof=-1;
	bool pending=false,dropDoorbell=false,badOutput=false,badPeerPc=false,badPeerReset=false;
	unsigned minStack=0x70000;
	unsigned nonce=0;
	uint32_t host[4096]={};
	bool dropPosted=false,partialPosted=false,pairedCorrupt=false;
	unsigned corruptHost=~0U,hostWrites=0,hostReads=0,comparedOriginal=0,comparedReturned=0;
	unsigned busyAddress=0,busyAfter=0;
	unsigned scaleStores=0,dropScale=0;
	std::vector<std::string> events;
	CoordinatorMachine(const char* path):peer(120) {
		std::string root(path); code=Read((root+"/code.bin").c_str()); workerImage=Read((root+"/worker.bin").c_str());
		auto boot=Read((root+"/boot.bin").c_str()); assert(boot.size()==4 && code.size()<=8192 && !(code.size()&3));
		unsigned size=compactImage?unsigned(TT_COMPACT_COORDINATOR_IMAGE_WORDS):fusedImage?unsigned(TT_FUSED_COORDINATOR_IMAGE_WORDS):readbackImage?unsigned(TT_COORDINATOR_READBACK_IMAGE_WORDS):fastImage?unsigned(TT_FAST_COORDINATOR_IMAGE_WORDS):unsigned(TT_COORDINATOR_IMAGE_WORDS);
		assert(Word(boot,0)==TT_COORDINATOR_BOOT && code.size()==size*4);
		for (unsigned i=0;i<TT_COORDINATOR_CODE_WORDS;i++) assert((compactImage?BlackholeCompactCoordinatorCodeWord(i):fusedImage?BlackholeFusedCoordinatorCodeWord(i):readbackImage?BlackholeCoordinatorReadbackCodeWord(i):fastImage?BlackholeFastCoordinatorCodeWord(i):BlackholeCoordinatorCodeWord(i))==(i<code.size()/4?Word(code,i*4):0));
		l1[0]=Word(boot,0);
		for (unsigned i=0;i<code.size()/4;i++) l1[i+1]=Word(code,i*4);
		for (auto& count:counts) count=0xfffffff8; counts[6]=counts[2];
	}
	unsigned Index(uint32_t a) override {
		if (a==0x6000) return 0;
		assert(a>=0x6c000 && a-0x6c000<code.size() && !(a&3)); return 1+(a-0x6c000)/4;
	}
	bool CodeAddress(uint32_t a) const override { return a==0x6000 || (a>=0x6c000 && a-0x6c000<code.size()); }
	unsigned StepLimit() const override { return 30000000; }
	static bool Range(uint32_t a,uint32_t first,unsigned bytes) { return a>=first && a-first<bytes; }
	void Setup(unsigned op,unsigned inputs,unsigned outputs,unsigned workerLimit,unsigned t=0) {
		operation=op; k=inputs; n=outputs; limit=workerLimit; trial=t; pc=0x6000; steps=0;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
		memory[0x140000]=0x31445243; memory[0x140004]=op; memory[0x140008]=limit;
		memory[0x14000c]=k; memory[0x140010]=n; memory[0x140014]=0x04000000;
		memory[0x140018]=++nonce;
		memory[0x14001c]=memory[0x140020]=memory[0x140024]=0;
		for (unsigned i=0;i<8192;i++) memory[0x84000+i*4]=0x7fc00000;
		if (!op) {
			for (unsigned w=1;w<=limit;w++) memory[0x140100+(w-1)*4]=0x70400+w*4;
			return;
		}
		unsigned blocks=n/32; workers=blocks<limit?blocks:limit; unsigned first=0;
		for (unsigned w=1;w<=workers;w++) {
			unsigned rows=(blocks/workers+(w<=blocks%workers))*32;
			if (!peer[w]) {
				peer[w].reset(new CoordinatedMachine(workerImage,k,rows,w));
				auto& m=*peer[w]; m.base=0x04000000+first*(k+k/8); m.BootCoord();
				for (unsigned i=0;i<8;i++) m.l1[m.Index(0x70500+i*4)]=0xdeadbeef; // make every lost descriptor store observable
				// Independent global-row weight pattern distinguishes peers/order.
				for (auto& value:m.weights) value=0;
				for (unsigned row=0;row<rows;row++) for (unsigned b=0;b<k/32;b++) {
					m.weights[PioneerQ8NativeF32ScaleByte(k,b,row)/4]=Bits(m.Half(m.WeightScale(first+row,b)));
					for (unsigned i=0;i<32;i++) {
						uint8_t q=0; assert(PioneerInt8UnpackEncode(m.NativeWeight(first+row,b*32+i),q));
						uint64_t at=PioneerQ8NativeF32ValueByte(k,b*32+i,row); m.weights[at/4]|=uint32_t(q)<<(8*(at&3));
					}
				}
			}
			assert(peer[w]->inputs==k && peer[w]->outputs==rows);
			memory[0x140100+(w-1)*4]=peer[w]->l1[peer[w]->Index(0x71304)]+1;
			first+=rows;
		}
		for (unsigned i=0;i<4096;i++) memory[0x80000+i*4]=0;
		for (unsigned i=0;i<k;i++) memory[0x80000+(i/2)*4]|=uint32_t(PioneerInt8Operand(Q8Machine::Input(i,t)))<<(16*(i&1));
		for (unsigned i=0;i<k/32;i++) memory[0x83000+i*4]=compactImage?0x7fc00000:Bits(Q8Machine::InputScale(i,t));
		if (compactImage) for (unsigned i=0;i<k/32;i++) memory[0x80000+k*2+i*4]=Bits(Q8Machine::InputScale(i,t));
	}
	void SetupFused(unsigned inputs,unsigned outputs,unsigned workerLimit,unsigned t=0) {
		Setup(1,inputs,outputs,workerLimit,t); operation=3; memory[0x140004]=3;
		memory[0x14001c]=0x80000000; memory[0x140020]=1; memory[0x140024]=16384;
		for (auto& word:host) word=0xffffffff;
	}
	void Advance() {
		for (auto& pointer:peer) if (pointer) {
			auto& m=*pointer;
			for (unsigned i=0;i<4096;i++) {
				if (m.pc==CoordFault || (m.pc==CoordWait && m.l1[m.Index(0x71300)]==m.l1[m.Index(0x71304)])) break;
				try { m.Step(); } catch (uint32_t marker) { assert(marker==0x4e4f4346); m.l1[m.Index(0x17ffc8)]=marker; m.pc=CoordFault; break; }
			}
		}
	}
	uint32_t PeerRead(unsigned w,uint32_t a) {
		if (a==0xffb13140) return operation?(badPeerPc?0x70000:peer[w]->pc):0x70400+w*4;
		if (a==0xffb121b0) return badPeerReset?0:operation?0x46800:0x47800;
		assert(operation && peer[w]); auto& m=*peer[w];
		if (Range(a,0x71340,64)) {
			uint32_t value=m.l1[m.Index(a)]; if (int((a-0x71340)/4)==corruptProof) value^=1; return value;
		}
		if (Range(a,0x71300,16) || Range(a,0x70500,32) || a==0x17ffc8) return m.l1[m.Index(a)];
		assert(Range(a,0x84000,m.outputs*4));
		uint32_t value=m.buffers[4096+(a-0x84000)/4];
		if (badOutput && a==0x84000+(m.outputs-1)*4) value=0x7fc00000;
		return value;
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) {
		assert(operation && peer[w]); auto& m=*peer[w];
		if (Range(a,0x70500,32)) {
			assert(m.pc==CoordWait && m.l1[m.Index(0x71300)]==m.l1[m.Index(0x71304)]);
			if (int((a-0x70500)/4)!=dropDescriptor) m.l1[m.Index(a)]=value;
			return;
		}
		assert(a==0x71300 && value==m.l1[m.Index(0x71304)]+1 && m.pc==CoordWait);
		issued++; if (dropDoorbell) return;
		m.previousReads+=m.responses; m.responses=m.requests=0; m.trial=trial;
		for (unsigned i=0;i<4096;i++) m.source[i]=memory.at(0x80000+i*4);
		m.l1[m.Index(a)]=value;
	}
	void Submit() {
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		bool write=command[7]==0x212; assert(write || !command[7]);
		uint32_t xy=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(!(write?command[1]:command[4]) && !command[6] && !command[9] && bytes && bytes<=16384
			&& !((local^remote)&31) && (write?command[2]:command[5])==129);
		if (xy==1555) {
			assert(operation==3 && n<=4096 && remote==0x80000000 && (write?command[4]:command[1])==1 && bytes==n*4
				&& local==(write?0x84000:0x144000));
			if (write) {
				hostWrites++;
				if (!dropPosted) for (unsigned i=0;i<n-(partialPosted?1:0);i++) host[i]=memory.at(0x84000+i*4)^(i==corruptHost?1:0);
				if (pairedCorrupt) { host[0]^=1; host[1]^=1; }
			} else { hostReads++; for (unsigned i=0;i<n;i++) memory[0x144000+i*4]=host[i]; }
		} else {
		assert(!command[1] && !command[4]);
		unsigned worker=0; for (unsigned w=1;w<120;w++) if (xy==(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6))) worker=w;
		assert(worker && worker<=limit);
		if (write) assert((remote==0x70500 && local==0x147100 && bytes==32) || (remote==0x71300 && local==0x147140 && bytes==4));
		else assert((remote==0xffb13140 && local==0x147040 && bytes==4) || (remote==0xffb121b0 && local==0x147090 && bytes==4)
			|| (remote==0x71300 && local==0x147000 && bytes==16) || (remote==0x70500 && local==0x147120 && bytes==32)
			|| (remote==0x17ffc8 && local==0x147088 && bytes==4) || (remote==0x71340 && local==0x147180 && bytes==64)
			|| (remote==0x84000 && Range(local,0x84000,32768) && bytes<=32768-(local-0x84000)));
		events.push_back(std::string(write?"write":"read")+":"+std::to_string(remote));
		Advance();
		for (unsigned i=0;i<bytes;i+=4) {
			if (write) PeerWrite(worker,remote+i,memory.at(local+i));
			else memory[local+i]=PeerRead(worker,remote+i);
		}
		}
		unsigned at=write?4:0;
		counts[at]++; counts[at+1]+=(bytes+63)/64; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3));
		if (compactImage && (Range(a,0x80000,k*2+k/8) || Range(a,0x83000,k/32*4))) return memory.at(a);
		if (operation==3 && hostReads) {
			if (Range(a,0x84000,n*4)) comparedOriginal++;
			if (Range(a,0x144000,n*4)) comparedReturned++;
		}
		if (a==busyAddress && transfers>=busyAfter) return 1;
		if (Range(a,0x6c000,unsigned(code.size()))) return l1[Index(a)];
		if (Range(a,0xffb20000,40)) return command[(a-0xffb20000)/4];
		if (a==0xffb20040) return pending;
		if (a==0xffb20840 || a==0xffb21040 || a==0xffb21840 || Range(a,0xffb20240,128)) return 0;
		const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
		for (unsigned i=0;i<8;i++) if (a==0xffb20000+offsets[i]) return counts[i];
		assert(Range(a,0x6e000,8192) || Range(a,0x140000,0x8000) || Range(a,0x84000,32768) || Range(a,0x17ffc0,12));
		assert(memory.count(a)); return memory.at(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert(!(a&3));
		if (compactImage && Range(a,0x83000,k/32*4)) {
			if (++scaleStores!=dropScale) memory[a]=value; return;
		}
		if (Range(a,0xffb20000,40)) { assert(!pending); if (++controlWrites!=dropControl) command[(a-0xffb20000)/4]=value; return; }
		if (a==0xffb20040) { assert(value==1 && !pending); Submit(); return; }
		assert(Range(a,0x6e000,8192) || Range(a,0x141000,0x3000) || Range(a,0x147000,0x1000) || Range(a,0x17ffc0,12));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,a);
		memory[a]=value;
	}
	bool RunCoord() {
		while (pc!=0x6c038 && pc!=0x6c058) Step();
		assert(l1[Index(pc)]==0x0000006f);
		if (pc==0x6c058) { assert(memory.at(0x17ffc8)==0x46445243 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6c00c && memory.at(0x17ffc8)==0x31445243 && !pending);
		if (operation==2) {
			if (!memory.at(0x17ffc4)) return false;
			assert(memory.at(0x17ffc4)==n && memory.at(0x143010)==n && memory.at(0x143014)==n
				&& memory.at(0x143018)==nonce && memory.at(0x14301c)==2 && memory.at(0x143004)==k);
			for (unsigned i=0;i<8;i++) assert(memory.at(0x143020+i*4)==memory.at(0x143040+i*4));
			uint32_t summary[24]; for (unsigned i=0;i<24;i++) summary[i]=memory.at(0x143000+i*4);
			assert(PioneerCoordinatedOutputSummary(summary,k,n,nonce,counts,counts));
			for (unsigned word=1;word<24;word++) {
				summary[word]^=1; assert(!PioneerCoordinatedOutputSummary(summary,k,n,nonce,counts,counts)); summary[word]^=1;
			}
			assert(!transfers && !issued && !controlWrites);
			return !memory.at(0x143000);
		}
		if (memory.at(0x17ffc4)) {
			unsigned count=operation?workers:limit;
			assert(memory.at(0x143010)==count && memory.at(0x143014)==n && memory.at(0x143018)==nonce && memory.at(0x14301c)==operation);
			unsigned read=memory.at(0x143000),readFlits=memory.at(0x143004),write=memory.at(0x143008),writeFlits=memory.at(0x14300c);
			bool fused=operation==3;
			assert(write==(operation?2*count:0)+(fused?1:0) && writeFlits==(operation?2*count:0)+(fused?n/16:0));
			assert(operation?read>=10*count:read==4*count);
			assert(readFlits==(operation && n<=8192?read-count+n/16+(fused?n/16-1:0):read));
			const unsigned expected[]={read,readFlits,read+write,read,write,writeFlits,read+write,write};
			for (unsigned i=0;i<8;i++) assert(uint32_t(memory.at(0x143040+i*4)-memory.at(0x143020+i*4))==expected[i]);
			if (operation) {
				uint32_t summary[24],before[8],after[8];
				for (unsigned i=0;i<24;i++) summary[i]=memory.at(0x143000+i*4);
				for (unsigned i=0;i<8;i++) { before[i]=summary[8+i]; after[i]=summary[16+i]; }
				assert(PioneerCoordinatedSummary(summary,n,count,nonce,before,after,fused));
				if (fused) {
					assert(hostWrites==1 && hostReads==1 && comparedOriginal==n && comparedReturned==n);
					for (unsigned i=0;i<n;i++) assert(host[i]==memory.at(0x84000+i*4));
					for (unsigned i=n;i<4096;i++) assert(host[i]==0xffffffff);
				}
			}
		}
		return memory.at(0x17ffc4)!=0;
	}
	void Verify() {
		assert(operation && memory.at(0x17ffc4)==workers && n<=8192);
		for (unsigned row=0;row<n;row++) {
			float expected=0;
			for (unsigned b=0;b<k/32;b++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=Q8Machine::Input(b*32+i,trial)*NativeQ8Machine::NativeWeight(row,b*32+i);
				expected=fmaf(float(dot),Q8Machine::Half(Q8Machine::WeightScale(row,b))*Q8Machine::InputScale(b,trial),expected);
			}
			assert(memory.at(0x84000+row*4)==Bits(expected));
		}
		for (unsigned row=n;row<8192;row++) assert(memory.at(0x84000+row*4)==0x7fc00000);
		for (unsigned w=1;w<=workers;w++) assert(peer[w]->pc==CoordWait && peer[w]->l1[peer[w]->Index(0x71300)]==peer[w]->l1[peer[w]->Index(0x71304)]);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2 || (argc==3 && (std::string(argv[2])=="--fast" || std::string(argv[2])=="--readback" || std::string(argv[2])=="--fused" || std::string(argv[2])=="--compact")));
	fastImage=argc==3;
	readbackImage=argc==3 && std::string(argv[2])=="--readback";
	compactImage=argc==3 && std::string(argv[2])=="--compact";
	fusedImage=argc==3 && (std::string(argv[2])=="--fused" || compactImage);
	if (compactImage) for (unsigned k:{1024U,2048U,3072U,5120U}) for (unsigned word=1;word<=k/32;word++) {
		CoordinatorMachine m(argv[1]); m.SetupFused(k,64,119); m.dropScale=word;
		assert(!m.RunCoord() && !m.issued && !m.transfers);
	}
	if (fusedImage) {
		for (unsigned k:{2048U,3072U,5120U,6144U}) { CoordinatorMachine m(argv[1]); m.SetupFused(k,64,119,2); assert(m.RunCoord()); m.Verify(); }
		for (unsigned n:{32U,64U,1024U,2048U,3072U,4096U}) {
			{ CoordinatorMachine m(argv[1]); m.SetupFused(1024,n,119,3); assert(m.RunCoord()); m.Verify(); }
			for (unsigned fault=0;fault<6;fault++) {
				CoordinatorMachine m(argv[1]); m.SetupFused(1024,n,119,3);
				m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
				m.corruptHost=fault==3?0:fault==4?n/2:fault==5?n-1:~0U;
				assert(!m.RunCoord() && !m.memory.at(0x17ffc4));
			}
			printf("OFFLINE fused owned-host output n%u exact all-word comparison/finite math/one write-read pair/partial or corrupted posted write rejection PASS\n",n);
		}
		for (unsigned fault=0;fault<8;fault++) {
			CoordinatorMachine m(argv[1]); m.SetupFused(1024,64,119);
			if (fault==0) m.memory[0x140020]=0; if (fault==1) m.memory[0x140020]=0x10000;
			if (fault==2) m.memory[0x14001c]=1; if (fault==3) m.memory[0x140024]=32768;
			if (fault==4) m.memory[0x140010]=8192; if (fault==5) m.memory[0x140004]=2;
			if (fault==6) m.memory[0x140004]=1; if (fault==7) m.memory[0x140000]^=1;
			assert(!m.RunCoord() && !m.issued && !m.transfers);
		}
		unsigned total=0;
		{ CoordinatorMachine m(argv[1]); m.SetupFused(1024,64,119); assert(m.RunCoord()); total=m.transfers; }
		for (unsigned command:{total-1,total}) for (unsigned fault=0;fault<2;fault++) {
			CoordinatorMachine m(argv[1]); m.SetupFused(1024,64,119);
			if (fault) m.badCounter=command; else m.failTransfer=command;
			assert(!m.RunCoord() && !m.memory.at(0x17ffc4));
		}
	}
	if (readbackImage) {
		for (unsigned n:{32U,64U,1024U,2048U,4096U}) for (unsigned offset:{0U,4096U}) {
			auto setup=[&](CoordinatorMachine& m) {
				m.Setup(0,0,0,1); m.operation=2; m.k=offset; m.n=n;
				m.memory[0x140004]=2; m.memory[0x14000c]=offset; m.memory[0x140010]=n; m.memory[0x140014]=0;
				for (unsigned i=0;i<n;i++) m.memory[0x84000+(offset+i)*4]=m.memory[0x144000+i*4]=0x3f000000+i*17;
			};
			{ CoordinatorMachine m(argv[1]); setup(m); assert(m.RunCoord()); }
			for (unsigned word=0;word<n;word++) {
				CoordinatorMachine m(argv[1]); setup(m); m.memory[0x144000+word*4]^=1U<<(word%32); assert(!m.RunCoord());
			}
			{ CoordinatorMachine m(argv[1]); setup(m); m.memory[0x144000]^=1; m.memory[0x144004]^=1; assert(!m.RunCoord()); }
			{ CoordinatorMachine m(argv[1]); setup(m); m.memory[0x84000+offset*4]=m.memory[0x144000]=0x7f800000; assert(!m.RunCoord()); }
			printf("OFFLINE actual output comparison all%u words,offset%u,single/paired corruptions,nonfinite,no NoC/control/payload writes PASS\n",n,offset);
		}
	}
	// A channel not used by this immutable program is checked at BOTH
	// job boundaries even in fast mode. No final publication when busy.
	for (unsigned address:{0xffb20840U,0xffb21040U,0xffb21840U,0xffb202bcU,0xffb20240U})
		for (unsigned after:{0U,1U,9U}) {
			CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119); m.busyAddress=address; m.busyAfter=after;
			assert(!m.RunCoord() && !m.memory.at(0x17ffc4));
			if (!after) assert(!m.transfers && !m.issued);
		}
	{ CoordinatorMachine m(argv[1]); m.Setup(0,0,0,119); assert(m.RunCoord() && m.transfers==476 && !m.issued); }
	for (unsigned n:{32U,64U,160U,3808U,6144U,8192U}) {
		CoordinatorMachine m(argv[1]); m.Setup(1,1024,n,n==8192?3:119); assert(m.RunCoord()); m.Verify();
		m.Setup(1,1024,n,n==8192?3:119,9); assert(m.RunCoord()); m.Verify();
		printf("OFFLINE actual coordinator + persistent exact math n%u workers%u changed inputs/gather/finite proof PASS\n",n,m.workers);
	}
	for (unsigned k:{2048U,3072U,5120U,6144U}) { CoordinatorMachine m(argv[1]); m.Setup(1,k,64,119,2); assert(m.RunCoord()); m.Verify(); }
	for (unsigned fault=0;fault<4;fault++) {
		CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119);
		m.dropDoorbell=fault==0; m.badOutput=fault==1; m.badPeerPc=fault==2; m.badPeerReset=fault==3;
		assert(!m.RunCoord() && !m.memory.at(0x17ffc4)); if (fault>=2) assert(!m.issued);
	}
	for (unsigned word=0;word<12;word++) { CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119); m.corruptProof=int(word); assert(!m.RunCoord()); }
	for (unsigned word=0;word<8;word++) { CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119); m.dropDescriptor=int(word); assert(!m.RunCoord() && !m.issued); }
	for (unsigned word=1;word<=10;word++) {
		CoordinatorMachine m(argv[1]); m.Setup(0,0,0,1); for (auto& v:m.command) v=0xdeadbeef; m.dropControl=word;
		assert(!m.RunCoord() && !m.transfers);
	}
	for (unsigned command:{1U,4U,9U,10U,11U,17U,20U}) for (unsigned fault=0;fault<2;fault++) {
		CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119);
		if (fault) m.badCounter=command; else m.failTransfer=command;
		assert(!m.RunCoord());
	}
	for (unsigned sequence:{0U,2U,0x7fffffffU}) { CoordinatorMachine m(argv[1]); m.Setup(1,1024,64,119); m.memory[0x140104]=sequence; assert(!m.RunCoord() && !m.issued); }
	for (unsigned fault=0;fault<11;fault++) {
		CoordinatorMachine m(argv[1]); m.Setup(0,0,0,1);
		m.memory[0x140004]=1; m.memory[0x14000c]=1024; m.memory[0x140010]=32;
		if (fault==0) m.memory[0x140000]^=1;
		if (fault==1) m.memory[0x14000c]=32;
		if (fault==2) m.memory[0x140010]=16;
		if (fault==3) m.memory[0x140010]=262176;
		if (fault==4) m.memory[0x140014]=0x04000001;
		if (fault==5) m.memory[0x140014]=0x82400000;
		if (fault==6) { m.memory[0x140010]=262144; m.memory[0x140014]=0x80000000; }
		if (fault==7) m.memory[0x140010]=8192; // >4096 rows on one worker
		if (fault==8) m.memory[0x140018]=0;
		if (fault==9) m.memory[0x140008]=120;
		if (fault==10) m.memory[0x140004]=2;
		assert(!m.RunCoord() && !m.transfers && !m.issued);
	}
	puts("PASS OFFLINE linked coordinator/worker image protocol,all119 routes,all K,gather bounds,fresh full proofs,changed inputs,no reset/DMA,timeout/stale/corruption/drop faults; NO DRIVER EXPOSURE / NATIVE SPEED CLAIM");
}
