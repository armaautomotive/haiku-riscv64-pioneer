/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION 1
#include "../PioneerQ38Bf16ProducerLayout.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 namespace P=PioneerQ38Bf16ProducerLayout;unsigned tested=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned role:{5U,6U,13U}){
  const unsigned width=role==13?6144:5120,chunks=width/256;
  for(unsigned chunk=0;chunk<chunks;chunk++){
   uint32_t d[64]={},source[16]={},job[16]={};const unsigned worker=chunk+1;
   d[0]=P::Magic;d[1]=1;d[2]=67108864;d[4]=0x04000000;d[5]=chunk;d[6]=9;d[7]=worker;
   d[8]=PioneerQ38ProjectionRoute(0);d[9]=PioneerQ38GlobalWorkerLayout::Peer(worker);
   d[10]=P::ProjectionInput+chunk*1024;d[11]=width;d[12]=P::NativeOffset+chunk*512;
   d[13]=P::LinesOffset+chunk*128;d[14]=P::Format;d[15]=chunks;
   for(unsigned i=0;i<16;i++)d[16+i]=source[i]=i+1;
   for(unsigned i=0;i<8;i++)d[32+i]=i+1;
   d[42]=layer;d[43]=role;
   job[0]=P::Magic;job[1]=256;job[2]=512;job[3]=9;job[4]=12;job[10]=P::Witness;
   job[11]=worker;job[12]=1;job[13]=5;
   assert(P::RecurrentQ8Role(d) && P::Width(d)==width && P::DescriptorShape(d));
   assert(P::Job(job,d,source,worker,d[9]));
   d[11]++;assert(!P::DescriptorShape(d));d[11]--;
   d[42]=3;assert(!P::RecurrentQ8Role(d));d[42]=layer;
   d[43]=7;assert(!P::RecurrentQ8Role(d) && !P::DescriptorShape(d));
   tested++;
  }
 }
 assert(tested==48*(20+20+24));
 std::printf("PASS %u recurrent BF16 producer chunks and malformed controls; NO DEVICE\n",tested);
}
