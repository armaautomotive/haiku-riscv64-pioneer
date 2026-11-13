/* SPDX-License-Identifier: MIT */
// Linked RV32 controller; binary pipe supplies verified simulated-worker
// boots or results. The pipe models host transport, not on-card NoC.
#define PIONEER_Q38_RECURRENT_ROOT_JOB_BRIDGE_HELPER_ONLY 1
#include "pioneer_q38_recurrent_root_job_bridge.cpp"

int main(int argc,char** argv){
 assert(argc==3 && (!std::strcmp(argv[2],"--jobs") || !std::strcmp(argv[2],"--publish") || !std::strcmp(argv[2],"--corrupt") || !std::strcmp(argv[2],"--corrupt-producer")));
 const bool jobs=!std::strcmp(argv[2],"--jobs"),corrupt=!std::strcmp(argv[2],"--corrupt"),badProducer=!std::strcmp(argv[2],"--corrupt-producer");auto root=std::make_unique<ActualRoot>(argv[1]);
 const auto builds=Symbols(argv[1],"Projection::BfBuildDenseSchedule("),reads=Symbols(argv[1],"Projection::BfBankRead(");
 const auto descriptor=Symbols(argv[1],"Projection::BfDescriptor("),producer=Symbols(argv[1],"Projection::BfProducerJob(");
 const auto gathers=Symbols(argv[1],"Projection::BfGatherMatrix("),producerGathers=Symbols(argv[1],"Projection::BfGatherProducer(");
 uint32_t boots[20][64];assert(fread(boots,sizeof(boots),1,stdin)==1);
 const unsigned d=0x74e00,out=0x78000;for(unsigned i=0;i<64;i++)root->memory[d+i*4]=0;
 root->memory[d+4*4]=0x04000000;root->memory[d+4*6]=2025;root->memory[d+4*8]=PioneerQ38ProjectionRoute(0);
 root->memory[d+4*11]=48;root->memory[d+4*14]=5120;root->memory[d+4*15]=8;
 root->memory[d+4*41]=248319; // Match the worker fixture's token identity.
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal proposal;
 auto workspace=std::make_unique<std::array<uint32_t,PioneerQ38SmallRecurrentWorkspace::Words>>();
 assert(canonical->Build() && PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*canonical,proposal)
  && PioneerQ38SmallRecurrentWorkspace::Build(*canonical,proposal,workspace->data(),workspace->size()));
 for(unsigned i=0;i<4;i++)root->memory[d+(32+i*2)*4]=i+1;
 for(unsigned w=0;w<20;w++)for(unsigned i=0;i<64;i++)root->memory[0x170000+w*256+i*4]=boots[w][i];
 for(unsigned kind=0;kind<2;kind++){
  const unsigned role=10+kind;
  root->memory[d+4*42]=0;root->memory[d+4*43]=role;
  assert(RunBuildWithStub(*root,builds[0],reads[0],d,
   PioneerQ38SmallRecurrentWorkspace::Offset+kind*128,workspace->data()+kind*32));
  for(unsigned i=0;i<64;i++)root->memory[0x80000+i*4]=0x7fc00001;
  if(jobs){
   for(unsigned w=0;w<20;w++){
    const unsigned producerSequence=1+kind*(w<2?2:1);
    uint32_t data[64];assert(Call(*root,descriptor[0],d,w,0,out,false));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
    assert(Call(*root,producer[0],d,w,producerSequence,out,true));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
   }
   for(unsigned w=0;w<2;w++){
    uint32_t data[64];const unsigned matrixSequence=2+kind*2;
    assert(root->Run(root->job[0],d,w,matrixSequence));
    for(unsigned i=0;i<64;i++)data[i]=root->memory.at(out+i*4);assert(fwrite(data,sizeof(data),1,stdout)==1);
    unsigned bank=0;while(bank<7 && !(*workspace)[kind*32+bank*3])bank++;
    assert(bank<7 && data[19]==(*workspace)[kind*32+bank*3]+w*174080
     && data[21]==PioneerQ38ProjectionRoute(bank) && data[32]==w*32 && data[20]==1);
   }
  }else{
   auto snapshots=std::make_unique<std::array<PioneerRecurrentWorkerSnapshot,119>>();
   for(unsigned w=0;w<20;w++)assert(fread(&(*snapshots)[w],sizeof((*snapshots)[w]),1,stdin)==1);
   if(corrupt)(*snapshots)[0].proof[19]^=1;
   if(badProducer)(*snapshots)[0].producer.proof[19]^=1;
   auto endpoint=std::make_unique<GatherRoot>(argv[1],*snapshots);endpoint->memory=root->memory;
   const unsigned owner=0x6e000,frame=owner+768;
   for(unsigned i=0;i<64;i++)endpoint->memory[frame+i*4]=endpoint->memory.at(d+i*4);
   endpoint->memory[frame+9*4]=129;endpoint->memory[frame+12*4]=48*4;endpoint->memory[frame+53*4]=0xe0000;
   for(unsigned i=0;i<64;i++)endpoint->memory[owner+i*4]=0;
   for(unsigned i=0;i<128;i++)endpoint->memory[owner+256+i*4]=endpoint->memory[0x75000+i*4]=0;
   for(unsigned i=0;i<16;i++){
    const unsigned value=0x10000+i,at=i==0?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;
    endpoint->memory[owner+(16+i)*4]=value;endpoint->memory[frame+(16+i)*4]=value;endpoint->memory[0xffb20000+at]=value;
   }
   for(unsigned i=0;i<8;i++)endpoint->memory[owner+(8+i)*4]=endpoint->memory.at(frame+(32+i)*4);
   endpoint->memory[owner+256+48*4]=1;endpoint->memory[owner+1024]=2;endpoint->memory[0x75400]=1;
   for(unsigned i=0;i<4;i++)endpoint->memory[0xffb20040+i*0x800]=0;
   for(unsigned i=0;i<32;i++)endpoint->memory[0xffb20240+i*4]=0;
   for(unsigned at:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})endpoint->memory[0xffb20000+at]=0;
   for(unsigned i=0;i<8;i++)endpoint->memory[0x6d000+i*4]=0;
   endpoint->memory[0x74800+6*4]=endpoint->memory[0x74800+7*4]=0;
   std::ifstream syms(std::string(argv[1])+"/program.demangled");std::string line;unsigned packetSeal=0;
   while(std::getline(syms,line))if(line.find(" T q38_layer_return_recurrent_packet_seal")!=std::string::npos)packetSeal=unsigned(std::stoul(line.substr(0,8),nullptr,16));
   assert(packetSeal);
   endpoint->fullPrepare=true;
   for(unsigned w=0;w<20;w++){
    const auto& s=(*snapshots)[w];endpoint->phase[w]=1;endpoint->memory[0x123000+w*4]=s.producer.doorbell;
    for(unsigned i=0;i<8;i++)endpoint->memory[0x122000+(w*8+i)*4]=s.initial.counters[i];
    for(unsigned i=0;i<64;i++)endpoint->memory[0x178000+w*256+i*4]=s.producer.job[i];
    for(unsigned i=0;i<128;i++)endpoint->memory[s.producer.descriptor[4]+s.producer.descriptor[12]+i*4]=s.producer.native[i];
    for(unsigned i=0;i<32;i++)endpoint->memory[s.producer.descriptor[4]+s.producer.descriptor[13]+i*4]=s.producer.line[i];
    const bool accepted=endpoint->RunGather(producerGathers[0],packetSeal,frame,0x6d000,w);
    if(badProducer){assert(!accepted && !w);for(unsigned i=0;i<64;i++)assert(endpoint->memory.at(0x80000+i*4)==0x7fc00001);
     puts("PASS corrupted producer receipt rejected before matrix publication; NO DEVICE");return 0;}
    assert(accepted);
   }
   endpoint->fullPrepare=false;
   for(unsigned w=0;w<2;w++){
    const auto& s=(*snapshots)[w];endpoint->memory[0x123000+w*4]=s.doorbell;
    for(unsigned i=0;i<8;i++)endpoint->memory[0x122000+(w*8+i)*4]=s.before[i];
    const bool accepted=endpoint->RunGather(gathers[0],packetSeal,frame,0x6d000,w);
    if(corrupt){assert(!accepted && !w);for(unsigned i=0;i<64;i++)assert(endpoint->memory.at(0x80000+i*4)==0x7fc00001);
     puts("PASS corrupted worker receipt rejected before publication; NO DEVICE");return 0;}
    assert(accepted);const unsigned valid=w?16:32;
    for(unsigned i=0;i<valid;i++)assert(endpoint->memory.at(0x80000+(w*32+i)*4)==s.output[i]);
   }
   for(unsigned i=48;i<64;i++)assert(endpoint->memory.at(0x80000+i*4)==0x7fc00001);
  }
 }
 assert(fgetc(stdin)==EOF);if(jobs)assert(!fflush(stdout));else puts("PASS linked ROOT gathered actual simulated worker receipts and published results; NO DEVICE");
}
