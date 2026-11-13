/* SPDX-License-Identifier: MIT */
// Inspection-only ROOT job emitter. stdin: 119 simulated worker boot records;
// stdout: linked ROOT producer descriptors/jobs, then 119 matrix jobs.
// The host pipe is inspection transport, not a simulated ROOT NoC.
#define main recurrent_root_helper_main
#include "pioneer_q38_recurrent_actual_root_test.cpp"
#undef main
#include <cstdlib>
#include "PioneerRecurrentWorkerSnapshot.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedRecurrentRecipe.h"

// ROOT NoC endpoint backed only by bytes captured from completed worker
// simulators. Packet sealing runs against a synthetic owner and MMIO state.
struct GatherRoot:ActualRoot {
 const std::array<PioneerRecurrentWorkerSnapshot,119>& snapshots;unsigned packets=0,seals=0,writes=0;
 bool controller=false;bool jobs[119]={},released[119]={};
 GatherRoot(const char* path,const std::array<PioneerRecurrentWorkerSnapshot,119>& s):ActualRoot(path),snapshots(s){}
 unsigned StepLimit()const override{return controller?200000000U:4000000U;}
 void Store(unsigned address,unsigned value)override{
  if(address!=0xffb20040 || value!=1){ActualRoot::Store(address,value);return;}
  const unsigned remote=memory.at(0xffb20000),route=memory.at(0xffb20008),local=memory.at(0xffb2000c);
  const unsigned command=memory.at(0xffb2001c),bytes=memory.at(0xffb20020);
  if(command==0x212){
   assert((remote&~31U)==0x78000 && route==129 && bytes && !(bytes&3) && bytes<=16384);
   const unsigned destination=memory.at(0xffb20014);unsigned peer=0;
   while(peer<119 && Q::Peer(peer+1)!=destination)peer++;
   if(controller && peer<119){
    if(local==Q::Job){assert(bytes==256 && !jobs[peer] && !released[peer]);
     for(unsigned i=0;i<64;i++)assert(memory.at(remote+i*4)==snapshots[peer].job[i]);jobs[peer]=true;
    }else if(local==Q::Doorbell){assert(bytes==4 && jobs[peer] && !released[peer]);
     assert(memory.at(remote)==snapshots[peer].doorbell);released[peer]=true;
    }else assert(false);
   }else{
    assert(remote==0x78000 && !(bytes&63));
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=memory.at(remote+i*4);writes++;
   }
   memory[0xffb20210]++;memory[0xffb20204]++;memory[0xffb20220]+=(bytes+63)/64;memory[0xffb20228]++;
   memory[address]=0;return;
  }
  assert(!command && bytes && !(bytes&3) && local==0x78000+(remote&31));
  unsigned w=0;while(w<119 && Q::Peer(w+1)!=route)w++;assert(w<119);const auto& s=snapshots[w];
  if(controller)assert(released[w]);
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
 bool RunController(unsigned function,unsigned capture,unsigned prepare,unsigned descriptor){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=descriptor-768;
  regs[10]=descriptor;pc=function;steps=0;controller=true;unsigned captures=0,prepares=0,last=0;
  while(pc!=0x60004 && pc!=0x60000){last=pc;
   if(pc==capture){captures++;Step();}
   else if(pc==prepare){prepares++;pc=regs[1];}
   else Step();
   assert(regs[2]>=0x6e000 && regs[2]<=0x70000);
  }
  if(pc==0x60000)std::fprintf(stderr,"RUN_PARK last=%x ra=%x packets=%u writes=%u\n",last,regs[1],packets,writes);
  return pc==0x60004 && captures==1 && prepares==1 && regs[10]==memory.at(descriptor+11*4);
 }
 bool RunScatter(unsigned function,unsigned descriptor,unsigned tracked){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=descriptor-768;
  regs[10]=descriptor;regs[11]=tracked;pc=function;steps=0;
  unsigned last=0;while(pc!=0x60004 && pc!=0x60000){last=pc;Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);}
  if(pc==0x60000)std::fprintf(stderr,"SCATTER_PARK last=%x ra=%x writes=%u\n",last,regs[1],writes);
  return pc==0x60004;
 }
 bool RunGather(unsigned function,unsigned seal,unsigned descriptor,unsigned tracked,unsigned worker){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=descriptor-768;
  regs[10]=descriptor;regs[11]=tracked;regs[12]=worker;pc=function;steps=0;
  unsigned last=0;while(pc!=0x60004 && pc!=0x60000){last=pc;if(pc==seal)seals++;Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);}
  if(pc==0x60000)std::fprintf(stderr,"GATHER_PARK worker=%u last=%x ra=%x packets=%u seals=%u\n",worker,last,regs[1],packets,seals);
  return pc==0x60004;
 }
};

