/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FACTORED_TILE_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_tile_test.cpp"

struct CounterAudit {
 unsigned counter=0,count=0;
 void Op(unsigned){}void Drain(){}
 void Config(unsigned i,unsigned v){if(i==28){counter=v;count++;}}
};
struct LegacyCounterIssue:FactoredTileIssue {
 explicit LegacyCounterIssue(FactoredTileMachine& m):FactoredTileIssue{m}{}
 void Config(unsigned i,unsigned v){m.Config(i,i==28?0x41004:v);}
};
static void CheckTile(const TileFixture& data,const FactoredTileMachine& m,unsigned group){
 for(unsigned i=0;i<1024;i++){
  const unsigned face=i/256,k=group*32+(face%2)*16+(i%256)/16,row=(face/2)*16+i%16;
  assert(m.l1[Tile::Native+64+i]==data.Code(row,k)*8);
 }
}
int main(){
 // RED on the exact old producer before any modeled result is inspected.
 CounterAudit audit;assert(Tile::PackCodes(audit,0));
 if(audit.count!=1 || audit.counter!=0x100){
  fprintf(stderr,"PACK_BOUNDARY_REGRESSION expected manual0x100 got0x%x;old_auto_last_configuration_is_not_safe\n",audit.counter);
  return 1;
 }
 unsigned cases=0,overwriteBytes=0;
 for(unsigned poison:{0U,0xa7U})for(unsigned trial=0;trial<4;trial++){
  TileFixture data(trial);auto good=std::make_unique<FactoredTileMachine>(),bad=std::make_unique<FactoredTileMachine>();
  data.Stage(*good);data.Stage(*bad);Tile::Constants(*good);Tile::Constants(*bad);
  memset(good->l1.data()+Tile::Native+64,int(poison),1024);
  memset(bad->l1.data()+Tile::Native+64,int(poison),1024);
  FactoredTileIssue e{*good};LegacyCounterIssue legacy(*bad);bad->reconstruct112AutoLast=true;
  Tile::Begin(e);Tile::Begin(legacy);
  for(unsigned group=0;group<8;group++){
   assert(Tile::PackCodes(e,group) && Tile::PackCodes(legacy,group));
   assert(good->Clean() && bad->Clean() && !good->autoLasts && bad->autoLasts==(group+1)*4);
   CheckTile(data,*good,group);
   // Empirical112 reconstruction: last face at the first face's address;
   // three quarters UNWRITTEN. Poison must survive, never become fake zeros.
   for(unsigned i=0;i<1024;i++){
    const unsigned expected=i<256?data.Code(16+i%16,group*32+16+i/16)*8:poison;
    assert(bad->l1[Tile::Native+64+i]==expected);overwriteBytes++;
   }
   cases++;
  }
 }
 // Without the diagnostic reconstruction opt-in, reject the historical
 // auto-Last config BEFORE even one PACR reads or writes any native data.
 TileFixture data(7);auto refused=std::make_unique<FactoredTileMachine>();data.Stage(*refused);Tile::Constants(*refused);
 LegacyCounterIssue invalid(*refused);Tile::Begin(invalid);const auto prior=refused->l1;
 bool fault=false;try{(void)Tile::PackCodes(invalid,0);}catch(const FactoredTileMachine::Fault&){fault=true;}
 assert(fault && !refused->packs && refused->l1==prior);
 assert(cases==64 && overwriteBytes==65536);
 printf("PASS_PACK_BOUNDARY manualLast_cfg0x100 completeTiles%u poisonedPriorTail0_a7 historical112_cfg0x41004_diagnosticOnly_wrongLastFace_overwriteBytes%u strictDefaultRejectsBeforePACR;LOCAL_NOT_HARDWARE_OR_SPEED_PROOF\n",cases,overwriteBytes);
 static_cast<void>(&Round);
}
