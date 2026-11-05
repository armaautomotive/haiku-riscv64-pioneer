/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TWO_PRODUCT_WORKER_TEST 1
#define PIONEER_Q38_INSTRUCTION_PREFETCH_WORKER_TEST 1
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"

// Actual complete ROM execution. Only this DISTINCT arithmetic image gets
// the block-integer oracle;the old teacher and its exact K16 checks stay.
struct BlockIntegerPeer:InstructionPrefetchPeer {
 bool integerMode;
 unsigned integerMads=0,integerCasts=0,halfAndBaseStores=0;
 BlockIntegerPeer(const char* fixture,const char* image,unsigned type,unsigned k,unsigned groups,bool selected=true,unsigned prefix=0,unsigned worker=69)
  :InstructionPrefetchPeer(fixture,image,type,k,groups,prefix,worker),integerMode(selected){registerAffineReplay=!selected;}
 void IssueSfpu(unsigned op)override{
  const unsigned code=op>>24,at=op&1023;
  const bool store=code==0x72 && (at&~3U)>=112 && (at&~3U)<124;
  if(!integerMode || (code!=0x84 && op!=0x90000330 && !store)){
   ExactQ4WorkPeer::IssueSfpu(op);return;
  }
  if(Tasks() || fpuPending || !enabled)Fail(__LINE__);math.cycle=tick;
  if(store){
   const unsigned reg=(op>>20)&15,mode=(op>>16)&15;
   assert(reg<8 && mode==3 && !(at&1) && !(op&0xfc00));
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+bool(at&2);
    if(!fpValid[Adj(row)] || !fpValid[Adj(row)+8])Fail(__LINE__);
    PutFloat(row,col,math.Read(reg,lane),tick+1);
   }
   storeDue=tick+1;sfpuPending=true;exactStores++;halfAndBaseStores++;
  }else{
   unsigned result[32],destination=3,latency=1;
   if(code==0x84){
    assert(op==0x84001440 || op==0x84023550 || op==0x84001660 || op==0x84023770);
    const unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15;destination=(op>>4)&15;latency=2;
    for(unsigned lane=0;lane<32;lane++)result[lane]=PioneerBlackholeFmaBits(math.Read(a,lane),math.Read(b,lane),math.Read(c,lane));
    integerMads++;
   }else{
    for(unsigned lane=0;lane<32;lane++){
     const unsigned sm=math.Read(3,lane),magnitude=sm&0x7fffffff;
     assert(magnitude<=14336 && (magnitude || !sm));
     result[lane]=Bits(float(int(magnitude)*(sm>>31?-1:1)));
    }
    integerCasts++;
   }
   for(unsigned lane=0;lane<32;lane++)math.lanes[destination][lane]=result[lane];
   math.ready[destination]=tick+latency;
  }
  if(!math.okay)Fail(__LINE__);memcpy(lr,math.lanes,sizeof(lr));vectorOps++;drained=false;
 }
 static unsigned Factor(const uint8_t* tile,unsigned group,unsigned row,bool minimum){
  if(group<4)return tile[((minimum?8:4)+group)*16+row]&63;
  const unsigned byte=tile[(8+group)*16+row];
  return minimum?(byte>>4)|((tile[(4+group)*16+row]>>6)<<4):(byte&15)|((tile[group*16+row]>>6)<<4);
 }
 bool CorrectRows()const override{
  if(!integerMode || type!=12)return InstructionPrefetchPeer::CorrectRows();
  if(matrixInstructions!=ExpectedMatrixInstructions())return false;
  const unsigned blocks=inputs/256;
  for(unsigned group=0;group<FfnPersistentPeer::groups;group++){
   float expected[16]={};double reference[16]={},expanded[16]={};
   for(unsigned block=0;block<blocks;block++){
    const auto& tile=tiles.at(group*blocks+block);int64_t total[16]={},minimum[16]={};
    for(unsigned p=0;p<16;p++){
     PioneerQ38Panel panel;assert(PioneerQ38UnpackPanel(12,tile.data(),unsigned(tile.size()),p,&panel));
     for(unsigned row=0;row<16;row++){
      const unsigned s=Factor(tile.data(),p/2,row,false),m=Factor(tile.data(),p/2,row,true);
      uint32_t db=0,mb=0;
      assert(PioneerQ38PanelScale(uint16_t(tile[row]|(tile[16+row]<<8)),1,db)
       && PioneerQ38PanelScale(uint16_t(tile[32+row]|(tile[48+row]<<8)),1,mb));
      const double d=Float(db),dm=Float(mb),activation=Float(scales[block]);
      for(unsigned k=0;k<16;k++){
       const unsigned element=k*16+row,encoded=panel.operand[element/2]>>((element%2)*16)&65535;
       const unsigned input=vector[block*256+p*16+k];assert(PioneerInt8OperandValid(uint16_t(input)) && !(encoded&0x8000));
       const int q=int(encoded&1023),x=int(input&1023)*(input&0x8000?-1:1);
       assert(q>=0 && q<=15);total[row]+=int64_t(q)*x*s;minimum[row]+=int64_t(x)*m;
       reference[row]+=(d*s*q-dm*m)*activation*x;
       expanded[row]+=(fabs(d*s*q)+fabs(dm*m))*fabs(activation*x);
      }
     }
    }
    for(unsigned row=0;row<16;row++){
     uint32_t db=0,mb=0;
     assert(PioneerQ38PanelScale(uint16_t(tile[row]|(tile[16+row]<<8)),1,db)
      && PioneerQ38PanelScale(uint16_t(tile[32+row]|(tile[48+row]<<8)),1,mb));
     volatile float d=Float(db)*Float(scales[block]),negativeMinimum=(-Float(mb))*Float(scales[block]);
     volatile float product=d*float(total[row]),correction=negativeMinimum*float(minimum[row]),value=product+correction;
     expected[row]+=value;
    }
   }
   for(unsigned row=0;row<16;row++){
    const double nu=double(blocks+8)*0x1p-24,bound=1e-6+nu/(1-nu)*expanded[row];
    if(buffers[4096+group*16+row]!=Bits(expected[row]) || fabs(double(expected[row])-reference[row])>bound){
     fprintf(stderr,"BLOCK_INTEGER_ROW_MISMATCH K%u group%u row%u actual%08x expected%08x\n",inputs,group,row,buffers[4096+group*16+row],Bits(expected[row]));return false;
    }
   }
  }
  for(unsigned row=FfnPersistentPeer::groups*16;row<4096;row++)if(!UnusedRowValue(row,buffers[4096+row]))return false;
  return true;
 }
 void CheckEnd()override{
  InstructionPrefetchPeer::CheckEnd();if(!integerMode || type!=12)return;
  const unsigned blocks=exactJobs*FfnPersistentPeer::groups*(inputs/256);
  assert(integerMads==blocks*64 && integerCasts==blocks*64 && halfAndBaseStores==blocks*6);
 }
};

