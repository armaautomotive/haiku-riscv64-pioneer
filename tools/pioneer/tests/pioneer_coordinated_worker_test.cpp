/* SPDX-License-Identifier: MIT */
#define PIONEER_PERSISTENT_MACHINE_ONLY
#include "pioneer_persistent_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeCoordinatedWorkerImage.h"
enum { CoordWait=0x71280,CoordWaitLast=0x71294,CoordStop=0x712c4,CoordFault=0x711ac,
	CoordReady=0x43524431,CoordStopped=0x43535450 };
struct CoordinatedMachine:PersistentMachine {
	uint32_t expectedSource[16]={};
	unsigned sourceReads=0,failSourceRead=0;
	unsigned busyOffset=0;
	CoordinatedMachine(const std::vector<uint8_t>& code,unsigned k,unsigned n,unsigned w):PersistentMachine(k,n,w) {
		assert(w && w<120 && code.size()>0x500 && code.size()<0x910);
		for (unsigned i=0;i<1024;i++) l1[Index(0x71000+i*4)]=i<code.size()/4?Word(code,i*4):0;
		for (unsigned i=0;i<16;i++) l1[Index(0x71380+i*4)]=expectedSource[i]=0x10000+i;
		if (getenv("PIONEER_COORDINATED_INLINE_MATH")) Inline();
		if (getenv("PIONEER_COORDINATED_PARTIAL_MATH")) Partial();
		if (getenv("PIONEER_COORDINATED_PHASE_MATH")) Phase();
	}
	bool CodeAddress(uint32_t address) const override {
		return (address>=0x71000 && address<0x71300) || (address>=0x71500 && address<0x71700)
			|| (inlineMath && address>=0x70ea8 && address<0x70fac)
			|| (address<0x71000 && BulkQ8Machine::CodeAddress(address));
	}
	uint32_t Load(uint32_t a) override {
		for (unsigned i=0;i<16;i++) if (a==0xffb20000+BlackholeDramSourceOffset(i)) {
			sourceReads++; return expectedSource[i]^(sourceReads==failSourceRead?1:0);
		}
		if (busyOffset && a==0xffb20000+busyOffset) return 1;
		if (a==0xffb20840 || a==0xffb21040 || a==0xffb21840 || (a>=0xffb20240 && a<0xffb202c0)) {
			assert(!pending && !inputPending); return 0;
		}
		return PersistentMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0x71340 && a<0x71380) { assert(!pending && !inputPending); l1[Index(a)]=v; return; }
		PersistentMachine::Store(a,v);
	}
	void UntilCoord(uint32_t target) {
		unsigned began=steps;
		do { Step(); } while (pc!=target && steps-began<40000000);
		assert(pc==target && drained && !pending && !inputPending);
	}
	void BootCoord() { UntilCoord(CoordWait); assert(l1[Index(0x7130c)]==CoordReady && !matrixInstructions && !requests); }
	void JobCoord(unsigned trial) {
		assert(pc==CoordWait && l1[Index(0x71300)]==jobs && l1[Index(0x71304)]==jobs);
		previousReads+=responses; requests=responses=0; this->trial=trial;
		for (unsigned k=0;k<inputs;k++) {
			if (!(k&1)) source[k/2]=0;
			source[k/2]|=uint32_t(PioneerInt8Operand(Input(k,trial)))<<(16*(k&1));
		}
		for (unsigned b=0;b<inputs/32;b++) source[3072+b]=Bits(InputScale(b,trial));
		l1[Index(0x71300)]=++jobs; UntilCoord(CoordWait);
		assert(l1[Index(0x71304)]==jobs && CorrectExpected() && Guards() && !memcmp(buffers,source,sizeof(source)));
		unsigned reads=ExpectedFixtures()/32;
		for (unsigned i=0;i<4;i++) assert(uint32_t(l1[Index(0x71350+i*4)]-l1[Index(0x71340+i*4)])==(i==1?reads*144+256:reads+1));
		assert(l1[Index(0x71360)]==0x703ec && l1[Index(0x71364)]==ExpectedFixtures()
			&& l1[Index(0x71368)]==0x4d415431 && l1[Index(0x7136c)]==jobs);
	}
	void StopCoord() {
		unsigned count=matrixInstructions;
		l1[Index(0x71308)]=1; l1[Index(0x71300)]=++jobs; UntilCoord(CoordStop);
		assert(l1[Index(0x71304)]==jobs && l1[Index(0x7130c)]==CoordStopped && matrixInstructions==count);
	}
};
#ifndef PIONEER_COORDINATED_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2); auto image=Read(argv[1]); assert(image.size()==TT_COORDINATED_WORKER_WORDS*4);
	for (unsigned i=0;i<1024;i++) assert(BlackholeCoordinatedWorkerCodeWord(i)==(i<image.size()/4?Word(image,i*4):0));
	for (unsigned w=1;w<120;w++) { CoordinatedMachine m(image,1024,32,w); m.BootCoord(); m.JobCoord(0); m.JobCoord(9); m.StopCoord(); }
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) { CoordinatedMachine m(image,k,64,119); m.BootCoord(); m.JobCoord(2); m.JobCoord(3); m.StopCoord(); }
	{ CoordinatedMachine m(image,1024,4096,1); m.BootCoord(); m.JobCoord(1); m.JobCoord(2); m.StopCoord(); }
	for (unsigned read=1;read<=32;read++) {
		CoordinatedMachine m(image,1024,32,1); m.BootCoord(); m.failSourceRead=read;
		bool failed=false; try { m.JobCoord(0); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.l1[m.Index(0x71304)]==0);
	}
	for (unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}) {
		CoordinatedMachine m(image,1024,32,1); m.BootCoord(); m.busyOffset=offset;
		bool failed=false; try { m.JobCoord(0); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && !m.matrixInstructions && m.l1[m.Index(0x71304)]==0);
	}
	puts("PASS OFFLINE coordinated-worker actual RISC/selected arithmetic oracle,all119 peers,all K,N4096,changed jobs without reset,fresh counter/status snapshots,STOP/source/NoC faults; NOT native speed or full-model quality");
}
#endif
