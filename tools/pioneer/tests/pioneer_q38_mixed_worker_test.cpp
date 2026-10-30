/* SPDX-License-Identifier: MIT */
// ACTUAL QMW1 instructions on ONE retained matrix/GDN/dense tensor model.
// Synthetic CONTROL and byte endpoints, not native or inference throughput.
#define PIONEER_Q38_RESIDENT_GDN_OUTPUT_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_gdn_output_operator_service_image_test.cpp"
#include "../PioneerQ38MixedWorkerLayout.h"
#include "../PioneerQ38DensePartitionLayout.h"
#include <fstream>
namespace M=PioneerQ38MixedWorkerLayout;
namespace MWD=PioneerQ38DensePartitionLayout;
struct MixedPeer:ResidentGdnOutputPeer {
 std::vector<uint8_t> front,dense,helper;
 std::array<uint32_t,64> boot={},denseHeader={};
 unsigned waitPC,stopPC,faultPC,sequence=0,phase=0,densePackets=0,phasePackets=0;
 unsigned envelopeFaultPacket=0,envelopeFaultWord=0,outerBootFaultPacket=0;
 bool denseActive=false;
 unsigned denseCommands=0,denseProducts=0;
 MixedPeer(const char* image,unsigned worker=1,unsigned type=8,unsigned k=6144,unsigned panels=1):ResidentGdnOutputPeer((std::string(image)+"/unchanged-worker").c_str(),worker,panels,type,k),
  front(Read((std::string(image)+"/mixed.bin").c_str())),dense(Read((std::string(image)+"/dense.bin").c_str())),helper(Read((std::string(image)+"/ffn.bin").c_str())){
  // Construct the old generic ISA fixture, then install ALL NEW immutable
  // ROM BEFORE its very first boot. Never replace code between jobs.
  code=Read((std::string(image)+"/text.bin").c_str());affineCode=Read((std::string(image)+"/affine.bin").c_str());
  service=Read((std::string(image)+"/service.bin").c_str());gdnCode=Read((std::string(image)+"/gdn.bin").c_str());
  for(unsigned i=0;i<2048;i++)l1[2048+i]=i*4<code.size()?Word(code,i*4):0;
  l1[0]=Word(Read((std::string(image)+"/boot.bin").c_str()),0);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string address,kind,name;
  waitPC=stopPC=faultPC=0;
  while(symbols>>address>>kind>>name){unsigned a=unsigned(std::stoul(address,nullptr,16));
   if(name=="block_peer_wait_load")waitPC=a;if(name=="block_peer_stopped")stopPC=a;if(name=="block_fault_stopped")faultPC=a;
  }assert(waitPC && stopPC && faultPC);
  boot[0]=M::Magic;boot[1]=1;boot[2]=64;boot[3]=worker;boot[4]=selfRoute;boot[5]=source[0];boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<16;i++)boot[16+i]=source[i];
  assert(M::BootShape(boot.data()));for(unsigned i=0;i<64;i++){memory[M::Boot+i*4]=boot[i];memory[M::Job+i*4]=0;}
  memory[M::Doorbell]=0;for(unsigned i=0;i<16;i++)memory[M::Status+i*4]=0xdeadbeef;
 }
 unsigned StepLimit()const override{return 2000000000U;}
 virtual const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const{
  for(auto p:{std::make_pair(unsigned(M::Code),&front),std::make_pair(unsigned(M::DenseCode),&dense),std::make_pair(0x60000U,&helper),std::make_pair(0x70b00U,&affineCode)})
   if(Range(a,p.first,unsigned(p.second->size()))){base=p.first;return p.second;}
  return nullptr;
 }
 unsigned Index(uint32_t a)override{unsigned base=0;if(auto* r=Rom(a,base)){assert(!(a&3));l1[8188]=Word(*r,a-base);return 8188;}return ResidentGdnOutputPeer::Index(a);}
 bool CodeAddress(uint32_t a)const override{unsigned base=0;return Rom(a,base) || ResidentGdnOutputPeer::CodeAddress(a);}
 uint32_t Load(uint32_t a)override{
  unsigned base=0;if(auto* r=Rom(a,base))return Word(*r,a-base);
  if(Range(a,M::Boot,512) || a==M::Doorbell || Range(a,M::Status,64))return memory.at(a);
  if(denseActive){
   if(a==0xffe000fc)return gpr63;
   if(Range(a,0xffef0000,224*4))return config[(a-0xffef0000)/4];
   if(Range(a,0xffbd8000,12*64)){assert(drained && !config[3]);unsigned at=(a-0xffbd8000)/4;return FloatDst(at/16,at%16);}
   if(Range(a,MWD::Header,256) || Range(a,MWD::Summary,256) || Range(a,MWD::Job,64) || a==MWD::Doorbell || Range(a,MWD::Parameters,64)
    || Range(a,MWD::HalfQuery,7168) || Range(a,MWD::PrivateQuery,7168) || Range(a,MWD::Scores,6*384*4) || Range(a,MWD::Local,256)
    || Range(a,MWD::Accumulate,12288) || Range(a,MWD::Partial,12288) || Range(a,MWD::Chunk,16384))return memory.at(a);
  }
  return ResidentGdnOutputPeer::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  unsigned base=0;assert(!Rom(a,base));
  if(Range(a,M::Status,64)){assert(!pending || (a==M::Status+8 && value==M::Fault));memory[a]=value;return;}
  assert(!Range(a,M::Boot,512) && a!=M::Doorbell);
  if(!activation && Range(pc,0x60000,unsigned(helper.size())) && Range(a,0xffb20000,40)){
   assert(!pending && drained);unsigned i=(a-0xffb20000)/4;
   assert(i==normalizationWords && value==(i==5?selfRoute:0));
   if(++normalizationWords==10){normalizationWords=0;normalizations++;}command[i]=value;return;
  }
  if(Range(pc,M::Code,unsigned(front.size())) && (Range(a,0x74000,96) || a==0x74100 || Range(a,MWD::Job,64) || a==MWD::Doorbell)){
   assert(!pending && drained);memory[a]=value;return;
  }
  if(denseActive){
   if(a==0xffe40000){Instruction(value);return;}
   if(a==0xffe000fc){assert(!value);gpr63=0;return;}
   if(Range(a,0xffef0000,224*4)){assert(drained);unsigned at=(a-0xffef0000)/4;config[at]=value;configWrites++;return;}
   if(Range(a,0xffbd8000,8*64)){assert(drained && !config[3]);unsigned at=(a-0xffbd8000)/4;StoreFloatDst(at/16,at%16,value);defined[Adj(at/16)]=true;return;}
   if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
   if(a==0xffb20040){assert(value==1 && !pending);SubmitDense();return;}
   if(Range(a,MWD::Summary,256) || Range(a,MWD::PrivateQuery,7168) || Range(a,MWD::Scores,6*384*4) || Range(a,MWD::Local,256)
    || Range(a,MWD::Accumulate,12288) || Range(a,MWD::Partial,12288) || Range(a,MWD::Chunk,16384)){assert(!pending);memory[a]=value;return;}
  }
  ResidentGdnOutputPeer::Store(a,value);
 }
 // Same config/thread/LRegs/physical Dst arrays as matrix and GDN. NO
 // detached dense simulator or state copies/resets at a job transition.
 void Instruction(uint32_t op)override{
  if(!denseActive){ResidentGdnOutputPeer::Instruction(op);return;}
  assert(!rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !recordRemaining && !gdnLatency && !gdnPendingWords);
  denseCommands++;unsigned code=op>>24;
  if(code==0xb2){unsigned i=(op>>16)&255;assert(i<68);thread[i]=op&65535;}
  else if(op==0x3700000f){assert(!thread[0]);}
  else if(op==0x8a000002)enabled=true;
  else if(op==0xa2101816){assert(!gpr63 && !backendWait);backendWait=true;}
  else if(op==0x4500017e){assert(backendWait && !gpr63);backendWait=false;gpr63=1;drained=true;drains++;return;}
  else if(code==0x70 || code==0x72){
   unsigned r=(op>>20)&15,mode=(op>>16)&15,at=op&1023;assert(r<8 && !(at&1) && !(op&0xfc00));
   assert(!thread[1] && !thread[36] && !config[3] && !config[6] && !config[7]);
   for(unsigned lane=0;lane<32;lane++){
    unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+unsigned(bool(at&2));assert(row<12);
    if(code==0x72){assert(mode==3 && row>=8);StoreFloatDst(row,col,Bits(Flush(Float(lr[r][lane]))));defined[Adj(row)]=true;}
    else if(mode==11)lr[r][lane]=0;
    else{assert(mode==3 && row<8);lr[r][lane]=FloatDst(row,col);}
   }
  }else if(code==0x71){
   unsigned r=(op>>20)&15,mode=(op>>16)&15;assert(r<8 && (mode==0 || mode==10));
   for(unsigned lane=0;lane<32;lane++)lr[r][lane]=mode?(lr[r][lane]&0xffff0000)|(op&65535):(op&65535)<<16;
  }else if(code==0x84 || code==0x85){
   unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15,mod=op&15;
   assert((a<8 || a==10) && (b<8 || b==10) && (c<8 || c==9) && d<8 && !(mod&~3));
   if(code==0x84)denseProducts++;
   for(unsigned lane=0;lane<32;lane++){
    float av=a==10?1.f:Float(lr[a][lane]),bv=b==10?1.f:Float(lr[b][lane]),cv=c==9?-0.f:Float(lr[c][lane]);
    if(mod&1)av=-av;if(mod&2)cv=-cv;lr[d][lane]=Bits(PioneerBlackholeFma(av,bv,cv));
   }
  }else if(op==0x7c000520)for(unsigned lane=0;lane<32;lane++)lr[2][lane]=lr[5][lane];
  else{fprintf(stderr,"UNQUALIFIED mixed dense op%08x\n",op);assert(false);}
  drained=false;
 }
 static uint16_t Half(float x){_Float16 v=_Float16(x);uint16_t b;memcpy(&b,&v,2);return b;}
 static float Value(unsigned token,unsigned dimension){return float(int((token*7+dimension*3)%19)-9)*.0625f;}
 virtual void SubmitDense(){
  assert(phase==1 || phase==3);unsigned global=denseHeader[48]+phasePackets*32;
  unsigned count=std::min(32U,std::min(denseHeader[49]-phasePackets*32,denseHeader[4]-global)),bytes=count*512;
  unsigned which=phase==3;const uint32_t expected[]={denseHeader[8+which]+global*512,0,denseHeader[12+which],MWD::Chunk,0,selfRoute,0,0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  densePackets++;phasePackets++;
  for(unsigned i=0;i<bytes/4;i++)memory[MWD::Chunk+i*4]=which?Half(Value(global+i/128,i%128*2))|(uint32_t(Half(Value(global+i/128,i%128*2+1)))<<16):0;
  countBase[0]++;countBase[1]+=bytes/64;countBase[2]++;countBase[3]++;countBase[6]++;
  if(densePackets==envelopeFaultPacket)memory[M::Job+envelopeFaultWord*4]^=1;
  if(densePackets==outerBootFaultPacket)memory[M::Boot+envelopeFaultWord*4]^=1;
 }
 void StepBlock()override{
  unsigned base=0;
  if(!Rom(pc,base)){ResidentGdnOutputPeer::StepBlock();return;}
  uint32_t i=Load(pc),op=i&127;
  if(op!=7 && op!=0x27 && op!=0x53){ResidentGdnOutputPeer::StepBlock();return;}
  assert(++steps<StepLimit());unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else{float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));switch(hi){
   case 0:fp[d]=Bits(Flush(a+b));break;case 4:fp[d]=Bits(Flush(a-b));break;case 8:fp[d]=Bits(Flush(a*b));break;
   case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
   case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
   case 0x68:assert(t<=1);fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
   case 0x70:if(d)regs[d]=fp[s];break;case 0x78:fp[d]=regs[s];break;
   default:fprintf(stderr,"UNQUALIFIED mixed scalar%08x PC%08x\n",i,pc);assert(false);
  }}regs[0]=0;pc+=4;
 }
 bool GdnFinished(unsigned seq)const override{return pc==waitPC && memory.at(M::Status+4)==seq;}
 bool GdnFaulted()const override{return pc==faultPC;}
 bool FinishMixed(unsigned seq,bool stop=false){
  while(!GdnFaulted() && !(stop?pc==stopPC:GdnFinished(seq)))StepBlock();
  if(GdnFaulted()){assert(memory.at(M::Status+8)==M::Fault);return false;}
  assert(!pending && drained && minStack>=0x6e000 && memory.at(M::Status+12)==M::Magic && !memory.at(M::Status+8));return true;
 }
 bool BootMixed(){PoisonDst();Poison();return FinishMixed(0);}
 void StageMixed(unsigned op,unsigned layer,unsigned position,unsigned epoch){
  sequence++;std::array<uint32_t,64> j={};j[0]=M::Magic;j[1]=sequence;j[2]=op;j[3]=logicalWorker;j[4]=selfRoute;j[5]=layer;j[6]=position;j[7]=epoch;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];
  if(op<M::DenseScores && op!=M::Stop){for(unsigned i=0;i<10;i++)j[16+i]=memory.at(0x74000+i*4);j[26]=memory.at(0x74038);}
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=sequence;
 }
 bool MatrixMixed(){
  denseActive=outputActive=gdnActive=convActive=activation=false;Poison();PoisonTail();
  for(unsigned i=0;i<16;i++)memory[0x74000+i*4]=matrixJob[i];memory[0x74038]=1;
  StageMixed(1,0,0,1000+sequence);
  if(!FinishMixed(sequence))return false;
  assert(requests==packets.size() && !dotProbeReads && CorrectRows());
  uint32_t next[8];for(unsigned i=0;i<8;i++)next[i]=Count(i);for(unsigned i=0;i<8;i++)countBase[i]=next[i];NextPackets();return true;
 }
 void StageDense(unsigned p,unsigned tokens=32,unsigned layer=3){
  denseActive=activation=true;gdnActive=convActive=outputActive=false;phase=p;phasePackets=0;
  if(p==1){
   denseHeader={};unsigned w=logicalWorker,group=(w-1)/24,ordinal=(w-1)%24;
   const unsigned first[]={MWD::Magic,1,64,layer,tokens-1,group,ordinal,tokens,0x10000000,0x20000000,0,1,MWD::Route(0),MWD::Route(1),selfRoute,w};
   for(unsigned i=0;i<16;i++)denseHeader[i]=first[i];
   for(unsigned i=0;i<16;i++)denseHeader[16+i]=source[i];for(unsigned i=0;i<8;i++)denseHeader[32+i]=boot[8+i];
   auto& d=denseHeader;d[40]=1000+sequence;d[41]=256;d[42]=0x3d800000;d[43]=32;d[44]=8192;d[45]=16384;
   d[48]=MWD::First(tokens,ordinal);d[49]=MWD::Count(tokens,ordinal);unsigned prior=d[48]>=d[4]?0:std::min(d[49],d[4]-d[48]);
   d[46]=2*((prior+31)>>5);d[47]=prior*16;d[50]=3;d[51]=6;d[52]=24;d[53]=384;d[54]=1536;d[55]=MWD::Magic;
   assert(MWD::Shape(d.data()));for(unsigned i=0;i<64;i++)memory[MWD::Header+i*4]=d[i];
   for(unsigned i=0;i<1536;i++)memory[MWD::HalfQuery+i*4]=Bits(float(int(i%7)-3)*.125f);
   for(unsigned i=0;i<128;i++){memory[MWD::NewK+i*4]=0;memory[MWD::NewV+i*4]=Half(Value(tokens-1,i*2))|(uint32_t(Half(Value(tokens-1,i*2+1)))<<16);}
  }
  for(unsigned i=0;i<16;i++)memory[MWD::Parameters+i*4]=i>=8 && i<14 && p==3?Bits(1.f/float(denseHeader[7])):0;
  StageMixed(M::DenseScores+p-1,denseHeader[3],denseHeader[4],denseHeader[40]);
 }
 void VerifyDense(){
  assert(memory.at(MWD::Summary)==MWD::Magic && memory.at(MWD::Summary+4)==sequence && memory.at(MWD::Local)==phase);
  for(unsigned h=0;h<6;h++)for(unsigned t=0;t<denseHeader[49];t++)assert(memory.at(MWD::Scores+(h*384+t)*4)==(phase==1?0:phase==2?Bits(1.f):Bits(1.f/float(denseHeader[7]))));
  if(phase==3)for(unsigned h=0;h<6;h++)for(unsigned dim=0;dim<256;dim++){
   double expected=0;for(unsigned t=0;t<denseHeader[49];t++)expected+=double(Value(denseHeader[48]+t,dim))/double(denseHeader[7]);
   double actual=double(Float(memory.at(MWD::Partial+(h*512+dim*2)*4)))+Float(memory.at(MWD::Partial+(h*512+dim*2+1)*4));assert(actual==expected);
  }
 }
 void InertMixed(){unsigned p=densePackets,g=gdnPackets,m=matrixInstructions,c=denseCommands;for(unsigned i=0;i<32;i++)StepBlock();assert(p==densePackets && g==gdnPackets && m==matrixInstructions && c==denseCommands);}
 void StopMixed(){StageMixed(M::Stop,0,0,1000+sequence);assert(FinishMixed(sequence,true));InertMixed();}
};
#ifndef PIONEER_Q38_MIXED_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--full")));setvbuf(stdout,nullptr,_IOLBF,0);bool full=argc==3;
 unsigned cases=0,faults=0,stack=0;
 for(unsigned head:{0U,31U,47U}){
  if(!full && head)continue;
  auto a=std::make_unique<MixedPeer>(argv[1],head*2+1),b=std::make_unique<MixedPeer>(argv[1],head*2+2);
  for(auto* p:{a.get(),b.get()}){
   assert(p->BootMixed() && p->MatrixMixed());
   p->StageConv(2,1002);p->StageMixed(7,0,0,1002);assert(p->FinishMixed(2));p->VerifyConv();
   p->StageGdn(3,1003);p->StageMixed(8,0,0,1003);
  }
  assert(RunPersistentGdnPair(*a,*b,3));a->VerifyGdn(3);b->VerifyGdn(3);
  for(auto* p:{a.get(),b.get()}){
   if(!head){
    p->StageOutput(4,0,0,1004);
    for(unsigned i=0;i<8;i++)p->memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+(32+i)*4]=p->descriptor[32+i]=p->boot[8+i];
    p->StageMixed(6,0,0,1004);assert(p->FinishMixed(4));p->VerifyOutput();
   }
   assert(p->MatrixMixed());
   for(unsigned phase=1;phase<=3;phase++){p->StageDense(phase,32);assert(p->FinishMixed(p->sequence));p->VerifyDense();}
   FreshInputs(*p);assert(p->MatrixMixed());
   p->denseActive=p->outputActive=false;p->StageGdn(p->sequence+1,1010,4,6,1,1,2);p->StageMixed(8,4,1,1010);
  }
  assert(RunPersistentGdnPair(*a,*b,a->sequence));a->VerifyGdn(a->sequence);b->VerifyGdn(b->sequence);
  for(auto* p:{a.get(),b.get()}){assert(p->MatrixMixed());p->StopMixed();stack=std::max(stack,0x70000-p->minStack);}
  printf("PASS ACTUAL paired matrix/Conv/GDN/%smatrix/dense1+2+3/matrix/changed-state GDN/matrix/STOP head%u nonce1..%u ONE shared config/thread/LRegs/physicalDst lifetime\n",head?"":"output/",head,a->sequence);cases+=2;
 }
 for(unsigned w=1;w<120;w++){
  if(!full && w!=1 && w!=2 && w!=24 && w!=25 && w!=96 && w!=119)continue;
  auto m=std::make_unique<MixedPeer>(argv[1],w);assert(m->BootMixed() && m->MatrixMixed());
  if(w<=96)for(unsigned n:{1U,32U,64U,8192U}){
   if(n==8192 && (!full || (w!=1 && w!=24 && w!=96)))continue;
   for(unsigned phase=1;phase<=3;phase++){m->StageDense(phase,n,n==1?3:n==32?7:63);assert(m->FinishMixed(m->sequence));m->VerifyDense();m->InertMixed();}
   FreshInputs(*m);assert(m->MatrixMixed());
  }
  m->StopMixed();stack=std::max(stack,0x70000-m->minStack);cases++;
  printf("PASS ACTUAL mixed matrix/dense1+2+3/changed frame/matrix/STOP worker%u nonce1..%u SAME tensor arrays+source+counters/no reboot or code reload\n",w,m->sequence);
 }
 for(unsigned type:{12U,14U})for(unsigned k:{5120U,6144U,17408U}){
  if(!full && k!=5120)continue;
  auto m=std::make_unique<MixedPeer>(argv[1],1,type,k,3);assert(m->BootMixed() && m->MatrixMixed());
  for(unsigned phase=1;phase<=3;phase++){m->StageDense(phase);assert(m->FinishMixed(m->sequence));m->VerifyDense();}
  FreshInputs(*m);assert(m->MatrixMixed());m->StopMixed();stack=std::max(stack,0x70000-m->minStack);cases++;
  printf("PASS ACTUAL mixed Q%u K%u three matrix groups/dense/matrix/STOP exact independent matrix output and private sequence\n",type,k);
 }
 // Every public envelope word must fail before a following packet after
 // an in-flight change. Private stack capture, not two writable mailboxes.
 for(unsigned kind=0;kind<2;kind++)for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<MixedPeer>(argv[1]);assert(m->BootMixed());m->StageDense(1,64);
  m->envelopeFaultWord=word;if(kind)m->outerBootFaultPacket=1;else m->envelopeFaultPacket=1;
  assert(!m->FinishMixed(m->sequence) && m->densePackets==1);m->InertMixed();faults++;
 }
 for(unsigned op:{1U,2U,9U,11U}){
  auto m=std::make_unique<MixedPeer>(argv[1]);assert(m->BootMixed());m->StageDense(1,32);assert(m->FinishMixed(m->sequence));
  m->StageMixed(op,3,31,m->denseHeader[40]);assert(!m->FinishMixed(m->sequence));m->InertMixed();faults++;
 }
 for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<MixedPeer>(argv[1]);assert(m->BootMixed());m->StageDense(1,64);m->memory[M::Job+word*4]^=1;
  assert(!m->FinishMixed(m->sequence) && !m->densePackets && !m->denseCommands);m->InertMixed();faults++;
 }
 for(unsigned word=0;word<64;word++){
  auto m=std::make_unique<MixedPeer>(argv[1]);assert(m->BootMixed());m->StageDense(1,64);assert(m->FinishMixed(m->sequence));
  m->StageDense(2);m->memory[MWD::Header+word*4]^=1;unsigned packets=m->densePackets,commands=m->denseCommands;
  assert(!m->FinishMixed(m->sequence) && m->densePackets==packets && m->denseCommands==commands);m->InertMixed();faults++;
 }
 for(unsigned c=0;c<8;c++){
  if(c==6)continue; // words2/6 are the SAME physical MMIO counter, not two sources
  auto m=std::make_unique<MixedPeer>(argv[1]);assert(m->BootMixed());m->countBase[c]++;m->StageDense(1,64);
  assert(!m->FinishMixed(m->sequence) && !m->densePackets && !m->denseCommands);m->InertMixed();faults++;
 }
 printf("PASS OFFLINE mixed positives%u/faults%u/maxObservedStack%u;synthetic CONTROL/weights/KV/structured max+inverse only;NO native/full64/token commit/card timing grant\n",cases,faults,stack);
}
#endif
