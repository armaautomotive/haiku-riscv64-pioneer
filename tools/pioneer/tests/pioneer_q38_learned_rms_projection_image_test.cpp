/* SPDX-License-Identifier: MIT */
// Actual embedding -> actual learned RMS -> actual op5/gather/matrix.
// Endpoint copies ONLY,not host-computed substituted vectors. NOT a full
// decoder or native source/producer/execution-lifetime qualification.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_PROJECTION_GATHER_MACHINE_ONLY
#include "pioneer_q38_projection_gather_image_test.cpp"
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#define PIONEER_Q38_EMBEDDING_REUSE_SCALAR
#include "pioneer_q38_embedding_image_test.cpp"
#define PIONEER_Q38_RMS_MACHINE_ONLY
#define PIONEER_Q38_RMS_REUSE_SCALAR
#include "pioneer_q38_learned_rms_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&PlanTests;(void)&RmsPlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned embeddings=0,norms=0,producers=0,gathers=0,matrices=0;
 for(const auto& selection:{std::pair<unsigned,unsigned>{0,2},{0,5},{64,1}}){
  unsigned layer=selection.first,role=selection.second,type=0,k=0,n=0;
  assert(PioneerQ38ProjectionGeometry(layer,role,type,k,n) && k==5120);
  unsigned normRole=layer==64?2:role==2?1:0;
  std::vector<std::unique_ptr<ProjectionPeer>> peers;
  for(unsigned chunk=0;chunk<20;chunk++){peers.emplace_back(new ProjectionPeer(argv[3],type,k,chunk+1));assert(peers.back()->BootNew());}
  std::array<uint32_t,16> first={};
  for(unsigned changed=0;changed<2;changed++){
   unsigned bank=changed?6:0;
   EmbeddingMachine embedding(argv[1],*canonical,changed?248319:0,changed,bank,changed);
   for(unsigned block=0;block<20;block++)for(unsigned lane=0;lane<16;lane++){
    uint16_t d=uint16_t(0x1800+((block+lane+changed)%7)*32),m=uint16_t(0x1000+((block*3+lane+changed)%5)*64);
    embedding.Put(block,lane,0,uint8_t(d));embedding.Put(block,lane,1,uint8_t(d>>8));
    embedding.Put(block,lane,2,uint8_t(m));embedding.Put(block,lane,3,uint8_t(m>>8));
   }
   assert(embedding.Run());embedding.Verify();embeddings++;
   LearnedRmsMachine rms(argv[2],*canonical,layer,normRole,bank,changed);
   rms.input=rms.resident=embedding.resident;assert(rms.Run());rms.Verify();norms++;
   bool normalized=false;for(unsigned i=0;i<5120;i++){assert(rms.backup[i]==embedding.resident[i]);normalized|=rms.resident[i]!=rms.backup[i];}assert(normalized);
   ProjectionGatherMachine root(argv[4],layer,role,bank,false,17+changed);
   for(unsigned chunk=0;chunk<20;chunk++){
    auto& peer=*peers[chunk];peer.Stage(changed?(chunk?2:3):1,layer,role,17+changed,bank,false);
    for(unsigned i=0;i<256;i++)peer.gate[i]=rms.resident[chunk*256+i]; // actual simulated GDDR bytes ONLY
    assert(peer.Finish(changed?(chunk?2:3):1));peer.Verify();producers++;
    memcpy(root.native.data()+chunk*128,peer.native.data(),512);root.lines[chunk]=peer.line;
   }
   assert(root.Run());root.Verify();gathers++;
   auto& matrix=*peers[0];FeedMatrix(matrix,root);assert(matrix.MatrixNext(changed?4:2) && !matrix.dotProbeReads);matrices++;
   bool different=false;for(unsigned row=0;row<16;row++){if(!changed)first[row]=matrix.buffers[4096+row];else different|=first[row]!=matrix.buffers[4096+row];}
   if(changed)assert(different);
  }
  for(unsigned chunk=0;chunk<20;chunk++)assert(peers[chunk]->Stop(chunk?3:5));
  printf("PASS ACTUAL embedding->learned RMS role%u->ALL20 unchanged op5/gather->SAME persistent learned type%u matrix,changed token/position,raw residual backup preserved,independent numerical references\n",normRole,type);
 }
 printf("PASS OFFLINE actual embedding%u/RMS%u/producers%u/gathers%u/matrices%u ALL3types,ALL peer STOP;SIMULATED_TRANSPORT_WEIGHTS_NOT_NATIVE_LIFETIME_OR_FULL_MODEL_SPEED\n",embeddings,norms,producers,gathers,matrices);
}
