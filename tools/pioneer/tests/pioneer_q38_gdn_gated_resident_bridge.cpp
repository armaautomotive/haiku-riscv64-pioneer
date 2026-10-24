/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_GATED_RESIDENT_MACHINE_ONLY
#include "pioneer_q38_gdn_gated_resident_image_test.cpp"
#include "pioneer_q38_gdn_gated_resident_bridge.h"
struct OriginalGatedResidentMachine:GatedResidentMachine {
 const std::vector<uint32_t>& workspace;ProjectionExecutionLearnedBytes& original;DecoderHistoryBytes& state;
 std::array<uint32_t,16> learnedBlock[2];bool readLearned[2]={};
 OriginalGatedResidentMachine(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned bank,const uint32_t* in,const uint32_t* st,
  const uint32_t* learnedNorm,const std::vector<uint32_t>& work,ProjectionExecutionLearnedBytes& bytes,DecoderHistoryBytes& device)
  :GatedResidentMachine(image,plan,in[5],in[7],bank),workspace(work),original(bytes),state(device){
  assert(original.Ready() && learnedNorm && workspace.size()>=2097152/4);
  for(unsigned i=0;i<64;i++){inputDescriptor[i]=in[i];descriptor[i]=st[i];}
  assert(PioneerQ38GdnRecurrentShape(in,st) && inputDescriptor[3]==bank);
  SetDescriptors();PioneerQ38GdnGatedPlan data;assert(data.Build(plan,in,st));
  for(unsigned i=0;i<64;i++)gatedDescriptor[i]=data.Descriptor()[i];SetGatedDescriptor();
  for(unsigned i=0;i<16;i++)seals[i]=in[16+i];
  for(unsigned i=0;i<128;i++){norm[i]=learnedNorm[i];z[i]=workspace.at(gatedDescriptor[42]/4+i);}
 }
 uint32_t RawPayload(unsigned p,unsigned i)override{
  if(p<5){unsigned offset=inputDescriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];return workspace.at(offset/4+i);}
  assert(p<7 && original.Ready());
  if(!readLearned[p-5]){
   uint8_t bytes[64];assert(!i && original.Read(inputDescriptor[p==5?13:41],inputDescriptor[p==5?12:15],bytes,64) && original.Ready());
   for(unsigned j=0;j<16;j++)learnedBlock[p-5][j]=uint32_t(bytes[j*4])|(uint32_t(bytes[j*4+1])<<8)|(uint32_t(bytes[j*4+2])<<16)|(uint32_t(bytes[j*4+3])<<24);
   readLearned[p-5]=true;
  }
  return learnedBlock[p-5][i];
 }
 bool StateTransfer(bool write,unsigned piece,unsigned local)override{
  if(write){for(unsigned i=0;i<4096;i++)next[piece*4096+i]=memory.at(local+i*4);}
  else{
   uint32_t values[4096];assert(state.ReadGdnState(descriptor[4],descriptor[5]+piece*16384,values,4096));
   for(unsigned i=0;i<4096;i++)memory[local+i*4]=prior[piece*4096+i]=values[i];
  }
  return true; // separate simulated NEXT endpoint;never mutate genuine native owner's state/lease
 }
};
void ExecuteGdnGatedResidentOriginalBytes(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned bank,
 const uint32_t input[2][64],const uint32_t stateDescriptor[2][64],const uint32_t norm[128],
 const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes& original,DecoderHistoryBytes& state,GatedResidentObserved& observed){
 (void)&TestWorker;assert(!(input[0][7]&1) && input[1][7]==input[0][7]+1);
 OriginalGatedResidentMachine a(image,plan,bank,input[0],stateDescriptor[0],norm,workspace,original,state);
 OriginalGatedResidentMachine b(image,plan,bank,input[1],stateDescriptor[1],norm,workspace,original,state);
 RunGatedPair(a,b,input[0][7]/2%2,32);assert(a.Passed() && b.Passed());a.VerifyGated();b.VerifyGated();
 unsigned side=0;for(auto* m:{&a,&b}){
  for(unsigned i=0;i<322;i++)observed.prepared[side][i]=m->memory.at(0x8e000+i*4);
  for(unsigned i=0;i<8192;i++)observed.next[side][i]=m->next[i];
  for(unsigned i=0;i<64;i++){observed.raw[side][i]=m->memory.at(0x8e800+i*4);observed.gated[side][i]=m->gatedOutput[i];}
  side++;
 }
}
