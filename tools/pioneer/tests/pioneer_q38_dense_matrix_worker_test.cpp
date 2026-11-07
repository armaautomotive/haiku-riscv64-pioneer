/* SPDX-License-Identifier: MIT */
// Execute the actual standalone QDM2 RV32 worker with synthetic NoC and
// independently modeled Blackhole matrix operations. No hardware or Work.
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_DENSE_PARTITION_MACHINE_ONLY
#include "pioneer_q38_dense_partition_image_test.cpp"
#include "pioneer_q38_native_bfp_machine.h"

struct MatrixPartitionMachine:PartitionMachine {
 NativeBfpMachine fpu;bool matrix=false;
 MatrixPartitionMachine(const char* image,const ProducedGroup& input,unsigned ordinal)
  :PartitionMachine(image,input,ordinal){fpu.expectedWeight=0xa5000;fpu.expectedInput=0xa6000;}
 uint32_t Load(uint32_t a)override{
  if((a>=0xa4ff0 && a<0xa5800) || (a>=0xa5ff0 && a<0xa6200)){
   assert(!(a&3));uint32_t v;memcpy(&v,fpu.l1.data()+a,4);return v;
  }
  if(matrix && a==0xffe000fc){fpu.Drain();return 1;}
  if(matrix && a>=0xffef0000 && a<0xffef0000+224*4)return fpu.config[(a-0xffef0000)/4];
  if(matrix && ((a>=0xffbd8800 && a<0xffbd8840) || (a>=0xffbd8a00 && a<0xffbd8a40))){
   unsigned row=(a-0xffbd8000)/64,col=((a-0xffbd8000)%64)/4;
   assert(fpu.valid[row] && !fpu.mathPending);return NativeBfpMachine::Bits(fpu.destination[row][col]);
  }
  return PartitionMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  if((a>=0xa4ff0 && a<0xa5800) || (a>=0xa5ff0 && a<0xa6200)){
   assert(!(a&3));memcpy(fpu.l1.data()+a,&v,4);return;
  }
  if(a==0xffe40000 && v==0xb2000000)matrix=false;
  if(a==0xffe40000 && v==0xb2020001){
   assert(!fpu.Pending() && !fpu.mathPending);
   std::fill(std::begin(fpu.thread),std::end(fpu.thread),0U);matrix=true;
  }
  if(matrix && a==0xffe000fc)return;
  if(matrix && a==0xffe40000){
   if(v!=0xa2101816 && v!=0x4500017e)fpu.Op(v);
   return;
  }
  if(matrix && a>=0xffef0000 && a<0xffef0000+224*4){fpu.Config((a-0xffef0000)/4,v);return;}
  PartitionMachine::Store(a,v);
 }
};

