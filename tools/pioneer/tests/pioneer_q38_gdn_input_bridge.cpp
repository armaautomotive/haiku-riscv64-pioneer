/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_INPUT_MACHINE_ONLY
#include "pioneer_q38_gdn_input_image_test.cpp"
#include "pioneer_q38_gdn_input_bridge.h"
struct OriginalGdnInputMachine:GdnInputMachine {
 const std::vector<uint32_t>& resident;ProjectionExecutionLearnedBytes& original;
 OriginalGdnInputMachine(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned half,unsigned bank,
  const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes& source)
  :GdnInputMachine(image,p,0,half,bank),resident(workspace),original(source){}
 std::vector<uint32_t> Payload(unsigned p)override{
  assert(p<7 && original.Ready());std::vector<uint32_t> out(p<2?128:p==2?64:16);
  if(p<5){
   unsigned offset=descriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];
   assert(resident.size()==524288 && !(offset&63) && offset/4+out.size()<=resident.size());
   for(unsigned i=0;i<out.size();i++)out[i]=resident[offset/4+i];
  }else{
   uint8_t bytes[64];assert(original.Read(descriptor[p==5?13:41],descriptor[p==5?12:15],bytes,64) && original.Ready());
   for(unsigned i=0;i<16;i++){const auto* b=bytes+i*4;out[i]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
  }
  return out; // RAW existing device products/ORIGINAL weights,NO host q/k/gates
 }
};
bool ExecuteGdnInputResidentBytes(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned half,unsigned bank,
 const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes& original,GdnInputEmulationResult& result){
 (void)&GdnInputPlans;result={};OriginalGdnInputMachine machine(image,p,half,bank,workspace,original);
 if(!machine.Run())return false;machine.VerifyTransport();
 result.values=machine.Values();result.packets=machine.packets;result.steps=machine.steps;result.stack=0x70000-machine.minStack;return true;
}
