/* SPDX-License-Identifier: MIT */
// Inspection-only ROOT job emitter. stdin: 119 simulated worker boot records;
// stdout: linked ROOT producer descriptors/jobs, then 119 matrix jobs.
// The host pipe is inspection transport, not a simulated ROOT NoC.
#define main recurrent_root_helper_main
#include "pioneer_q38_recurrent_actual_root_test.cpp"
#undef main
#include <cstdlib>
#include "PioneerRecurrentWorkerSnapshot.h"

// ROOT NoC endpoint backed only by bytes captured from completed worker
// simulators. Packet source seals are explicitly stubbed by RunGather.
struct GatherRoot:ActualRoot {
 const std::array<PioneerRecurrentWorkerSnapshot,119>& snapshots;unsigned packets=0,seals=0;
 GatherRoot(const char* path,const std::array<PioneerRecurrentWorkerSnapshot,119>& s):ActualRoot(path),snapshots(s){}
 void Store(unsigned address,unsigned value)override{
  if(address!=0xffb20040 || value!=1){ActualRoot::Store(address,value);return;}
  const unsigned remote=memory.at(0xffb20000),route=memory.at(0xffb20008),local=memory.at(0xffb2000c);
  const unsigned command=memory.at(0xffb2001c),bytes=memory.at(0xffb20020);
  assert(!command && bytes && !(bytes&3) && local==0x78000+(remote&31));
  unsigned w=0;while(w<119 && Q::Peer(w+1)!=route)w++;assert(w<119);const auto& s=snapshots[w];
  for(unsigned i=0;i<bytes/4;i++){
   const unsigned at=remote+i*4;uint32_t word=0;
   if(at==0xffb121b0)word=0x46800;
   else if(at==0xffb13140)word=s.pc;
   else if(at==Q::Doorbell)word=s.doorbell;
   else if(at>=Q::Boot && at<Q::Boot+256)word=memory.at(0x170000+w*256+at-Q::Boot);
   else if(at>=Q::Status && at<Q::Status+64)word=s.status[(at-Q::Status)/4];
   else if(at>=Q::Job && at<Q::Job+256)word=s.job[(at-Q::Job)/4];
   else if(at>=0x83f00 && at<0x84000)word=s.proof[(at-0x83f00)/4];
   else if(at>=0x84000 && at<0x84400)word=s.output[(at-0x84000)/4];
   else if(at>=0xffb20000 && at<0xffb21000){
    const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};bool found=false;
    for(unsigned c=0;c<8;c++)if(at==0xffb20000+offsets[c]){word=s.after[c];found=true;break;}
    assert(found);
   }else{std::fprintf(stderr,"UNMAPPED_WORKER_READ %x bytes%u\n",at,bytes);assert(false);}
   memory[local+i*4]=word;
  }
  memory[0xffb20208]++;memory[0xffb2020c]+=(bytes+63)/64;memory[0xffb20210]++;memory[0xffb20214]++;
  memory[address]=0;packets++;
 }
 bool RunGather(unsigned function,unsigned seal,unsigned descriptor,unsigned tracked,unsigned worker){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;
  regs[10]=descriptor;regs[11]=tracked;regs[12]=worker;pc=function;steps=0;
  unsigned last=0;while(pc!=0x60004 && pc!=0x60000){last=pc;if(pc==seal){seals++;pc=regs[1];}else Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);}
  if(pc==0x60000)std::fprintf(stderr,"GATHER_PARK worker=%u last=%x ra=%x packets=%u seals=%u\n",worker,last,regs[1],packets,seals);
  return pc==0x60004;
 }
};

