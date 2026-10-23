/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_MEMORY_FIXTURE_ONLY
#include "pioneer_q38_projection_memory_test.cpp"
#include "pioneer_q38_projection_execution_machine.h"
#include "pioneer_q38_projection_native_transport.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphOwner.h"
struct ProjectionNativeTransport::Impl:ProjectionMemoryIO,ProjectionExecutionFabric {
 std::vector<uint32_t>& workspace;std::unique_ptr<ProjectionExecutionMachine> machine;
 unsigned boots=0,starts=0,stops=0,fault=0;bool damaged=false;
 const PioneerQ38ProjectionGraphResult* result=nullptr;
 ProjectionExecutionLearnedBytes* learned=nullptr;
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
 unsigned validationReads=0,validationFault=0,validationFaultPage=0;bool validationPending=false;
 void ValidationRead(){
  assert(!current && boots==119 && starts==5 && machine->Stops()==119 && validationReads<18 && !validationPending);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
  unsigned page=validationReads++;
  uint32_t base=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+4*4),route=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+8*4);
  assert(FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+5*4)==2);
  const uint32_t expected[]={base+TT_Q38_PROJECTION_RESULT_BASE+page*4096,0,route,0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==expected[i]);
  for(unsigned i=0;i<1024;i++)assert(FullRead(0,0x80000+i*4)==0x7fc30000+page*1024+i);
  if(validationReads==validationFaultPage && validationFault==1){validationPending=true;return;}
  for(unsigned i=0;i<1024;i++)if(!(validationReads==validationFaultPage && validationFault==2 && i==1023))
   WriteLocal(0,0x80000+i*4,workspace.at(TT_Q38_PROJECTION_RESULT_BASE/4+page*1024+i));
  // Pure endpoint bytes/counters ONLY,never independent reference results.
  C().counters[0]++;C().counters[1]+=64;C().accepted++;C().counters[3]++;
  if(validationReads==validationFaultPage && validationFault==3)C().accepted++;
  if(validationReads==validationFaultPage && validationFault==4)C().source[11]^=1;
 }
