/* SPDX-License-Identifier: MIT */
#define PIONEER_Q4_COMPACT_STREAM_LOADER_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_loader_test.cpp"
#define PIONEER_Q38_COMPACT_FPU_PIPELINE_MACHINE_ONLY
#include "pioneer_q38_q4_fpu_compact_pipeline_test.cpp"
#include <memory>
struct CompactStreamPipeline:CompactFpuPipelineMachine {
 StreamIO* bus=nullptr;unsigned blocks=0;mutable unsigned readerStart=0;
 unsigned OperandOffset()const override{return ((inputLoads/8)&1)*0x10000;}
 bool OperandReady(unsigned address,unsigned bytes)const override{
  if(address==0x97000)return bytes==1024;
  const unsigned slot=(inputLoads/8)&1;
  const bool valid=bus && bus->ready[slot] && bus->copies[slot]==SL::Packets && bus->dataBlock[slot]==(inputLoads/8)%blocks
   && address>=SL::Raw+slot*0x10000 && address+bytes<=0x963c0+slot*0x10000;
  if(valid && address==SL::Metadata+slot*0x10000 && !(compactImports%2)){bus->reader=slot;readerStart=matrixOps;}
  return valid && bus->reader==slot;
 }
};
struct CompactStreamEmitter {
 CompactStreamPipeline& machine;StreamIO& bus;
 void Op(unsigned op){machine.Op(op);bus.Progress();}void Config(unsigned i,unsigned v){machine.Config(i,v);bus.Progress();}
 void Drain(){machine.Drain();bus.Progress();}
};
struct CompactStreamData {
 std::vector<Q4PipelineFixture> blocks;std::array<uint32_t,32> expected{};
 explicit CompactStreamData(unsigned width,unsigned trial){for(unsigned k=0;k<width/256;k++)blocks.emplace_back(trial+k);Refresh();}
 void Refresh(){
  expected.fill(0);for(auto& block:blocks)block.Refresh();
  for(const auto& block:blocks)for(unsigned row=0;row<32;row++){
   float weight[256];assert(PioneerDecodeQ4K(block.original.data()+row*144,weight));
   for(unsigned first=0;first<256;first+=16){
    uint16_t a[16],b[16];for(unsigned i=0;i<16;i++){a[i]=uint16_t(Round(Q4PipelineFixture::Bits(weight[first+i]))>>16);b[i]=block.input[first+i];}
    for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]));
   }
  }
 }
 void Stage(StreamIO& io)const{
  assert(io.width/256==blocks.size());
  for(unsigned k=0;k<blocks.size();k++){
   std::array<uint8_t,4608> compact;
   assert(PioneerQ38Q4FpuCompactData::Prepare(blocks[k].original.data(),blocks[k].original.size(),compact.data(),compact.size()));
   memcpy(io.dram.data()+k*4608,compact.data(),compact.size());
   for(unsigned i=0;i<256;i++){
    const unsigned a=SL::DramInput-SL::DramWeight+k*512+i*2;io.dram[a]=uint8_t(blocks[k].input[i]);io.dram[a+1]=uint8_t(blocks[k].input[i]>>8);
   }
  }
  memset(io.l1.data()+SL::Zero-64,0,1088);
 }
};
#ifndef PIONEER_Q4_COMPACT_STREAM_PIPELINE_ONLY
int main(){
 unsigned passed=0;
 for(unsigned width:{5120U,17408U}){
  CompactStreamData data(width,0);auto m=std::make_unique<CompactStreamPipeline>();m->blocks=width/256;
  std::fill(m->l1.begin(),m->l1.end(),0xa7);m->delay[0]=100003;m->delay[1]=7;
  for(unsigned pass=0;pass<2;pass++){
   StreamIO io(width,17|(12<<6),&m->l1);m->bus=&io;data.Stage(io);Loader loader;CompactStreamEmitter e{*m,io};
   assert(loader.Begin(io,width,io.route) && loader.Open(io,0,0));for(unsigned s=0;s<2;s++)assert(loader.Slice(io));assert(loader.Wait(io));
   PioneerQ38Q4FpuCompactStreamCompute::Begin(e);
   for(unsigned block=0;block<width/256;block++){
    const unsigned buffer=block&1;const bool next=block+1<width/256;assert(loader.Acquire(block,buffer));io.reader=buffer;
    if(next){assert(loader.Open(io,block+1,buffer^1));assert(loader.Slice(io));}
    assert(PioneerQ38Q4FpuCompactStreamCompute::LoadMetadata(e,buffer));
    for(unsigned g=0;g<8;g++){if(next && !g)assert(loader.Slice(io));assert(PioneerQ38Q4FpuCompactStreamCompute::Group(e,buffer,g));}
    io.reader=2;assert(loader.Release(block,buffer));if(next)assert(loader.Wait(io));
   }
   assert(loader.Complete(io) && m->Clean() && m->Results()==data.expected && io.overlap);
   assert(m->zeroLoads==5*(pass+1) && m->extraZeroLoads==7*(pass+1) && m->metadataLoads==0
    && m->compactImports==2*(width/256)*(pass+1));NoMore(loader,io);passed++;
  }
  printf("PASS_EMITTED_COMPACT_FULL_K width%u twoJobs_all64Exact_oneInitializationPerJob_canonicalNoC_L1_Dst_and_accumulatorPreserved;NO_NATIVE_QUALITY_OR_SPEED\n",width);
 }
 CompactStreamData data(5120,3);auto m=std::make_unique<CompactStreamPipeline>();m->blocks=20;
 StreamIO io(5120,17|(12<<6),&m->l1);m->bus=&io;data.Stage(io);Loader loader;
 assert(loader.Begin(io,5120,io.route) && loader.Open(io,0,0));for(unsigned s=0;s<2;s++)assert(loader.Slice(io));
 CompactStreamEmitter e{*m,io};PioneerQ38Q4FpuCompactStreamCompute::Begin(e);bool refused=false;
 try{PioneerQ38Q4FpuCompactStreamCompute::LoadMetadata(e,0);}catch(Q4FpuPipelineMachine::Fault){refused=true;}
 assert(refused && !m->matrixOps && !m->compactImports && !m->outputReads && passed==4);
 puts("PASS_COMPACT_FULL_K_MISSING_PRODUCER_VISIBILITY_REFUSED_BEFORE_RAW_METADATA_OR_MATH;NOT_NATIVE_AUTHORITY");
}
#endif
