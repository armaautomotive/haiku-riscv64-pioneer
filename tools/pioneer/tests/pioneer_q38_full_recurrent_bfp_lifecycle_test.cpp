/* SPDX-License-Identifier: MIT */
// One full recurrent layer across 119 retained simulated QGWF workers. The
// scheduler and NoC are modeled by this fixture; NO device or Work grant.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_BFP_GDN_TRANSITION_TEST 1
#define PIONEER_Q38_RECURRENT_GATE_QKV_HELPER_ONLY 1
#include "pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp"
#include "PioneerQ38BfpGdnWorker.h"
#include "../PioneerQ38RecurrentSmallMatrixSchedule.h"
#define PIONEER_Q38_CONV_MACHINE_ONLY 1
#define PIONEER_GRAPH_MATH_BASE_ALREADY_INCLUDED 1
#undef PIONEER_Q38_GDN_MUTABLE_STATE
#include "pioneer_q38_conv_image_test.cpp"
#define main independent_bfp_helper_main
#include "pioneer_dense_band_independent_check.cpp"
#undef main

static std::vector<uint8_t> LoadProjection(const char* path,unsigned index,unsigned width,unsigned rows){
 std::ifstream f(path,std::ios::binary);std::vector<uint8_t> data{std::istreambuf_iterator<char>(f),{}};
 const uint32_t header[]={0x31525051,1,index,width,rows,rows/32,width/32,1088};
 assert(data.size()==32+size_t(rows/32)*(size_t(width/32)*1088+128) && !std::memcmp(data.data(),header,sizeof(header)));
 return data;
}
static std::array<uint32_t,64> Job(PioneerQ38BfpGdnWorker& image,unsigned sequence,unsigned role,unsigned first,unsigned bands,unsigned rows,unsigned width){
 std::array<uint32_t,64> j{};j[0]=HW::Magic;j[1]=sequence;j[2]=1;j[3]=image.boot[3];j[4]=image.boot[4];j[7]=2025;
 for(unsigned i=0;i<8;i++)j[8+i]=image.boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=image.transfer.job.words[i];
 j[18]=width;j[20]=bands;j[27]=sequence;j[32]=first;j[33]=rows;j[34]=role;j[35]=8;
 assert(HW::JobShape(j.data(),image.boot.data(),sequence-1,0));return j;
}
static std::vector<uint32_t> IndependentOutput(const std::vector<uint8_t>& projection,const std::vector<uint8_t>& input){
 assert(input.size()==6144*2);std::vector<uint32_t> result(5120);
 for(unsigned band=0;band<160;band++){
  std::vector<uint32_t> words(192*1104/4,0);
  for(unsigned t=0;t<192;t++)std::memcpy(reinterpret_cast<uint8_t*>(words.data())+t*1104+16,
   projection.data()+32+size_t(band)*(192*1088+128)+t*1088,1088);
  for(unsigned row=0;row<32;row++){
   uint32_t value=0;for(unsigned t=0;t<192;t++)for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){a[c]=Weight(words,t,row,half*16+c);unsigned at=(t*32+half*16+c)*2;b[c]=uint16_t(input[at]|unsigned(input[at+1])<<8);}
    for(unsigned phase=0;phase<4;phase++)value=Instruction(a,b,phase,value);
   }result[band*32+row]=value;
  }
 }return result;
}
int main(int argc,char** argv){
 assert(argc==10 || (argc==11 && (!std::strcmp(argv[10],"--controller-input") || !std::strcmp(argv[10],"--capture-controller"))));
 const bool capture=argc==11 && !std::strcmp(argv[10],"--capture-controller");
 auto frames=std::make_unique<std::array<std::array<std::array<uint32_t,64>,42>,2>>();
 auto captured=std::make_unique<std::array<std::array<PioneerRecurrentWorkerSnapshot,119>,2>>();
 if(capture)assert(fread(frames->data(),sizeof(*frames),1,stdin)==1 && fgetc(stdin)==EOF);
 auto gate=LoadProjection(argv[3],3,5120,6144),qkvWeights=LoadProjection(argv[4],5,5120,10240);
 std::array<std::vector<uint8_t>,2> small;
 for(unsigned k=0;k<2;k++){std::ifstream f(argv[5+k],std::ios::binary);small[k].assign(std::istreambuf_iterator<char>(f),{});assert(small[k].size()==32+2*(160*1088+128));}
 std::ifstream lf(argv[7],std::ios::binary);std::vector<uint32_t> learned(41184);assert(lf.read(reinterpret_cast<char*>(learned.data()),learned.size()*4) && lf.peek()==EOF);
 auto outputWeights=LoadProjection(argv[8],16,6144,5120);
 std::vector<uint8_t> input(5120*2);uint32_t raw[256],packed[128];
 for(unsigned chunk=0;chunk<20;chunk++){
  for(unsigned i=0;i<256;i++){unsigned at=chunk*256+i;raw[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  assert(PioneerQ38Bf16Input::Pack(raw,256,packed,128));std::memcpy(input.data()+chunk*512,packed,512);
 }
 std::vector<std::unique_ptr<PioneerQ38BfpGdnWorker>> workers;workers.reserve(119);std::array<unsigned,119> sequence{};
 for(unsigned w=0;w<119;w++){
  auto image=std::make_unique<PioneerQ38BfpGdnWorker>(argv[1],argv[2],5120,3,w+1);image->Boot();
  if(w<20){for(unsigned i=0;i<256;i++){unsigned at=w*256+i;image->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
   image->StageProducer(5,1,false,0);image->Check(1);sequence[w]=1;}
  workers.push_back(std::move(image));
 }
 std::vector<uint32_t> gates(6144),qkv(10240);std::vector<uint8_t> gateSeen(6144),qkvSeen(10240);
 for(unsigned phase=0;phase<2;phase++)for(unsigned w=0;w<119;w++){
  const unsigned groups=phase?320:192,base=groups/119,extra=groups%119;
  unsigned bands=base+unsigned(w<extra),first=(w*base+std::min(w,extra))*32,role=phase?6:5,rows=phase?10240:6144;
  auto& image=*workers[w];auto j=Job(image,++sequence[w],role,first,bands,rows,5120);
  Matrix(image,j,phase?qkvWeights:gate,input,phase?qkv:gates,phase?qkvSeen:gateSeen,nullptr);
 }
 for(auto v:gateSeen)assert(v==1);for(auto v:qkvSeen)assert(v==1);
 std::fprintf(capture?stderr:stdout,"FULL_RECURRENT_PROGRESS all6144 gate and10240 QKV rows from119 retained workers\n");std::fflush(capture?stderr:stdout);
 std::array<std::vector<uint32_t>,2> smallRows={std::vector<uint32_t>(64),std::vector<uint32_t>(64)};
 std::array<std::vector<uint8_t>,2> smallSeen={std::vector<uint8_t>(64),std::vector<uint8_t>(64)};uint32_t alphaBeta[96]={};
 for(unsigned kind=0;kind<2;kind++){
  if(capture)for(unsigned w=0;w<119;w++){
   SnapshotStage(*workers[w],(*captured)[kind][w].initial,kind && w>=2 && w<20);
   for(unsigned i=0;i<64;i++)(*captured)[kind][w].initial.proof[i]=workers[w]->Get(0x83f00+i*4);
  }
  // Match the ROOT's real fan-out: all 20 input chunks are produced before
  // the same first two peers execute each padded small matrix.
  for(unsigned w=0;w<20;w++){
   auto& image=*workers[w];image.StageProducer(10+kind,++sequence[w],false,0);image.Check(sequence[w]);
   if(capture){
    for(unsigned i=0;i<64;i++)assert((*frames)[kind][w*2][i]==image.descriptor[i] && (*frames)[kind][w*2+1][i]==image.Get(HW::Job+i*4));
    SnapshotStage(image,(*captured)[kind][w].producer,true);
   }
   for(unsigned i=0;i<128;i++){const uint32_t actual=image.bank.at(image.descriptor[4]+image.descriptor[12]+i*4);
    assert(!std::memcmp(input.data()+w*512+i*4,&actual,4));std::memcpy(input.data()+w*512+i*4,&actual,4);}
  }
  for(unsigned w=0;w<2;w++){
   auto& image=*workers[w];auto j=capture?(*frames)[kind][40+w]:Job(image,sequence[w]+1,10+kind,w*32,1,64,5120);++sequence[w];
   assert(j[1]==sequence[w]);Matrix(image,j,small[kind],input,smallRows[kind],smallSeen[kind],capture?&(*captured)[kind][w]:nullptr);
  }
  for(unsigned i=0;i<48;i++)alphaBeta[kind*48+i]=smallRows[kind][i];
 }
 for(unsigned k=0;k<2;k++)for(unsigned i=0;i<64;i++){assert(smallSeen[k][i]==1);if(i>=48)assert(!smallRows[k][i]);}
 if(argc==11 && !capture){
  uint32_t fromController[96];assert(fread(fromController,sizeof(fromController),1,stdin)==1 && fgetc(stdin)==EOF);
  for(unsigned i=0;i<96;i++)assert(fromController[i]==alphaBeta[i]);
  std::memcpy(alphaBeta,fromController,sizeof(alphaBeta));
 }
 std::vector<uint32_t> conv(10240);std::vector<uint8_t> convSeen(10240);
 for(unsigned w=0;w<119;w++){
  auto& image=*workers[w];image.steps=0;image.StageConv(++sequence[w],qkv,learned);assert(image.FinishCombined(sequence[w]));image.VerifyConv(sequence[w]);
  for(unsigned g=0;g<PioneerQ38ConvBatchCount(w+1);g++)for(unsigned c=0;c<16;c++){
   unsigned row=image.table[g*64+7]+c;assert(row<10240 && !convSeen[row]++);conv[row]=image.group[g].output[c];
  }
 }
 // The six ROOT-owned groups use a separate actual QCV1 instruction image;
 // the parent ROOT lifecycle itself is NOT executed by this fixture.
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 for(unsigned g=0;g<6;g++){
  unsigned first=g*120*16;ConvMachine root(argv[9],*canonical,0,first,0,0,0,0);
  for(unsigned c=0;c<16;c++){root.input[c]=qkv[first+c];for(unsigned t=0;t<4;t++)root.weight[c*4+t]=learned[(first+c)*4+t];}
  assert(root.Run());root.Verify();for(unsigned c=0;c<16;c++){assert(!convSeen[first+c]++);conv[first+c]=root.output[c];}
 }
 for(auto v:convSeen)assert(v==1);
 std::fprintf(capture?stderr:stdout,"FULL_RECURRENT_PROGRESS all10240 Conv rows; ROOT groups separate QCV1 simulation\n");std::fflush(capture?stderr:stdout);
 for(unsigned w=0;w<96;w++){
  auto& image=*workers[w];image.peer=workers[w^1].get();image.StageGdn(++sequence[w],0,0,false,alphaBeta,&learned,&gates,nullptr,nullptr,&conv);
 }
 std::vector<uint32_t> gated(6144);
 for(unsigned pair=0;pair<48;pair++){
  auto* a=workers[pair*2].get();auto* b=workers[pair*2+1].get();unsigned rounds=0;
  while((!a->Done(sequence[pair*2]) && a->pc!=a->faultPC) || (!b->Done(sequence[pair*2+1]) && b->pc!=b->faultPC)){
   assert(++rounds<1000000);for(unsigned side=0;side<2;side++){auto* image=side?b:a;unsigned seq=sequence[pair*2+side];
    for(unsigned i=0;i<128 && !image->Done(seq) && image->pc!=image->faultPC;i++)image->StepGdn();}
  }
  assert(a->pc!=a->faultPC && b->pc!=b->faultPC);a->VerifyGdn(sequence[pair*2]);b->VerifyGdn(sequence[pair*2+1]);
  for(unsigned side=0;side<2;side++){auto* image=side?b:a;for(unsigned i=0;i<64;i++)gated[(pair*2+side)*64+i]=image->gated[i];image->gdn=false;}
 }
 std::fprintf(capture?stderr:stdout,"FULL_RECURRENT_PROGRESS all6144 gated values from48 retained worker pairs\n");std::fflush(capture?stderr:stdout);
 std::vector<uint8_t> packedGated(6144*2);
 for(unsigned chunk=0;chunk<24;chunk++){
  assert(PioneerQ38Bf16Input::Pack(gated.data()+chunk*256,256,packed,128));std::memcpy(packedGated.data()+chunk*512,packed,512);
  auto& image=*workers[chunk];for(unsigned i=0;i<256;i++)image.input[i]=gated[chunk*256+i];
  image.StageProducer(13,++sequence[chunk],false,0);image.Check(sequence[chunk]);
 }
 auto reference=IndependentOutput(outputWeights,packedGated);std::vector<uint32_t> output(5120);std::vector<uint8_t> outputSeen(5120);
 for(unsigned w=0;w<119;w++){
  const unsigned groups=160,base=groups/119,extra=groups%119,bands=base+unsigned(w<extra),first=(w*base+std::min(w,extra))*32;
  auto& image=*workers[w];auto j=Job(image,++sequence[w],13,first,bands,5120,6144);
  Matrix(image,j,outputWeights,packedGated,output,outputSeen,nullptr,&reference);
 }
 for(auto v:outputSeen)assert(v==1);
 if(capture){assert(fwrite(captured->data(),sizeof(*captured),1,stdout)==1 && !fflush(stdout));}
 std::fprintf(capture?stderr:stdout,"PASS layer0 recurrent attention gate/QKV/alpha/beta/Conv/paired GDN/output across119 retained simulated workers, all outputs checked; synthetic normalized input; separate ROOT Conv; NO DEVICE/FULL LAYER/FULL MODEL/WORK/TPS\n");
}
