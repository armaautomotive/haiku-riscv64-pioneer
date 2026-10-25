/* SPDX-License-Identifier: MIT */
// ACTUAL emitted matrix/Conv/GDN instructions on ONE shared hardware model.
// SYNTHETIC byte endpoints only,not native/model-token/silicon throughput.
#define PIONEER_Q38_RESIDENT_OPERATOR_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_operator_service_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnServiceShape.h"
struct ResidentGdnOperatorPeer:ResidentOperatorPeer {
 std::vector<uint8_t> gdnCode;
 ResidentGdnOperatorPeer* peer=nullptr;
 std::array<uint32_t,64> input={},state={},gated={};
 std::array<uint32_t,324> raw={};std::array<std::array<uint32_t,16>,4> learned={};
 std::array<uint32_t,8192> prior={},next={};std::array<uint32_t,128> norm={},z={};
 std::array<uint32_t,64> gatedOutput={};uint32_t gdnBefore[8]={};
 bool gdnActive=false,gdnWaiting=false;
 unsigned gdnPackets=0,gdnPhase=0,gdnPolls=0,gdnImports=0,gdnOps=0,gdnLatency=0;
 unsigned gdnPendingWords=0,gdnPendingSource=0,gdnPendingRow=0,gdnX0=0,gdnX1=0;
 unsigned gdnNextWrites=0,gdnOutputWrites=0,gdnGatedWrites=0,gdnCsrReads=0;
 unsigned gdnFailPacket=0,gdnSourcePacket=0,gdnCounterPacket=0,gdnDescriptorPacket=0;
 unsigned gdnDescriptorBase=TT_Q38_GDN_INPUT_DESCRIPTOR,gdnDescriptorWord=40,gdnFailGatedPhase=0;
 bool dropMatrixConfig1=false;unsigned droppedMatrixConfig1=0;
 bool gdnLateJob=false,gdnLateBoot=false,gdnLateDoorbell=false,gdnNeverPeer=false;
 unsigned clockReads=0,clockFault=0;uint64_t clockEpoch=UINT64_C(1)<<40,clockTime=0,clockGap=1000000;
 ResidentGdnOperatorPeer(const char* path,unsigned worker=1,unsigned type=12)
  :ResidentOperatorPeer(path,worker,type),gdnCode(Read((std::string(path)+"/gdn.bin").c_str())){
  assert(!gdnCode.empty() && !(gdnCode.size()&3) && gdnCode.size()<=0x10000);
 }
 unsigned Index(uint32_t at)override{
  if(Range(at,0xb8000,unsigned(gdnCode.size()))){assert(!(at&3));l1[8189]=Word(gdnCode,at-0xb8000);return 8189;}
  return ResidentOperatorPeer::Index(at);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0xb8000,unsigned(gdnCode.size())) || ResidentOperatorPeer::CodeAddress(at);}
 // No second detached tensor simulator:the SAME config/thread/LReg/Dst/
 // row-valid/drain arrays used by matrix execution are read/written here.
 void Instruction(uint32_t op)override{
  if(!gdnActive){ResidentOperatorPeer::Instruction(op);return;}
  assert(!rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !recordRemaining);
  unsigned code=op>>24;
  if(code==0xb2){assert(!gdnPendingWords);unsigned at=(op>>16)&255;assert(at<68);thread[at]=op&65535;return;}
  if(op==0xa2101816){assert(!gdnWaiting && !gpr63);gdnWaiting=true;backendWait=true;return;}
  if(op==0x4500017e){
   assert(gdnWaiting && backendWait && !gpr63 && !gdnLatency);gdnWaiting=backendWait=false;
   if(gdnPendingWords){
    for(unsigned i=0;i<gdnPendingWords;i++)StoreFloatDst(gdnPendingRow+i/16,i%16,memory.at(gdnPendingSource+i*4));
    for(unsigned r=gdnPendingRow;r<gdnPendingRow+gdnPendingWords/16;r++)defined[Adj(r)]=true;
    gdnPendingWords=0;
   }
   drained=true;drains++;gpr63=1;return;
  }
  if((op&0xfff00000)==0x5e200000){assert(!gdnPendingWords);gdnX0=op&1023;gdnX1=(op>>10)&1023;return;}
  if(op==0x5120000a || op==0x5420000f){assert(!gdnPendingWords);return;}
  if(op==0x42000081){
   assert(!gdnPendingWords && !thread[5] && config[0]==0x4231 && config[1]==0xa0200000
    && !config[3] && !config[49] && config[64]==0x100010 && config[65]==16 && !config[72] && config[73]==0x13 && !gdnX0);
   unsigned sourceAt=(config[76]+1)*16,row=(config[84]&65535)/16-4,words=gdnX1+1;
   assert((config[84]&0xffff0000)==0x400000 && row<512 && !(words%16) && words<=8192-row*16);
   assert((!gdnImports && sourceAt==0x8e400 && !row && words==64)
    || (gdnImports && gdnImports<9 && sourceAt==0x90000+(gdnImports-1)*4096 && row==(gdnImports-1)*64 && words==1024));
   gdnImports++;gdnPendingSource=sourceAt;gdnPendingRow=row;gdnPendingWords=words;drained=false;
   for(unsigned r=row;r<row+words/16;r++)defined[Adj(r)]=false; // real new UNPACR ownership,not a test reset
   return;
  }
  assert(!gdnPendingWords && thread[2]==1 && thread[3]==1 && thread[5]==4 && config[0]==0x4231 && config[1]==0xa0200000);
  gdnOps++;drained=false;
  if(op==0x8f000000){assert(gdnLatency==1);gdnLatency=0;return;}
  assert(!gdnLatency);
  if(op==0x8a000002){enabled=true;return;}
  assert(enabled);
  if(code==0x70 || code==0x72){
   unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
   assert(mode==3 && reg<8 && !(at&1) && at<512 && !(op&0xfc00));
   if(code==0x70 && reg)assert(gdnImports==1 && (reg==6 || reg==7) && !(op&0x3fc));
   if(code==0x72 && reg)assert(gdnNextWrites==8192 && (reg==4 || reg==5) && !(op&0x3fc));
   for(unsigned lane=0;lane<32;lane++){
    unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+((at&2)!=0);assert(row<512 && col<16 && defined[Adj(row)]);
    if(code==0x70)lr[reg][lane]=FloatDst(row,col);
    else {uint32_t bits=lr[reg][lane];StoreFloatDst(row,col,bits&0x7f800000?bits:bits&0x80000000);}
   }return;
  }
  for(unsigned lane=0;lane<32;lane++){
   if(code==0x71){unsigned r=(op>>20)&15,mode=(op>>16)&15;assert(r<8);
    if(!mode)lr[r][lane]=(op&65535)<<16;else if(mode==10)lr[r][lane]=(lr[r][lane]&0xffff0000)|(op&65535);
    else {assert(mode==2);lr[r][lane]=op&65535;}
   }else if(code==0x8d){unsigned s=(op>>8)&15,d=(op>>4)&15;assert(s<8 && d<8 && !(op&15));lr[d][lane]^=lr[s][lane];
   }else {
    assert(code==0x85 || code==0x86);unsigned a=code==0x86?(op>>16)&15:10,b=(op>>12)&15,c=code==0x86?9:(op>>8)&15,d=(op>>4)&15;
    assert(d<8 && b<8 && ((code==0x86 && a<8 && (op&15)==2) || (code==0x85 && c<8 && !(op&15))));
    lr[d][lane]=PioneerBlackholeFmaBits(a==10?0x3f800000:lr[a][lane],lr[b][lane],c==9?0x80000000:lr[c][lane]);
   }
  }
  if(code==0x85 || code==0x86)gdnLatency=1;
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(gdnActive && (csr==0xb00 || csr==0xb80));unsigned at=clockReads++;
  if(at%3==0)clockTime=clockEpoch+uint64_t(steps)*19+uint64_t(at/3)*clockGap;
  uint64_t now=clockTime;if(clockFault==2 && at>=3)now-=UINT64_C(1)<<39;if(clockFault==3)now=clockEpoch;
  uint32_t value=csr==0xb00?uint32_t(now):uint32_t(now>>32);if(clockFault==1 && csr==0xb80)value+=at;
  return value;
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0xb8000,unsigned(gdnCode.size())))return Word(gdnCode,at-0xb8000);
  if(gdnActive){
   if(Range(at,0xffb20000,0x2000)){
    nocReads++;unsigned offset=at-0xffb20000;
    for(unsigned i=0;i<8;i++)if(offset==CounterOffset(i))return Count(i);
    for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return source[i];
    if(offset<40){commandReads++;return command[offset/4];}
    for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
    if(offset>=0x240 && offset<0x2c0)return 0;
    assert(false && "unqualified GDN service NoC read");
   }
   if(at==0xffe000fc)return gpr63;
   if(Range(at,0xffef0000,224*4))return config[(at-0xffef0000)/4];
   if(Range(at,0xffbd8000,32768)){
    assert(drained && !gdnPendingWords && !gdnLatency && !config[3] && gdnImports==9);gdnCsrReads++;
    unsigned word=(at-0xffbd8000)/4;return FloatDst(word/16,word%16);
   }
   if(Range(at,0x74000,0x1000) || Range(at,0x80000,0x1100) || Range(at,0x83f00,256)
    || Range(at,0x8e000,0xb00) || Range(at,0x90000,32768) || Range(at,0xa0000,32768))return memory.at(at);
  }
  return ResidentOperatorPeer::Load(at);
 }
 void SubmitGdn(){
  assert(gdnActive && !pending);unsigned p=gdnPackets,local=0,remote=0,route=0,bytes=0;bool write=false;
  if(p<7){
   const unsigned l[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},n[]={512,512,256,64,64,64,64};
   const unsigned r[]={input[4]+input[42],input[4]+input[43],input[4]+input[44],input[4]+input[45],input[4]+input[46],input[12],input[15]};
   const unsigned xy[]={input[8],input[8],input[8],input[8],input[8],input[13],input[41]};local=l[p];remote=r[p];route=xy[p];bytes=n[p];
  }else if(p<11){
   write=p>=9;unsigned piece=(p-7)%2;local=(write?0xa0000:0x90000)+piece*16384;
   remote=state[write?7:5]+piece*16384;route=state[write?13:12];bytes=16384;
  }else {
   assert(peer && gdnPhase<5 && gdnImports==9 && gdnNextWrites==8192 && gdnOutputWrites==64 && drained);
   unsigned phase=gdnPhase;write=phase==4;const unsigned l[]={TT_Q38_GDN_GATED_PEER_READY,0x80000+((gated[7]^1)&1)*256,0x80200,0x80400,0x8ea00};
   const unsigned r[]={gated[44],gated[45],gated[12],gated[4]+gated[42],gated[4]+gated[43]},xy[]={gated[15],gated[15],gated[13],gated[8],gated[8]},n[]={64,256,512,512,256};
   local=l[phase];remote=r[phase];route=xy[phase];bytes=n[phase];
  }
  const uint32_t expected[]={write?local:remote,0,write?selfRoute:route,write?remote:local,0,write?route:selfRoute,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  gdnPackets++;if(gdnPackets==gdnFailPacket || (p>=11 && gdnPhase+1==gdnFailGatedPhase)){pending=true;return;}
  if(p<7){for(unsigned i=0;i<bytes/4;i++){
    assert(memory.at(local+i*4)==0x7fc60000+p*1024+i);
    memory[local+i*4]=p<3?raw[(p==0?0:p==1?128:256)+i]:learned[p-3][i];
   }
  }else if(p<11){
   unsigned piece=(p-7)%2;
   for(unsigned i=0;i<4096;i++){if(write)next[piece*4096+i]=memory.at(local+i*4);else memory[local+i*4]=prior[piece*4096+i];}
  }else {
   for(unsigned i=0;i<bytes/4;i++){
    if(write)gatedOutput[i]=memory.at(local+i*4);
    else {
     assert(memory.at(local+i*4)==0x7fc70000+gdnPhase*1024+i);
     memory[local+i*4]=!gdnPhase?(gdnNeverPeer?0:peer->memory.at(TT_Q38_GDN_GATED_READY+i*4)):gdnPhase==1?peer->memory.at(0x8e800+i*4):gdnPhase==2?norm[i]:z[i];
    }
   }
   if(!gdnPhase){gdnPolls++;if(memory.at(TT_Q38_GDN_GATED_PEER_READY))gdnPhase=1;}else gdnPhase++;
  }
  unsigned base=write?4:0;countBase[base]++;countBase[base+1]+=(bytes+63)/64;countBase[base+3]++;countBase[2]++;countBase[6]++;
  if(gdnPackets==gdnCounterPacket)countBase[write?0:4]++;
  if(gdnPackets==gdnSourcePacket)source[11]^=1;
  if(gdnPackets==gdnDescriptorPacket)memory[gdnDescriptorBase+gdnDescriptorWord*4]^=1;
  if(gdnPhase==5){if(gdnLateJob)memory[0x74000+4]^=1;if(gdnLateBoot)memory[0x74040+4]^=1;if(gdnLateDoorbell)memory[0x74100]++;}
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!Range(at,0xb8000,unsigned(gdnCode.size())));
  if(!gdnActive && dropMatrixConfig1 && at==0xffef0004){droppedMatrixConfig1++;return;}
  if(gdnActive){
   if(Range(at,0xffb20000,40)){assert(!pending && !(at&3));command[(at-0xffb20000)/4]=value;return;}
   if(at==0xffb20040){assert(value==1 && !pending);SubmitGdn();return;}
   if(at==0xffe000fc){assert(!value);gpr63=0;return;}
   if(at==0xffe40000){Instruction(value);return;}
   if(Range(at,0xffef0000,224*4)){
    assert(drained && !gdnPendingWords && !gdnLatency);unsigned index=(at-0xffef0000)/4;
    const unsigned allowed[]={0,1,2,3,6,7,220,20,24,28,49,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
    bool found=false;for(unsigned i:allowed)found|=index==i;assert(found);config[index]=value;configWrites++;return;
   }
   if(Range(at,0xa0000,32768)){assert(drained && gdnImports==9 && gdnCsrReads==gdnNextWrites+1);gdnNextWrites++;memory[at]=value;return;}
   if(Range(at,0x8e800,256)){assert(drained && gdnNextWrites==8192);gdnOutputWrites++;memory[at]=value;return;}
   if(Range(at,0x8ea00,256)){assert(gdnPhase==4 && !pending);gdnGatedWrites++;memory[at]=value;return;}
   if(Range(at,0x74140,64) || Range(at,0x83f00,256)){
    assert(!pending || (at==0x74148 && value==TT_Q38_FFN_PERSISTENT_FAULT));memory[at]=value;return;
   }
   if(Range(at,0x74200,384) || Range(at,0x74600,128) || Range(at,0x74700,16)
    || Range(at,0x74900,128) || Range(at,0x74b00,0x240) || Range(at,0x80000,0x1100) || Range(at,0x8e000,1288)){
    assert(!pending);memory[at]=value;return;
   }
   assert(!Range(at,0x74000,64) && !Range(at,0x74040,32)); // persistent job/boot MUST stay immutable
  }
  ResidentOperatorPeer::Store(at,value);
 }
 void StepBlock()override{
  if(!Range(pc,0xb8000,unsigned(gdnCode.size()))){ResidentOperatorPeer::StepBlock();return;}
  uint32_t i=Word(gdnCode,pc-0xb8000),op=i&127;
  if((i&0x707f)==0x23){
   assert(++steps<StepLimit());unsigned a=(i>>15)&31,b=(i>>20)&31;
   uint32_t at=regs[a]+Sign(((i>>25)<<5)|((i>>7)&31),12);assert(Range(at,0x6e000,8192));
   uint32_t aligned=at&~3U,shift=(at&3)*8;memory[aligned]=(memory[aligned]&~(255U<<shift))|((regs[b]&255)<<shift);
   minStack=std::min(minStack,aligned);pc+=4;return;
  }
  if(op!=7 && op!=0x27 && op!=0x53){ResidentOperatorPeer::StepBlock();return;}
  assert(++steps<StepLimit());unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else {float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));switch(hi){
   case 0:assert(f==7 || !f);fp[d]=Bits(Flush(a+b));break;case 4:assert(f==7 || !f);fp[d]=Bits(Flush(a-b));break;
   case 8:assert(f==7 || !f);fp[d]=Bits(Flush(a*b));break;
   case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
   case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
   case 0x68:assert(t<=1);fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
   case 0x70:assert(!t && !f);if(d)regs[d]=fp[s];break;case 0x78:assert(!t && !f);fp[d]=regs[s];break;
   default:fprintf(stderr,"UNQUALIFIED GDN service FPU instruction%08x PC%08x\n",i,pc);assert(false);
  }}regs[0]=0;pc+=4;
 }
 void StageGdn(unsigned sequence,unsigned epoch=17,unsigned layer=0,unsigned bank=0,unsigned position=0,unsigned slot=0,unsigned trial=1){
  assert(logicalWorker<=96 && !pending && !gdnLatency && !gdnPendingWords && !gdnWaiting);ClearTensorState();
  gdnActive=activation=true;convActive=false;gdnPackets=gdnPhase=gdnPolls=gdnImports=gdnOps=clockReads=0;
  gdnNextWrites=gdnOutputWrites=gdnGatedWrites=gdnCsrReads=0;
  auto plan=std::make_unique<PioneerQ38GdnInputPlan>();PioneerQ38GdnIdentity id={1,2,3,4};unsigned half=logicalWorker-1;
  assert(plan->Build(*canonical,layer,half,position,bank,id,source,epoch));
  for(unsigned i=0;i<64;i++)input[i]=plan->Descriptor()[i];state={};
  state[0]=TT_Q38_RGDN_MARKER;state[1]=1;state[2]=128;state[3]=64;state[8]=epoch;state[9]=half;state[10]=layer;state[11]=slot;
  state[14]=32768;state[15]=selfRoute;state[40]=position;for(unsigned i=16;i<40;i++)state[i]=input[i];
  for(unsigned side=0;side<2;side++){
   bool found=false;for(unsigned i=0;i<canonical->RecurrentCount();i++){
    const auto& r=canonical->RecurrentRegions()[i];unsigned h=half/2;
    if(r.layer!=layer || r.slot!=(slot^side) || h<r.firstHead || h-r.firstHead>=r.heads)continue;
    state[4+side*2]=r.bank;state[5+side*2]=canonical->Base+r.offset+(h-r.firstHead)*65536+(half&1)*32768;
    state[12+side]=PioneerQ38SpanRoute(r.bank);found=true;break;
   }assert(found);
  }
  auto paired=std::make_unique<PioneerQ38GdnRendezvousPlan>();assert(paired->Build(*canonical,input.data(),state.data()));
  for(unsigned i=0;i<64;i++){
   gated[i]=paired->Descriptor()[i];memory[TT_Q38_GDN_INPUT_DESCRIPTOR+i*4]=input[i];memory[TT_Q38_RGDN_DESCRIPTOR+i*4]=state[i];memory[TT_Q38_GDN_GATED_DESCRIPTOR+i*4]=gated[i];
  }
  unsigned h=half/2,kh=h%16;
  for(unsigned i=0;i<128;i++){
   raw[i]=Bits(float(sin(kh*.117+i*.071+trial*.03)*2.7));raw[128+i]=Bits(float(cos(kh*.213+i*.091-trial*.02)*1.9));
   norm[i]=Bits(float(.5+sin(i*.07)*.3));z[i]=Bits(float(cos((h*128+i)*.017+trial*.11)*5));
  }
  for(unsigned i=0;i<64;i++)raw[256+i]=Bits(float(sin((h*128+(half&1)*64+i)*.035+trial*.13)));
  raw[320]=Bits(float(h)*.13f-2);raw[321]=Bits(float(h)*.07f-2);raw[322]=Bits(-float(h+1)*.25f);raw[323]=Bits(-3.5f);
  for(unsigned p=0;p<4;p++)for(unsigned i=0;i<16;i++)learned[p][i]=Bits(p==2?-float(i+1)*.25f:float(i+1)*.01f);
  for(unsigned p=0;p<4;p++)learned[p][input[47]]=raw[320+p];
  // Poison ONLY new operand/output scratch,never tensor registers,Dst,
  // configuration or the byte endpoints containing canonical prior state.
  for(unsigned i=0;i<384;i++)memory[0x80000+i*4]=0x7fc00000;
  for(unsigned i=0;i<322;i++)memory[0x8e000+i*4]=0x7fc00000;
  for(unsigned i=0;i<8192;i++)memory[0xa0000+i*4]=0x7fc00000;
  for(unsigned i=0;i<64;i++)memory[0x8e800+i*4]=memory[0x8ea00+i*4]=0x7fc00000;
  const uint32_t job[]={TT_Q38_RESIDENT_GDN_SERVICE_JOB,64,768,epoch,0,0,0,0,0,0,TT_Q38_FFN_PERSISTENT_MARKER,logicalWorker,sequence,8,0,0};
  for(unsigned i=0;i<16;i++){memory[0x74000+i*4]=job[i];memory[TT_Q38_GDN_GATED_READY+i*4]=0;memory[TT_Q38_GDN_GATED_PEER_READY+i*4]=0x7fc00000;memory[TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4]=0;}
  for(unsigned i=0;i<8;i++)gdnBefore[i]=Count(i);memory[0x74100]=sequence;
 }
 bool GdnFinished(unsigned sequence)const{return pc==TT_Q38_PEER_WAIT_LOAD && memory.at(0x74144)==sequence;}
 bool GdnFaulted()const{return pc==TT_Q38_PEER_FAULT_PARK;}
 void VerifyGdn(unsigned sequence){
  assert(GdnFinished(sequence) && !pending && gdnPhase==5 && gdnPolls && gdnImports==9 && gdnNextWrites==8192
   && gdnOutputWrites==64 && gdnGatedWrites==64 && gdnCsrReads==8256 && drained && !gdnLatency && !gdnPendingWords && peer);
  assert(memory.at(0x74700)==0x56383351 && memory.at(0x74000)==TT_Q38_RESIDENT_GDN_SERVICE_JOB);
  float qs=0,ks=0;for(unsigned i=0;i<128;i++){float q=Float(raw[i]),k=Float(raw[128+i]);qs+=q*q;ks+=k*k;}
  float qi=1.f/sqrtf(qs+1e-6f),ki=1.f/sqrtf(ks+1e-6f);
  auto close=[](uint32_t bits,float expected){return isfinite(Float(bits)) && fabs(double(Float(bits))-expected)<=3e-6*(1+fabs(double(expected)));};
  for(unsigned i=0;i<128;i++){assert(close(memory.at(0x8e000+i*4),Float(raw[i])*qi));assert(close(memory.at(0x8e200+i*4),Float(raw[128+i])*ki));}
  float decay=Float(memory.at(0x8e500)),beta=Float(memory.at(0x8e504));
  double x=Float(raw[320])+Float(raw[323]);float softplus=float(x>0?x+log1p(exp(-x)):log1p(exp(x)));
  assert(close(Bits(decay),expf(Float(raw[322])*softplus)));double b=Float(raw[321]),e=exp(-fabs(b));assert(close(Bits(beta),float(b>=0?1/(1+e):e/(1+e))));
  const float expectedPositive[]={expf(Float(raw[322])*softplus),float(b>=0?1/(1+e):e/(1+e))},actualPositive[]={decay,beta};
  for(unsigned i=0;i<2;i++)if(expectedPositive[i]>=Float(0x00800000))assert(actualPositive[i]>0 && fabs(double(actualPositive[i])-expectedPositive[i])<=3e-6*expectedPositive[i]);else assert(actualPositive[i]==0);
  for(unsigned c=0;c<64;c++){
   float predicted=0;for(unsigned k=0;k<128;k++)predicted+=(Float(prior[k*64+c])*decay)*Float(memory.at(0x8e200+k*4));
   float delta=(Float(raw[256+c])-predicted)*beta,out=0;
   for(unsigned k=0;k<128;k++){
    float expected=Float(prior[k*64+c])*decay+Float(memory.at(0x8e200+k*4))*delta;
    assert(next[k*64+c]==Bits(expected) && memory.at(0x90000+(k*64+c)*4)==prior[k*64+c]);
    out+=expected*Float(memory.at(0x8e000+k*4));
   }out*=1.f/sqrtf(128.f);assert(memory.at(0x8e800+c*4)==Bits(out));
  }
  float sum=0,all[128];for(unsigned i=0;i<128;i++){auto* one=i/64==(state[9]&1)?this:peer;all[i]=Float(one->memory.at(0x8e800+i%64*4));sum+=all[i]*all[i];}
  float inverse=1.f/sqrtf(sum/128+1e-6f);
  for(unsigned i=0;i<64;i++){unsigned at=(state[9]&1)*64+i;float weighted=(all[at]*inverse)*Float(norm[at]);double expected=double(weighted)*Float(z[at])/(1+exp(-double(Float(z[at]))));
   assert(isfinite(Float(gatedOutput[i])) && fabs(double(Float(gatedOutput[i]))-expected)<=3e-6*(1+fabs(expected)));
   if(fabs(expected)>=Float(0x00800000))assert(fabs(double(Float(gatedOutput[i]))-expected)<=3e-6*fabs(expected));else assert(Float(gatedOutput[i])==0);}
  uint32_t proof[64],job[16],after[8],first[32],recurrent[32],summary[32],clock[16];
  for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);for(unsigned i=0;i<16;i++){job[i]=memory.at(0x74000+i*4);clock[i]=memory.at(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=Count(i);for(unsigned i=0;i<32;i++){first[i]=memory.at(TT_Q38_GDN_INPUT_SUMMARY+i*4);recurrent[i]=memory.at(TT_Q38_RGDN_SUMMARY+i*4);summary[i]=memory.at(TT_Q38_GDN_GATED_SUMMARY+i*4);}
  assert(PioneerQ38ResidentGdnServiceProof(proof,job,gated.data(),input.data(),state.data(),source,logicalWorker,selfRoute,gdnBefore,after,first,recurrent,summary,clock));
  for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!PioneerQ38ResidentGdnServiceProof(proof,job,gated.data(),input.data(),state.data(),source,logicalWorker,selfRoute,gdnBefore,after,first,recurrent,summary,clock));proof[i]^=1;}
 }
 bool MatrixAfterGdn(unsigned sequence){gdnActive=false;return MatrixAfterConv(sequence);}
 void GdnInert(){unsigned reads=nocReads,packets=gdnPackets,ops=gdnOps,writes=gdnNextWrites,math=matrixInstructions;
  for(unsigned i=0;i<64;i++)StepBlock();assert(reads==nocReads && packets==gdnPackets && ops==gdnOps && writes==gdnNextWrites && math==matrixInstructions);}
};
static bool RunPersistentGdnPair(ResidentGdnOperatorPeer& a,ResidentGdnOperatorPeer& b,unsigned sequence,unsigned delayed=0){
 a.peer=&b;b.peer=&a;
 if(delayed)while(a.gdnPolls<delayed && !a.GdnFinished(sequence) && !a.GdnFaulted())a.StepBlock();
 unsigned rounds=0;while((!a.GdnFinished(sequence) && !a.GdnFaulted()) || (!b.GdnFinished(sequence) && !b.GdnFaulted())){
  assert(++rounds<1000000);for(auto* one:{&a,&b})for(unsigned i=0;i<128 && !one->GdnFinished(sequence) && !one->GdnFaulted();i++)one->StepBlock();
 }
 a.GdnInert();b.GdnInert();return a.GdnFinished(sequence) && b.GdnFinished(sequence);
}
#ifndef PIONEER_Q38_RESIDENT_GDN_OPERATOR_SERVICE_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--smoke")));setvbuf(stdout,nullptr,_IOLBF,0);bool smoke=argc==4;
 unsigned positive=0,faults=0,minStack=0x70000;
 for(unsigned head:{0U,31U,47U})for(unsigned type:{12U,14U}){
  if(smoke && (head || type!=12))continue;
  ResidentGdnOperatorPeer a(argv[1],head*2+1,type),b(argv[1],head*2+2,type);
  for(auto* m:{&a,&b}){assert(m->BootNew() && m->MatrixAfterGdn(1));m->StageConv(2);assert(m->Finish(2));m->VerifyConv();m->StageGdn(3);}
  assert(RunPersistentGdnPair(a,b,3));a.VerifyGdn(3);b.VerifyGdn(3);
  for(auto* m:{&a,&b}){assert(m->config[1]==0xa0200000);FreshInputs(*m);assert(m->MatrixAfterGdn(4));m->prior=m->next;m->StageGdn(5,23,1,6,1,1,2);}
  assert(RunPersistentGdnPair(a,b,5));a.VerifyGdn(5);b.VerifyGdn(5);
  for(auto* m:{&a,&b}){FreshInputs(*m);assert(m->MatrixAfterGdn(6) && m->Stop(7));minStack=std::min(minStack,m->minStack);}positive+=2;
  printf("PASS SAME shared cfg/thread/LRegs/physicalDst matrix/Conv/pairedGDN/matrix/changed-stateGDN/matrix/STOP head%u type%u sequences1..7 no reset/code reload or detached tensor state\n",head,type);
 }
 if(smoke){printf("PASS SMOKE actual returning paired GDN/matrix/Conv shared-state handoff/maxObservedStack%u;SYNTHETIC endpoints,NO native/fullmodel/card speed grant\n",0x70000-minStack);return 0;}
 for(unsigned head=0;head<48;head++){
  ResidentGdnOperatorPeer a(argv[1],head*2+1),b(argv[1],head*2+2);unsigned layer=head/3*4+head%3;
  for(auto* m:{&a,&b}){assert(m->BootNew());m->StageGdn(1,17,layer,head%7,(head&1)?8191:0,head&1,head%4+1);}
  assert(RunPersistentGdnPair(a,b,1));a.VerifyGdn(1);b.VerifyGdn(1);positive+=2;
  for(auto* m:{&a,&b}){minStack=std::min(minStack,m->minStack);assert(m->Stop(2));}
 }
 puts("PASS ALL96 actual paired half streams/ALL48recurrent layers/7banks/positions0+8191/bothslots/strict NORMAL relative gates;no fabricated peer readiness");
 for(unsigned head:{0U,47U}){
  ResidentGdnOperatorPeer a(argv[1],head*2+1),b(argv[1],head*2+2);assert(a.BootNew() && b.BootNew());
  for(unsigned token=0;token<16;token++){
   for(auto* m:{&a,&b})m->StageGdn(token+1,17+token,0,token%7,token,token&1,token+1);
   assert(RunPersistentGdnPair(a,b,token+1));a.VerifyGdn(token+1);b.VerifyGdn(token+1);positive+=2;
   for(auto* m:{&a,&b}){m->prior=m->next;minStack=std::min(minStack,m->minStack);}
  }assert(a.Stop(17) && b.Stop(17));
 }
 puts("PASS SAME two pairs x16 successive GDN commands/actual NEXT carries/monotonic sequence+position/fresh cells/all counters preserved;component ONLY,not full-model token commit");
 for(unsigned delayed:{512U,2048U}){
  ResidentGdnOperatorPeer a(argv[1]),b(argv[1],2);assert(a.BootNew() && b.BootNew());a.StageGdn(1);b.StageGdn(1);
  assert(RunPersistentGdnPair(a,b,1,delayed));a.VerifyGdn(1);b.VerifyGdn(1);assert(a.gdnPolls>=delayed);positive+=2;
 }
 auto finishFault=[](ResidentGdnOperatorPeer& m){while(!m.GdnFinished(1) && !m.GdnFaulted())m.StepBlock();assert(m.GdnFaulted());m.GdnInert();};
 for(unsigned word=0;word<16;word++){
  ResidentGdnOperatorPeer m(argv[1]);assert(m.BootNew());m.StageGdn(1);m.memory[0x74000+word*4]^=1;finishFault(m);
  assert(!m.gdnPackets && !m.gdnOps && !m.clockReads);faults++;
 }
 for(unsigned part=0;part<3;part++)for(unsigned word=0;word<64;word++){
  ResidentGdnOperatorPeer m(argv[1]);assert(m.BootNew());m.StageGdn(1);uint32_t job[16];for(unsigned i=0;i<16;i++)job[i]=m.memory.at(0x74000+i*4);
  auto in=m.input,st=m.state,ga=m.gated;auto& d=part==0?in:part==1?st:ga;d[word]^=1;
  if(PioneerQ38ResidentGdnServiceJob(job,ga.data(),in.data(),st.data(),m.source,1,m.selfRoute))continue;
  unsigned address=part==0?TT_Q38_GDN_INPUT_DESCRIPTOR:part==1?TT_Q38_RGDN_DESCRIPTOR:TT_Q38_GDN_GATED_DESCRIPTOR;
  m.memory[address+word*4]^=1;finishFault(m);assert(!m.gdnPackets && !m.gdnOps && !m.clockReads);faults++;
 }
 for(unsigned word=0;word<16;word++){
  ResidentGdnOperatorPeer m(argv[1]);assert(m.BootNew());m.StageGdn(1);m.memory[TT_Q38_GDN_GATED_READY+word*4]=1;finishFault(m);
  assert(!m.gdnPackets && !m.gdnOps && !m.clockReads);faults++;
 }
 for(unsigned part=0;part<3;part++)for(unsigned word:{0U,6U,32U,40U,63U}){
  ResidentGdnOperatorPeer m(argv[1]);assert(m.BootNew());m.StageGdn(1);
  m.gdnDescriptorBase=part==0?TT_Q38_GDN_INPUT_DESCRIPTOR:part==1?TT_Q38_RGDN_DESCRIPTOR:TT_Q38_GDN_GATED_DESCRIPTOR;
  m.gdnDescriptorWord=word;m.gdnDescriptorPacket=1;finishFault(m);
  assert(m.gdnPackets==(part==0?1U:part==1?7U:11U) && !m.gdnGatedWrites);faults++;
 }
 for(unsigned kind=0;kind<3;kind++)for(unsigned packet:{1U,7U,9U,11U}){
  ResidentGdnOperatorPeer m(argv[1]);assert(m.BootNew());m.StageGdn(1);
  if(!kind)m.gdnFailPacket=packet;if(kind==1)m.gdnCounterPacket=packet;if(kind==2)m.gdnSourcePacket=packet;
  finishFault(m);assert(m.gdnPackets==packet && !m.gdnGatedWrites);faults++;
 }
 for(unsigned kind:{1U,2U,3U}){
  ResidentGdnOperatorPeer a(argv[1]),b(argv[1],2);assert(a.BootNew() && b.BootNew());a.StageGdn(1);b.StageGdn(1);
  a.clockFault=kind;b.clockGap=4000000000ULL;assert(!RunPersistentGdnPair(a,b,1) && a.GdnFaulted() && !a.gdnGatedWrites);faults++;
 }
 {ResidentGdnOperatorPeer a(argv[1]),b(argv[1],2);assert(a.BootNew() && b.BootNew());a.StageGdn(1);b.StageGdn(1);
  a.gdnNeverPeer=true;a.clockGap=4000000000ULL;assert(!RunPersistentGdnPair(a,b,1) && a.GdnFaulted() && !a.gdnGatedWrites && a.gdnPolls<256);faults++;}
 for(unsigned kind=0;kind<4;kind++){
  printf("CHECK returning GDN terminal fault kind%u (0 lost output,1 job,2 boot,3 doorbell)\n",kind);
  ResidentGdnOperatorPeer a(argv[1]),b(argv[1],2);assert(a.BootNew() && b.BootNew());a.StageGdn(1);b.StageGdn(1);
  if(!kind)a.gdnFailGatedPhase=5;if(kind==1)a.gdnLateJob=true;if(kind==2)a.gdnLateBoot=true;if(kind==3)a.gdnLateDoorbell=true;
  assert(!RunPersistentGdnPair(a,b,1) && a.GdnFaulted());faults++;
 }
 {ResidentGdnOperatorPeer a(argv[1]),b(argv[1],2);assert(a.BootNew() && b.BootNew());a.StageGdn(1);b.StageGdn(1);
  assert(RunPersistentGdnPair(a,b,1));a.VerifyGdn(1);b.VerifyGdn(1);unsigned math=a.matrixInstructions;
  a.dropMatrixConfig1=true;FreshInputs(a);assert(!a.MatrixAfterGdn(2) && a.droppedMatrixConfig1 && a.matrixInstructions==math);a.GdnInert();faults++;}
 {ResidentOperatorPeer old(argv[2]);ResidentGdnOperatorPeer data(argv[1]);assert(old.BootNew());data.StageGdn(1);
  for(unsigned i=0;i<16;i++)old.memory[0x74000+i*4]=data.memory.at(0x74000+i*4);old.memory[0x74100]=1;
  assert(!old.Finish(1) && !old.requests && !old.matrixInstructions && !old.convPackets);old.ServiceInert();faults++;}
 printf("PASS actual returning GDN service positive%u/fault%u/maxObservedStack%u/SAME hardware arrays/matrix rearm loss refused/old service refusesop8/UNKNOWN inert;SYNTHETIC endpoints+clock,NOT native/ROOT/fullmodel or physical throughput qualification\n",positive,faults,0x70000-minStack);
}
#endif
