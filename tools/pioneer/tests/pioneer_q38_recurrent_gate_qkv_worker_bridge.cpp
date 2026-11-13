/* SPDX-License-Identifier: MIT */
// Sequential per-peer gate -> QKV with one retained simulated worker.  No card.
#define PIONEER_Q38_RECURRENT_WORKER_BRIDGE_HELPER_ONLY 1
#include "pioneer_q38_recurrent_worker_job_bridge.cpp"

struct EmittedJobs {
 std::array<std::array<uint32_t,64>,24> descriptor,producer;
 std::array<std::array<uint32_t,64>,119> matrix;
 void Read(unsigned chunks){
  assert(chunks==20 || chunks==24);for(unsigned w=0;w<chunks;w++){
   assert(fread(descriptor[w].data(),4,64,stdin)==64);
   assert(fread(producer[w].data(),4,64,stdin)==64);
  }
  assert(fread(matrix.data(),4,119*64,stdin)==119*64);
 }
};
static std::vector<uint8_t> Projection(const char* path,unsigned tensor,unsigned width,unsigned rows){
 std::ifstream f(path,std::ios::binary);std::vector<uint8_t> data{std::istreambuf_iterator<char>(f),{}};
 const uint32_t head[]={0x31525051,1,tensor,width,rows,rows/32,width/32,1088};
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
 const unsigned width=job[18],bands=job[20],first=job[32],sequence=job[1];assert(width==5120 || width==6144);
 assert(bands && bands<=image.native.maxBands && first+bands*32<=rows.size());
 image.RearmTestAccounting();image.steps=0;image.native.tiles=width/32;image.transfer.job.words[4]=bands;
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
 assert(argc==5 || argc==6);const bool third=argc==6;
 auto gate=Projection(argv[3],3,5120,6144),qkv=Projection(argv[4],5,5120,10240);
 std::vector<uint8_t> out;if(third)out=Projection(argv[5],16,6144,5120);
 EmittedJobs first,second,last;first.Read(20);second.Read(20);if(third)last.Read(24);assert(fgetc(stdin)==EOF);
 std::vector<uint8_t> input(6144*2);uint32_t in[256],packed[128];
 for(unsigned chunk=0;chunk<24;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;in[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(in,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
 }
 const std::vector<uint8_t> gateInput(input.begin(),input.begin()+5120*2);
 std::vector<uint32_t> gates(6144),qkvs(10240),outputs(third?5120:0);
 std::vector<uint8_t> gateSeen(gates.size()),qkvSeen(qkvs.size()),outSeen(outputs.size());
 std::array<PioneerRecurrentWorkerSnapshot,119> snapshots{},outSnapshots{};
 for(unsigned w=0;w<119;w++){
  const unsigned maxBands=std::max(std::max(first.matrix[w][20],second.matrix[w][20]),third?last.matrix[w][20]:0U);
  DenseProducerImage image(argv[1],argv[2],5120,maxBands,w+1);image.Boot();
  if(w<20)Producer(image,w,5,1,first);
  Matrix(image,first.matrix[w],gate,gateInput,gates,gateSeen,nullptr);
  auto& s=snapshots[w];SnapshotStage(image,s.initial,false);
  for(unsigned i=0;i<64;i++)s.initial.proof[i]=image.Get(0x83f00+i*4);
  if(w<20){Producer(image,w,6,3,second);SnapshotStage(image,s.producer,true);}
  Matrix(image,second.matrix[w],qkv,gateInput,qkvs,qkvSeen,&s);
  assert(s.initial.doorbell==(w<20?2U:1U) && s.doorbell==(w<20?4U:2U));
  if(third){
   auto& o=outSnapshots[w];SnapshotStage(image,o.initial,false);
   for(unsigned i=0;i<64;i++)o.initial.proof[i]=image.Get(0x83f00+i*4);
   image.native.tiles=6144/32;
   if(w<24){Producer(image,w,13,w<20?5:3,last);SnapshotStage(image,o.producer,true);}
   Matrix(image,last.matrix[w],out,input,outputs,outSeen,&o);
   assert(o.initial.doorbell==(w<20?4U:2U) && o.doorbell==(w<20?6U:w<24?4U:3U));
  }
 }
 for(auto v:gateSeen)assert(v==1);for(auto v:qkvSeen)assert(v==1);
 for(auto v:outSeen)assert(v==1);
 assert(fwrite(gates.data(),4,gates.size(),stdout)==gates.size());
 assert(fwrite(qkvs.data(),4,qkvs.size(),stdout)==qkvs.size());
 assert(fwrite(snapshots.data(),sizeof(snapshots[0]),snapshots.size(),stdout)==snapshots.size());
 if(third){assert(fwrite(outputs.data(),4,outputs.size(),stdout)==outputs.size());
  assert(fwrite(outSnapshots.data(),sizeof(outSnapshots[0]),outSnapshots.size(),stdout)==outSnapshots.size());}
 assert(!fflush(stdout));
}
