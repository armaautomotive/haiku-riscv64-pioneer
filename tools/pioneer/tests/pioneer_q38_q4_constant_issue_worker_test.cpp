/* SPDX-License-Identifier: MIT */
// Execute old and new full worker bytes, including real queued second jobs.
// Only NoC/raw/control endpoints are synthetic; no native speed inference.
#define PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST 1
#define PIONEER_Q38_TWIN_BATCH_QUEUED_WORKER_MACHINE_ONLY
#include "pioneer_q38_twin_batch_prefetch_queued_worker_test.cpp"
struct ConstantIssuePeer:TwinBatchQueuedPeer {
	using TwinBatchQueuedPeer::TwinBatchQueuedPeer;
	std::vector<uint32_t> issued;
	unsigned instructionBufferWrites=0;
	void Instruction(uint32_t word)override {
		if(matrixScope)issued.push_back(word);
		TwinBatchQueuedPeer::Instruction(word);
	}
	void Store(uint32_t at,uint32_t word)override {
		if(matrixScope && at==0xffe40000)instructionBufferWrites++;
		// A real Q4/K17408 FFN-down advances the worker's model layer.
		// The inherited narrow queued fixture only tested K5120, so its
		// producer leaves next.layer=0. Publish the correct NEXT descriptor
		// at the same real final-commit boundary, for BOTH old and new ROM.
		// Never change worker PC/registers/current seal or skip its check.
		const bool nextLayer=queueAtCommit && pushes==1 && at==0xffb20040
		 && command[7]==0x212 && type==12 && inputs==17408;
		TwinBatchQueuedPeer::Store(at,word);
		if(nextLayer) {
			assert(pushes==2 && memory.at(M::Job+4)==2
			 && memory.at(M::Job+7*4)==2026 && !memory.at(M::Job+5*4));
			memory[M::Job+5*4]=1;
		}
	}
};
static void Compare(ConstantIssuePeer& old,ConstantIssuePeer& next) {
	assert(old.issued==next.issued);
	assert(old.matrixInstructions==next.matrixInstructions && old.rawReads==next.rawReads
	 && old.rawUnpacks==next.rawUnpacks && old.drains==next.drains
	 && old.vectorOps==next.vectorOps && old.replayRecords==next.replayRecords
	 && old.replayExpansions==next.replayExpansions
	 && old.requests==next.requests && old.responses==next.responses
	 && old.sumReads==next.sumReads && old.sumWrites==next.sumWrites
	 && old.affineRecords==next.affineRecords && old.affineExpansions==next.affineExpansions
	 && old.baseWrites==next.baseWrites && old.copyWrites==next.copyWrites
	 && !old.dotProbeReads && !next.dotProbeReads);
	for(unsigned row=0;row<old.FfnPersistentPeer::groups*16;row++)
		assert(old.buffers[4096+row]==next.buffers[4096+row]);
	for(unsigned i=0;i<64;i++)assert(old.mailbox[i]==next.mailbox[i]);
	if(old.type==12)assert(next.instructionBufferWrites<old.instructionBufferWrites);
}
int main(int argc,char** argv) {
	assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0,faults=0,stack=0,orderFaults=0;
	(void)&RunPersistentGdnPair;
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U}) {
		printf("CONSTANT_Q4_CASE type%u K%u groups%u\n",type,k,groups);
		auto old=std::make_unique<ConstantIssuePeer>(argv[1],argv[2],69,type,k,groups);
		auto next=std::make_unique<ConstantIssuePeer>(argv[1],argv[3],69,type,k,groups);
		unsigned before[2];
		for(unsigned side=0;side<2;side++) {
			auto& p=side?*next:*old;p.delay=3;p.BootPushed();p.Prepare();before[side]=p.steps;
			p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
		}
		Compare(*old,*next);
		printf("CONSTANT_Q4_EMULATOR_ONLY job1 type%u K%u groups%u RV%u/%u instructionBufferWrites%u/%u;NOT_DEVICE_TIME\n",
		 type,k,groups,old->steps-before[0],next->steps-before[1],old->instructionBufferWrites,next->instructionBufferWrites);
		std::array<uint32_t,16> first[2];
		for(unsigned side=0;side<2;side++) {
			auto& p=side?*next:*old;
			if(type==12 && k==17408 && groups==1) {
				fflush(nullptr);pid_t child=fork();assert(child>=0);
				if(!child) {
					p.memory[M::Job+5*4]=0; // deliberately repeat the completed layer
					p.ReceiveQueued();
					while(p.pc!=p.faultPC && p.pc!=p.stopPC)p.StepBlock();
					assert(p.pc==p.faultPC && !p.firstRead && !p.requests && !p.pushes);
					auto memory=p.memory;
					for(unsigned n=0;n<2048;n++)p.StepBlock();
					assert(p.pc==p.faultPC && p.memory==memory && !p.requests && !p.pushes);
					_exit(0);
				}
				int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
				orderFaults++;
			}
			for(unsigned i=0;i<16;i++)first[side][i]=p.buffers[4096+i];
			p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first[side]);
			assert(p.memory.at(M::Status+6*4)==unsigned(type==12 && k==17408));
			stack=std::max(stack,0x70000-p.minStack);
		}
		Compare(*old,*next);cases++;
	}
	for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}) {
		auto p=std::make_unique<ConstantIssuePeer>(argv[1],argv[3],69,type,5120,3);
		p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
		p->faultAt=which==0?1:which==1?p->inputReads+1
		 :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
		p->ReadFault();faults++;
	}
	printf("PASS_CONSTANT_Q4_ACTUAL_WORKER shapes%u retainedChangedInputJobs%u NoCLossInert%u outOfOrderRefusals%u maxCombinedStack%u/8192;"
	 "SAME complete Tensix stream,integer+FP32 outputs,rawUNPACK,drains,replay,ACK,source,NoC and STOP;"
	 "ONLY synthetic endpoints,NO physical/native/whole-model/Work/TPS grant\n",cases,cases*2,faults,orderFaults,stack);
}
