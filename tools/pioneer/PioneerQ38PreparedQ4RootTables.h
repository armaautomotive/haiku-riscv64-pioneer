/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_ROOT_TABLES_H
#define PIONEER_Q38_PREPARED_Q4_ROOT_TABLES_H
#include "PioneerQ38PreparedQ4UploadPlan.h"
#include "PioneerQ38PreparedQ4RootLayout.h"
#include "PioneerQwen38FleetPlan.h"
#include "PioneerQ38TokenStateLayout.h"
// Pre-device CPU serialization of INTEGER source metadata,not inference.
// A future distinct native owner overlays these tables in existing package
// slack. No old StartupPlan/native/ROOT silently selects or uploads them.
template<class Upload=PioneerQ38PreparedQ4UploadPlan<>>
class PioneerQ38PreparedQ4RootTables {
 using L=PioneerQ38PreparedQ4RootLayout;
 using Table=std::array<unsigned,L::Bytes/4>;
 std::vector<Table> tables;const Upload* source=nullptr;const PioneerQwen38Gguf* model=nullptr;
 bool attempted=false,ready=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Output(const void* output,unsigned bytes)const{
  const auto o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),t=reinterpret_cast<uintptr_t>(tables.data());
  return output && o<=UINTPTR_MAX-bytes && !(o<self+sizeof(*this) && self<o+bytes)
   && !(o<t+tables.size()*sizeof(Table) && t<o+bytes);
 }
public:
 PioneerQ38PreparedQ4RootTables()=default;
 PioneerQ38PreparedQ4RootTables(const PioneerQ38PreparedQ4RootTables&)=delete;
 PioneerQ38PreparedQ4RootTables& operator=(const PioneerQ38PreparedQ4RootTables&)=delete;
 bool Ready()const{
  if(!ready || !source || !model || !source->Ready())return false;
  const auto* plan=source->OriginalPlan();return plan && plan->MatchesSource(*model) && source->Ready();
 }
 bool Build(const Upload& upload,const PioneerQwen38Gguf& original,std::string& error){
  if(attempted)return Fail(error,"Prepared ROOT tables are one-shot.");attempted=true;
  if(!upload.Ready())return Fail(error,"Prepared ROOT tables require the SAME upload source.");
  const auto* layout=upload.OriginalPlan();
  if(!layout || !layout->MatchesSource(original))return Fail(error,"Prepared ROOT source withdrew its plan.");
  const auto& plan=*layout;unsigned selected[64][3]={};
  for(auto& layer:selected)for(auto& tensor:layer)tensor=851;
  for(const auto& p:plan.Parts())if(!p.kind && p.layer<64 && p.role>=2 && p.role<=4)selected[p.layer][p.role-2]=p.tensor;
  std::vector<Table> staged(192);
  for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
   if(selected[layer][role-2]>=original.Tensors().size())return Fail(error,"Prepared FFN tensor missing.");
   PioneerQwen38FleetPlan fleet;
   if(!fleet.Build(original,plan,selected[layer][role-2],119,16) || fleet.Jobs().size()!=119)return Fail(error,"Prepared FFN schedule mismatch.");
   auto& table=staged[layer*3+role-2];const unsigned header[]={L::Magic,1,layer,role,role==4?17408U:5120U,L::Workers,16,L::Bytes,
    L::PhaseOffset(role),L::RootTable,L::RootLedger,1,0,0,0,0};
   for(unsigned i=0;i<16;i++)table[i]=header[i];
   for(const auto& part:fleet.Jobs()){
    if(part.wave || part.worker>=119)return Fail(error,"Unexpected multi-wave FFN schedule.");
    typename Upload::Job job;
    if(!upload.Describe(part.fragment,part.firstRow,part.groups,part.worker+1,1,job,error))return false;
    const unsigned at=16+part.worker*16;
    for(unsigned i=0;i<16;i++)table[at+i]=i==2?0:job.descriptor[i];
    const unsigned ledger=L::LedgerOffset/4+part.worker*4;
    table[ledger]=job.extraReads;table[ledger+1]=job.extraFlits;table[ledger+2]=part.firstRow;table[ledger+3]=part.bank;
   }
  }
  if(!upload.Ready() || !plan.MatchesSource(original))return Fail(error,"Prepared source changed during ROOT table planning.");
  source=&upload;model=&original;tables.swap(staged);ready=true;return Ready();
 }
 bool Read(unsigned layer,unsigned role,unsigned offset,uint8_t* output,unsigned bytes)const{
  if(!Ready() || layer>=64 || !L::PhaseOffset(role) || !bytes || ((offset|bytes)&63) || offset>L::Bytes
   || bytes>L::Bytes-offset || !Output(output,bytes))return false;
  const auto& table=tables[layer*3+role-2];
  std::array<uint8_t,L::Bytes> stage{};
  for(unsigned at=0;at<bytes;at++)stage[at]=uint8_t(table[(offset+at)/4]>>(((offset+at)&3)*8));
  if(!Ready())return false;
  memcpy(output,stage.data(),bytes);return true;
 }
};
static_assert(unsigned(PioneerQ38PreparedQ4RootLayout::Offset)>=unsigned(PioneerQ38LayerChainLayout::DenseBytes)
 && unsigned(PioneerQ38PreparedQ4RootLayout::Offset)>=unsigned(PioneerQ38RecurrentPrefixLayout::Bytes)
 && PioneerQ38PreparedQ4RootLayout::Offset+3*PioneerQ38PreparedQ4RootLayout::Stride<=PioneerQ38TokenStateLayout::Offset,
 "prepared metadata uses proven package slack,not numeric recipes,frames or recurrent state");
#endif