int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!std::strcmp(argv[3],"--store") || !std::strcmp(argv[3],"--gather"))));
 const bool store=argc==4 && !std::strcmp(argv[3],"--store"),gather=argc==4 && !std::strcmp(argv[3],"--gather");
 const unsigned role=unsigned(std::atoi(argv[2]));assert(role==5 || role==6 || role==13);
 const unsigned width=role==13?6144:5120,rows=role==5?6144:role==6?10240:5120,slot=role==5?0:role==6?1:2;
 std::array<std::array<uint32_t,64>,119> boots;
 assert(fread(boots.data(),sizeof(uint32_t),119*64,stdin)==119*64);
 std::vector<uint32_t> results;if(store){results.resize(rows);assert(fread(results.data(),sizeof(uint32_t),rows,stdin)==rows);}
 auto snapshots=std::make_unique<std::array<PioneerRecurrentWorkerSnapshot,119>>();
 if(gather)assert(fread(snapshots->data(),sizeof(PioneerRecurrentWorkerSnapshot),119,stdin)==119);
 assert(fgetc(stdin)==EOF);
 for(unsigned w=0;w<119;w++)assert(Q::BootShape(boots[w].data()) && boots[w][3]==w+1);
 auto m=std::make_unique<ActualRoot>(argv[1]);const unsigned d=0x74e00;
 for(unsigned i=0;i<64;i++)m->memory[d+i*4]=0;
 m->memory[d+4*4]=0x04000000;m->memory[d+6*4]=2025;m->memory[d+8*4]=PioneerQ38ProjectionRoute(0);
 m->memory[d+14*4]=width;m->memory[d+15*4]=8;m->memory[d+41*4]=248319;m->memory[d+42*4]=0;m->memory[d+43*4]=role;
 for(unsigned i=0;i<4;i++)m->memory[d+(32+i*2)*4]=i+1;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridRecurrentQ8MatrixPlan::Proposal proposal;
 auto workspace=std::make_unique<std::array<uint32_t,PioneerQ38RecurrentQ8MatrixWorkspace::Words>>();
 assert(canonical->Build() && PioneerQ38HybridRecurrentQ8MatrixPlan::Build(*canonical,proposal)
  && PioneerQ38RecurrentQ8MatrixWorkspace::Build(*canonical,proposal,workspace->data(),workspace->size()));
 const auto builds=Symbols(argv[1],"Projection::BfBuildDenseSchedule(");
 const auto reads=Symbols(argv[1],"Projection::BfBankRead(");
 assert(RunBuildWithStub(*m,builds[0],reads[0],d,0x3000200+slot*128,workspace->data()+slot*32));
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<64;i++)m->memory[0x170000+w*256+i*4]=boots[w][i];
 if(gather){
  auto endpoint=std::make_unique<GatherRoot>(argv[1],*snapshots);endpoint->memory=m->memory;endpoint->memory[d+9*4]=129;endpoint->memory[d+11*4]=rows;
  for(unsigned i=0;i<4;i++)endpoint->memory[0xffb20040+i*0x800]=0;
  for(unsigned i=0;i<32;i++)endpoint->memory[0xffb20240+i*4]=0;
  for(unsigned at:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})endpoint->memory[0xffb20000+at]=0;
  for(unsigned i=0;i<8;i++)endpoint->memory[0x6d000+i*4]=0;
  endpoint->memory[0x74800+6*4]=endpoint->memory[0x74800+7*4]=0;
  for(unsigned w=0;w<119;w++){
   const auto& s=(*snapshots)[w];endpoint->memory[0x123000+w*4]=s.doorbell;
   for(unsigned i=0;i<8;i++)endpoint->memory[0x122000+(w*8+i)*4]=s.before[i];
  }
  const auto gathers=Symbols(argv[1],"Projection::BfGatherMatrix(");const auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");
  std::ifstream syms(std::string(argv[1])+"/program.demangled");std::string line;unsigned packetSeal=0;
  while(std::getline(syms,line))if(line.find(" T q38_layer_return_recurrent_packet_seal")!=std::string::npos)packetSeal=unsigned(std::stoul(line.substr(0,8),nullptr,16));
  assert(packetSeal);
  for(unsigned w=0;w<119;w++)assert(endpoint->RunGather(gathers[0],packetSeal,d,0x6d000,w));
  assert(endpoint->Run(seals[0],d) && endpoint->packets && endpoint->seals);
  results.resize(rows);for(unsigned row=0;row<rows;row++)results[row]=endpoint->memory.at(0x80000+row*4);
  assert(fwrite(results.data(),sizeof(uint32_t),rows,stdout)==rows && !fflush(stdout));return 0;
 }
 if(store){
  m->memory[d+11*4]=rows;const auto stores=Symbols(argv[1],"Projection::BfStoreDenseMatrix(");
  const auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");std::vector<uint8_t> seen(rows,0);
  for(unsigned row=0;row<rows;row++)m->memory[0x80000+row*4]=0x7fc00001;
  for(unsigned w=0;w<119;w++){
   const unsigned first=m->memory.at(0x16ebc0+w*16+8),count=m->memory.at(0x16ebc0+w*16+12)*32;
   assert(count && first<rows && count<=rows-first);
   for(unsigned i=0;i<count;i++){assert(!seen[first+i]++);m->memory[0x78000+(64+i)*4]=results[first+i];}
   assert(m->Run(stores[0],d,w));
  }
  assert(m->Run(seals[0],d)); // actual full pre-scatter seal, called by this fixture
  for(unsigned row=0;row<rows;row++){assert(seen[row]==1);results[row]=m->memory.at(0x80000+row*4);}
  assert(fwrite(results.data(),sizeof(uint32_t),rows,stdout)==rows && !fflush(stdout));return 0;
 }
 const auto descriptors=Symbols(argv[1],"Projection::BfDescriptor(");
 const auto producers=Symbols(argv[1],"Projection::BfProducerJob(");
 for(unsigned w=0;w<width/256;w++){
  uint32_t descriptor[64],producer[64];
  assert(Call(*m,descriptors[0],d,w,0,0x78000,false));
  for(unsigned i=0;i<64;i++)descriptor[i]=m->memory.at(0x78000+i*4);
  assert(Call(*m,producers[0],d,w,1,0x78000,true));
  for(unsigned i=0;i<64;i++)producer[i]=m->memory.at(0x78000+i*4);
  assert(fwrite(descriptor,sizeof(uint32_t),64,stdout)==64 && fwrite(producer,sizeof(uint32_t),64,stdout)==64);
 }
 for(unsigned w=0;w<119;w++){
  const unsigned previous=w<width/256?1:0;assert(m->Run(m->job[0],d,w,previous+1));uint32_t job[64];
  for(unsigned i=0;i<64;i++)job[i]=m->memory.at(0x78000+i*4);
  assert(Q::JobShape(job,boots[w].data(),previous,0));assert(fwrite(job,sizeof(uint32_t),64,stdout)==64);
 }
 assert(!fflush(stdout));
}