#endif
 Impl(const char* root,const char* peer,std::vector<uint32_t>& gddr):workspace(gddr){
  (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
  machine.reset(new ProjectionExecutionMachine(root,peer,*this));track=true;
 }
 uint32_t ReadLocal(unsigned w,uint32_t a)override{unsigned old=current;current=w;uint32_t v=FullRead(w,a);current=old;return v;}
 void WriteLocal(unsigned w,uint32_t a,uint32_t v)override{
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)core[w]->entry=v;
  else if(a>=0x70000 && a<0x74000)core[w]->view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=v;
  else data.at(Index(w,a))=v;
 }
 uint32_t NoC(unsigned w,unsigned a)override{return core[w]->ResidentNocRead(a);}
 void SetControls(unsigned w,const uint32_t* command,const uint32_t* counts)override{
  auto& p=*core[w];for(unsigned i=0;i<10;i++)p.command[i]=command[i];
  p.counters[0]=counts[0];p.counters[1]=counts[1];p.accepted=counts[2];p.counters[3]=counts[3];
  p.writeCounters[0]=counts[4];p.writeCounters[1]=counts[5];p.writeCounters[3]=counts[7];
 }
 uint32_t Reset(unsigned w)override{return core[w]->coreReset;}
 std::vector<uint32_t>& Gddr()override{return workspace;}
 ProjectionExecutionLearnedBytes* LearnedBytes()override{return learned;}
 uint32_t LocalRead(uint32_t a){return PioneerQ38ProjectionGraphSnapshotContains(current,a)?FullRead(current,a):C().Q38PanelRead(a);}
 void LocalWrite(uint32_t a,uint32_t v){
  if(PioneerQ38ProjectionGraphSnapshotContains(current,a))FullWrite(current,a,v);else C().Q38PanelWrite(a,v);
 }
 unsigned ResetTarget(){
  // Existing per-worker mock windows normalize their private aperture to
  // (1,2). A selected writable window still addresses current,not ROOT.
  // The actual ALL-held/probe walk programs fixed read-only physical XY.
  bool selected=true;for(unsigned i=0;i<3;i++)selected&=C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);
  if(selected)return current;
  for(unsigned w=0;w<120;w++)for(bool readOnly:{false,true}){
   bool same=true;for(unsigned i=0;i<3;i++)same&=C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(w),PioneerWorkerY(w),readOnly,i);
   if(same)return w;
  }
  assert(false);return 120;
 }
};
ProjectionNativeTransport::ProjectionNativeTransport(const char* root,const char* peer,std::vector<uint32_t>& workspace):implementation(new Impl(root,peer,workspace)){}
ProjectionNativeTransport::~ProjectionNativeTransport(){delete implementation;}
void ProjectionNativeTransport::SeedRoot(const uint32_t* addresses,const uint32_t* values,unsigned count,
 const uint32_t* source,const uint32_t* command,const uint32_t* counters){
 auto& p=*implementation;assert(!p.boots && !p.starts && !p.current);
 // Exact device state after genuine QRN1. Do not create/rebind a context,
 // borrowed proof, model object, private RMS lease, or completion seal.
 for(unsigned i=0;i<count;i++)p.FullWrite(0,addresses[i],values[i]);
 for(unsigned w=0;w<120;w++){
  auto& c=*p.core[w];c.config=source[0];
  for(unsigned i=0;i<16;i++)c.source[i]=source[i];
  c.source[0]&=~0x10000U; // fixed DRAM NIU source,not worker config register
  if(w)c.config&=~0x10000U;
 }
 p.SetControls(0,command,counters);
}
bool ProjectionNativeTransport::Select(unsigned w){
 auto& p=*implementation;p.Touch();return p.CodeIO::SelectRuntimeCore(w);
}
bool ProjectionNativeTransport::MapPanel(){auto& p=*implementation;p.Touch();return p.C().mapping;}
bool ProjectionNativeTransport::PanelStable(){return implementation->C().mapping;}
uint32_t ProjectionNativeTransport::Read32(Read kind,unsigned a,unsigned b,unsigned c){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case Source:return p.C().SourceRead(a);
  case Reset:return p.core[p.ResetTarget()]->coreReset;
  case ComputeNoc:{unsigned target=p.ResetTarget();auto& node=*p.core[target];
   if(a==0x44)return 0x1c611000|BlackholeDramSourceX(node.source,PioneerWorkerX(target))|(PioneerWorkerY(target)<<6);
   if(a==0x48)return 0x10000|target;
   for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i))return i?node.source[i]:node.config;
   if(a==0x40 || a==0x840 || a==0x1040 || a==0x1840 || (a>=0x240 && a<0x280))return 0;
   assert(false);return 0;}
  case Tlb:return p.C().DramTlbRead(a,b);
  case Local:return a>=2032 && a<2040?p.LocalRead(0x17e000+a*4):p.C().ResidentL1Read(a);
  case Code:assert(!a);return p.C().entry;
  case Control:return p.C().RuntimeControlRead(a);
  case Pc:if(p.C().coreReset==0x46800)p.machine->Advance(p.current);return p.machine->Pc(p.current);
  case Status:
   if(!p.current && p.C().coreReset==0x46800)p.machine->Advance(0);
   if(p.fault>=3 && p.fault<=8 && !p.damaged && p.starts && !p.current && p.C().coreReset==0x46800 && p.machine->Pc(0)==TT_Q38_DISPATCH_PARK){
    p.damaged=true;if(p.fault==3)p.C().source[11]^=1;if(p.fault==4)p.C().coreReset=0x47800;
    if(p.fault==5)p.C().command[9]^=1;if(p.fault==6)p.C().counters[0]++;
    if(p.fault==7)p.data.at(p.Index(0,0x64000))^=1;if(p.fault==8)p.C().mapping=false;
   }
   return p.LocalRead(0x17ffc0+a*4);
  case Noc:
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
   if(a==0x40 && p.validationPending)return 1;
