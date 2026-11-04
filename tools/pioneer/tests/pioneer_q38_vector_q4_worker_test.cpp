/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CONSTANT_ISSUE_WORKER_MACHINE_ONLY
#include "pioneer_q38_q4_constant_issue_worker_test.cpp"
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"

// Execute the ACTUAL complete new ROM. Only the newly compiled helper's
// dynamic extent uses this strict additional observer;all existing matrix,
// NoC,source-bank,format,replay,deferred-store and publisher checks remain
// unchanged. No PC skips or supplied model results in a positive run.
struct VectorMetadataPeer:ConstantIssuePeer {
	unsigned metadataPC=0,metadataReturn=0,metadataBlocks=0,metadataCommands=0;
	bool active=false,storesSeen[64][16]={};
	MetadataMachine arithmetic;
	std::array<uint32_t,64> oldAccumulator={};
	VectorMetadataPeer(const char* fixture,const char* image,unsigned type,unsigned k,unsigned groups)
	 :ConstantIssuePeer(fixture,image,69,type,k,groups) {
		std::ifstream symbols(std::string(image)+"/program.symbols");std::string at,kind,name;
		while(symbols>>at>>kind>>name)if(name.find("PioneerQ38VectorQ4Metadata5Block")!=std::string::npos) {
			assert(!metadataPC);metadataPC=unsigned(std::stoul(at,nullptr,16));
		}
		assert(metadataPC);
	}
	void StepBlock()override {
		if(pc==metadataPC) {
			assert(matrixScope && type==12 && !active && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
			 && !ownerA && !sources[0] && !sources[1] && drained && !recordRemaining);
			active=true;metadataReturn=regs[1];arithmetic=MetadataMachine{};
			memcpy(arithmetic.lanes,lr,sizeof(lr));memset(storesSeen,0,sizeof(storesSeen));
			for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++)oldAccumulator[r*16+c]=FloatDst(108+r,c);
		}
		ConstantIssuePeer::StepBlock();
		if(active && pc==metadataReturn) {
			assert(drained && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
			 && !ownerA && !sources[0] && !sources[1] && thread[5]==4);
			for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++)assert(oldAccumulator[r*16+c]==FloatDst(108+r,c));
			uint32_t observed=0;
			for(unsigned slot=0;slot<16;slot++) {
				for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++)assert(storesSeen[slot*4+r][c]);
				observed|=1U<<slot;
			}
			assert(observed==65535);blockImportMask=observed; // observed real stores,NOT a fabricated import
			for(unsigned g=0;g<8;g++) {
				PioneerQ38Panel reference={};
				assert(PioneerQ38UnpackPanel(12,tiles.at(CurrentTile()).data(),unsigned(tile.size()),g*2,&reference));
				for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++) {
					assert(FloatDst(128+g*8+r,c)==(r?0:reference.coefficient[c]));
					assert(FloatDst(132+g*8+r,c)==(r?0:reference.minimum[c]));
				}
			}
			active=false;metadataReturn=0;metadataBlocks++;
		}
	}
	void Instruction(uint32_t op)override {
		const unsigned code=op>>24;
		if(!active || !((code>=0x70 && code<=0x90) || op==0x8f000000)) {
			ConstantIssuePeer::Instruction(op);if(active)arithmetic.cycle++;return;
		}
		assert(!rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !ownerA
		 && !sources[0] && !sources[1] && (!pending || DisjointPrefetchCompute())
		 && thread[5]==4 && config[220]==0 && config[1]==0xa03c8000);
		if(pending)overlapOps++;
		metadataCommands++;vectorOps++;drained=false;
		if(op==0x8a000002){enabled=true;arithmetic.cycle++;return;}
		assert(enabled);
		if(code==0x70 || code==0x72) {
			const unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
			assert(reg<8 && !(op&0xfc00) && !(at&1));
			for(unsigned lane=0;lane<32;lane++) {
				unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+bool(at&2);
				if(code==0x70) {
					if(mode==13) {
						assert(row<16 && defined[row] && wrote[row][col]);
						arithmetic.lanes[reg][lane]=BlackholeRawSfpLoad(physical[row][col],13);
					} else {assert(mode==3 && row>=184 && row<192);arithmetic.lanes[reg][lane]=FloatDst(row,col);}
				} else {
					assert(mode==3 && row>=128 && row<192);
					uint32_t value=arithmetic.Read(reg,lane);
					assert(!(value&0x7fffffff) || ((value&0x7f800000) && (value&0x7f800000)!=0x7f800000));
					StoreFloatDst(row,col,value);defined[Adj(row)]=true;storesSeen[row-128][col]=true;
				}
			}
			if(code==0x70)arithmetic.ready[reg]=arithmetic.cycle+1;
			arithmetic.cycle++;
		} else if(code==0x8c) {
			assert(op==0x8c000000);uint32_t old[8][32];
			for(unsigned r=0;r<8;r++)for(unsigned l=0;l<32;l++)old[r][l]=arithmetic.Read(r,l);
			for(unsigned b:{0U,4U})for(unsigned r=0;r<4;r++)for(unsigned g=0;g<4;g++)for(unsigned c=0;c<8;c++)
				arithmetic.lanes[b+r][g*8+c]=old[b+g][r*8+c];
			for(auto& r:arithmetic.ready)r=arithmetic.cycle+1;
			arithmetic.cycle++;
		} else arithmetic.Op(op);
		assert(arithmetic.okay);memcpy(lr,arithmetic.lanes,sizeof(lr));
	}
};

