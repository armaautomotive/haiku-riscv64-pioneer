/* SPDX-License-Identifier: MIT */
// Actual QGW8 ROOT FFN pair construction using plan-produced manifest.
// Simulated instructions/data only; no whole token, model, device or TPS.
#define PIONEER_Q38_HYBRID_ACTUAL_ROOT_MACHINE_ONLY
#include "pioneer_q38_hybrid_actual_root_test.cpp"

int main(int argc,char** argv){
 assert(argc==3);std::array<uint32_t,64*3*7> manifest={};std::ifstream file(argv[2],std::ios::binary);
 file.read(reinterpret_cast<char*>(manifest.data()),sizeof(manifest));assert(file.gcount()==sizeof(manifest) && file.peek()==EOF);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned compared=0;
 for(unsigned component=0;component<2;component++)for(unsigned layer:{47U,48U,49U,50U,60U,61U})for(unsigned role:{2U,4U}){
  PioneerQ38PairedProjectionJobs paired;assert(paired.Build(*canonical,layer,role,0));
  auto original=std::make_unique<PioneerQ38ProjectionJobs>();assert(original->Build(*canonical,layer,role,0));
  RootProbe source{*original};auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00;
  for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
  m->memory[d+6*4]=2025;m->memory[d+11*4]=paired.Outputs();m->memory[d+14*4]=paired.Inputs();m->memory[d+15*4]=12;
  m->memory[d+42*4]=layer;m->memory[d+43*4]=role;for(unsigned i=0;i<4;i++)m->memory[d+(32+i*2)*4]=i+1;
  for(unsigned i=0;i<manifest.size();i++)m->memory[0x16c400+i*4]=manifest[i];
  for(unsigned b=0;b<8;b++)m->memory[0x76d40+b*4]=PioneerQ38ProjectionRoute(b);
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<8;i++)m->memory[0x77000+w*32+i*4]=i==5?0x14000:source.Part(w,i);
   m->memory[0x76e00+w*4]=source.Row(w);
   uint32_t boot[64]={};boot[0]=HW::Magic;boot[1]=1;boot[2]=64;boot[3]=w+1;boot[4]=HW::Peer(w+1);
   boot[5]=0x14000;boot[6]=8192;boot[7]=64;for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
   boot[16]=boot[5];for(unsigned i=0;i<3;i++)for(unsigned lane=0;lane<6;lane++)boot[17+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=0;i<2;i++)for(unsigned lane=0;lane<6;lane++)boot[23+i]|=(i*6+lane)<<(lane*5);
   for(unsigned i=40;i<=50;i++)boot[i]=HW::BootWord(i);assert(HW::BootShape(boot));
   for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boot[i];
  }
  assert(m->Run(m->build[component],d));
  for(unsigned w:{0U,59U,118U}){
   assert(m->Run(m->job[component],d,w,7));uint32_t frame[64],boot[64];
   for(unsigned i=0;i<64;i++){frame[i]=m->memory.at(0x78000+i*4);boot[i]=m->memory.at(0x170000+w*256+i*4);}
   const auto* part=paired.Part(w);assert(part);const auto* stripe=canonical->ProjectionStripe(layer,role,part->stripe);assert(stripe);
   const bool eight=layer>=50 && layer<=60;const unsigned bandBytes=paired.Inputs()/32*(eight?1088:576);
   const unsigned destination=manifest[(layer*3+role-2)*7+part->bank]+(part->firstRow-stripe->firstRow)/32*bandBytes;
   assert(frame[16]==(eight?PioneerQ38Bfp8WorkerJob::Magic:PioneerQ38Bfp4WorkerJob::Magic)
    && frame[18]==paired.Inputs() && frame[19]==destination && frame[20]==part->groups/2
    && frame[32]==part->firstRow && frame[33]==paired.Outputs() && frame[34]==role && frame[35]==(eight?8U:4U)
    && HW::JobShape(frame,boot,6,0));compared++;
  }
  m->memory[0x16c400+(layer*3+role-2)*7*4]^=1;assert(!m->Run(m->job[component],d,0,8));
 }
 assert(compared==72);
 puts("PASS actual QGW8 recurrent/dense ROOT pair jobs: 72 jobs incl BFP4 layers48/49 and BFP8 50/60, exact repacked manifest, corrupted manifest refused; synthetic simulation only");
}
