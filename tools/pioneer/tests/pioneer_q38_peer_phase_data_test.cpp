/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerPhaseData.h"
static void Refused(const uint32_t* d,unsigned worker=7,unsigned nonce=31,unsigned type=12,unsigned k=17408,unsigned groups=3){
 uint32_t out[7],before[7];for(unsigned w=0;w<7;w++)out[w]=before[w]=0x12345678+w;
 assert(!PioneerQ38PeerPhaseReadback(d,worker,nonce,type,k,groups,out)&&!memcmp(out,before,sizeof(out)));
}
int main(){
 uint32_t d[16]={PioneerQ38PeerPhaseValid,1,31,7,12,17408,3,1,1,2,3,4,5,6,0,0},out[7],stats[64]={};unsigned cases=0;
 for(unsigned worker=1;worker<120;worker++)for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++){
  d[3]=worker;d[4]=type;d[5]=k;d[6]=groups;
  assert(PioneerQ38PeerPhaseReadback(d,worker,31,type,k,groups,out)&&out[0]==PioneerQ38PeerPhaseValid);
  for(unsigned w=1;w<7;w++)assert(out[w]==d[w+7]);cases++;
 }
 d[3]=7;d[4]=12;d[5]=17408;d[6]=3;
 for(unsigned word=0;word<8;word++)for(unsigned bit=0;bit<32;bit++){
  d[word]^=1U<<bit;Refused(d);d[word]^=1U<<bit;
 }
 for(unsigned word:{14U,15U})for(unsigned bit=0;bit<32;bit++){d[word]=1U<<bit;Refused(d);d[word]=0;}
 Refused(nullptr);Refused(d,0);Refused(d,120);Refused(d,7,0);Refused(d,7,0x7fffffff);Refused(d,7,31,7);Refused(d,7,31,12,5121);Refused(d,7,31,12,17408,0);Refused(d,7,31,12,17408,17);
 for(unsigned word=8;word<14;word++)for(unsigned bit=0;bit<32;bit++){
  d[word]^=1U<<bit;assert(PioneerQ38PeerPhaseReadback(d,7,31,12,17408,3,out));assert(out[word-7]==d[word]);d[word]^=1U<<bit;
 }
 d[7]=0;for(unsigned w=8;w<14;w++)d[w]=0;
 assert(PioneerQ38PeerPhaseReadback(d,7,31,12,17408,3,out)&&out[0]==PioneerQ38PeerPhaseInvalid);
 for(unsigned w=1;w<7;w++)assert(!out[w]);
 for(unsigned word=8;word<14;word++)for(unsigned bit=0;bit<32;bit++){d[word]=1U<<bit;Refused(d);d[word]=0;}
 for(unsigned w=0;w<7;w++)stats[57+w]=out[w];assert(PioneerQ38PeerPhaseEnvelopeValid(stats));
 for(unsigned word=58;word<64;word++)for(unsigned bit=0;bit<32;bit++){stats[word]=1U<<bit;assert(!PioneerQ38PeerPhaseEnvelopeValid(stats));stats[word]=0;}
 stats[57]=PioneerQ38PeerPhaseValid;for(unsigned w=58;w<64;w++)stats[w]=UINT32_MAX;assert(PioneerQ38PeerPhaseEnvelopeValid(stats));
 for(unsigned phase=0;phase<3;phase++)assert(PioneerQ38PeerPhaseValue(stats,phase)==UINT64_MAX);
 for(unsigned phase=0;phase<3;phase++)for(unsigned bit=0;bit<64;bit++){
  for(unsigned w=58;w<64;w++)stats[w]=0;
  stats[58+phase*2+bit/32]=1U<<(bit%32);
  for(unsigned p=0;p<3;p++)assert(PioneerQ38PeerPhaseValue(stats,p)==(p==phase?UINT64_C(1)<<bit:0));
 }
 assert(!PioneerQ38PeerPhaseValue(stats,3)&&!PioneerQ38PeerPhaseValue(stats,UINT32_MAX)&&!PioneerQ38PeerPhaseValue(nullptr,0));
 stats[57]=PioneerQ38PeerPhaseInvalid;for(unsigned w=58;w<64;w++)stats[w]=0;
 for(unsigned phase=0;phase<3;phase++)assert(!PioneerQ38PeerPhaseValue(stats,phase));
 stats[57]=0x31435951;assert(!PioneerQ38PeerPhaseEnvelopeValid(stats)&&!PioneerQ38PeerPhaseEnvelopeValid(nullptr));
 printf("PASS %u full privateidentity DATA mappings;320 header/reservedbit+9shape/null atomic refusals;192 validclockbit mutations remainDATAONLY;192 badclock partialpayload refusals;invalid-clock envelope accepted withzeroDATA,distinct from coordinatorclock marker;NO IO/source/ownership/cleanup/native authority\n",cases);
}
