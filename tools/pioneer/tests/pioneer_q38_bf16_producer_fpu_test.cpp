/* SPDX-License-Identifier: MIT */
// Actual returning gated producer -> canonical byte transport -> returning
// full-K matrix,including the SAME worker's real private ordering history.
// Synthetic raw gate/up/weights. NOT executed ROOT/native/full-model/TPS.
#define PIONEER_BF16_FULL_FPU_MACHINE_ONLY
#include "pioneer_q38_bf16_worker_fpu_test.cpp"
#include <map>
namespace BP=PioneerQ38Bf16ProducerLayout;
struct BfFabric {
 std::vector<uint8_t> root=std::vector<uint8_t>(0x180000,0xa7);
 std::map<uint64_t,uint32_t> dram;
 static uint64_t Key(unsigned route,unsigned at){return uint64_t(route)<<32|at;}
 unsigned Get(unsigned route,unsigned at)const{return route==129?::Word(root,at):dram.at(Key(route,at));}
 void Put(unsigned route,unsigned at,unsigned value){assert(route!=129);dram[Key(route,at)]=value;}
 void TransportToRoot(unsigned route,unsigned remote,unsigned local,unsigned bytes){
  assert(!(bytes&3) && local+bytes<=root.size());
  for(unsigned i=0;i<bytes;i+=4){const unsigned value=Get(route,remote+i);for(unsigned b=0;b<4;b++)root[local+i+b]=uint8_t(value>>(b*8));}
 } // byte transport ONLY,no packer,activation,expected product or ready setter.
};
struct ProducerFpuPeer:FullFpuPeer {
 BfFabric& fabric;bool producing=false;unsigned producerPackets=0;uint32_t fp[32]={};
 std::array<uint32_t,64> descriptor={};std::array<uint32_t,8> beforeProducer={};
 ProducerFpuPeer(const char* image,const char* leaf,CachedPipeline& engine,const CL::Job& job,BfFabric& shared)
  :FullFpuPeer(image,leaf,engine,job,shared.root),fabric(shared){}
 static float Float(unsigned bits){float value;memcpy(&value,&bits,4);return value;}
 static float Flush(float value){const unsigned bits=Q4PipelineFixture::Bits(value);return bits&0x7f800000?value:Float(bits&0x80000000);}
 unsigned Load(unsigned a)override{
  if(producing && (Range(a,BP::Descriptor,256) || Range(a,0x90000,3712) || Range(a,0x74000,96)))return Get(a);
  return FullFpuPeer::Load(a);
 }
 void ProducerPacket(){
  const unsigned reads=BP::Reads(descriptor.data()),p=producerPackets++;
  assert(p<reads+2 && native.Clean() && bus.queued.empty() && bus.responses.empty());
  const bool write=p>=reads,trailer=p==reads+1;
  const unsigned local=write?(trailer?0x90e00:0x90c00):(p?0x90400:0x90000),bytes=write?(trailer?128U:512U):1024U;
  const bool rootInput=!write && reads==1;
  const unsigned offset=write?descriptor[trailer?13:12]:descriptor[10+p],remote=rootInput?offset:descriptor[4]+offset;
  const unsigned expected[]={write?local:remote,0,write?boot[4]:rootInput?129U:descriptor[8],write?remote:local,0,
   write?descriptor[8]:boot[4],0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(bus.command[i]==expected[i]);
  for(unsigned i=0;i<bytes/4;i++){
   if(write)fabric.Put(descriptor[8],remote+i*4,Get(local+i*4));
   else Put(local+i*4,fabric.Get(rootInput?129U:descriptor[8],remote+i*4));
  }
  // Actual transport counters advance on this response,not on a caller
  // completion flag. They remain live into the subsequent FPU loader.
  bus.counters[4]++;bus.counters[14]++;
  if(write){bus.counters[1]++;bus.counters[8]+=bytes/64;bus.counters[10]++;}
  else{bus.counters[2]++;bus.counters[3]+=bytes/64;bus.counters[5]++;}
 }
 void Store(unsigned a,unsigned value)override{
  if(producing && Range(a,0xffb20000,40)){bus.command[(a-0xffb20000)/4]=value;return;}
  if(producing && a==0xffb20040){assert(value==1);ProducerPacket();return;}
  if(producing && Range(a,0x90800,1664)){Put(a,value);return;}
  FullFpuPeer::Store(a,value);
 }
 void StepWorker()override{
  const unsigned i=l1[Index(pc)],op=i&127;
  if(op!=7 && op!=0x27 && op!=0x53){FullFpuPeer::StepWorker();return;}
  // SAME scalar RV32F semantics as ScalarMachine::StepScalar. This extends
  // the integer-only cached-FPU harness;no instruction is skipped and no
  // mathematical product is substituted for the emitted producer code.
  assert(producing && ++steps<StepLimit() && CodeAddress(pc));
  const unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(i>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else{
   const float a=Flush(Float(fp[s])),b=Flush(Float(fp[t]));
   switch(hi){
    case 0:assert(f==7 || !f);fp[d]=Q4PipelineFixture::Bits(Flush(a+b));break;
    case 4:assert(f==7 || !f);fp[d]=Q4PipelineFixture::Bits(Flush(a-b));break;
    case 8:assert(f==7 || !f);fp[d]=Q4PipelineFixture::Bits(Flush(a*b));break;
    case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
    case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
    case 0x68:assert(t<=1);fp[d]=Q4PipelineFixture::Bits(t?float(regs[s]):float(int32_t(regs[s])));break;
    case 0x70:assert(!t && !f);if(d)regs[d]=fp[s];break;
    case 0x78:assert(!t && !f);fp[d]=regs[s];break;
    default:fprintf(stderr,"unqualified scalar FP%08x PC%08x\n",i,pc);assert(false);
   }
  }
  regs[0]=0;pc+=4;assert(regs[27]==seal);Progress();
  assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);
 }
 void StageProducer(unsigned phase){
  assert(pc==waitPC && !producing && !producerPackets);producing=true;
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)beforeProducer[i]=bus.counters[indices[i]];
  auto& d=descriptor;const bool gated=phase==4;
  d[0]=BP::Magic;d[1]=1;d[2]=67108864;d[3]=0;d[4]=0x4000000;d[5]=boot[3]-1;d[6]=2025;d[7]=boot[3];d[8]=PioneerQ38ProjectionRoute(0);d[9]=boot[4];
  d[10]=(gated?unsigned(BP::GateOffset):unsigned(BP::ProjectionInput))+d[5]*1024;d[11]=gated?BP::UpOffset+d[5]*1024:5120;
  d[12]=BP::NativeOffset+d[5]*512;d[13]=BP::LinesOffset+d[5]*128;d[14]=BP::Format;d[15]=gated?68:20;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];d[41]=248319;d[43]=phase;
  assert(BP::DescriptorShape(d.data()));std::array<uint32_t,64> j={};
  const unsigned h[]={BW::Magic,1,gated?3U:5U,boot[3],boot[4],0,0,2025};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=boot[8+i];}
  j[16]=BP::Magic;j[17]=256;j[18]=512;j[19]=2025;j[20]=12;j[27]=d[41];j[28]=phase;j[29]=gated?17408:5120;j[30]=BP::Format;j[31]=BP::Witness;
  assert(BW::JobShape(j.data(),boot.data(),0,0));
  for(unsigned i=0;i<64;i++){Put(BP::Descriptor+i*4,d[i]);Put(BW::Job+i*4,j[i]);}Put(BW::Doorbell,1);
 }
 void FinishProducer(){
  Finish(1);assert(producerPackets==BP::Reads(descriptor.data())+2);uint32_t proof[64],job[16],after[8],line[32];
  for(unsigned i=0;i<64;i++)proof[i]=Get(0x83f00+i*4);for(unsigned i=0;i<16;i++)job[i]=Get(0x74000+i*4);
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)after[i]=bus.counters[indices[i]];
  assert(BP::Proof(proof,job,descriptor.data(),source.data(),boot[3],boot[4],beforeProducer.data(),after));
  for(unsigned i=0;i<32;i++)line[i]=fabric.Get(descriptor[8],descriptor[4]+descriptor[13]+i*4);
  assert(BP::Line(line,descriptor.data(),1));producing=false;
 }
};
int main(int argc,char** argv){
 assert(argc==3);BfFabric fabric;unsigned producers=0,packets=0,maxStack=0;
 const unsigned route=PioneerQ38ProjectionRoute(0),workspace=0x4000000;
 for(unsigned i=0;i<17408;i++){
  fabric.Put(route,workspace+BP::GateOffset+i*4,Q4PipelineFixture::Bits(float(int(i%257)-128)*.03125f));
  fabric.Put(route,workspace+BP::UpOffset+i*4,Q4PipelineFixture::Bits(float(int((i*17)%257)-128)*.015625f));
 }
 PioneerPairedPartProbe part;assert(ProbePairedPart(0,4,1,part));
 auto engine=std::make_unique<CachedPipeline>();auto job=Job(17408,part.groups,1,PioneerQ38ProjectionRoute(part.bank));job.words[3]=part.address;job.words[11]=1;
 auto first=std::make_unique<ProducerFpuPeer>(argv[1],argv[2],*engine,job,fabric);first->Boot();
 for(unsigned w=1;w<=68;w++){
  std::unique_ptr<CachedPipeline> extraEngine;std::unique_ptr<ProducerFpuPeer> extra;
  ProducerFpuPeer* p=first.get();if(w>1){extraEngine=std::make_unique<CachedPipeline>();auto j=Job(17408,2,w,route);j.words[11]=1;extra=std::make_unique<ProducerFpuPeer>(argv[1],argv[2],*extraEngine,j,fabric);p=extra.get();p->Boot();}
  p->StageProducer(4);p->FinishProducer();producers++;packets+=p->producerPackets;maxStack=std::max(maxStack,0x70000-p->minSP);
  if(w>1)p->Stop(2,0);
 }
 fabric.TransportToRoot(route,workspace+BP::NativeOffset,CL::InputBase,17408*2);
 // Numeric reference is POSTHOC observer only. The guest sees only raw
 // packed weights and the bytes written by the actual returning producers.
 job.words[11]=2;CachedData data(17408,job.words[4],0);data.expected.assign(job.words[4]/2,{});
 for(unsigned pair=0;pair<job.words[4]/2;pair++)for(unsigned block=0;block<68;block++)for(unsigned row=0;row<32;row++){
  float weights[256];assert(PioneerDecodeQ4K(data.pairs[pair][block].original.data()+row*144,weights));
  for(unsigned group=0;group<16;group++){
   uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){
    a[c]=uint16_t(Round(Q4PipelineFixture::Bits(weights[group*16+c]))>>16);
    const unsigned at=CL::InputBase+(block*256+group*16+c)*2;b[c]=uint16_t(fabric.root[at]|unsigned(fabric.root[at+1])<<8);
   }
   for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,data.expected[pair][row],data.expected[pair][row]));
  }
 }
 first->Stage(job,4,part.firstRow);first->bus.dramBase=job.words[3];data.StageCache(first->bus);first->Finish(2);
 for(unsigned pair=0;pair<job.words[4]/2;pair++)for(unsigned row=0;row<32;row++)assert(first->Get(CL::Output+(pair*32+row)*4)==data.expected[pair][row]);
 maxStack=std::max(maxStack,0x70000-first->minSP);first->Stop(3,1);
 printf("PASS ACTUAL CONNECTED gated producers%u/packets%u/17408 BF16 values -> SAME worker1 sealed private history and live NoC/FPU/L1 -> fullK17408 Q4 matrix rows%u/exact independent arithmetic/real STOP/maxCombinedStack%u;byte transport only,no separate pack image or expected-product upload;NOT executed ROOT/all119/full-model/native/hardware/TPS proof\n",producers,packets,part.groups*16,maxStack);
}
