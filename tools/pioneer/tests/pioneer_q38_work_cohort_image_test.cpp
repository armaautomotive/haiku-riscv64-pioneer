/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned workers:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		DispatchMachine checked(argv[1],workers,type,k); assert(checked.RunRoot());
		DispatchMachine work(argv[1],workers,type,k); work.memory[0x74020]=1;
		assert(work.RunRoot() && !work.readbacks && work.memory.at(0x7f8c0)==1);
		assert(checked.packets-work.packets==5*workers); // ONLY redundant prior audit+descriptor echo
		work.Configure(2); work.memory[0x74020]=1; assert(work.RunRoot());
		work.Configure(3); assert(work.RunRoot() && work.memory.at(0x7f8c0)==0 && work.readbacks==workers);
		work.active=119; work.Configure(4,true); assert(work.RunRoot() && work.readbacks==119);
	}
	for (unsigned field=0;field<64;field++) {
		DispatchMachine m(argv[1],7); m.memory[0x74020]=1; m.corruptWorker=7; m.corruptProof=field;
		assert(!m.RunRoot() && m.unknown);
	}
	for (unsigned worker:{1U,4U,7U}) {
		DispatchMachine m(argv[1],7); m.memory[0x74020]=1; m.nonfiniteWorker=worker;
		assert(!m.RunRoot() && m.unknown);
		DispatchMachine pc(argv[1],7); pc.memory[0x74020]=1; pc.badPcWorker=worker;
		assert(!pc.RunRoot() && pc.unknown && pc.doorbells==7 && !pc.gathers); // CURRENT physical audit remains mandatory
	}
	for (unsigned packet:{1U,20U,40U}) {
		DispatchMachine m(argv[1],7); m.memory[0x74020]=1; m.faultAt=packet;
		assert(!m.RunRoot() && m.unknown && m.pending && m.packets==packet);
	}
	for (unsigned flag:{2U,3U,UINT32_MAX}) {
		DispatchMachine m(argv[1],7); m.memory[0x74020]=flag; assert(!m.RunRoot() && !m.packets);
	}
	DispatchMachine stop(argv[1],119); stop.Configure(1,true); stop.memory[0x74020]=1;
	assert(!stop.RunRoot() && !stop.packets);
	puts("PASS ACTUAL WORK ROOT:exactly5 fewer NoC packets/worker,all1/2/7/119 workers xQ8/Q4/Q6,checked/work/repeated/STOP isolation,ALL64 final peer proofs/nonfinite/CURRENT physical-PC failures retained,first unknown stops without another packet;NO DEVICE IO");
}
