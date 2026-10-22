/* SPDX-License-Identifier: MIT */
// THREE actual emitted images across simulated GDDR. This qualifies the
// transport/middle chain ONLY,not the learned gate/up/down matrix arithmetic,
// native scheduling,workspace authority,full FFN/model or throughput.
#define PIONEER_Q38_FFN_OUTPUT_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_output_image_test.cpp"
#define PIONEER_Q38_FFN_GATHER_RESIDENT_MACHINE_INCLUDED
#define PIONEER_Q38_FFN_GATHER_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_gather_image_test.cpp"
int main(int argc,char** argv) {
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned sinks=0,producers=0,gathers=0;
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}) {
  const unsigned layer=bank*9+unsigned(edge);
  std::vector<uint32_t> gate(17408),up(17408);std::vector<float> activated(17408);
  // These scoped numerical fixtures stand in for matrix outputs. The
  // matrix producer is deliberately NOT a fabricated completion/proof.
  for(unsigned block=0;block<68;block++) {
   ResidentActivationMachine input(argv[2],bank,edge,block,(bank+unsigned(edge)+block)%8);
   for(unsigned i=0;i<256;i++){gate[block*256+i]=input.remoteGate[i];up[block*256+i]=input.remoteUp[i];}
  }
  OutputMachine gateSink(argv[1],bank,edge,0,119,1),upSink(argv[1],bank,edge,1,96,2);
  auto stage=[&](OutputMachine& sink,const std::vector<uint32_t>& data) {
   sink.descriptor[42]=layer;sink.SetDescriptor();
   for(unsigned worker=0;worker<sink.descriptor[7];worker++)for(unsigned i=0;i<sink.rowMap[worker][1];i++)
    sink.memory[0x80000+(sink.rowMap[worker][2]+i)*4]=data[sink.rowMap[worker][0]+i];
   assert(sink.RunOutput() && sink.remoteOutput==data);sinks++;
  };
  stage(gateSink,gate);stage(upSink,up);
  GatherMachine root(argv[3],bank,edge,layer);
  for(unsigned block=0;block<68;block++) {
   ResidentActivationMachine producer(argv[2],bank,edge,block,(bank+unsigned(edge)+block)%8);
   for(unsigned i=0;i<256;i++) {
    // ONLY the completed sinks supply these GDDR read endpoints.
    producer.remoteGate[i]=gateSink.remoteOutput[block*256+i];producer.remoteUp[i]=upSink.remoteOutput[block*256+i];
   }
   producer.descriptor[42]=layer;producer.SetDescriptor();assert(producer.RunResident());producer.VerifyResident(activated.data()+block*256);producers++;
   memcpy(root.nativeOperands.data()+block*128,producer.remoteNative.operand,512);
   memcpy(root.scaleLines[block].data(),producer.remoteNative.scale,64);
  }
  assert(root.RunGather());root.VerifyGather();gathers++;
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(12,17408,activated.data(),expected));
  for(unsigned i=0;i<8704;i++)assert(root.memory.at(0xa0000+i*4)==expected.activation[i]);
  for(unsigned i=0;i<80;i++)assert(root.memory.at(0xab000+i*4)==expected.scales[i]);
  printf("PASS OFFLINE actual sink->GDDRgate/up->68actualproducers->GDDRnative/scales->actualROOTgather bank%u edge%u/noCPUintermediateproducts/oldmatrixinputbyteexact\n",bank,unsigned(edge));
 }
 printf("PASS OFFLINE THREE ACTUAL IMAGES sinks%u/producers%u/gathers%u;ALL7banks/2edges/17408lanes/shuffledmatrixroworder;NO_LEARNED_MATRIX_NATIVE_SCHEDULER_FULL_FFN_OR_SPEED_CLAIM\n",sinks,producers,gathers);
}