#endif
   return p.C().ResidentNocRead(a);
  case Panel:{uint32_t v=p.LocalRead(a);if(p.starts && !p.current && ((p.fault==1 && a==TT_Q38_PROJECTION_GRAPH_STATS+56*4)
    || (p.fault==2 && a==TT_Q38_PROJECTION_GRAPH_STATS+6*4)))v^=0x80000000;return v;}
  case Pci:return p.C().HostPciRead(a,uint16_t(b),uint8_t(c));
 }
 assert(false);return 0;
}
void ProjectionNativeTransport::Write32(Write kind,unsigned a,uint32_t v,unsigned b){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case SetTlb:p.C().DramTlbWrite(a,b,v);return;
  case SetLocal:
   if(p.fault==10 && p.result && p.result->phase==9 && p.current==119 && a==2039)v^=1;
   if(a>=2032 && a<2040)p.LocalWrite(0x17e000+a*4,v);else p.C().ResidentL1Write(a,v);return;
  case SetCode:assert(!a);p.C().RuntimeCodeWrite(0,v);return;
  case SetStatus:p.LocalWrite(0x17ffc0+a*4,v);return;
  case SetNoc:
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
   if(a==0x40){assert(v==1);p.ValidationRead();return;}
#endif
   p.C().ResidentNocWrite(a,v);return;
  case SetPanel:p.LocalWrite(a,v);return;
  case SetReset:
   p.C().coreReset=v;p.C().resets.push_back(v);
   if(v==0x46800){p.machine->Start(p.current);if(p.current)p.boots++;else p.starts++;}
   if(v==0x47800)p.stops++;return;
 }
 assert(false);
}
uint32_t ProjectionNativeTransport::MappedRead(uint32_t a){auto& p=*implementation;p.Touch();return p.FullRead(p.current,a);}
void ProjectionNativeTransport::MappedWrite(uint32_t a,uint32_t v){auto& p=*implementation;p.Touch();p.FullWrite(p.current,a,v);}
uint32_t* ProjectionNativeTransport::PanelSaved(){auto& p=*implementation;p.Touch();return p.C().Q38PanelSavedWords();}
uint32_t* ProjectionNativeTransport::Backup(unsigned words){auto& p=*implementation;p.Touch();return p.C().HostReadBackup(words);}
void ProjectionNativeTransport::Quarantine(){implementation->quarantined=true;} // MEMORY ONLY
void ProjectionNativeTransport::Pause(){implementation->now++;}
uint64_t ProjectionNativeTransport::Now()const{const auto& p=*implementation;return p.starts && p.fault==11?p.now+30000001:p.starts && p.fault==12?0:p.now;}
unsigned ProjectionNativeTransport::Boots()const{return implementation->boots;}
unsigned ProjectionNativeTransport::Starts()const{return implementation->starts;}
unsigned ProjectionNativeTransport::Stops()const{return implementation->stops;}
unsigned ProjectionNativeTransport::Matrices()const{return implementation->machine->Matrices();}
unsigned ProjectionNativeTransport::Producers()const{return implementation->machine->Producers();}
unsigned ProjectionNativeTransport::Touches()const{return implementation->touches;}
void ProjectionNativeTransport::VerifyOutput(){implementation->machine->VerifyOutput();}
void ProjectionNativeTransport::Diagnostic()const{implementation->machine->Diagnostic();}
void ProjectionNativeTransport::Fault(unsigned kind){implementation->fault=kind;}
void ProjectionNativeTransport::Observe(const PioneerQ38ProjectionGraphResult& result){implementation->result=&result;}
void ProjectionNativeTransport::LearnedSource(ProjectionExecutionLearnedBytes& source){
 auto& p=*implementation;assert(!p.learned && !p.boots && !p.starts && source.Ready());p.learned=&source;
}
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
void ProjectionNativeTransport::ValidationFault(unsigned kind,unsigned page){
 assert(kind>=1 && kind<=4 && page>=1 && page<=18);implementation->validationFault=kind;implementation->validationFaultPage=page;
}
unsigned ProjectionNativeTransport::ValidationReads()const{return implementation->validationReads;}
#endif
