/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DENSE_PREPARE_MACHINE_ONLY
#include "pioneer_q38_dense_prepare_image_test.cpp"
#include "pioneer_q38_dense_attention_sfpu_model.h"
#include "../PioneerQ38DenseAttentionLayout.h"
namespace A=PioneerQ38DenseAttentionLayout;
static float HalfValue(uint16_t bits){_Float16 x;memcpy(&x,&bits,2);return float(x);}
struct DenseAttentionMachine:DensePrepareMachine {
 DenseSfpuModel sfpu;
 std::vector<uint16_t> priorK,priorV;
 std::vector<unsigned> scoreWrites;
 std::vector<uint32_t> actualScores,actualExp;
 uint32_t command[10]={},counts[8]={},before[8]={},source[16]={};
 unsigned packets=0,failPacket=0,badCounter=0,sourceFault=0,headerFault=0,finalWrites=0;
 bool pending=false,attentionStarted=false;
 DenseAttentionMachine(const char* image,unsigned head,unsigned tokens,unsigned trial=0):DensePrepareMachine(image,A::Group(head),tokens-1,trial),priorK((tokens-1)*256),priorV((tokens-1)*256),scoreWrites(tokens),actualScores(tokens),actualExp(tokens){
  header.fill(0);const unsigned start[]={A::Magic,1,64,trial&1?63U:3U,tokens-1,head,A::Group(head),tokens,
   0x10000000,0x20000000,0,1,A::Route(0),A::Route(1),A::Peer(head),head+1};
  for(unsigned i=0;i<16;i++){header[i]=start[i];source[i]=header[16+i]=0x16005+i*0x13579;}
  for(unsigned i=32;i<40;i++)header[i]=i-31;
  const unsigned tail[]={17,256,0x3d800000,32,8192,16384,2*((tokens+30)>>5),(tokens-1)*16};
  for(unsigned i=0;i<8;i++)header[40+i]=tail[i];assert(A::Shape(header.data()));SetHeader();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(unsigned i=0;i<priorK.size();i++){
   priorK[i]=HalfReference(float(sin((i+head*37)*.031)*1.3));priorV[i]=HalfReference(float(cos((i+head*71)*.017)*1.7));
   if(trial==4){priorK[i]=uint16_t(i&1023);priorV[i]=uint16_t((i&1?0x8000:0)|(i&1023));}
   if(trial==5){priorK[i]=priorV[i]=uint16_t((i&1?0x8000:0)|0x7bff);}
  }
  for(unsigned a=A::Scores;a<A::Scores+32768;a+=4)memory[a]=0xdeadbeef;
  for(unsigned a=A::Accumulate;a<A::Accumulate+2048;a+=4)memory[a]=0xdeadbeef;
  for(unsigned a=A::PreGate;a<A::Output+1024;a+=4)memory[a]=0xdeadbeef;
  for(unsigned a=A::Chunk;a<A::Chunk+16384;a+=4)memory[a]=0x7fc00000;
  for(unsigned i=0;i<64;i++)memory[A::Summary+i*4]=0;
  for(unsigned a:std::array<unsigned,5>{A::Summary-4,A::Summary+256,A::Accumulate+2048,A::Output+1024,A::Chunk+16384})memory[a]=0xdecafbad;
 }
 static unsigned SourceOffset(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,12*64))return sfpu.Load(a);
  if(Range(a,0xffb20000,0x2000)){
   unsigned o=a-0xffb20000;if(o<40)return command[o/4];
   for(unsigned i=0;i<4;i++)if(o==0x40+i*0x800)return i?0:unsigned(pending);
   if(o>=0x240 && o<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(o==SourceOffset(i))return source[i];
   const unsigned at[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(o==at[i])return counts[i];assert(!"unqualified dense NoC read");
  }
  if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,A::Header,256) || Range(a,A::Summary,256) || Range(a,L::Qg,16640)
   || Range(a,A::Scores,32768) || Range(a,A::Accumulate,2048) || Range(a,A::PreGate,2048)
   || Range(a,A::Chunk,16384) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(a==0xffe40000 || a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,8*64)){
   if(a==0xffe40000 && !attentionStarted){assert(value==0xb2000000);attentionStarted=true;}
   sfpu.Store(a,value);return;
  }
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(a,0x6e000,8192) || Range(a,A::Summary,256) || Range(a,L::Qg,4096) || Range(a,A::Scores,32768)
   || Range(a,A::Accumulate,2048) || Range(a,A::PreGate,2048) || Range(a,A::Chunk,16384) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,A::PreGate,2048))finalWrites++;
  if(attentionStarted && Range(a,A::Scores,32768)){
   unsigned t=(a-A::Scores)/4;assert(t<scoreWrites.size());
   if(!scoreWrites[t])actualScores[t]=value;else if(scoreWrites[t]==1)actualExp[t]=value;
   assert(++scoreWrites[t]<=3);
  }
  memory[a]=value; // NO input-header/code/reset/PCI/canonical-KV write
 }
 void Submit(){
  unsigned per=(header[4]+31)>>5;assert(per && packets<per*2);unsigned which=packets/per,chunk=packets%per,first=chunk*32;
  unsigned count=std::min(32U,header[4]-first),bytes=count*512;
  const unsigned expected[]={header[8+which]+first*512,0,header[12+which],A::Chunk,0,header[14],0,0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);packets++;
  if(packets==failPacket){pending=true;return;}
  const auto& values=which?priorV:priorK;
  for(unsigned i=0;i<bytes/4;i++)memory[A::Chunk+i*4]=values[first*256+i*2]|(uint32_t(values[first*256+i*2+1])<<16);
  counts[0]++;counts[1]+=bytes/64;counts[2]++;counts[3]++;counts[6]++;
  if(packets==badCounter)counts[3]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==headerFault)memory[A::Header+40*4]^=1;
 }
 unsigned StepLimit()const override{return 2000000000U;}
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:std::array<unsigned,5>{A::Summary-4,A::Summary+256,A::Accumulate+2048,A::Output+1024,A::Chunk+16384})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=1024;i<raw.size();i++)assert(memory.at(L::Qg+i*4)==raw[i]);
  unsigned p=packets,c=sfpu.commands,w=finalWrites;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && c==sfpu.commands && w==finalWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==A::Fault && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==A::Magic && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==header[7];
 }
 double Verify(bool require=true){
  assert(!pending && sfpu.drained && finalWrites==512 && packets==header[46] && memory.at(A::Summary)==A::Magic);
  unsigned head=header[5]-header[6]*6;auto q=Reference(L::Qg+head*2048,L::QWeight);auto k=Reference(L::Key,L::KWeight);
  for(unsigned i=0;i<256;i++){
   assert(memory.at(A::Query+i*4)==PioneerAttentionBits(q[i]));
   assert(memory.at(A::Gate+i*4)==raw[head*512+256+i]);
   assert(memory.at(A::HalfQuery+i*4)==PioneerAttentionBits(float(_Float16(q[i]))));
  }
  for(unsigned i=0;i<128;i++){
   assert(memory.at(A::NewK+i*4)==(HalfReference(k[i*2])|(uint32_t(HalfReference(k[i*2+1]))<<16)));
   assert(memory.at(A::NewV+i*4)==(HalfReference(Raw(L::Value+i*8))|(uint32_t(HalfReference(Raw(L::Value+i*8+4)))<<16)));
  }
  std::vector<float> scores(header[7]);float maximum=-INFINITY;double denominator=0;
  for(unsigned t=0;t<header[7];t++){
   double sum=0;for(unsigned d=0;d<256;d++){
    float key=t<header[4]?HalfValue(priorK[t*256+d]):float(_Float16(k[d]));sum+=double(float(_Float16(q[d]))*key);
   }
   scores[t]=float(sum)*.0625f;maximum=fmaxf(maximum,scores[t]);
   if(require)assert(scoreWrites[t]==3 && actualScores[t]==PioneerAttentionBits(scores[t]));
  }
  for(unsigned t=0;t<scores.size();t++){
   scores[t]=expf(scores[t]-maximum);denominator+=double(scores[t]);
   if(require){double actual=PioneerAttentionFloat(actualExp[t]);assert(isfinite(actual) && fabs(actual-scores[t])<=1e-6);}
  }
  float inverse=float(1.0/denominator);for(float& s:scores)s=float(_Float16(s*inverse));
  if(require)for(unsigned t=0;t<scores.size();t++)assert(memory.at(A::Scores+t*4)==PioneerAttentionBits(scores[t]));
  double delta=0,norm=0;
  for(unsigned d=0;d<256;d++){
   double sum=0;for(unsigned t=0;t<header[7];t++){
    float value=t<header[4]?HalfValue(priorV[t*256+d]):float(_Float16(Raw(L::Value+d*4)));sum+=double(scores[t]*value);
   }
   float expected=float(sum),actual=PioneerAttentionFloat(memory.at(A::PreGate+d*4));assert(isfinite(actual));
   delta+=(double(actual)-expected)*(double(actual)-expected);norm+=double(expected)*expected;
   if(require){
    float gate=Raw(L::Qg+(head*512+256+d)*4);double e=exp(-fabs(double(gate))),sigmoid=gate>=0?1/(1+e):e/(1+e);
    float wanted=actual*float(sigmoid),got=PioneerAttentionFloat(memory.at(A::Output+d*4));
    if(fabsf(wanted)>=0x1p-126f)assert(isfinite(got) && fabs(double(got)-wanted)<=3e-6*fabs(double(wanted)));
    else assert(got==0);
   }
  }
  double error=sqrt(delta/(norm?norm:1));
  if(require && !(error<2e-5))fprintf(stderr,"DENSE_ATTENTION_NUMERIC head%u tokens%u L2%.12g\n",header[5],header[7],error);
  if(require)assert(error<2e-5);
  for(unsigned i=0;i<8;i++){
   unsigned change=i==1?header[47]:i==0 || i==2 || i==3 || i==6?header[46]:0;
   assert(uint32_t(counts[i]-before[i])==change && memory.at(A::Summary+(8+i)*4)==before[i] && memory.at(A::Summary+(16+i)*4)==counts[i]);
  }
  assert(memory.at(A::Summary+6*4)==header[46] && memory.at(A::Summary+7*4)==header[47] && memory.at(A::Summary+51*4)==0);
  return error;
 }
};
#ifndef PIONEER_Q38_DENSE_ATTENTION_MACHINE_ONLY
int main(int argc,char** argv){
 (void)&D::Run; // included HOST helper is not executed or used as feedback
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--full")));setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned positives=0,refusals=0,maxSteps=0,maxStack=0;double maximum=0;
 for(unsigned head=0;head<24;head++){
  unsigned tokens=head%3==0?1:head%3==1?17:33;DenseAttentionMachine m(argv[1],head,tokens);assert(m.Run());maximum=std::max(maximum,m.Verify());positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  printf("PASS ACTUAL wide attention head%u tokens%u/currKVfromACTUAL-QDP/noKVwrite;NOT inference timing\n",head,tokens);
 }
 for(unsigned n:{2U,31U,32U,65U,128U,257U}){
  DenseAttentionMachine m(argv[1],23,n);assert(m.Run());maximum=std::max(maximum,m.Verify());positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  printf("PASS ACTUAL wide boundary tokens%u;NOT inference timing\n",n);
 }
 for(unsigned trial=1;trial<=5;trial++){
  DenseAttentionMachine m(argv[1],23,129,trial);assert(m.Run());maximum=std::max(maximum,m.Verify());positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  printf("PASS ACTUAL wide numeric trial%u tokens129/per-tokenQK+probability bits/whole-output oracle\n",trial);
 }
 for(unsigned fault=0;fault<4;fault++)for(unsigned packet:{1U,3U,4U,6U}){
  DenseAttentionMachine m(argv[1],0,66);if(fault==0)m.failPacket=packet;if(fault==1)m.badCounter=packet;
  if(fault==2)m.sourceFault=packet;if(fault==3)m.headerFault=packet;
  bool ok=m.Run();
  if(ok || m.packets!=packet || m.finalWrites || m.memory.at(A::Summary))
   fprintf(stderr,"DENSE_FAULT_CHECK kind%u packet%u ok%u actualPackets%u finalWrites%u summary%08x\n",fault,packet,ok,m.packets,m.finalWrites,m.memory.at(A::Summary));
  assert(!ok && m.packets==packet && !m.finalWrites && !m.memory.at(A::Summary));refusals++;
 }
 for(unsigned field=0;field<64;field++){
  DenseAttentionMachine m(argv[1],0,17);m.header[field]^=0x80000000U;
  if(A::Shape(m.header.data()) && !(field>=16 && field<32))continue;
  m.SetHeader();assert(!m.Run() && !m.packets && !m.finalWrites && !m.sfpu.commands);refusals++;
 }
 for(bool key:{false,true})for(unsigned last:{0U,1U}){
  DenseAttentionMachine m(argv[1],0,65);auto& data=key?m.priorK:m.priorV;data[last?data.size()-1:0]=0x7c00;
  assert(!m.Run() && !m.finalWrites && !m.memory.at(A::Summary));refusals++;
 }
 {DenseAttentionMachine m(argv[1],0,1);m.sfpu.dropAck=true;assert(!m.Run() && !m.packets && !m.finalWrites);refusals++;}
 for(bool transpose:{false,true}){
  DenseAttentionMachine m(argv[1],0,17);m.sfpu.dropProduct=!transpose;m.sfpu.transpose=transpose;assert(m.Run());assert(m.Verify(false)>2e-5);refusals++;
 }
 for(float gate:{-80.f,-81.f,-84.f,-87.f,-88.f,80.f,65504.f,-65504.f}){
  DenseAttentionMachine m(argv[1],0,1);for(unsigned d=0;d<256;d++)m.raw[256+d]=PioneerAttentionBits(gate);m.SetRaw();
  assert(m.Run());maximum=std::max(maximum,m.Verify());positives++;
 }
 if(argc==3){
  puts("BEGIN ACTUAL WIDE_FULL8192 one complete head,real SFPU+READONLY prefix+provisional currentKV;NOT silicon/time");
  for(unsigned trial:{0U,3U,4U}){
   DenseAttentionMachine m(argv[1],23,8192,trial);assert(m.Run());maximum=std::max(maximum,m.Verify());positives++;
   maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
   printf("PASS ACTUAL WIDE_FULL8192 trial%u/512reads/131056flits/EVERYQK+probability bits/noKVwrite/nohost intermediate products\n",trial);
  }
 }
 printf("PASS ACTUAL QDA1 wide256 streaming QDP/QK/softmax/PV/sigmoid positives%u/refusals%u/maxSteps%u/maxStack%u/maxIndependentL2%.9g;24heads/4KV,currenttokenKV PROVISIONAL fromactualprep,READONLY priorprefix;NO original-model/native/committed64layer-token/Work/silicon/inference-speed grant\n",positives,refusals,maxSteps,maxStack,maximum);
}
#endif