int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!std::strcmp(argv[3],"--store") || !std::strcmp(argv[3],"--gather") || !std::strcmp(argv[3],"--bad-source") || !std::strcmp(argv[3],"--bad-scatter") || !std::strcmp(argv[3],"--controller") || !std::strcmp(argv[3],"--bad-recipe"))));
 const bool store=argc==4 && !std::strcmp(argv[3],"--store"),badSource=argc==4 && !std::strcmp(argv[3],"--bad-source");
 const bool badScatter=argc==4 && !std::strcmp(argv[3],"--bad-scatter");
 const bool badRecipe=argc==4 && !std::strcmp(argv[3],"--bad-recipe");
 const bool controller=argc==4 && (!std::strcmp(argv[3],"--controller") || badRecipe);
 const bool gather=argc==4 && (!std::strcmp(argv[3],"--gather") || badSource || badScatter || controller);
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
  auto endpoint=std::make_unique<GatherRoot>(argv[1],*snapshots);endpoint->memory=m->memory;
  const unsigned owner=0x6e000,frame=owner+768;
  for(unsigned i=0;i<64;i++)endpoint->memory[frame+i*4]=endpoint->memory.at(d+i*4);
  endpoint->memory[frame+9*4]=129;endpoint->memory[frame+11*4]=rows;endpoint->memory[frame+12*4]=rows*4;
  endpoint->memory[frame+53*4]=0xe0000;
  if(controller){
   endpoint->memory[frame+0*4]=0x31505251;endpoint->memory[frame+1*4]=1;endpoint->memory[frame+2*4]=67108864;
   endpoint->memory[frame+7*4]=119;endpoint->memory[frame+10*4]=PioneerQ38MixedRecurrentOffset(role);
   endpoint->memory[frame+13*4]=0x76800;endpoint->memory[frame+44*4]=119;endpoint->memory[frame+45*4]=1;
   endpoint->memory[frame+48*4]=1;endpoint->memory[frame+49*4]=119;endpoint->memory[frame+50*4]=0x75800;
   endpoint->memory[frame+51*4]=0x76b00;endpoint->memory[frame+52*4]=0x76a00;
   endpoint->memory[frame+53*4]=role==13?0x12a000:0x140000;endpoint->memory[frame+54*4]=0x80000;
   endpoint->memory[frame+55*4]=0x178000;endpoint->memory[frame+56*4]=0x123000;endpoint->memory[frame+57*4]=0x170000;
  }
  for(unsigned i=0;i<64;i++)endpoint->memory[owner+i*4]=0;
  for(unsigned i=0;i<128;i++)endpoint->memory[owner+256+i*4]=endpoint->memory[0x75000+i*4]=0;
  for(unsigned i=0;i<16;i++){
   const unsigned value=0x10000+i,at=i==0?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;
   endpoint->memory[owner+(16+i)*4]=value;endpoint->memory[frame+(16+i)*4]=value;endpoint->memory[0xffb20000+at]=value;
  }
  for(unsigned i=0;i<8;i++)endpoint->memory[owner+(8+i)*4]=endpoint->memory.at(frame+(32+i)*4);
  endpoint->memory[owner+256+48*4]=1;endpoint->memory[owner+1024]=2;endpoint->memory[0x75400]=1;
  if(badSource)endpoint->memory[0xffb20100]^=1;
  for(unsigned i=0;i<4;i++)endpoint->memory[0xffb20040+i*0x800]=0;
  for(unsigned i=0;i<32;i++)endpoint->memory[0xffb20240+i*4]=0;
  for(unsigned at:{0x204U,0x208U,0x20cU,0x210U,0x214U,0x220U,0x228U})endpoint->memory[0xffb20000+at]=0;
  for(unsigned i=0;i<8;i++)endpoint->memory[0x6d000+i*4]=0;
  endpoint->memory[0x74800+6*4]=endpoint->memory[0x74800+7*4]=0;
  for(unsigned w=0;w<119;w++){
   const auto& s=(*snapshots)[w];assert(s.doorbell);endpoint->memory[0x123000+w*4]=s.doorbell-unsigned(controller);
   for(unsigned i=0;i<8;i++)endpoint->memory[0x122000+(w*8+i)*4]=s.before[i];
  }
  const auto gathers=Symbols(argv[1],"Projection::BfGatherMatrix(");const auto seals=Symbols(argv[1],"Projection::BfDenseSeal(");
  std::ifstream syms(std::string(argv[1])+"/program.demangled");std::string line;unsigned packetSeal=0;
  while(std::getline(syms,line))if(line.find(" T q38_layer_return_recurrent_packet_seal")!=std::string::npos)packetSeal=unsigned(std::stoul(line.substr(0,8),nullptr,16));
  assert(packetSeal);
  if(badSource){assert(!endpoint->RunGather(gathers[0],packetSeal,frame,0x6d000,0) && !endpoint->packets && endpoint->seals);return 0;}
  if(!controller){
   for(unsigned w=0;w<119;w++)assert(endpoint->RunGather(gathers[0],packetSeal,frame,0x6d000,w));
   assert(endpoint->Run(seals[0],frame) && endpoint->packets && endpoint->seals);
  }
  for(unsigned i=0;i<64;i++){
   endpoint->memory[0x75300+i*4]=endpoint->memory.at(owner+i*4);
   endpoint->memory[0x74600+i*4]=endpoint->memory.at(frame+i*4);
  }
  for(unsigned i=0;i<width;i++)endpoint->memory[0xe0000+i*4]=0;
  if(controller)for(unsigned i=0;i<width;i++)endpoint->memory[endpoint->memory.at(frame+53*4)+i*4]=0;
  for(unsigned i=0;i<width/2;i++)endpoint->memory[0xa0000+i*4]=endpoint->memory[0xf2000+i*4]=0;
  for(unsigned i=0;i<119*8;i++)endpoint->memory[0x75800+i*4]=endpoint->memory[0x77000+i*4]=0;
  for(unsigned i=0;i<119;i++)endpoint->memory[0x76800+i*4]=endpoint->memory[0x76e00+i*4]=0;
  for(unsigned i=0;i<16;i++)endpoint->memory[0x76b00+i*4]=endpoint->memory[0x76d00+i*4]=0;
  for(unsigned i=0;i<8;i++)endpoint->memory[0x76a00+i*4]=endpoint->memory[0x76d40+i*4]=0;
  if(controller){
   uint32_t recipe[64],header[16]={},banks[8]={},parts[119][8]={},first[119]={};
   for(unsigned i=0;i<64;i++)recipe[i]=i>=48?0:endpoint->memory.at(frame+i*4);
   recipe[0]=TT_Q38_MIXED_RECURRENT_MATRIX;recipe[13]=TT_Q38_DENSE_MATRIX_ROWS;
   const unsigned fields[]={TT_Q38_MIXED_RECURRENT_MATRIX,1,1,8,width,119,2025,TT_Q38_DENSE_MATRIX_INPUT,1};
   for(unsigned i=0;i<9;i++)header[i]=fields[i];
   for(unsigned i=0;i<7;i++)banks[i]=PioneerQ38ProjectionRoute(i);
   unsigned next=0;const unsigned groups=rows/16;
   for(unsigned w=0;w<119;w++){
    parts[w][0]=0x10000000+w*0x100000;parts[w][1]=w%7;parts[w][2]=groups/119+unsigned(w<groups%119);
    parts[w][4]=banks[w%7];parts[w][5]=1;first[w]=next;next+=parts[w][2]*16;
    for(unsigned i=0;i<8;i++)endpoint->memory[0x75800+(w*8+i)*4]=parts[w][i];
    endpoint->memory[0x76800+w*4]=first[w];
   }
   assert(next==rows && PioneerQ38MixedRecurrentRecipe(recipe,header,banks,parts,first));
   for(unsigned i=0;i<16;i++)endpoint->memory[0x76b00+i*4]=header[i];
   for(unsigned i=0;i<8;i++)endpoint->memory[0x76a00+i*4]=banks[i];
  }
  endpoint->memory[0x75000+48*4]=1;
  std::ifstream scatterSymbols(std::string(argv[1])+"/program.demangled");unsigned scatter=0;
  while(std::getline(scatterSymbols,line))if(line.find("Projection::Scatter(")!=std::string::npos)scatter=unsigned(std::stoul(line.substr(0,8),nullptr,16));
  if(badScatter){endpoint->memory[0xffb20100]^=1;assert(scatter && !endpoint->RunScatter(scatter,frame,0x6d000) && !endpoint->writes);return 0;}
  if(controller){
   std::ifstream entries(std::string(argv[1])+"/program.demangled");unsigned run=0,capture=0,prepare=0;
   while(std::getline(entries,line)){
    const unsigned address=unsigned(std::stoul(line.substr(0,8),nullptr,16));
    if(!run && line.find("Projection::Run(")!=std::string::npos)run=address;
    if(line.find("Projection::CaptureRecipe(")!=std::string::npos)capture=address;
    if(!prepare && line.find("Projection::BfPrepare(")!=std::string::npos)prepare=address;
   }
   assert(run && capture && prepare);
   if(badRecipe){endpoint->memory[0x75800+8]=0;
    assert(!endpoint->RunController(run,capture,prepare,frame) && !endpoint->packets && !endpoint->writes);
    for(unsigned w=0;w<119;w++)assert(!endpoint->jobs[w] && !endpoint->released[w]);return 0;
   }
   assert(endpoint->RunController(run,capture,prepare,frame));
   for(unsigned w=0;w<119;w++)assert(endpoint->jobs[w] && endpoint->released[w]);
  }else assert(scatter && endpoint->RunScatter(scatter,frame,0x6d000));
  assert(endpoint->writes==(rows*4+16383)/16384);
  results.resize(rows);for(unsigned row=0;row<rows;row++)results[row]=endpoint->memory.at(0x80000+row*4);
  const unsigned target=0x04000000+(controller?PioneerQ38MixedRecurrentOffset(role):0);
  for(unsigned row=0;row<rows;row++)assert(endpoint->memory.at(target+row*4)==results[row]);
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
