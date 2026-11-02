/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSweepRoutes.h"
struct Routes {
 uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
  0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
 uint32_t tlb[2][3]={{1,2,3},{4,5,6}};
 unsigned reads=0,writes=0,dropWrite=0,corruptRead=0,mask=0;
 bool retained=false,logicalLabel=false;
 void Touch(){assert(!retained && "NO route endpoint IO after quarantine");}
 uint32_t Read(uint32_t value){Touch();return ++reads==corruptRead?value^1:value;}
 uint32_t SourceRead(unsigned i){assert(i<16);return Read(source[i]);}
 uint32_t DramTlbRead(unsigned s,unsigned i){assert(s<2 && i<3);return Read(tlb[s][i]);}
 void DramTlbWrite(unsigned s,unsigned i,uint32_t v){Touch();assert(s<2 && i<3);if(++writes!=dropWrite)tlb[s][i]=v;}
 uint32_t DramIdRead(unsigned word){
  assert(word<4);const unsigned ys[]={12,15,18,12,15,18,21};unsigned bank=7;
  for(unsigned b=0;b<7;b++){
   bool match=true;
   for(unsigned i=0;i<3;i++)match&=tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,b<3?18:17,ys[b],true,i);
   if(match)bank=b;
  }
  assert(bank<7);mask|=1U<<bank;
  if(!word)return Read(0x1c611000|BlackholeDramPhysicalX(bank)|(BlackholeDramPhysicalY(bank)<<6));
  if(word==1)return Read(0x80000|BlackholeDramBank(bank));
  if(word==2)return Read(0xe005);
  return Read(logicalLabel?((bank<3?18:17)|(ys[bank]<<6)):
   BlackholeDramPhysicalX(bank)|(BlackholeDramPhysicalY(bank)<<6));
 }
 void SweepQuarantineMemoryOnly(){assert(!retained);retained=true;}
 bool Original() const {const uint32_t before[2][3]={{1,2,3},{4,5,6}};return !memcmp(before,tlb,sizeof(tlb));}
};
int main(){
 tt_arc_telemetry_result t={};t.version=1;t.state=TT_ARC_TELEMETRY_OK;t.tlb_restored=1;t.present_mask=0xfff;
 t.values[0]=0x12050001;t.values[6]=0xf7;t.values[7]=0x5515;t.values[9]=0xfff;
 tt_tensix_reset_result c={};c.version=1;c.state=TT_TENSIX_RESET_HELD;c.tlb_restored=1;c.niu_config=0x6005;
 for(auto& v:c.reset)v=0x47800;
 unsigned valid=0,refused=0,reads=0,writes=0;
 for(bool logical:{false,true}){
  Routes io;io.logicalLabel=logical;tt_dram_samples_result r={};
  assert(BlackholeDramSweepRoutes(io,t,c,r) && io.mask==127 && io.Original() && !io.retained);
  assert(r.routes_verified==7 && r.data_restored && r.tlb_restored && !r.recovery_required);
  assert(!r.saved_regions && !r.modified_regions && !r.passes && !r.mismatches);
  reads=io.reads;writes=io.writes;valid++;
 }
 for(unsigned fault=1;fault<=reads;fault++){
  Routes io;io.corruptRead=fault;tt_dram_samples_result r={};
  if(BlackholeDramSweepRoutes(io,t,c,r)){assert(io.Original() && !io.retained);valid++;}
  else{assert(io.retained && r.recovery_required);refused++;}
 }
 for(unsigned drop=1;drop<=writes;drop++){
  Routes io;io.dropWrite=drop;tt_dram_samples_result r={};
  if(BlackholeDramSweepRoutes(io,t,c,r)){assert(io.Original() && !io.retained);valid++;}
  else{assert(io.retained && r.recovery_required && io.writes==drop);refused++;}
 }
 for(unsigned fault=0;fault<5;fault++){
  Routes io;auto h=t;auto core=c;tt_dram_samples_result r={};
  if(fault==0)h.values[6]^=1;
  if(fault==1)core.reset[2]=0;
  if(fault==2)io.source[3]^=1U<<10;
  if(fault==3)h.tlb_restored=0;
  if(fault==4)core.niu_config^=1;
  assert(!BlackholeDramSweepRoutes(io,h,core,r) && io.retained && !io.writes);refused++;
 }
 printf("strict read-only route endpoint model: %u valid / %u refused; NO payload API or IO after quarantine; NOT hardware\n",valid,refused);
}