#ifndef PIONEER_Q38_BLOCK_INTEGER_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5 && (!strcmp(argv[4],"--pilot") || !strcmp(argv[4],"--all-shapes") || !strcmp(argv[4],"--controls")));setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned cases=0,stack=0,controls=0,faults=0,semantic=0;
 const bool pilot=!strcmp(argv[4],"--pilot"),controlOnly=!strcmp(argv[4],"--controls");
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  if(controlOnly || (pilot && (type!=12 || k!=5120 || groups!=1)))continue;
  printf("BLOCK_INTEGER_ACTUAL_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<BlockIntegerPeer>(argv[1],argv[2],type,k,groups,false);
  auto next=std::make_unique<BlockIntegerPeer>(argv[1],argv[3],type,k,groups);
  for(auto* p:{old.get(),next.get()}){p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed(true);p->CheckEnd();}
  for(unsigned job=1;job<=2;job++){
   assert(old->requests==next->requests && old->responses==next->responses && old->matrixInstructions==next->matrixInstructions);
   if(type!=12)for(unsigned row=0;row<groups*16;row++)assert(old->buffers[4096+row]==next->buffers[4096+row]);
   for(unsigned i=0;i<56;i++)assert(old->mailbox[i]==next->mailbox[i]);
   if(job==2)break;
   for(auto* p:{old.get(),next.get()}){
    std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p->buffers[4096+i];
    p->ReceivePreparedQueued();p->AwaitSecond();p->FinishSecond(first);stack=std::max(stack,0x70000-p->minStack);
   }
  }
  assert(old->prefetchEvents==18 && next->prefetchEvents==18);
  printf("PASS_BLOCK_INTEGER_ACTUAL_CASE type%u K%u groups%u changedJobs2 RV9f%u candidate%u integerMads%u casts%u halfBaseStores%u REAL_STOP;NEW_EXACT_ORACLE_NOT_OLD_K16;EMULATOR_COUNTS_NOT_TIME_OR_TPS\n",type,k,groups,old->steps,next->steps,next->integerMads,next->integerCasts,next->halfAndBaseStores);cases++;
 }
 if(controlOnly){
  for(unsigned nonzero:{0U,1U,239U,240U,256U}){
   auto p=std::make_unique<BlockIntegerPeer>(argv[1],argv[3],12,5120,3,true,13);
   for(unsigned t=0;t<p->tiles.size();t++)for(unsigned panel=0;panel<16;panel++)for(unsigned i=0;i<256;i++){
    const unsigned row=panel*16+i/16,q=(row+t*17)%256<nonzero?15:7;
    auto& byte=p->tiles[t][PreparedQ4Peer::Codec::OriginalAt(panel,i)];const unsigned shift=panel&2?4:0;
    byte=uint8_t((byte&~(15U<<shift))|(q<<shift));
   }
   p->PrepareSource(1);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->StopLocal();
   assert(p->prefetchEvents==18);stack=std::max(stack,0x70000-p->minStack);controls++;
   printf("PASS_BLOCK_INTEGER_FRAGMENT prefix13 groups3 nonzeroRows%u REAL_STOP_NEW_EXACT_ORACLE\n",nonzero);
  }
  for(unsigned type:{8U,12U,14U})for(unsigned which=0;which<(type==12?8U:4U);which++){
   auto p=std::make_unique<BlockIntegerPeer>(argv[1],argv[3],type,17408,3);
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   p->faultAt=which==0?1:which==1?p->inputReads+1:which==2?p->inputReads+2:which==3?unsigned(p->packets.size())
    :which==4?p->inputReads+3:which==5?p->inputReads+4:which==6?p->inputReads+5:p->inputReads+6;
   p->ReadFault();faults++;
  }
  for(unsigned field=0;field<16;field++){
   auto p=std::make_unique<BlockIntegerPeer>(argv[1],argv[3],12,5120,1);p->descriptor[field]^=0x80000001U;
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   while(p->pc!=p->faultPC && !p->heldPayload)p->StepBlock();
   assert(p->pc==p->faultPC && !p->heldPayload && !p->matrixInstructions && !p->pushes && !p->mailbox[0]);
   const auto memory=p->memory;const unsigned requests=p->requests,responses=p->responses;
   for(unsigned i=0;i<2048;i++)p->StepBlock();
   assert(p->pc==p->faultPC && p->memory==memory && p->requests==requests && p->responses==responses);semantic++;
  }
 }
 printf("PASS_BLOCK_INTEGER_COMPLETE_WORKER cases%u changedJobs%u maxCombinedStack%u/8192;actualROM_UNPACK_MVMUL_SFPU_sourcebanks_NoC_ACK_commit_STOP;SYNTHETIC_ENDPOINTS_NO_NATIVE_MODEL_QUALITY_OR_PHYSICAL_TPS\n",cases,cases*2,stack);
 if(controlOnly)printf("PASS_BLOCK_INTEGER_WORKER_CONTROLS fragmentedMasks%u NoCLossInert%u descriptorRefusals%u;NO_NATIVE_MODEL_HARDWARE_OR_TPS\n",controls,faults,semantic);
}
#endif
