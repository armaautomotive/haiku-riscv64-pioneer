/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_WORKER_MACHINE_ONLY
#include "pioneer_attention_worker_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGluWorkerImage.h"
int main(int argc,char** argv)
{
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==TT_GLU_WORKER_IMAGE_WORDS*4);
	for (unsigned i=0;i<1024;i++) assert(BlackholeGluWorkerCodeWord(i)==(i<code.size()/4?Word(code,i*4):0));
	for (unsigned w=1;w<=6;w++) {
		AttentionWorker m(code,w); m.BootCoord(); m.JobCoord(0); m.Enter(true); m.CompleteStub(512,true);
		m.Enter(); m.CompleteStub(1); m.l1[m.Index(0x7051c)]=0; m.JobCoord(9); m.StopCoord();
	}
	{ AttentionWorker m(code,1); m.BootCoord(); m.Enter(true); unsigned done=m.l1[m.Index(0x71304)]; m.failSourceRead=m.sourceReads+1;
		bool rejected=false; try { m.CompleteStub(512,true); } catch (uint32_t marker) { rejected=marker==0x4e4f4346; }
		assert(rejected && m.l1[m.Index(0x71304)]==done); }
	puts("PASS OFFLINE actual matrix/GLU/attention dispatch/return/live STOP,all6 GLU peers,source fault prevents finished publication;arithmetic entries stubbed here and separately instruction-tested,NOT native qualification");
}
