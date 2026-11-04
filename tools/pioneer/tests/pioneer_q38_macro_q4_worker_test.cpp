/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"

// Observe REAL instructions from the complete linked worker. All prior
// raw source,NoC,SrcA bank,matrix,metadata,affine,publication and STOP checks
// are inherited unchanged. Only the NEW config/macro/native-reg1 subset
// has this additional strict cycle-aware implementation.
struct MacroQ4Peer:VectorMetadataPeer {
 unsigned setupPC=0,decodePC=0,finishPC=0,macroReturn=0,extent=0;
 unsigned clock=0,ready[8]={},templates[4]={},sequence[4]={},misc=0;
 unsigned macroLoads=0,macroStores=0,macroPanels=0,macroSetups=0,macroFinishes=0;
 unsigned firstLoad=0,firstStore=0;
 struct Pending {unsigned at,reg;uint32_t op;};std::vector<Pending> pendingSimple;
 MacroQ4Peer(const char* fixture,const char* image,unsigned type,unsigned k,unsigned groups)
 :VectorMetadataPeer(fixture,image,type,k,groups) {
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string at,kind,name;
  while(symbols>>at>>kind>>name)if(name.find("PioneerQ38MacroQ4Decode")!=std::string::npos) {
   unsigned value=unsigned(std::stoul(at,nullptr,16));
   if(name.find("5Setup")!=std::string::npos){assert(!setupPC);setupPC=value;}
   else if(name.find("6Decode")!=std::string::npos){assert(!decodePC);decodePC=value;}
   else if(name.find("6Finish")!=std::string::npos){assert(!finishPC);finishPC=value;}
  }
  assert(setupPC && decodePC && finishPC);
 }
 void StepBlock()override {
  if(pc==setupPC || pc==decodePC || pc==finishPC) {
   assert(!extent && !active && matrixScope && type==12 && !rawPending && phaseQueue.empty() && pendingSimple.empty());
   extent=pc==setupPC?1:pc==decodePC?2:3;macroReturn=regs[1];
   firstLoad=macroLoads;firstStore=macroStores;
  }
  VectorMetadataPeer::StepBlock();
  if(extent && pc==macroReturn) {
   assert(pendingSimple.empty());
   if(extent==1){assert(sequence[0]==4 && sequence[1]==5 && !sequence[2] && !sequence[3] && !misc);macroSetups++;}
   else if(extent==2){assert(macroLoads-firstLoad==8 && macroStores-firstStore==8);macroPanels++;}
   else {for(unsigned s:sequence)assert(!s);assert(!misc);macroFinishes++;}
   extent=macroReturn=0;
  }
 }
 void Instruction(uint32_t op)override {
  const unsigned code=op>>24;
  if(extent==1 && (op==0x8a000002 || code==0x71)) {
   assert(!active && !ownerA && !sources[0] && !sources[1] && !rawPending
    && phaseQueue.empty() && pendingAccumulator.empty() && pendingSimple.empty() && thread[5]==4);
   if(op==0x8a000002)enabled=true;
   else {
    unsigned reg=(op>>20)&15,mode=(op>>16)&15;
    assert(enabled && !reg && (mode==0 || mode==10));
    for(unsigned l=0;l<32;l++)lr[0][l]=mode==0?(op&65535)<<16:(lr[0][l]&0xffff0000)|(op&65535);
    ready[0]=clock+1;
   }
   vectorOps++;drained=false;issued.push_back(op);
  } else if(code==0x91) {
   assert((extent==1 || extent==3) && enabled && pendingSimple.empty() && !active && thread[5]==4);
   unsigned vd=(op>>4)&15,mod=op&15,value=(op>>8)&65535;
   assert(vd<=8 && (mod==0 || mod==1));
   if(!mod){assert(ready[0]<=clock);value=lr[0][0];for(unsigned l=0;l<32;l++)assert(lr[0][l]==value);}
   if(vd<4){assert(extent==1 && vd<2 && value==(vd?0x7affc001U:0x7e004001U));templates[vd]=value;}
   else if(vd<8){assert(value==(extent==1 && vd<6?vd:0));sequence[vd-4]=value;}
   else {assert(!value);misc=value;}
   vectorOps++;drained=false;issued.push_back(op);
  } else if(code==0x93) {
   assert(extent==2 && !active && enabled && ownerA && sources[mathBank] && thread[5]==4
    && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && config[220]==0
    && config[1]==0xa03c8000 && (!pending || DisjointPrefetchCompute()));
   unsigned index=(op>>22)&3,reg=(op>>20)&3,addr=op&1023;
   assert(reg<2 && index<2 && ((op>>16)&15)==13 && !(op&0xfc01) && addr<16 && !misc);
   assert(sequence[index]==4+index && templates[index]==(index?0x7affc001U:0x7e004001U));
   for(const auto& p:pendingSimple)assert(p.reg!=reg && p.at!=clock+1);
   for(unsigned l=0;l<32;l++) {
    unsigned row=(addr&~3U)+l/8,col=(l%8)*2+bool(addr&2);
    assert(defined[row] && wrote[row][col]);lr[reg][l]=BlackholeRawSfpLoad(physical[row][col],13);
    assert(lr[reg][l]<=255);
   }
   ready[reg]=clock+2;pendingSimple.push_back({clock+1,reg,templates[index]});macroLoads++;
   vectorOps++;drained=false;issued.push_back(op);
  } else if(extent==2 && code==0x72) {
   assert(enabled && ownerA && sources[mathBank] && thread[5]==4 && config[220]==0);
   unsigned reg=(op>>20)&15,addr=op&1023;
   assert(reg<2 && ((op>>16)&15)==5 && !(op&0xfc01) && ready[reg]<=clock);
   for(unsigned l=0;l<32;l++) {
    unsigned row=(addr&~3U)+l/8,col=(l%8)*2+bool(addr&2);
    assert((row>=128 && row<136) || (row>=144 && row<152));
    unsigned logical=64+row-128-(row>=144?8:0),hi=Adj(logical),lo=hi+8;
    assert(hi==row && defined[hi] && wrote[lo][col] && !physical[lo][col] && !decoded[logical-64][col]);
    uint32_t value=lr[reg][l];assert(value<=15);
    physical[row][col]=uint16_t((value<<5)|16);wrote[row][col]=decoded[logical-64][col]=true;
   }
   macroStores++;nativeStoreOps++;vectorOps++;drained=false;issued.push_back(op);
  } else {
   if(extent==2 && code==8)assert(pendingSimple.empty());
   VectorMetadataPeer::Instruction(op);
   if(code==0x71){unsigned reg=(op>>20)&15;assert(reg<8);ready[reg]=clock+1;}
  }
  for(unsigned i=0;i<pendingSimple.size();) {
   auto p=pendingSimple[i];if(p.at!=clock){i++;continue;}
   for(unsigned l=0;l<32;l++) {
    if(p.op==0x7e004001){assert(ready[4]<=clock && lr[4][l]==15);lr[p.reg][l]&=lr[4][l];}
    else {assert(p.op==0x7affc001);lr[p.reg][l]>>=4;}
   }
   ready[p.reg]=clock+1;pendingSimple.erase(pendingSimple.begin()+i);
  }
  clock++;
 }
};
int main(int argc,char** argv) {
 assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--pilot") || !strcmp(argv[4],"--faults-only"))));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool faultsOnly=argc==5 && !strcmp(argv[4],"--faults-only");
 unsigned cases=0,stack=0;(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}) {
  if(faultsOnly || (argc==5 && (k!=5120 || groups!=1)))continue;
  printf("MACRO_Q4_ACTUAL_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<VectorMetadataPeer>(argv[1],argv[2],type,k,groups);
  auto next=std::make_unique<MacroQ4Peer>(argv[1],argv[3],type,k,groups);
  for(unsigned side=0;side<2;side++) {
   auto& p=side?static_cast<VectorMetadataPeer&>(*next):*old;
   p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
  }
  for(unsigned job=1;job<=2;job++) {
   assert(old->requests==next->requests && old->responses==next->responses
    && old->matrixInstructions==next->matrixInstructions && old->sumReads==next->sumReads && old->sumWrites==next->sumWrites
    && old->rawReads==next->rawReads && old->rawUnpacks==next->rawUnpacks && old->metadataBlocks==next->metadataBlocks
    && !next->extent && !next->active && !next->dotProbeReads && !next->blockImports);
   assert(next->macroPanels==(type==12?job*groups*(k/256)*16:0));
   assert(next->macroSetups==(type==12?job:0) && next->macroFinishes==next->macroSetups);
   for(unsigned r=0;r<groups*16;r++)assert(old->buffers[4096+r]==next->buffers[4096+r]);
   for(unsigned i=0;i<64;i++)assert(old->mailbox[i]==next->mailbox[i]);
   if(job==2)break;
   for(unsigned side=0;side<2;side++) {
    auto& p=side?static_cast<VectorMetadataPeer&>(*next):*old;
    std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
    p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first);stack=std::max(stack,0x70000-p.minStack);
   }
  }
  cases++;
  printf("PASS_MACRO_Q4_ACTUAL_CASE type%u K%u groups%u jobs2 macroPanels%u RV%u/%u vectorIssued%u/%u;EMULATOR_COUNTS_NOT_TPS\n",
   type,k,groups,next->macroPanels,old->steps,next->steps,old->vectorOps,next->vectorOps);
 }
 unsigned faults=0;
 if(argc==4 || faultsOnly)for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}) {
  auto p=std::make_unique<MacroQ4Peer>(argv[1],argv[3],type,5120,3);
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->faultAt=which==0?1:which==1?p->inputReads+1
   :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
  p->ReadFault();faults++;
 }
 printf("%s_MACRO_Q4_COMPLETE_WORKER shapes%u retainedChangedInputJobs%u NoCLossInert%u stack%u/8192;ACTUAL_ROM/noCPUinfer/nochangedweights/NO_NATIVE_HARDWARE_TPS_GRANT\n",
  argc==5?"PARTIAL":"PASS",cases,cases*2,faults,stack);
}
