/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_cooperative_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto root=std::make_unique<ArithmeticParent>(argv[1],argv[2],true,1,6);
 // Simulated owner staging is explicit:these are NOT a native NextLayerInput.
 // Real instructions keep the raw residual and previous role slots untouched.
 const unsigned raw=0x18000/4;for(unsigned i=0;i<5120;i++)root->gddr.at(raw+i)=0x3f000000+i;
 std::array<std::vector<uint32_t>,4> retained;unsigned rows=0;
 const unsigned roles[]={10,11,5,6};
 for(unsigned layer:{1U,62U}){
  retained={};
  for(unsigned slot=0;slot<4;slot++){
   if(layer!=1 || slot)root->Retarget(layer,roles[slot],false);
   assert(root->Run());root->VerifyArithmetic();rows+=root->fixture.jobs->Outputs();
   uint32_t receipt[64],before[8],after[8];
   for(unsigned i=0;i<64;i++)receipt[i]=root->memory.at(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
   for(unsigned i=0;i<8;i++){before[i]=receipt[16+i];after[i]=root->counts[i==6?2:i];}
   assert(PioneerQ38ProjectionGraphReceipt(receipt,root->fixture.graph->Manifest(),before,after));
   unsigned offset=PioneerQ38ProjectionOutputOffset(layer,roles[slot])/4;
   retained[slot].assign(root->gddr.begin()+offset,root->gddr.begin()+offset+root->fixture.jobs->Outputs());
   for(unsigned previous=0;previous<=slot;previous++){
    unsigned at=PioneerQ38ProjectionOutputOffset(layer,roles[previous])/4;
    for(unsigned i=0;i<retained[previous].size();i++)assert(root->gddr.at(at+i)==retained[previous][i]);
   }
   for(unsigned i=0;i<5120;i++)assert(root->gddr.at(raw+i)==0x3f000000+i);
   printf("PASS ACTUAL QPS2+119retained QFP3 layer%u role%u rows%u FP32+independentFP64/all earlier slots+fresh raw residual unchanged;NOT_NATIVE_TIME\n",layer,roles[slot],root->fixture.jobs->Outputs());
  }
 }
 root->Retarget(62,6,true);assert(root->Run());root->VerifyArithmetic();assert(root->completedStops==119 && root->boots==119);
 printf("PASS ACTUAL QPS2 arithmetic:8projections/rows%u/producers%u/matrixJobs%u/instructions%llu/119bootsONCE+119STOP;all four disjoint slots and raw residual preserved;SYNTHETIC weights/input staging,NOT genuine native producer/full-model/speed\n",rows,root->completedProducers,root->completedMatrices,(unsigned long long)root->matrixInstructions);
}
