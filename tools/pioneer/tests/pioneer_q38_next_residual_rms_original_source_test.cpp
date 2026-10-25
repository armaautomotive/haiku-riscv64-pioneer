/* SPDX-License-Identifier: MIT */
// ORIGINAL model byte endpoints + actual QRR2 instructions. This is NOT a
// native owner/initialization/prefix proof;activation inputs are test data.
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_MACHINE_ONLY
#include "pioneer_q38_next_residual_rms_image_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextResidualRmsLearned.h"
struct OriginalNormMachine final:NextResidualRmsMachine {
 DecoderOriginalSource& original;
 OriginalNormMachine(const char* image,const PioneerQ38CanonicalModelPlan& plan,DecoderOriginalSource& source)
  :NextResidualRmsMachine(image,plan,0,0),original(source){}
 void CopyPacket(bool write,bool learned,bool rawRead,bool normalized,unsigned offset,unsigned local,unsigned bytes)override{
  if(!write && learned){
   std::vector<uint8_t> values(bytes);
   assert(original.Read(descriptor[12],descriptor[11]+offset,values.data(),bytes));
   for(unsigned i=0;i<bytes/4;i++){
    const auto* b=values.data()+i*4;uint32_t value=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
    memory[local+i*4]=value;weight[offset/4+i]=value;
   }
  }else NextResidualRmsMachine::CopyPacket(write,learned,rawRead,normalized,offset,local,bytes);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&NextResidualRmsPlanTests;
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 const PioneerQ38CanonicalPart* norm=nullptr;
 for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind==plan->WeightKind && p->layer==1 && p->role==1){assert(!norm);norm=p;}}
 assert(norm && !norm->type && norm->rows==1 && norm->bytes==20480 && norm->allocated==20480);
 const unsigned route=PioneerQ38ProjectionRoute(norm->bank),address=plan->Base+norm->offset;
 auto source=OpenNextResidualRmsOriginalSource(argv[1]);assert(source && !source->NextResidualRmsWeightsReady());
 std::vector<uint8_t> out(20480,0xa5),saved=out;
 assert(!source->Read(route,address,out.data(),16384) && out==saved && !source->NextResidualRmsReads());
 assert(source->Prepare(0) && source->Ready() && source->ReferenceReads()==1673 && !source->NextResidualRmsWeightsReady());
 assert(!source->Read(route,address,out.data(),16384) && out==saved && !source->NextResidualRmsReads());
 assert(source->SealBeforeNativeContext() && source->NextResidualRmsWeightsReady() && source->NextGdnOutputWeightsReady());
 unsigned refusals=0,pageFaults=0;
 for(unsigned offset:{0U,64U,4096U,16384U,20416U,20480U})for(unsigned bytes:{0U,64U,256U,512U,576U,2304U,4096U,16384U,20480U}){
  if((!offset && bytes==16384) || (offset==16384 && bytes==4096))continue;
  assert(!source->Read(route,address+offset,out.data(),bytes) && out==saved && !source->NextResidualRmsReads());refusals++;
 }
 assert(!source->Read(129,address,out.data(),16384) && !source->Read(route,address+1,out.data(),16384)
  && !source->Read(route,address,nullptr,16384) && out==saved && !source->NextResidualRmsReads());refusals+=3;
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);
 unsigned first=norm->bank*unsigned(plan->Capacity/4096)+norm->offset/4096;
 assert(!PioneerQ38NextResidualRmsLearned(*plan,ledger.data(),unsigned(ledger.size())));
 for(unsigned i=0;i<5;i++)ledger[first+i]=1;
 assert(PioneerQ38NextResidualRmsLearned(*plan,ledger.data(),unsigned(ledger.size())));
 for(unsigned i=0;i<5;i++)for(uint8_t bad:{uint8_t(0),uint8_t(2)}){
  ledger[first+i]=bad;assert(!PioneerQ38NextResidualRmsLearned(*plan,ledger.data(),unsigned(ledger.size())));ledger[first+i]=1;pageFaults++;
 }
 assert(!PioneerQ38NextResidualRmsLearned(*plan,nullptr,unsigned(ledger.size()))
  && !PioneerQ38NextResidualRmsLearned(*plan,ledger.data(),unsigned(ledger.size()-1)));
 OriginalNormMachine machine(argv[2],*plan,*source);
 std::vector<uint32_t> before(524288,0xdecafbad),after=before;
 for(unsigned i=0;i<5120;i++){before[TT_Q38_RMS_BACKUP/4+i]=machine.raw[i];before[TT_Q38_NEXT_GDN_OUTPUT_RESULT/4+i]=machine.projected[i];}
 assert(!source->CheckNextResidualRms(before,after) && machine.Run());machine.Verify();
 for(unsigned i=0;i<5120;i++){after[i]=machine.resident[i];after[TT_Q38_RMS_BACKUP/4+i]=machine.backup[i];}
 assert(source->NextResidualRmsReads()==2 && source->CheckNextResidualRms(before,after));
 unsigned mutations=0;
 for(unsigned offset:{0U,unsigned(TT_Q38_RMS_BACKUP/4)})for(unsigned i=0;i<5120;i++){
  after[offset+i]^=1;assert(!source->CheckNextResidualRms(before,after));after[offset+i]^=1;mutations++;
 }
 std::vector<uint32_t> absent;
 assert(!source->CheckNextResidualRms(absent,after) && !source->CheckNextResidualRms(before,absent)
  && !source->NextGdnOutputReads() && !source->NextGdnReads() && !source->NextConvReads() && !source->NextProjectionReads()
  && !source->NextRmsReads() && !source->FfnReads() && !source->ResidualRmsReads() && !source->GdnOutputReads()
  && !source->ProjectionReads() && !source->ConvolutionReads() && !source->Prepare(0) && !source->SealBeforeNativeContext());
 printf("PASS ORIGINAL c600/ALL851schema/layer1 F32 post-attention norm5pages BEFORE context/2exact learned reads/%u atomic endpoint refusals/%u missing-corrupt pages/ACTUAL QRR2 all5120 sum+norm BIT-EXACT/%u output mutations/NO native owner or speed grant\n",refusals,pageFaults,mutations);
}