int main(int argc,char** argv) {
	assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--pilot") || !strcmp(argv[4],"--faults-only"))));setvbuf(stdout,nullptr,_IOLBF,0);
	const bool pilot=argc==5 && !strcmp(argv[4],"--pilot"),faultsOnly=argc==5 && !strcmp(argv[4],"--faults-only");
	unsigned cases=0,stack=0;(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}) {
		if(faultsOnly || (pilot && (k!=5120 || groups!=1)))continue;
		printf("VECTOR_METADATA_ACTUAL_CASE type%u K%u groups%u\n",type,k,groups);
		auto old=std::make_unique<ConstantIssuePeer>(argv[1],argv[2],69,type,k,groups);
		auto next=std::make_unique<VectorMetadataPeer>(argv[1],argv[3],type,k,groups);
		for(unsigned side=0;side<2;side++) {
			auto& p=side?static_cast<ConstantIssuePeer&>(*next):*old;
			p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
		}
		for(unsigned job=1;job<=2;job++) {
			assert(old->requests==next->requests && old->responses==next->responses
			 && old->matrixInstructions==next->matrixInstructions && old->sumReads==next->sumReads && old->sumWrites==next->sumWrites
			 && !next->active && !next->dotProbeReads && !next->blockImports);
			assert(next->metadataBlocks==(type==12?job*groups*(k/256):0));
			for(unsigned r=0;r<groups*16;r++)assert(old->buffers[4096+r]==next->buffers[4096+r]);
			for(unsigned i=0;i<64;i++)assert(old->mailbox[i]==next->mailbox[i]);
			if(job==2)break;
			for(unsigned side=0;side<2;side++) {
				auto& p=side?static_cast<ConstantIssuePeer&>(*next):*old;
				std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
				p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first);stack=std::max(stack,0x70000-p.minStack);
			}
		}
		cases++;
		printf("PASS_VECTOR_METADATA_ACTUAL_CASE type%u K%u groups%u jobs2 vectorBlocks%u scalarMetadataReads%u/%u RV%u/%u;EMULATOR_COUNTS_NOT_SILICON_TPS\n",
		 type,k,groups,next->metadataBlocks,old->metadataReads,next->metadataReads,old->steps,next->steps);
	}
	unsigned faults=0,finiteRefusals=0;
	if(!pilot) {
		for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}) {
			auto p=std::make_unique<VectorMetadataPeer>(argv[1],argv[3],type,5120,3);
			p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
			p->faultAt=which==0?1:which==1?p->inputReads+1
			 :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
			p->ReadFault();faults++;
		}
		for(unsigned kind=0;kind<2;kind++)for(unsigned word=0;word<4;word++) {
			auto p=std::make_unique<VectorMetadataPeer>(argv[1],argv[3],12,5120,1);
			p->tiles[0][kind*32+16+word*4+3]|=0x7c; // exponent31 in each high-byte word
			p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
			while(p->pc!=p->faultPC && !p->heldPayload)p->StepBlock();
			assert(p->pc==p->faultPC && !p->heldPayload && !p->metadataBlocks && !p->active
			 && !p->matrixInstructions && !p->pushes && !p->mailbox[0]);
			auto memory=p->memory;unsigned issued=p->requests,completed=p->responses;
			for(unsigned i=0;i<2048;i++)p->StepBlock();
			assert(p->pc==p->faultPC && p->memory==memory && p->requests==issued && p->responses==completed && !p->pushes);
			finiteRefusals++;
		}
	}
	printf("%s_VECTOR_METADATA_COMPLETE_WORKER shapes%u retainedChangedInputJobs%u NoCLossInert%u compiledFiniteRefusals%u stack%u/8192;ACTUAL complete ROM/original quant bytes/matrix+NoC+replay+publisher+STOP checks unchanged;SYNTHETIC_ENDPOINTS_ONLY,NO_NATIVE_HARDWARE_MODEL_TPS_GRANT\n",argc==5?"PARTIAL":"PASS",cases,cases*2,faults,finiteRefusals,stack);
}
