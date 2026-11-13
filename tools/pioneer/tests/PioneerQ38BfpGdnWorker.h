/* SPDX-License-Identifier: MIT */
// Paired GDN extension of the SAME BFP/Conv simulator. Instruction, Dst,
// configuration and counter state stay attached to the existing worker.
#ifndef PIONEER_Q38_BFP_GDN_WORKER_H
#define PIONEER_Q38_BFP_GDN_WORKER_H
#include "PioneerQ38BfpConvWorker.h"
#include "../PioneerBlackholeFma.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnServiceShape.h"
struct PioneerQ38BfpGdnWorker:PioneerQ38BfpConvWorker {
 PioneerQ38BfpGdnWorker* peer=nullptr;
 std::array<uint32_t,64> inputDesc={},stateDesc={},gatedDesc={};
 std::array<uint32_t,324> raw={};std::array<std::array<uint32_t,16>,4> learned={};
 std::array<uint32_t,8192> prior={},next={};std::array<uint32_t,128> norm={},z={};std::array<uint32_t,64> gated={};
 uint32_t beforeGdn[8]={},lr[8][32]={};unsigned gdnPackets=0,phase=0,polls=0,imports=0,csrReads=0,nextWrites=0,outputWrites=0,gatedWrites=0;
 unsigned pendingWords=0,pendingSource=0,pendingRow=0,x0=0,x1=0,latency=0,clockReads=0;
 uint64_t clockBase=UINT64_C(1)<<40;
 bool gdn=false,waiting=false,drained=true,enabled=false;
 using PioneerQ38BfpConvWorker::PioneerQ38BfpConvWorker;
 static bool GdnLocal(unsigned a){return Range(a,0x74140,0xec0) || Range(a,0x80000,0x1100) || Range(a,0x8e000,0xb00)
  || Range(a,0x90000,32768) || Range(a,0xa0000,32768);}
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(gdn && (csr==0xb00 || csr==0xb80));unsigned n=clockReads++;
  uint64_t value=clockBase+uint64_t(steps)*19+uint64_t(n/3)*1000000;
  return csr==0xb00?uint32_t(value):uint32_t(value>>32);
 }
 uint32_t Load(uint32_t a)override{
  if(gdn){
   if(Range(a,0xffef0000,224*4))return native.config[(a-0xffef0000)/4];
   if(Range(a,0xffbd8000,32768)){
    assert(drained && !pendingWords && !latency && !native.config[3]);unsigned word=(a-0xffbd8000)/4;
    assert(word<8192 && native.valid[word/16]);csrReads++;return NativeBfpMachine::Bits(native.destination[word/16][word%16]);
   }
   if(a==0x74100 || Range(a,0x74000,96) || Range(a,0x83f00,256))return Get(a);
   if(GdnLocal(a))return WordRead(a);
  }
  return PioneerQ38BfpConvWorker::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(gdn){
   if(Range(a,0xffb20000,40)){transfer.command[(a-0xffb20000)/4]=value;return;}
   if(a==0xffb20040){assert(value==1 && transfer.pending.empty());PacketGdn();return;}
   if(a==0xffe000fc){assert(!value);gpr63=0;return;}
   if(a==0xffe40000){InstructionGdn(value);return;}
   if(Range(a,0xffef0000,224*4)){assert(drained && !pendingWords && !latency);native.Config((a-0xffef0000)/4,value);return;}
   if(Range(a,0xa0000,32768))nextWrites++;
   if(Range(a,0x8e800,256))outputWrites++;
   if(Range(a,0x8ea00,256))gatedWrites++;
   if(Range(a,0x83f00,256)){Put(a,value);return;}
   if(GdnLocal(a)){WordWrite(a,value);return;}
  }
  PioneerQ38BfpConvWorker::Store(a,value);
 }
 void InstructionGdn(uint32_t op){
  const unsigned code=op>>24;
  if(code==0xb2){assert(!pendingWords);unsigned at=(op>>16)&255;assert(at<68);native.thread[at]=op&65535;return;}
  if(op==0xa2101816){assert(!waiting && !gpr63);waiting=true;return;}
  if(op==0x4500017e){
   assert(waiting && !gpr63 && !latency);waiting=false;
   if(pendingWords){for(unsigned i=0;i<pendingWords;i++)native.destination[pendingRow+i/16][i%16]=NativeBfpMachine::Float(WordRead(pendingSource+i*4));
    for(unsigned r=pendingRow;r<pendingRow+pendingWords/16;r++)native.valid[r]=true;pendingWords=0;}
   drained=true;gpr63=1;return;
  }
  if((op&0xfff00000)==0x5e200000){assert(!pendingWords);x0=op&1023;x1=(op>>10)&1023;return;}
  if(op==0x5120000a || op==0x5420000f){assert(!pendingWords);return;}
  if(op==0x42000081){
   assert(!pendingWords && !native.thread[5] && native.config[0]==0x4231 && native.config[1]==0xa0200000
    && !native.config[3] && !native.config[49] && native.config[64]==0x100010 && native.config[65]==16
    && !native.config[72] && native.config[73]==0x13 && !x0);
   unsigned sourceAt=(native.config[76]+1)*16,row=(native.config[84]&65535)/16-4,words=x1+1;
   assert((native.config[84]&0xffff0000)==0x400000 && row<512 && !(words%16) && words<=8192-row*16);
   assert((!imports && sourceAt==0x8e400 && !row && words==64)
    || (imports && imports<9 && sourceAt==0x90000+(imports-1)*4096 && row==(imports-1)*64 && words==1024));
   imports++;pendingSource=sourceAt;pendingRow=row;pendingWords=words;drained=false;
   for(unsigned r=row;r<row+words/16;r++)native.valid[r]=false;return;
  }
  assert(!pendingWords && native.thread[2]==1 && native.thread[3]==1 && native.thread[5]==4 && native.config[0]==0x4231 && native.config[1]==0xa0200000);
  drained=false;if(op==0x8f000000){assert(latency==1);latency=0;return;}assert(!latency);
  if(op==0x8a000002){enabled=true;return;}assert(enabled);
  if(code==0x70 || code==0x72){
   unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;assert(mode==3 && reg<8 && !(at&1) && at<512 && !(op&0xfc00));
   for(unsigned lane=0;lane<32;lane++){
    unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+((at&2)!=0);assert(row<512 && col<16 && native.valid[row]);
    if(code==0x70)lr[reg][lane]=NativeBfpMachine::Bits(native.destination[row][col]);
    else {uint32_t bits=lr[reg][lane];native.destination[row][col]=NativeBfpMachine::Float(bits&0x7f800000?bits:bits&0x80000000);}
   }return;
  }
  for(unsigned lane=0;lane<32;lane++){
   if(code==0x71){unsigned r=(op>>20)&15,mode=(op>>16)&15;assert(r<8);
    if(!mode)lr[r][lane]=(op&65535)<<16;else if(mode==10)lr[r][lane]=(lr[r][lane]&0xffff0000)|(op&65535);else{assert(mode==2);lr[r][lane]=op&65535;}
   }else if(code==0x8d){unsigned s=(op>>8)&15,d=(op>>4)&15;assert(s<8 && d<8 && !(op&15));lr[d][lane]^=lr[s][lane];
   }else{
    assert(code==0x85 || code==0x86);unsigned a=code==0x86?(op>>16)&15:10,b=(op>>12)&15,c=code==0x86?9:(op>>8)&15,d=(op>>4)&15;
    assert(d<8 && b<8 && ((code==0x86 && a<8 && (op&15)==2) || (code==0x85 && c<8 && !(op&15))));
    lr[d][lane]=PioneerBlackholeFmaBits(a==10?0x3f800000:lr[a][lane],lr[b][lane],c==9?0x80000000:lr[c][lane]);
   }
  }
  if(code==0x85 || code==0x86)latency=1;
 }
 void PacketGdn(){
  assert(peer && transfer.pending.empty());unsigned p=gdnPackets,local=0,remote=0,route=0,bytes=0;bool write=false;
  if(p<7){
   const unsigned l[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},n[]={512,512,256,64,64,64,64};
   const unsigned r[]={inputDesc[4]+inputDesc[42],inputDesc[4]+inputDesc[43],inputDesc[4]+inputDesc[44],inputDesc[4]+inputDesc[45],inputDesc[4]+inputDesc[46],inputDesc[12],inputDesc[15]};
   const unsigned xy[]={inputDesc[8],inputDesc[8],inputDesc[8],inputDesc[8],inputDesc[8],inputDesc[13],inputDesc[41]};local=l[p];remote=r[p];route=xy[p];bytes=n[p];
  }else if(p<11){write=p>=9;unsigned piece=(p-7)%2;local=(write?0xa0000:0x90000)+piece*16384;
   remote=stateDesc[write?7:5]+piece*16384;route=stateDesc[write?13:12];bytes=16384;
  }else{
   assert(phase<5 && imports==9 && nextWrites==8192 && outputWrites==64 && drained);
   write=phase==4;const unsigned l[]={TT_Q38_GDN_GATED_PEER_READY,0x80000+((gatedDesc[7]^1)&1)*256,0x80200,0x80400,0x8ea00};
   const unsigned r[]={gatedDesc[44],gatedDesc[45],gatedDesc[12],gatedDesc[4]+gatedDesc[42],gatedDesc[4]+gatedDesc[43]};
   const unsigned xy[]={gatedDesc[15],gatedDesc[15],gatedDesc[13],gatedDesc[8],gatedDesc[8]},n[]={64,256,512,512,256};
   local=l[phase];remote=r[phase];route=xy[phase];bytes=n[phase];
  }
  const uint32_t command[]={write?local:remote,0,write?boot[4]:route,write?remote:local,0,write?route:boot[4],0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(transfer.command[i]==command[i]);gdnPackets++;
  for(unsigned i=0;i<bytes/4;i++){
   uint32_t value=0;
   if(p<7)value=p<3?raw[(p==0?0:p==1?128:256)+i]:learned[p-3][i];
   else if(p<11){unsigned index=((p-7)%2)*4096+i;if(write)next[index]=WordRead(local+i*4);else value=prior[index];}
   else if(write)gated[i]=WordRead(local+i*4);
   else value=!phase?peer->WordRead(TT_Q38_GDN_GATED_READY+i*4):phase==1?peer->WordRead(0x8e800+i*4):phase==2?norm[i]:z[i];
   if(!write)WordWrite(local+i*4,value);
  }
  if(p>=11){if(!phase){polls++;if(WordRead(TT_Q38_GDN_GATED_PEER_READY))phase=1;}else phase++;}
  auto& c=transfer.counters;c[4]++;c[14]++;
  if(write){c[1]++;c[8]+=(bytes+63)/64;c[10]++;}else{c[2]++;c[3]+=(bytes+63)/64;c[5]++;}
 }
 void StageGdn(unsigned sequence,unsigned position=0,unsigned slot=0,bool retained=false,const uint32_t* alphaBeta=nullptr,
  const std::vector<uint32_t>* model=nullptr,const std::vector<uint32_t>* gateValues=nullptr,const uint32_t* partialQ=nullptr,const uint8_t* qMask=nullptr){
  assert(boot[3]<=96 && pc==waitPC && native.Clean() && !gdn && !conv && transfer.pending.empty());
  assert(position<8192 && slot<2 && !waiting && !pendingWords && !latency && drained);
  assert(bool(partialQ)==bool(qMask) && (!model || model->size()==41184) && (!gateValues || gateValues->size()==6144));
  if(clockReads)clockBase+=UINT64_C(1)<<36; // simulated hardware clock never moves backward across jobs
  if(retained)prior=next;else prior.fill(0);
  next.fill(0);gated.fill(0);gdnPackets=phase=polls=imports=csrReads=nextWrites=outputWrites=gatedWrites=clockReads=0;
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  auto plan=std::make_unique<PioneerQ38GdnInputPlan>();PioneerQ38GdnIdentity identity={1,2,3,4};unsigned half=boot[3]-1;
  assert(plan->Build(*canonical,0,half,position,0,identity,source.data(),2025));
  for(unsigned i=0;i<64;i++)inputDesc[i]=plan->Descriptor()[i];stateDesc={};
  stateDesc[0]=TT_Q38_RGDN_MARKER;stateDesc[1]=1;stateDesc[2]=128;stateDesc[3]=64;stateDesc[8]=2025;stateDesc[9]=half;stateDesc[11]=slot;
  stateDesc[14]=32768;stateDesc[15]=boot[4];stateDesc[40]=position;for(unsigned i=16;i<40;i++)stateDesc[i]=inputDesc[i];
  for(unsigned side=0;side<2;side++){
   bool found=false;for(unsigned i=0;i<canonical->RecurrentCount();i++){
    const auto& r=canonical->RecurrentRegions()[i];unsigned h=half/2;
    if(r.layer || r.slot!=(slot^side) || h<r.firstHead || h-r.firstHead>=r.heads)continue;
    stateDesc[4+side*2]=r.bank;stateDesc[5+side*2]=canonical->Base+r.offset+(h-r.firstHead)*65536+(half&1)*32768;
    stateDesc[12+side]=PioneerQ38SpanRoute(r.bank);found=true;break;
   }assert(found);
  }
  auto paired=std::make_unique<PioneerQ38GdnRendezvousPlan>();assert(paired->Build(*canonical,inputDesc.data(),stateDesc.data()));
  for(unsigned i=0;i<64;i++){
   gatedDesc[i]=paired->Descriptor()[i];WordWrite(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4,inputDesc[i]);
   WordWrite(TT_Q38_RGDN_DESCRIPTOR+i*4,stateDesc[i]);WordWrite(TT_Q38_GDN_GATED_DESCRIPTOR+i*4,gatedDesc[i]);
  }
  unsigned h=half/2,kh=h%16;
  for(unsigned i=0;i<128;i++){
   raw[i]=PioneerAttentionBits(float(std::sin(kh*.117+i*.071+.03)*2.7));raw[128+i]=PioneerAttentionBits(float(std::cos(kh*.213+i*.091-.02)*1.9));
   if(partialQ && qMask && qMask[i])raw[i]=partialQ[i];
   norm[i]=model?model->at(41056+i):PioneerAttentionBits(float(.5+std::sin(i*.07)*.3));
   z[i]=gateValues?gateValues->at(h*128+i):PioneerAttentionBits(float(std::cos((h*128+i)*.017+.11)*5));
  }
  for(unsigned i=0;i<64;i++)raw[256+i]=PioneerAttentionBits(float(std::sin((h*128+(half&1)*64+i)*.035+.13)));
  raw[320]=PioneerAttentionBits(float(h)*.13f-2);raw[321]=PioneerAttentionBits(float(h)*.07f-2);
  raw[322]=PioneerAttentionBits(-float(h+1)*.25f);raw[323]=PioneerAttentionBits(-3.5f);
  if(alphaBeta){raw[320]=alphaBeta[h];raw[321]=alphaBeta[48+h];}
  if(model){raw[322]=model->at(40960+h);raw[323]=model->at(41008+h);}
  for(unsigned p=0;p<4;p++)for(unsigned i=0;i<16;i++)learned[p][i]=PioneerAttentionBits(p==2?-float(i+1)*.25f:float(i+1)*.01f);
  for(unsigned p=0;p<4;p++)learned[p][inputDesc[47]]=raw[320+p];
  for(unsigned i=0;i<16;i++){WordWrite(TT_Q38_GDN_GATED_READY+i*4,0);WordWrite(TT_Q38_GDN_GATED_PEER_READY+i*4,0);WordWrite(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4,0);}
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)beforeGdn[i]=transfer.counters[indices[i]];
  uint32_t job[64]={};job[0]=HW::Magic;job[1]=sequence;job[2]=8;job[3]=boot[3];job[4]=boot[4];job[5]=0;job[6]=position;job[7]=2025;
  for(unsigned i=0;i<8;i++)job[8+i]=boot[8+i];job[16]=TT_Q38_RESIDENT_GDN_SERVICE_JOB;job[17]=64;job[18]=768;job[19]=2025;
  assert(HW::JobShape(job,boot.data(),sequence-1,0));for(unsigned i=0;i<64;i++)Put(HW::Job+i*4,job[i]);
  gdn=true;steps=0;Put(HW::Doorbell,sequence);
 }
 void StepGdn(){
  uint32_t i=Load(pc);if((i&0x707f)==0x23){
   assert(++steps<StepLimit());unsigned a=(i>>15)&31,b=(i>>20)&31;
   uint32_t at=regs[a]+Sign(((i>>25)<<5)|((i>>7)&31),12);assert(Range(at,0x6e000,8192));
   uint32_t aligned=at&~3U,shift=(at&3)*8,old=Load(aligned);Store(aligned,(old&~(255U<<shift))|((regs[b]&255)<<shift));pc+=4;return;
  }
  StepCombined();
 }
 bool Done(unsigned sequence){return pc==waitPC && Get(HW::Status+4)==sequence;}
 void VerifyGdn(unsigned sequence){
  assert(Done(sequence) && phase==5 && polls && imports==9 && nextWrites==8192 && outputWrites==64 && gatedWrites==64
   && csrReads==8256 && drained && !latency && !pendingWords && peer);
  uint32_t proof[64],job[16],after[8],first[32],recurrent[32],summary[32],clock[16];const unsigned indices[]={2,3,4,5,1,8,4,10};
  for(unsigned i=0;i<64;i++)proof[i]=Get(0x83f00+i*4);for(unsigned i=0;i<16;i++){job[i]=Get(0x74000+i*4);clock[i]=WordRead(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=transfer.counters[indices[i]];
  for(unsigned i=0;i<32;i++){first[i]=WordRead(TT_Q38_GDN_INPUT_SUMMARY+i*4);recurrent[i]=WordRead(TT_Q38_RGDN_SUMMARY+i*4);summary[i]=WordRead(TT_Q38_GDN_GATED_SUMMARY+i*4);}
  assert(PioneerQ38ResidentGdnServiceProof(proof,job,gatedDesc.data(),inputDesc.data(),stateDesc.data(),source.data(),boot[3],boot[4],beforeGdn,after,first,recurrent,summary,clock));
  for(unsigned i=0;i<64;i++){
   proof[i]^=1;assert(!PioneerQ38ResidentGdnServiceProof(proof,job,gatedDesc.data(),inputDesc.data(),stateDesc.data(),source.data(),boot[3],boot[4],beforeGdn,after,first,recurrent,summary,clock));proof[i]^=1;
  }
  float qs=0,ks=0;for(unsigned i=0;i<128;i++){float q=PioneerAttentionFloat(raw[i]),k=PioneerAttentionFloat(raw[128+i]);qs+=q*q;ks+=k*k;}
  float qi=1.f/std::sqrt(qs+1e-6f),ki=1.f/std::sqrt(ks+1e-6f);
  auto close=[](uint32_t bits,float expected){return std::isfinite(PioneerAttentionFloat(bits)) && std::fabs(double(PioneerAttentionFloat(bits))-expected)<=3e-6*(1+std::fabs(double(expected)));};
  for(unsigned i=0;i<128;i++){
   assert(close(WordRead(0x8e000+i*4),PioneerAttentionFloat(raw[i])*qi));
   assert(close(WordRead(0x8e200+i*4),PioneerAttentionFloat(raw[128+i])*ki));
  }
  double x=double(PioneerAttentionFloat(raw[320]))+PioneerAttentionFloat(raw[323]);
  float softplus=float(x>0?x+std::log1p(std::exp(-x)):std::log1p(std::exp(x)));
  assert(close(WordRead(0x8e500),std::exp(PioneerAttentionFloat(raw[322])*softplus)));
  float b=PioneerAttentionFloat(raw[321]);double e=std::exp(-std::fabs(double(b)));float beta=float(b>=0?1/(1+e):e/(1+e));
  assert(close(WordRead(0x8e504),beta));
  const float decay=PioneerAttentionFloat(WordRead(0x8e500)),actualBeta=PioneerAttentionFloat(WordRead(0x8e504));
  for(unsigned c=0;c<64;c++){
   float predicted=0;for(unsigned k=0;k<128;k++)predicted+=(PioneerAttentionFloat(prior[k*64+c])*decay)*PioneerAttentionFloat(WordRead(0x8e200+k*4));
   float delta=(PioneerAttentionFloat(raw[256+c])-predicted)*actualBeta,out=0;
   for(unsigned k=0;k<128;k++){float expected=PioneerAttentionFloat(prior[k*64+c])*decay+PioneerAttentionFloat(WordRead(0x8e200+k*4))*delta;
    assert(next[k*64+c]==PioneerAttentionBits(expected) && WordRead(0x90000+(k*64+c)*4)==prior[k*64+c]);out+=expected*PioneerAttentionFloat(WordRead(0x8e000+k*4));}
   out*=1.f/std::sqrt(128.f);assert(WordRead(0x8e800+c*4)==PioneerAttentionBits(out));
  }
  float sum=0,all[128];for(unsigned i=0;i<128;i++){auto* one=i/64==(stateDesc[9]&1)?this:peer;all[i]=PioneerAttentionFloat(one->WordRead(0x8e800+i%64*4));sum+=all[i]*all[i];}
  float inverse=1.f/std::sqrt(sum/128+1e-6f);
  for(unsigned i=0;i<64;i++){unsigned at=(stateDesc[9]&1)*64+i;double x=PioneerAttentionFloat(z[at]);
   double expected=double((all[at]*inverse)*PioneerAttentionFloat(norm[at]))*x/(1+std::exp(-x));
   double actual=PioneerAttentionFloat(gated[i]);assert(std::isfinite(actual) && std::fabs(actual-expected)<=3e-6*(1+std::fabs(expected)));}
 }
};
#endif
