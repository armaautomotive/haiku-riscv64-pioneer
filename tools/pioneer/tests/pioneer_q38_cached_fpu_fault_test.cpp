/* SPDX-License-Identifier: MIT */
#define PIONEER_CACHED_FPU_IMAGE_ONLY
#include "pioneer_q38_cached_fpu_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);unsigned refused=0;
 const auto valid=Job(5120,2,119,786);CachedData data(5120,2,0);
 std::vector<uint8_t> rootBytes(0x180000,0xa7);auto root=std::make_unique<ServiceMachine>(argv[1],rootBytes);
 const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,5120,0xb0000,0xa0000,17,0,0,0,0,0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);data.StageFp32(*root);assert(root->Execute(PioneerQ38Bf16Input::Magic));
 for(unsigned field=0;field<16;field++){
  auto job=valid;job.words[field]^=field==3?1U:field==4?1U:0x80000000U;
  assert(!CL::Shape(job));auto native=std::make_unique<CachedPipeline>();std::fill(native->l1.begin(),native->l1.end(),0xa7);
  auto matrix=std::make_unique<CachedMatrixRV32>(argv[2],*native,valid,rootBytes);
  for(unsigned i=0;i<16;i++)matrix->Put(0x74000+i*4,job.words[i]);
  const bool ran=matrix->Execute(CL::Magic);
  // The harness publishes its zero count using ONE actual FENCE. A fence
  // is not an MMIO read/write;the rejected service must perform neither.
  assert(ran && !matrix->Get(0x17ffc4) && matrix->bus.events==1 && !matrix->bus.reads && !matrix->bus.writes
   && !matrix->stores && !matrix->forcedZero && !native->matrixOps);refused++;
 }
 for(unsigned fault:{1U,2U,3U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}){
  auto native=std::make_unique<CachedPipeline>();std::fill(native->l1.begin(),native->l1.end(),0xa7);
  auto matrix=std::make_unique<CachedMatrixRV32>(argv[2],*native,valid,rootBytes);matrix->bus.fault=fault;
  for(unsigned i=0;i<16;i++)matrix->Put(0x74000+i*4,valid.words[i]);data.StageCache(matrix->bus);
  assert(!matrix->Execute(CL::Magic) && !matrix->stores);
  const unsigned events=matrix->bus.events,issues=matrix->bus.issues,math=native->matrixOps;
  for(unsigned i=0;i<2048;i++){matrix->Step();matrix->Progress();}
  assert(matrix->bus.events==events && matrix->bus.issues==issues && native->matrixOps==math);refused++;
 }
 for(unsigned lane:{0U,127U,255U}){
  std::vector<uint8_t> bytes(0x180000,0xa7);auto broken=std::make_unique<ServiceMachine>(argv[1],bytes);
  for(unsigned i=0;i<16;i++)broken->Put(0x74000+i*4,frame[i]);data.StageFp32(*broken);broken->Put(0xb0000+lane*4,0x7f800000);
  assert(!broken->Execute(PioneerQ38Bf16Input::Magic) && !broken->stores);refused++;
 }
 printf("PASS actual cached-FPU fault/refusal cases%u:all16 dirty frame fields before MMIO;NIU/source/counter/command faults park with no next IO/math/output;bad producer lane atomic;2048 actual fault-loop instructions;NOT full native/lifetime/failure/hardware/model-speed qualification\n",refused);
}