static uint16_t HalfFixture(unsigned n){
 return uint16_t((n%5==0?0x8000:0) | (0x2c00+(n*37%0x1000)));
}
static void RunCase(const char* image,const char* reference,unsigned tokens,bool synthetic=false){
 std::unique_ptr<ProducedGroup> source(synthetic?new ProducedGroup(0,tokens):new ProducedGroup(reference,0,tokens));
 ProducedGroup& input=*source;
 if(synthetic){
  input.keys.resize((tokens-1)*256);input.values.resize((tokens-1)*256);
  for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++)
   input.query[h*256+d]=PioneerAttentionBits(PioneerAttentionHalf(HalfFixture(h*271+d*3+1)));
  for(unsigned t=0;t<tokens-1;t++)for(unsigned d=0;d<256;d++){
   input.keys[t*256+d]=HalfFixture(t*257+d*7+5);
   input.values[t*256+d]=HalfFixture(t*263+d*11+7);
  }
  for(unsigned pair=0;pair<128;pair++){
   input.currentK[pair]=HalfFixture(pair*2+13)|(unsigned(HalfFixture(pair*2+14))<<16);
   input.currentV[pair]=HalfFixture(pair*2+17)|(unsigned(HalfFixture(pair*2+18))<<16);
  }
  for(unsigned h=0;h<6;h++){
   input.scores[h].resize(tokens);
   for(unsigned t=0;t<W::Count(tokens,0);t++){
    double score=0;
    for(unsigned d=0;d<256;d++)score+=double(PioneerAttentionFloat(input.query[h*256+d]))*PioneerAttentionHalf(input.keys[t*256+d]);
    input.scores[h][t]=PioneerAttentionBits(float(score/16));
   }
  }
 }
 MatrixPartitionMachine worker(image,input,0);
 assert(worker.Boot() && worker.RunPhase(1));
 const unsigned first=worker.header[48],count=worker.header[49],chunks=(count+31)/32;
 double scoreError=0,scoreNorm=0;uint32_t maxima[6];
 for(unsigned h=0;h<6;h++)for(unsigned t=0;t<count;t++){
  const double actual=PioneerAttentionFloat(worker.memory.at(W::Scores+(h*384+t)*4));
  const double expected=PioneerAttentionFloat(input.scores[h][first+t]);
  scoreError+=(actual-expected)*(actual-expected);scoreNorm+=expected*expected;
 }
 for(unsigned h=0;h<6;h++)maxima[h]=worker.memory.at(W::Local+(4+h)*4);
 worker.Stage(2);
 for(unsigned h=0;h<6;h++)worker.memory[W::Parameters+h*4]=maxima[h];
 assert(worker.Wait(2));
 uint32_t inverse[6];
 for(unsigned h=0;h<6;h++){
  const unsigned hi=worker.memory.at(W::Local+(16+h*4)*4),lo=worker.memory.at(W::Local+(17+h*4)*4);
  const int exponent=int32_t(worker.memory.at(W::Local+(18+h*4)*4));
  const double sum=std::ldexp(double(hi)*4294967296.0+double(lo),exponent-52);
  assert(sum>=1);inverse[h]=PioneerAttentionBits(float(1.0/sum));
 }
 worker.Stage(3);
 for(unsigned h=0;h<6;h++){
  worker.memory[W::Parameters+h*4]=maxima[h];
  worker.memory[W::Parameters+(8+h)*4]=inverse[h];
 }
 assert(worker.Wait(3));
 double error=0,norm=0;
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  const double actual=double(PioneerAttentionFloat(worker.memory.at(W::Partial+(h*512+d*2)*4)))
   +double(PioneerAttentionFloat(worker.memory.at(W::Partial+(h*512+d*2+1)*4)));
  double expected=0;
  for(unsigned t=0;t<count;t++){
   const unsigned global=first+t;uint16_t half;
   if(global==tokens-1){unsigned pair=input.currentV[d/2];half=uint16_t(d&1?pair>>16:pair);}
   else half=input.values[global*256+d];
   const float probability=PioneerAttentionFloat(worker.memory.at(W::Scores+(h*384+t)*4));
   expected+=double(PioneerAttentionHalf(half))*probability;
  }
  error+=(actual-expected)*(actual-expected);norm+=expected*expected;
 }
 assert(std::sqrt(error/norm)<0.01 && std::sqrt(scoreError/scoreNorm)<0.01);
 assert(worker.fpu.matrixOps==6*chunks*(128+4*8*16) && worker.fpu.Clean());
 worker.Stop();
 printf("PASS simulated ACTUAL QDM2 phases 1/2/3/STOP, six heads, tokens=%u partition=%u; score_relative_l2=%.9g PV_relative_l2=%.9g matrix_ops=%u; synthetic local controller/NoC, NOT hardware or speed\n",
  tokens,count,std::sqrt(scoreError/scoreNorm),std::sqrt(error/norm),worker.fpu.matrixOps);
}
static void RunFullGroup(const char* image,const char* reference,unsigned tokens){
 ProducedGroup input(reference,0,tokens);
 std::vector<std::unique_ptr<MatrixPartitionMachine>> pool;
 for(unsigned ordinal=0;ordinal<24;ordinal++){
  pool.emplace_back(new MatrixPartitionMachine(image,input,ordinal));
  assert(pool.back()->Boot() && pool.back()->RunPhase(1));
 }
 double scoreDifference=0,scoreNorm=0;
 for(const auto& worker:pool)for(unsigned h=0;h<6;h++)for(unsigned t=0;t<worker->header[49];t++){
  const double candidate=PioneerAttentionFloat(worker->memory.at(W::Scores+(h*384+t)*4));
  const double expected=PioneerAttentionFloat(input.scores[h][worker->header[48]+t]);
  scoreDifference+=(candidate-expected)*(candidate-expected);scoreNorm+=expected*expected;
 }
 uint32_t maxima[6],inverse[6];
 for(unsigned h=0;h<6;h++){
  float maximum=-3.4028234663852886e38f;
  for(const auto& worker:pool)maximum=std::max(maximum,PioneerAttentionFloat(worker->memory.at(W::Local+(4+h)*4)));
  maxima[h]=PioneerAttentionBits(maximum);
 }
 for(auto& worker:pool){
  worker->Stage(2);for(unsigned h=0;h<6;h++)worker->memory[W::Parameters+h*4]=maxima[h];
  assert(worker->Wait(2));
 }
 for(unsigned h=0;h<6;h++){
  double sum=0;
  for(const auto& worker:pool){
   const unsigned hi=worker->memory.at(W::Local+(16+h*4)*4),lo=worker->memory.at(W::Local+(17+h*4)*4);
   const int exponent=int32_t(worker->memory.at(W::Local+(18+h*4)*4));
   sum+=std::ldexp(double(hi)*4294967296.0+double(lo),exponent-52);
  }
  assert(sum>=1);inverse[h]=PioneerAttentionBits(float(1.0/sum));
 }
 for(auto& worker:pool){
  worker->Stage(3);
  for(unsigned h=0;h<6;h++){
   worker->memory[W::Parameters+h*4]=maxima[h];
   worker->memory[W::Parameters+(8+h)*4]=inverse[h];
  }
  assert(worker->Wait(3));
 }
 double error=0,norm=0,pvError=0,pvNorm=0,probError=0,probNorm=0;
 for(const auto& worker:pool)for(unsigned h=0;h<6;h++)for(unsigned t=0;t<worker->header[49];t++){
  const double probability=PioneerAttentionFloat(worker->memory.at(W::Scores+(h*384+t)*4));
  const double expected=PioneerAttentionFloat(input.probability[h][worker->header[48]+t]);
  probError+=(probability-expected)*(probability-expected);probNorm+=expected*expected;
 }
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  double sum=0,exactPV=0;
  for(const auto& worker:pool){
   sum+=double(PioneerAttentionFloat(worker->memory.at(W::Partial+(h*512+d*2)*4)));
   sum+=double(PioneerAttentionFloat(worker->memory.at(W::Partial+(h*512+d*2+1)*4)));
   for(unsigned t=0;t<worker->header[49];t++){
    const unsigned global=worker->header[48]+t;uint16_t half;
    if(global==tokens-1){const unsigned pair=input.currentV[d/2];half=uint16_t(d&1?pair>>16:pair);}
    else half=input.values[global*256+d];
    exactPV+=double(PioneerAttentionHalf(half))*PioneerAttentionFloat(worker->memory.at(W::Scores+(h*384+t)*4));
   }
  }
  const double expected=input.output[h][d];error+=(sum-expected)*(sum-expected);norm+=expected*expected;
  pvError+=(sum-exactPV)*(sum-exactPV);pvNorm+=exactPV*exactPV;
 }
 const double relative=std::sqrt(error/norm);
 if(relative>=0.01){fprintf(stderr,"FAIL 24-partition tokens=%u total_l2=%.9g qk_l2=%.9g probability_l2=%.9g pv_l2=%.9g threshold=0.01\n",tokens,relative,std::sqrt(scoreDifference/scoreNorm),std::sqrt(probError/probNorm),std::sqrt(pvError/pvNorm));fflush(stderr);}
 assert(relative<0.01);
 unsigned ops=0;for(auto& worker:pool){assert(worker->fpu.Clean());ops+=worker->fpu.matrixOps;worker->Stop();}
 printf("PASS simulated ACTUAL QDM2 24 partitions, six heads, tokens=%u; merged_PV_relative_l2=%.9g matrix_ops=%u; synthetic global controller/NoC, NOT hardware or speed\n",tokens,relative,ops);
}
int main(int argc,char** argv){
 assert(argc==3);(void)&D::Run;(void)&Group;(void)&Geometry;(void)&Faults;
 for(unsigned tokens: {1U,33U,65U})RunCase(argv[1],argv[2],tokens);
 RunCase(argv[1],argv[2],769,true); // 33-token partition spans two chunks
 for(unsigned tokens: {1U,33U,65U})RunFullGroup(argv[1],argv[2],tokens);
}
