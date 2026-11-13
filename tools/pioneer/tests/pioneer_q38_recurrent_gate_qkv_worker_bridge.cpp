/* SPDX-License-Identifier: MIT */
// Sequential per-peer gate -> QKV with one retained simulated worker.  No card.
#define PIONEER_Q38_RECURRENT_WORKER_BRIDGE_HELPER_ONLY 1
#include "pioneer_q38_recurrent_worker_job_bridge.cpp"

struct EmittedJobs {
 std::array<std::array<uint32_t,64>,20> descriptor,producer;
 std::array<std::array<uint32_t,64>,119> matrix;
 void Read(){
  for(unsigned w=0;w<20;w++){
   assert(fread(descriptor[w].data(),4,64,stdin)==64);
   assert(fread(producer[w].data(),4,64,stdin)==64);
  }
  assert(fread(matrix.data(),4,119*64,stdin)==119*64);
 }
};
static std::vector<uint8_t> Projection(const char* path,unsigned tensor,unsigned rows){
 std::ifstream f(path,std::ios::binary);std::vector<uint8_t> data{std::istreambuf_iterator<char>(f),{}};
 constexpr unsigned width=5120;const uint32_t head[]={0x31525051,1,tensor,width,rows,rows/32,width/32,1088};
 assert(data.size()==32+size_t(rows/32)*((width/32)*1088+128));assert(!std::memcmp(data.data(),head,sizeof(head)));
 return data;
}
static void Producer(DenseProducerImage& image,unsigned w,unsigned role,unsigned seq,const EmittedJobs& jobs){
 image.steps=0;
 for(unsigned i=0;i<256;i++){const unsigned at=w*256+i;
  image.input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
 image.StageProducer(role,seq,false,0);
 for(unsigned i=0;i<64;i++)assert(image.descriptor[i]==jobs.descriptor[w][i]
  && image.Get(HW::Job+i*4)==jobs.producer[w][i]);
 image.Check(seq);
}
static void Matrix(DenseProducerImage& image,const std::array<uint32_t,64>& job,const std::vector<uint8_t>& projection,
 const std::vector<uint8_t>& input,std::vector<uint32_t>& rows,std::vector<uint8_t>& seen,PioneerRecurrentWorkerSnapshot* snapshot){
 constexpr unsigned width=5120;const unsigned bands=job[20],first=job[32],sequence=job[1];
 assert(bands && bands<=image.native.maxBands && first+bands*32<=rows.size());
 image.RearmTestAccounting();image.steps=0;image.transfer.job.words[4]=bands;
 for(unsigned i=0;i<16;i++)image.transfer.job.words[i]=job[16+i];
 image.transfer.weights.resize(size_t(bands)*(width/32)*1088);image.transfer.root=input;
 for(unsigned band=0;band<bands;band++){
  const size_t bytes=size_t(width/32)*1088,source=32+size_t(first/32+band)*(bytes+128);
  assert(source+bytes+128<=projection.size());
  std::memcpy(image.transfer.weights.data()+size_t(band)*bytes,projection.data()+source,bytes);
 }
 assert(HW::JobShape(job.data(),image.boot.data(),sequence-1,0));
 const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],closed[64];
 for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);image.Put(HW::Doorbell,sequence);
 assert(image.FinishCombined(sequence) && image.native.Clean());
 for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);closed[i]=image.Get(HW::Job+i*4);}
 for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
 assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,closed,image.boot.data(),before,after));
 for(unsigned band=0;band<bands;band++)for(unsigned r=0;r<32;r++){
  const size_t bytes=size_t(width/32)*1088,at=32+size_t(first/32+band)*(bytes+128)+bytes+r*4;
  uint32_t expected;std::memcpy(&expected,projection.data()+at,4);
  const unsigned value=image.Get(W::Output+(band*32+r)*4);assert(value==expected && !seen[first+band*32+r]++);rows[first+band*32+r]=value;
  if(snapshot)snapshot->output[band*32+r]=value;
 }
 if(snapshot){
  snapshot->pc=image.pc;snapshot->doorbell=image.Get(HW::Doorbell);
  for(unsigned i=0;i<8;i++){snapshot->before[i]=before[i];snapshot->after[i]=after[i];}
  for(unsigned i=0;i<16;i++)snapshot->status[i]=image.Get(HW::Status+i*4);
  for(unsigned i=0;i<64;i++){snapshot->job[i]=closed[i];snapshot->proof[i]=proof[i];}
 }
}
int main(int argc,char** argv){
 assert(argc==5);auto gate=Projection(argv[3],3,6144),qkv=Projection(argv[4],5,10240);
 EmittedJobs first,second;first.Read();second.Read();assert(fgetc(stdin)==EOF);
 std::vector<uint8_t> input(5120*2);uint32_t in[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;in[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(in,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
 }
 std::vector<uint32_t> gates(6144),qkvs(10240);std::vector<uint8_t> gateSeen(gates.size()),qkvSeen(qkvs.size());
 std::array<PioneerRecurrentWorkerSnapshot,119> snapshots{};
 for(unsigned w=0;w<119;w++){
  const unsigned maxBands=std::max(first.matrix[w][20],second.matrix[w][20]);
  DenseProducerImage image(argv[1],argv[2],5120,maxBands,w+1);image.Boot();
  if(w<20)Producer(image,w,5,1,first);
  Matrix(image,first.matrix[w],gate,input,gates,gateSeen,nullptr);
  auto& s=snapshots[w];SnapshotStage(image,s.initial,false);
  for(unsigned i=0;i<64;i++)s.initial.proof[i]=image.Get(0x83f00+i*4);
  if(w<20){Producer(image,w,6,3,second);SnapshotStage(image,s.producer,true);}
  Matrix(image,second.matrix[w],qkv,input,qkvs,qkvSeen,&s);
  assert(s.initial.doorbell==(w<20?2U:1U) && s.doorbell==(w<20?4U:2U));
 }
 for(auto v:gateSeen)assert(v==1);for(auto v:qkvSeen)assert(v==1);
 assert(fwrite(gates.data(),4,gates.size(),stdout)==gates.size());
 assert(fwrite(qkvs.data(),4,qkvs.size(),stdout)==qkvs.size());
 assert(fwrite(snapshots.data(),sizeof(snapshots[0]),snapshots.size(),stdout)==snapshots.size());
 assert(!fflush(stdout));
}
