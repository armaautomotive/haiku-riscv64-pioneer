/* SPDX-License-Identifier: MIT */
// ACTUAL QPE1 -> simulated GDDR copies -> ACTUAL op5/gather/matrix.
// This is raw embedding/projection composition,NOT a decoder:learned RMS,
// residual/attention/GDN scheduling and native producer lifetime are OWED.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_PROJECTION_GATHER_MACHINE_ONLY
#include "pioneer_q38_projection_gather_image_test.cpp"
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#define PIONEER_Q38_EMBEDDING_REUSE_SCALAR
#include "pioneer_q38_embedding_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&PlanTests;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned embeddings=0,producers=0,gathers=0,matrices=0;
 // SAME5120-vector,ALL three learned wire formats. Test weights/endpoints
 // only;independent references OBSERVE actual bytes,never populate them.
 for(const auto& selection:{std::pair<unsigned,unsigned>{0,2},{0,5},{64,1}}){
  unsigned layer=selection.first,role=selection.second,type=0,k=0,n=0;
  assert(PioneerQ38ProjectionGeometry(layer,role,type,k,n) && k==5120);
  std::vector<std::unique_ptr<ProjectionPeer>> peers;
  for(unsigned chunk=0;chunk<20;chunk++){peers.emplace_back(new ProjectionPeer(argv[2],type,k,chunk+1));assert(peers.back()->BootNew());}
  std::array<uint32_t,16> first={};
  for(unsigned changed=0;changed<2;changed++){
   EmbeddingMachine embedding(argv[1],*canonical,changed?248319:0,changed,changed?6:0,changed);
   // Real model scales are small. Retain rich packed scale/min/nibble
   // variation but avoid deliberately max-half stress values here,which
   // exceed the unchanged downstream native quantizer's scale domain.
   for(unsigned block=0;block<20;block++)for(unsigned lane=0;lane<16;lane++){
    uint16_t d=uint16_t(0x1800+((block+lane+changed)%7)*32),m=uint16_t(0x1000+((block*3+lane+changed)%5)*64);
    embedding.Put(block,lane,0,uint8_t(d));embedding.Put(block,lane,1,uint8_t(d>>8));
    embedding.Put(block,lane,2,uint8_t(m));embedding.Put(block,lane,3,uint8_t(m>>8));
   }
   assert(embedding.Run());embedding.Verify();embeddings++;
   ProjectionGatherMachine root(argv[3],layer,role,changed?6:0,false,17+changed);
   for(unsigned chunk=0;chunk<20;chunk++){
    auto& peer=*peers[chunk];peer.Stage(changed?(chunk?2:3):1,layer,role,17+changed,changed?6:0,false);
    // Transport bytes ONLY. No host dequantization/quantization/products
    // enter this simulated device input. TEST-only Verify runs afterwards.
    for(unsigned i=0;i<256;i++)peer.gate[i]=embedding.resident[chunk*256+i];
    assert(peer.Finish(changed?(chunk?2:3):1));peer.Verify();producers++;
    memcpy(root.native.data()+chunk*128,peer.native.data(),512);root.lines[chunk]=peer.line;
   }
   assert(root.Run());root.Verify();gathers++;
   auto& matrix=*peers[0];FeedMatrix(matrix,root);
   assert(matrix.MatrixNext(changed?4:2) && !matrix.dotProbeReads);matrices++;
   bool different=false;
   for(unsigned row=0;row<16;row++){if(!changed)first[row]=matrix.buffers[4096+row];else different|=first[row]!=matrix.buffers[4096+row];}
   if(changed)assert(different);
  }
  for(unsigned chunk=0;chunk<20;chunk++)assert(peers[chunk]->Stop(chunk?3:5));
  printf("PASS actual embedding->ALL20 op5 producers->gather->SAME persistent peer learned type%u/5120inputs/16rows,2changed tokens+positions,independent matrix references,NO host vector substitution\n",type);
 }
 printf("PASS OFFLINE actual embedding%u/producers%u/gathers%u/changed-input matrices%u;ALL3types/live peers STOP;SIMULATED transport+weights,NO_RMS_NATIVE_PRODUCER_OR_FULL_DECODER_SPEED_EVIDENCE\n",embeddings,producers,gathers,matrices);
}
