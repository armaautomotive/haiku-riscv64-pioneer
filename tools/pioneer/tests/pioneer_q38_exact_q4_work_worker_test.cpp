/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"
// NEW observer only. All original NoC/descriptor/publisher/ACK/STOP paths
// execute through the unchanged strict RV decoder and synthetic fabric.
// Shared PHYSICAL Dst contains BOTH actual FPU and metadata/SFPU writes.
struct ExactWorkVector:MetadataMachine {
 void Op(uint32_t word){
  if(word==0x90000660){
   assert(okay);uint32_t out[32];
   for(unsigned lane=0;lane<32;lane++){
    const unsigned sm=Read(6,lane);assert((sm&0x7fffffff)<=14336 && ((sm&0x7fffffff) || !sm));
    out[lane]=Bits(float(int(sm&0x7fffffff)*(sm&0x80000000?-1:1)));
   }
   for(unsigned lane=0;lane<32;lane++)lanes[6][lane]=out[lane];ready[6]=cycle+1;cycle++;return;
  }
  if(word!=0x90000440){MetadataMachine::Op(word);return;}
  assert(okay);uint32_t out[32];
  for(unsigned lane=0;lane<32;lane++){
   const unsigned sm=Read(4,lane);assert((sm&0x7fffffff)<=2048);
   out[lane]=Bits(float(int(sm&0x7fffffff)*(sm&0x80000000?-1:1)));
  }
  for(unsigned lane=0;lane<32;lane++)lanes[4][lane]=out[lane];ready[4]=cycle+1;cycle++;
 }
};
struct ExactQ4WorkPeer:ConstantIssuePeer {
 struct Fault {unsigned line;};
 [[noreturn]] void Fail(unsigned line)const{
  fprintf(stderr,"EXACT_Q4_WORK_FAULT line%u pc%08x tick%u tasks%d fpu%d sfpu%d dirty%d matrix%u thread5%u A%u/%u B%u/%u\n",
   line,pc,tick,Tasks(),fpuPending,sfpuPending,dirty,matrixInstructions,thread[5],unpack[0],matrix[0],unpack[1],matrix[1]);
  throw Fault{line};
 }
 unsigned exactPC=0,exactReturn=0,exactJobs=0,nativeUnpacks=0,exactStores=0,tick=0;
 bool exact=false,dirty=false,fpuPending=false,sfpuPending=false,twoProducts=false;
 unsigned storeDue=0,unpack[2]={},matrix[2]={},partial[2]={},rwcA=0,rwcB=0,crA=0,crB=0;
 bool owned[2][2]={};float srcA[2][64][16]={},srcB[2][8][16]={};
 bool fpValid[1024]={};unsigned fpReady[1024]={};ExactWorkVector math;
 struct Task{bool active=false,flip=false,dst=false,zero=false;unsigned address=0,row=0,count=0,format=0,exponent=0;} tasks[2];
 ExactQ4WorkPeer(const char* old,const char* image,unsigned type,unsigned k,unsigned groups,unsigned worker=69):ConstantIssuePeer(old,image,worker,type,k,groups){
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string at,kind,name;
  while(symbols>>at>>kind>>name)if(name=="_ZL21PioneerQ38ExactQ4WorkPVKj"){assert(!exactPC);exactPC=unsigned(std::stoul(at,nullptr,16));}
  assert(exactPC);
 }
 unsigned MatrixPhasesPerPanel()const override{return type==12?(twoProducts?2:3):ConstantIssuePeer::MatrixPhasesPerPanel();}
 bool Tasks()const{return tasks[0].active || tasks[1].active;}
 bool MathIdle()const{return !Tasks() && !fpuPending && !sfpuPending;}
 bool BanksFree()const{return !owned[0][0] && !owned[0][1] && !owned[1][0] && !owned[1][1]
  && unpack[0]==matrix[0] && unpack[1]==matrix[1] && !partial[0] && !partial[1];}
 virtual void NativeOperand(const Task& t,unsigned w){
  if(w)assert(t.address==0x9d040);
  else assert((t.row==0 && t.address==0x9c040 && !t.flip && !partial[unpack[0]])
   || (t.row==16 && t.address==0x9c140 && !t.flip && partial[unpack[0]]==1)
   || (t.row==32 && t.address==0x9c240 && t.flip && partial[unpack[0]]==3));
  for(unsigned a=t.address-16;a<t.address;a+=4)assert(!memory.at(a));
 }
 static bool Scratch(unsigned a){return Range(a,0x9c030,144) || Range(a,0x9c130,144) || Range(a,0x9c230,80)
  || Range(a,0x9c340,32) || Range(a,0x9d030,272);}
 unsigned Byte(unsigned at)const{const unsigned word=memory.at(at&~3U);return (word>>((at&3)*8))&255;}
 void PutFloat(unsigned row,unsigned col,unsigned bits,unsigned due){
  StoreFloatDst(row,col,bits);const unsigned hi=Adj(row),lo=hi+8;
  defined[hi]=defined[lo]=fpValid[hi]=fpValid[lo]=true;fpReady[hi]=fpReady[lo]=due;
 }
 unsigned GetFloat(unsigned row,unsigned col)const{
  const unsigned hi=Adj(row),lo=hi+8;
  if(!fpValid[hi] || !fpValid[lo] || !wrote[hi][col] || !wrote[lo][col] || fpReady[hi]>tick || fpReady[lo]>tick)Fail(__LINE__);
  return FloatDst(row,col);
 }
 void CompleteMath(){
  for(unsigned w=0;w<2;w++)if(tasks[w].active){
   const auto t=tasks[w];
   if(t.dst){
    if(t.format==0){
     if(!t.zero || t.address!=0xa0000 || t.count!=256)Fail(__LINE__);
     for(unsigned i=0;i<t.count;i++){
      const unsigned raw=memory.at(t.address+i*4);assert(PioneerInt8OperandValid(uint16_t(raw)) && PioneerInt8OperandValid(uint16_t(raw>>16)));
      PutFloat(t.row+i/16,i%16,0,tick);
     }
    }else{
     if(t.format!=14 || t.address!=RawInputBase() || t.count!=256)Fail(__LINE__);
     for(unsigned i=0;i<t.count;i++){
      const unsigned row=t.row+i/16,col=i%16,byte=Byte(t.address+i);
      physical[row][col]=uint16_t((byte<<5)|(byte?16:0));wrote[row][col]=true;defined[row]=true;
     }
    }
   }else{
    for(unsigned i=0;i<t.count;i++){
     float value;
     if(w){value=Float((Byte(t.address+i*2)|(Byte(t.address+i*2+1)<<8))<<16);assert(value>=-128 && value<=127 && value==float(int(value)));srcB[unpack[w]][i/16][i%16]=value;}
     else{
      const unsigned bits=t.format==7?4:2,raw=(Byte(t.address+i*bits/8)>>((i*bits)%8))&((1U<<bits)-1);
      const unsigned magnitude=raw&((1U<<(bits-1))-1);assert(raw!=(1U<<(bits-1)));
      value=std::ldexp(float(raw>>(bits-1)?-int(magnitude):int(magnitude)),int(t.exponent)-127-int(bits-2));
      srcA[unpack[w]][t.row+i/16][i%16]=value;
     }
    }
    if(!w){partial[unpack[0]]|=1U<<(t.row/16);if(t.flip && partial[unpack[0]]!=(twoProducts?3U:7U))Fail(__LINE__);}
    if(t.flip){if(owned[w][unpack[w]])Fail(__LINE__);owned[w][unpack[w]]=true;unpack[w]^=1;}
   }
   tasks[w].active=false;
  }
  tick=std::max(tick,storeDue);for(unsigned r=0;r<8;r++)tick=std::max(tick,math.ready[r]);
  fpuPending=sfpuPending=false;rawPending=false;drained=true;drains++;
  ownerA=owned[0][matrix[0]];for(unsigned bank=0;bank<2;bank++)sources[bank]=owned[0][bank];
  unpackBank=unpack[0];mathBank=matrix[0];
 }
 void IssueUnpack(unsigned op){
  const unsigned w=(op>>23)&1,base=64+w*48;auto& t=tasks[w];
  if(t.active || fpuPending || sfpuPending)Fail(__LINE__);
  t={};t.active=true;t.flip=bool(op&64);t.address=(config[base+12]+1)*16;t.format=config[base]&15;t.count=adcX1[w]+1;t.exponent=config[50]&255;
  if(thread[5]==0){
   if(w || t.flip || owned[0][unpack[0]] || config[73]!=0x13)Fail(__LINE__);
   t.dst=true;t.zero=bool(op&16);t.row=(config[84]&65535)/16-4;
   assert((t.format==0 && config[1]==0x20200000 && config[65]==16 && config[72]==0)
    || (t.format==14 && config[1]==0xa03c8000 && config[65]==16 && config[72]==14 && t.row==0));
  }else{
   if(thread[5]!=4 || dirty || owned[w][unpack[w]])Fail(__LINE__);
   if(w){assert(t.format==5 && config[112]==0x00800015 && t.count==128 && t.flip);t.row=0;}
   else{
    t.row=(config[84]&65535)/16-4;
    const unsigned escapeRow=twoProducts?16:32;
    assert(t.count==256 && (t.row==0 || t.row==16 || (!twoProducts && t.row==32)) && t.format==(t.row==escapeRow?15U:7U));
    assert(config[65]==0x10001 && config[73]==0xf010f && config[50]==(t.row==escapeRow?0x17fU:0x181U));
   }
   NativeOperand(t,w);nativeUnpacks++;
  }
  rawPending=true;drained=false;
 }
 void IssueMatrix(unsigned op){
  assert(!Tasks() && !sfpuPending && config[0]==0x42b5 && config[1]==0x60aa0000 && !config[2] && !config[3]
   && thread[2]==1 && thread[3]==1 && !thread[11] && !thread[13] && !thread[48]);
  assert(op==0x26004020 || op==0x26c04020);
  const unsigned ab=matrix[0],bb=matrix[1],row=rwcA&0x38;assert(!rwcB && (row==0 || row==16 || (!twoProducts && row==32)));
  if(!owned[0][ab] || !owned[1][bb] || partial[ab]!=(twoProducts?3U:7U))Fail(__LINE__);
  for(unsigned r=0;r<8;r++)for(unsigned n=0;n<16;n++){
   int dot=0;for(unsigned k=0;k<16;k++){
    const float a=srcA[ab][row+k][n],b=srcB[bb][r][k];assert(a==float(int(a)) && b==float(int(b)) && a>=-7 && a<=7);
    dot+=int(a)*int(b);
   }
   const float old=valid[32+r]?Float(GetFloat(32+r,n)):0;
   PutFloat(32+r,n,Bits(old+float(dot)),tick);
  }
  for(unsigned r=32;r<40;r++)valid[r]=true;
  if(op==0x26c04020){owned[0][ab]=owned[1][bb]=false;partial[ab]=0;matrix[0]^=1;matrix[1]^=1;}
  ownerA=owned[0][matrix[0]];for(unsigned bank=0;bank<2;bank++)sources[bank]=owned[0][bank];mathBank=matrix[0];
  matrixInstructions++;fpuPending=true;drained=false;
 }
 void IssueSfpu(unsigned op){
  if(Tasks() || fpuPending)Fail(__LINE__);const unsigned code=op>>24;math.cycle=tick;
  if(op==0x8a000002){enabled=true;return;}if(!enabled)Fail(__LINE__);
  if(code==0x70 || code==0x72){
   const unsigned mode=(op>>16)&15,reg=(op>>20)&15,at=op&1023;assert(reg<8 && !(at&1) && !(op&0xfc00));
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+bool(at&2);
    if(code==0x70){
     if(mode==13){assert(row<16 && defined[row] && wrote[row][col]);math.lanes[reg][lane]=BlackholeRawSfpLoad(physical[row][col],13);}
     else{assert(mode==3);math.lanes[reg][lane]=GetFloat(row,col);}
    }else{
     assert(mode==3 && ((row>=128 && row<192) || (row>=108 && row<112)));
     if(!fpValid[Adj(row)] || !fpValid[Adj(row)+8])Fail(__LINE__);
     PutFloat(row,col,math.Read(reg,lane),tick+1);
    }
   }
   if(code==0x70)math.ready[reg]=tick+1;else{storeDue=tick+1;sfpuPending=true;exactStores++;}
  }else if(code==0x8c){
   assert(op==0x8c000000);unsigned old[8][32];for(unsigned r=0;r<8;r++)for(unsigned lane=0;lane<32;lane++)old[r][lane]=math.Read(r,lane);
   for(unsigned b:{0U,4U})for(unsigned r=0;r<4;r++)for(unsigned g=0;g<4;g++)for(unsigned c=0;c<8;c++)math.lanes[b+r][g*8+c]=old[b+g][r*8+c];
   for(auto& ready:math.ready)ready=tick+1;
  }else math.Op(op);
  if(!math.okay)Fail(__LINE__);memcpy(lr,math.lanes,sizeof(lr));vectorOps++;drained=false;
 }
 void Instruction(unsigned op)override{
  if(!exact){ConstantIssuePeer::Instruction(op);return;}
  tick++;
  if(pending){if(!DisjointPrefetchCompute())Fail(__LINE__);overlapOps++;}
  if(op==0xa2101816){assert(!gpr63 && !backendWait);backendWait=true;return;}
  if(op==0x4500017e){assert(!gpr63 && backendWait);CompleteMath();backendWait=false;gpr63=1;return;}
  const unsigned code=op>>24;
  if(code==0xb2){if(!MathIdle())Fail(__LINE__);const unsigned i=(op>>16)&255;assert(i<68);thread[i]=op&65535;return;}
  if(code==0x37){
   const unsigned mask=op&63,cr=(op>>18)&15;assert(!(mask&~15U) && !(cr&~3U));
   if(mask&1){rwcA=((op>>6)&15)+((cr&1)?crA:0);crA=rwcA;}
   if(mask&2){rwcB=((op>>10)&15)+((cr&2)?crB:0);crB=rwcB;}
   return;
  }
  if(code==0x10){
   if(!MathIdle())Fail(__LINE__);assert(op==0x10080004 || op==0x10080005);
   for(unsigned row=op==0x10080004?64:80;row<(op==0x10080004?80U:96U);row++)defined[row]=fpValid[row]=false;
   for(unsigned row=32;row<48;row++)valid[row]=false;return;
  }
  if(code==0x5e){const unsigned select=(op>>21)&7;assert(select && !(select&4));for(unsigned w=0;w<2;w++)if(select&(1U<<w)){assert(!(op&1023));adcX0[w]=0;adcX1[w]=(op>>10)&2047;}return;}
  if(op==0x5120000a || op==0x5420000f || op==0x5160000b || op==0x5460000f)return;
  if(code==0x42){IssueUnpack(op);return;}
  if(code==0x26){IssueMatrix(op);return;}
  if((code>=0x70 && code<=0x90)){IssueSfpu(op);return;}
  fprintf(stderr,"UNKNOWN_EXACT_WORK_TENSIX %08x\n",op);assert(false);
 }
 void StepBlock()override{
  if(pc==exactPC){assert(type==12 && !exact && !exactReturn && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && BanksFree());exact=true;exactReturn=regs[1];memcpy(math.lanes,lr,sizeof(lr));}
  // Index fetches ALL real code sections (including service and text),
  // whereas Rom only covers the additional worker wrappers/fetch helper.
  // Observe the actual retired RV fence;never fabricate device completion.
  if(exact && (l1[Index(pc)]&127)==15)dirty=false;
  ConstantIssuePeer::StepBlock();
  if(exact && pc==exactReturn){assert(MathIdle() && BanksFree() && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && drained);exact=false;exactReturn=0;exactJobs++;}
 }
 unsigned Load(unsigned a)override{
  if(exact && Range(a,0xffef0000,224*4)){if(!MathIdle())Fail(__LINE__);return config[(a-0xffef0000)/4];}
  if(exact && Scratch(a)){assert(!(a&3));return memory.at(a);}
  if(exact && Range(a,0xffbd9b00,64)){if(!MathIdle() || config[3])Fail(__LINE__);outputReads++;return GetFloat(108,(a%64)/4);}
  return ConstantIssuePeer::Load(a);
 }
 void Store(unsigned a,unsigned value)override{
  if(exact && Range(a,0xffef0000,224*4)){if(!MathIdle())Fail(__LINE__);config[(a-0xffef0000)/4]=value;configWrites++;return;}
  if(exact && Scratch(a)){if(!MathIdle())Fail(__LINE__);assert(!(a&3));memory[a]=value;dirty=true;return;}
  if(exact && Range(a,0xffb20000,68) && !MathIdle())Fail(__LINE__);
  ConstantIssuePeer::Store(a,value);
 }
};
#ifndef PIONEER_Q38_EXACT_Q4_WORK_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--pilot") || !strcmp(argv[4],"--faults-only"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 const bool pilot=argc==5 && !strcmp(argv[4],"--pilot"),faultsOnly=argc==5 && !strcmp(argv[4],"--faults-only");
 unsigned cases=0,stack=0,faults=0,finite=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  if(faultsOnly || (pilot && (type!=12 || k!=5120 || groups!=1)))continue;
  printf("EXACT_Q4_WORK_ACTUAL_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<VectorMetadataPeer>(argv[1],argv[2],type,k,groups);
  auto next=std::make_unique<ExactQ4WorkPeer>(argv[1],argv[3],type,k,groups);
  for(unsigned side=0;side<2;side++){
   auto& p=side?static_cast<ConstantIssuePeer&>(*next):static_cast<ConstantIssuePeer&>(*old);
   p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
  }
  for(unsigned job=1;job<=2;job++){
   assert(old->requests==next->requests && old->responses==next->responses
    && old->sumReads==next->sumReads && old->sumWrites==next->sumWrites
    && !next->exact && !next->dotProbeReads && !next->blockImports);
   assert(next->exactJobs==(type==12?job:0));
   for(unsigned r=0;r<groups*16;r++)assert(old->buffers[4096+r]==next->buffers[4096+r]);
   for(unsigned i=0;i<64;i++)assert(old->mailbox[i]==next->mailbox[i]);
   if(job==2)break;
   for(unsigned side=0;side<2;side++){
    auto& p=side?static_cast<ConstantIssuePeer&>(*next):static_cast<ConstantIssuePeer&>(*old);
    std::array<uint32_t,16> first;for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
    p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first);stack=std::max(stack,0x70000-p.minStack);
   }
  }
  cases++;
  printf("PASS_EXACT_Q4_WORK_ACTUAL_CASE type%u K%u groups%u changedInputJobs2 nativeUnpacks%u exactStores%u RV%u/%u;NOT_SILICON_TPS\n",
   type,k,groups,next->nativeUnpacks,next->exactStores,old->steps,next->steps);
 }
 if(!pilot){
  for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}){
   auto p=std::make_unique<ExactQ4WorkPeer>(argv[1],argv[3],type,5120,3);
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   p->faultAt=which==0?1:which==1?p->inputReads+1
    :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
   p->ReadFault();faults++;
  }
  for(unsigned kind=0;kind<2;kind++)for(unsigned word=0;word<4;word++){
   auto p=std::make_unique<ExactQ4WorkPeer>(argv[1],argv[3],12,5120,1);
   p->tiles[0][kind*32+16+word*4+3]|=0x7c;
   p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
   while(p->pc!=p->faultPC && !p->heldPayload)p->StepBlock();
   assert(p->pc==p->faultPC && !p->heldPayload && !p->matrixInstructions && !p->pushes && !p->mailbox[0]);
   auto memory=p->memory;unsigned issued=p->requests,completed=p->responses;
   for(unsigned i=0;i<2048;i++)p->StepBlock();
   assert(p->pc==p->faultPC && p->memory==memory && p->requests==issued && p->responses==completed && !p->pushes);finite++;
  }
 }
 printf("%s_EXACT_Q4_WORK_ACTUAL_RETURNING_IMAGE shapes%u changedInputJobs%u NoCLossInert%u nonfiniteRefusals%u stack%u/8192;"
  "originalFP32Outputs sourceNoCPublisherACK retainedPhysicalState realStopCases%u;NO_NATIVE_HARDWARE_MODEL_OR_TPS\n",
  pilot || faultsOnly?"PARTIAL":"PASS",cases,cases*2,faults,finite,stack,cases);
}
#endif
