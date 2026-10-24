/* SPDX-License-Identifier: MIT */
// ACTUAL emitted ONE worker,simulated NoC/GDDR only. Host references OBSERVE
// AFTER execution;no normalized q/k/gates or recurrent products enter image.
#define PIONEER_Q38_RESIDENT_GDN_IMAGE_MODEL_ONLY
#include "pioneer_q38_resident_gdn_image_test.cpp"
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRecurrentShape.h"
#include "../PioneerQ38DeltaStateLayout.h"
#include <memory>
struct GdnRecurrentMachine:ResidentGdnImage {
 std::array<uint32_t,64> inputDescriptor={};
 std::array<uint32_t,324> raw={};std::array<std::array<uint32_t,16>,4> blocks={};
 uint32_t initialCounts[8]={};unsigned prepareWrites=0,prepareHeaderWrites=0,prepareDescriptorWrites=0;
 unsigned poisonFault=0,summaryFault=0,returnWrites=0,boundaryFault=0,boundaryReads=0;
 GdnRecurrentMachine(const char* path,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned bank,unsigned trial=1)
  :ResidentGdnImage(path,trial,0,6,half){
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38GdnInputPlan plan;
  assert(plan.Build(canonical,layer,half,0,bank,id,seals,17));
  for(unsigned i=0;i<64;i++)inputDescriptor[i]=plan.Descriptor()[i];
  descriptor[8]=17;descriptor[9]=half;descriptor[10]=layer;descriptor[11]=trial&1;
  for(unsigned i=16;i<40;i++)descriptor[i]=inputDescriptor[i];
  descriptor[40]=inputDescriptor[40];descriptor[41]=0;
  for(unsigned side=0;side<2;side++){
   bool found=false;unsigned slot=descriptor[11]^side;
   for(unsigned i=0;i<canonical.RecurrentCount();i++){
    const auto& r=canonical.RecurrentRegions()[i];unsigned h=half/2;
    if(r.layer!=layer || r.slot!=slot || h<r.firstHead || h-r.firstHead>=r.heads)continue;
    descriptor[4+side*2]=r.bank;descriptor[5+side*2]=canonical.Base+r.offset+(h-r.firstHead)*65536+(half&1)*32768;
    descriptor[12+side]=PioneerQ38SpanRoute(r.bank);found=true;break;
   }
   assert(found);
  }
  assert(PioneerQ38GdnRecurrentShape(inputDescriptor.data(),descriptor.data()));SetDescriptors();
  for(unsigned i=0;i<8;i++)initialCounts[i]=counts[i];
  unsigned h=half/2,kh=h%16;
  for(unsigned i=0;i<128;i++){
   raw[i]=Bits(float(sin(kh*.117+i*.071+trial*.03)*2.7));
   raw[128+i]=Bits(float(cos(kh*.213+i*.091-trial*.02)*1.9));
  }
  for(unsigned i=0;i<64;i++)raw[256+i]=Bits(float(sin((h*128+(half&1)*64+i)*.035+trial*.13)));
  raw[320]=Bits(float(h)*.13f-2);raw[321]=Bits(float(h)*.07f-2);raw[322]=Bits(-float(h+1)*.25f);raw[323]=Bits(-3.5f);
  for(unsigned p=0;p<4;p++)for(unsigned i=0;i<16;i++)blocks[p][i]=Bits(p==2?-float(i+1)*.25f:float(i+1)*.01f);
  for(unsigned p=0;p<4;p++)blocks[p][inputDescriptor[47]]=raw[320+p];
  for(unsigned i=0;i<324;i++)memory[0x80000+i*4]=0x7fc00000;
  for(unsigned i=0;i<64;i++)memory[0x80800+i*4]=0x7fc00000;
  for(unsigned i=0;i<322;i++)memory[uint32_t(L::Query)+i*4]=0x7fc00000;
  for(unsigned i=0;i<32;i++)memory[TT_Q38_GDN_INPUT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=0xdeadbeef;
  for(unsigned a:{0x63ffcU,0x68000U,0x73ffcU,0x74010U,0x741fcU,0x74380U,0x745fcU,0x74680U,
   0x747fcU,0x74980U,0x7fffcU,0x80510U,0x807fcU,0x80900U,0x8dffcU,0x8e508U,0x8e7fcU,0x8e900U,0x8fffcU,0x98000U,0x9fffcU,0xa8000U})memory[a]=0xdecafbad;
 }
 void SetDescriptors(){
  for(unsigned i=0;i<64;i++){
   memory[TT_Q38_GDN_INPUT_DESCRIPTOR+i*4]=inputDescriptor[i];memory[TT_Q38_RGDN_DESCRIPTOR+i*4]=descriptor[i];
  }
 }
 unsigned Index(uint32_t at)override{
  if(at==0x6000)return 0;
  if(!Range(at,0x64000,unsigned(code.size())) || (at&3))fprintf(stderr,"QRJ1 bad instruction/data address%08x pc%08x steps%u\n",at,pc,steps);
  assert(Range(at,0x64000,unsigned(code.size())) && !(at&3));return 1+(at-0x64000)/4;
 }
 bool CodeAddress(uint32_t at)const override{return at==0x6000 || Range(at,0x64000,unsigned(code.size()));}
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x64000,unsigned(code.size())))return l1[Index(at)];
  if(Range(at,TT_Q38_GDN_INPUT_DESCRIPTOR,256) || Range(at,TT_Q38_GDN_INPUT_SUMMARY,128)
   || Range(at,uint32_t(L::Descriptor),16) || Range(at,0x74600,128) || Range(at,0x80000,1296) || Range(at,0x80800,256))return memory.at(at);
  if(packets==7 && prepareWrites==322){
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x228};
   bool counter=false;for(unsigned offset:offsets)counter|=at==0xffb20000+offset;
   if(counter && ++boundaryReads==9){ // afterQGI's8final reads,before recurrence
    if(boundaryFault){unsigned field=boundaryFault-1;if(field==6)field=2;counts[field]^=1;counts[6]=counts[2];}
    if(summaryFault==7)memory[TT_Q38_GDN_INPUT_SUMMARY+16*4]^=1;
   }
  }
  return ResidentGdnImage::Load(at);
 }
 void SubmitCombined(){
  unsigned p=packets++;assert(p<11);
  if(p<7){
   const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},size[]={512,512,256,64,64,64,64};
   const auto& d=inputDescriptor;
   const unsigned remote[]={d[4]+d[42],d[4]+d[43],d[4]+d[44],d[4]+d[45],d[4]+d[46],d[12],d[15]};
   const unsigned route[]={d[8],d[8],d[8],d[8],d[8],d[13],d[41]};
   const unsigned expected[]={remote[p],0,route[p],local[p],0,d[9],0,0,size[p],0};
   for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
   for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc60000+p*1024+i);
   assert(!unit.ops && !nextWrites && !resultWrites && !returnWrites);
   if(packets==failPacket){pending=true;return;}
   for(unsigned i=0;i<size[p]/4;i++)if(packets!=poisonFault || i!=size[p]/4-1)
    memory[local[p]+i*4]=p<3?raw[(p==0?0:p==1?128:256)+i]:blocks[p-3][i];
   counts[0]++;counts[1]+=size[p]/64;counts[2]++;counts[3]++;counts[6]++;
  }else{
   assert(prepareWrites==322 && prepareDescriptorWrites==5 && prepareHeaderWrites==32 && !returnWrites);
   bool write=p>=9;unsigned piece=(p-7)%2,local=(write?0xa0000:0x90000)+piece*16384;
   unsigned remote=descriptor[write?7:5]+piece*16384;
   const uint32_t expected[]={write?local:remote,0,write?descriptor[15]:descriptor[12],write?remote:local,0,
    write?descriptor[13]:descriptor[15],0,write?0x212U:0,16384,0};
   for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
   if(write)assert(nextWrites==8192 && resultWrites==64 && unit.drained);
   else assert(!unit.ops && !nextWrites && !resultWrites);
   if(packets==failPacket){pending=true;return;}
   for(unsigned i=0;i<4096;i++){
    if(write)next[piece*4096+i]=memory.at(local+i*4);else memory[local+i*4]=prior[piece*4096+i];
   }
   unsigned base=write?4:0;counts[base]++;counts[base+1]+=256;counts[2]++;counts[6]=counts[2];counts[base+3]++;
  }
  if(packets==badCounter)counts[p<9?3:7]++;
  if(packets==foreignCounter)counts[p<9?4:0]++;
  if(packets==sourceFault)seals[11]^=1;
  if(packets==descriptorFault)memory[(p<7?TT_Q38_GDN_INPUT_DESCRIPTOR:TT_Q38_RGDN_DESCRIPTOR)+40*4]^=1;
  if(packets==11 && summaryFault==11)memory[TT_Q38_GDN_INPUT_SUMMARY+16*4]^=1;
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,0xffb20000,40)){
   assert(!pending);command[(at-0xffb20000)/4]=value;
   if(++commandWrites==dropCommand)command[(at-0xffb20000)/4]^=1; // corrupted readback,not an idempotent omitted write
   return;
  }
  if(at==0xffb20040){assert(value==1 && !pending);SubmitCombined();return;}
  if(Range(at,0x80000,1296) || Range(at,0x80800,256) || Range(at,TT_Q38_GDN_INPUT_SUMMARY,128)){memory[at]=value;return;}
  if(Range(at,0x74600,128)){prepareHeaderWrites++;memory[at]=value;return;}
  if(Range(at,uint32_t(L::Query),1288)){assert(packets==7 && !unit.ops);prepareWrites++;memory[at]=value;return;}
  if(Range(at,uint32_t(L::Descriptor),16)){assert(packets==7 && !unit.ops);prepareDescriptorWrites++;memory[at]=value;return;}
  if(Range(at,0x17ffc0,12))returnWrites++;
  ResidentGdnImage::Store(at,value);
 }
 void StepCombined(){
  uint32_t instruction=l1[Index(pc)];
  if((instruction&0x707f)==0x23){ // actual SB from the fixed freestanding descriptor-table memcpy
   assert(++steps<StepLimit());unsigned a=(instruction>>15)&31,b=(instruction>>20)&31;
   uint32_t at=regs[a]+Sign(((instruction>>25)<<5)|((instruction>>7)&31),12);
   assert(Range(at,0x6e000,8192));uint32_t aligned=at&~3U,shift=(at&3)*8;
   memory[aligned]=(memory[aligned]&~(255U<<shift))|((regs[b]&255)<<shift);minStack=std::min(minStack,aligned);pc+=4;return;
  }
  StepScalar();
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepCombined();
  assert(minStack>=0x6e000 && l1[Index(pc)]==0x0000006f);
  for(unsigned a:{0x63ffcU,0x68000U,0x73ffcU,0x74010U,0x741fcU,0x74380U,0x745fcU,0x74680U,
   0x747fcU,0x74980U,0x7fffcU,0x80510U,0x807fcU,0x80900U,0x8dffcU,0x8e508U,0x8e7fcU,0x8e900U,0x8fffcU,0x98000U,0x9fffcU,0xa8000U})assert(memory.at(a)==0xdecafbad);
  unsigned p=packets,reads=nocReads,w=commandWrites,ops=unit.ops,n=nextWrites,results=resultWrites,prep=prepareWrites;
  for(unsigned i=0;i<32;i++)StepCombined();
  assert(p==packets && reads==nocReads && w==commandWrites && ops==unit.ops && n==nextWrites && results==resultWrites && prep==prepareWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_GDN_RECURRENT_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_GDN_RECURRENT_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==64;
 }
 void Verify(){
  assert(packets==11 && !pending && prepareWrites==322 && prepareDescriptorWrites==5 && prepareHeaderWrites==32 && returnWrites==3);
  float qs=0,ks=0;for(unsigned i=0;i<128;i++){float q=Float(raw[i]),k=Float(raw[128+i]);qs+=q*q;ks+=k*k;}
  float qi=1.f/sqrtf(qs+1e-6f),ki=1.f/sqrtf(ks+1e-6f); // no pre-scaling:exact post-dot primitive
  for(unsigned i=0;i<128;i++)for(unsigned kind=0;kind<2;kind++){
   double expected=Float(raw[kind*128+i])*(kind?ki:qi),actual=Float(memory.at(uint32_t(kind?L::Key:L::Query)+i*4));
   assert(isfinite(actual) && fabs(actual-expected)<=3e-6*(1+fabs(expected)));
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(uint32_t(L::Value)+i*4)==raw[256+i]);
  float shifted=Float(raw[320])+Float(raw[323]);double sp=shifted>0?shifted+log1p(exp(-shifted)):log1p(exp(shifted));
  float product=Float(raw[322])*float(sp);double expected[]={exp(product),1/(1+exp(-double(Float(raw[321]))))};
  for(unsigned i=0;i<2;i++){
   double actual=Float(memory.at(uint32_t(i?L::Beta:L::Decay)));
   if(expected[i]>=double(Float(0x00800000)))assert(actual>0 && fabs(actual-expected[i])<=3e-6*expected[i]);else assert(actual==0);
  }
  DeltaVectorImage::Verify(); // independent scalar FP32 checks ALL8192next+64output bits
  for(unsigned i=0;i<8192;i++)assert(prior[i]==memory.at(0x90000+i*4) && next[i]==memory.at(0xa0000+i*4));
  // Independently observe the checkpoint's TRANSPOSED [head][value][key]
  // layout,not a second key-major primitive. No checkpoint/reference bytes
  // were supplied to the image;this happens strictly AFTER its terminal.
  unsigned h=descriptor[9]/2,half=descriptor[9]&1;
  std::vector<uint32_t> checkpoint(PIONEER_Q38_GDN_CHECKPOINT_WORDS,0xdeadbeef),written(checkpoint.size(),0xdeadbeef);
  assert(PioneerQ38GdnExportHalf(prior.data(),unsigned(prior.size()),h,half,checkpoint.data(),unsigned(checkpoint.size()))
   && PioneerQ38GdnExportHalf(next.data(),unsigned(next.size()),h,half,written.data(),unsigned(written.size())));
  float decay=Float(memory.at(uint32_t(L::Decay))),beta=Float(memory.at(uint32_t(L::Beta)));
  for(unsigned value=0;value<64;value++){
   float prediction=0;
   for(unsigned key=0;key<128;key++)prediction+=(Float(checkpoint[h*16384+(half*64+value)*128+key])*decay)*Float(memory.at(uint32_t(L::Key)+key*4));
   float delta=(Float(raw[256+value])-prediction)*beta,output=0;
   for(unsigned key=0;key<128;key++){
    unsigned at=h*16384+(half*64+value)*128+key;
    float updated=Float(checkpoint[at])*decay+Float(memory.at(uint32_t(L::Key)+key*4))*delta;
    assert(written[at]==Bits(updated));output+=updated*Float(memory.at(uint32_t(L::Query)+key*4));
   }
   assert(memory.at(uint32_t(L::Output)+value*4)==Bits(output*(1.f/sqrtf(128.f))));
  }
  uint32_t prep[32],step[32];for(unsigned i=0;i<32;i++){prep[i]=memory.at(TT_Q38_GDN_INPUT_SUMMARY+i*4);step[i]=memory.at(TT_Q38_RGDN_SUMMARY+i*4);}
  assert(PioneerQ38GdnInputSummary(prep,inputDescriptor.data(),initialCounts,step+8)
   && PioneerQ38ResidentGdnSummary(step,descriptor.data(),step+8,counts));
  const unsigned delta[]={9,536,11,9,2,512,11,2};
  for(unsigned i=0;i<8;i++)assert(prep[16+i]==step[8+i] && uint32_t(counts[i]-initialCounts[i])==delta[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4)==inputDescriptor[i] && memory.at(TT_Q38_RGDN_DESCRIPTOR+i*4)==descriptor[i]);
 }
};
int main(int argc,char** argv){
 (void)&TestWorker;assert(argc==2 || (argc==3 && !strcmp(argv[2],"--double-scale-negative")));setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 if(argc==3){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);assert(m.Run());
  float sum=0;for(unsigned i=0;i<128;i++){float q=Float(m.raw[i]);sum+=q*q;}
  float inverse=1.f/sqrtf(sum+1e-6f);unsigned wrong=0;
  for(unsigned i=0;i<128;i++){
   double expected=Float(m.raw[i])*inverse,actual=Float(m.memory.at(uint32_t(L::Query)+i*4));
   if(fabs(actual-expected)>3e-6*(1+fabs(expected)))wrong++;
  }
  assert(wrong>120);puts("PASS ACTUAL superseded double-scale image NEGATIVE:normalized-query observer rejects>120of128values;doNOT qualify prototype as correct combined recurrence");return 0;
 }
 unsigned positive=0,negative=0,maxSteps=0,maxStack=0;
 for(unsigned half=0;half<96;half++){
  GdnRecurrentMachine m(argv[1],*canonical,(half%48)/3*4+half%3,half,half%7,1+half%4);
  assert(m.Run());m.Verify();positive++;maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned half:{0U,1U,94U,95U}){
  std::array<uint32_t,8192> carry={};
  for(unsigned token=0;token<16;token++){
   GdnRecurrentMachine m(argv[1],*canonical,0,half,token%7,1+token);
   m.prior=carry;assert(m.Run());m.Verify();carry=m.next;positive++;
  }
 }
 printf("PASS ACTUAL OFFLINE QRJ1 combined worker positives%u/all96halves/7banks/bothslots/four16stepNONRESETTINGchains/ALL8256bits exact+transposed-checkpoint-observer;rawQKV+originalA-dt->QK/gates->CURRENTstate->SFPU->NEXT,11packets9reads2writes/NO host stage boundary;maxSteps%u/maxStack%u;NOT native/card/tokencommit/speed\n",positive,maxSteps,maxStack);
 for(unsigned kind=0;kind<5;kind++)for(unsigned packet=1;packet<=11;packet++){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);
  if(!kind)m.failPacket=packet;if(kind==1)m.badCounter=packet;if(kind==2)m.foreignCounter=packet;
  if(kind==3)m.sourceFault=packet;if(kind==4)m.descriptorFault=packet;
  assert(!m.Run() && m.packets==packet);negative++;
 }
 for(unsigned packet=1;packet<=7;packet++){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);m.poisonFault=packet;assert(!m.Run() && m.packets==packet && !m.unit.ops);negative++;
 }
 for(unsigned side=0;side<2;side++)for(unsigned field=0;field<64;field++)for(unsigned bit=0;bit<32;bit++){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);(side?m.descriptor:m.inputDescriptor)[field]^=1U<<bit;
  if(PioneerQ38GdnRecurrentShape(m.inputDescriptor.data(),m.descriptor.data()))continue;
  m.SetDescriptors();assert(!m.Run() && !m.packets && !m.nocReads && !m.unit.ops && !m.nextWrites);negative++;
 }
 for(unsigned word=1;word<=110;word++){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);m.dropCommand=word;assert(!m.Run() && m.packets==(word-1)/10);negative++;
 }
 for(unsigned packet:{7U,11U}){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);m.summaryFault=packet;assert(!m.Run() && m.packets==packet);negative++;
 }
 for(unsigned field=1;field<=8;field++){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);m.boundaryFault=field;
  assert(!m.Run() && m.packets==7 && !m.unit.ops && !m.nextWrites);negative++;
 }
 for(unsigned at:{0U,8191U}){
  GdnRecurrentMachine m(argv[1],*canonical,0,95,6);m.prior[at]=0x7fc00000;
  assert(!m.Run() && m.packets==9 && !m.unit.ops && !m.nextWrites);negative++;
 }
 printf("PASS ACTUAL OFFLINE QRJ1 faults%u:all11timeouts/counters/sources/descriptors,7rawpoison,ALLpaired-descriptorbits,110command-readback-corruptions,8boundary-counter-faultsBEFOREstateread,seam-summary+nonfinite-state;inertfaultNO laterNoC/SFPU/publication;NO initialized/native/current-stateauthority\n",negative);
}
