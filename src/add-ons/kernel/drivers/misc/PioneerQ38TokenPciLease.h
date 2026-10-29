/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PCI_LEASE_H
#define PIONEER_Q38_TOKEN_PCI_LEASE_H
#include <stdint.h>
// PCI configuration journal ONLY. Native IO must separately derive genuine
// VM-closed/terminal/originals/clock/FD lifetime authority. No MMIO or host
// release is performed here;never call a legacy Unmap/rollback/retry.
class PioneerQ38TokenPciLease {
 const void *record=nullptr,*transaction=nullptr,*transport=nullptr;
 uint64_t endpoint=0;
 uint32_t bars[6]={},trial[6]={},bridge[3]={},node[3]={};
 bool captured=false,attempted=false,writesStarted=false,closed=false,unsafe=false;
 unsigned writes=0,reads=0;
 template<class R> bool Fail(R& r){unsafe=true;r.state=FailureState;r.restored=0;return false;}
 template<class IO,class R,class T> bool Read(IO& io,R& r,T& t,bool root,unsigned offset,unsigned width,uint32_t& value){
  if(!Matches(io,r,t) || !io.TokenPciScope() || !io.TokenPciReadChecked(root,offset,width,value))return false;
  reads++;return true;
 }
 template<class IO,class R,class T> bool Expect(IO& io,R& r,T& t,bool root,unsigned offset,unsigned width,uint32_t expected){
  uint32_t value=0;return Read(io,r,t,root,offset,width,value) && value==expected;
 }
 template<class IO,class R,class T> bool Write(IO& io,R& r,T& t,bool root,unsigned offset,unsigned width,uint32_t value){
  if(!Matches(io,r,t) || !io.TokenPciScope() || !PermitsWrite(root,offset,width,value)
   || !io.TokenPciWriteChecked(root,offset,width,value))return false;
  writes++;return Expect(io,r,t,root,offset,width,value);
 }
 template<class IO,class R,class T> bool Quiet(IO& io,R& r,T& t,bool root){
  // Same read-only bounded capability/PCIe-status policy as
  // BlackholeCardControlsQuiet + BlackholeHostPcieStatus;no W1C clearing.
  uint32_t cap=0;if(!Read(io,r,t,root,0x34,1,cap) || !cap)return false;
  uint64_t visited=0;bool pcie=false;
  for(unsigned hops=0;cap && hops<48;hops++){
   if(cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4))))return false;
   visited|=UINT64_C(1)<<(cap/4);uint32_t header=0,control=0;
   if(!Read(io,r,t,root,cap,2,header) || header>0xffff)return false;
   const unsigned id=header&255;
   if(id==5 || id==0x11){
    if(!Read(io,r,t,root,cap+2,2,control) || control>0xffff || (control&(id==5?1U:0x8000U)))return false;
   }
   if(id==1 && (cap>0xf8 || !Read(io,r,t,root,cap+4,2,control) || (control&3)))return false;
   if(id==0x10){
    if(pcie || cap>0xf4 || !Read(io,r,t,root,cap+2,2,control) || ((control&15)!=1 && (control&15)!=2))return false;
    if(!Read(io,r,t,root,cap+10,2,control) || control>0xffff || (control&0x2f))return false;
    pcie=true;
   }
   cap=(header>>8)&255;
  }
  return !cap && pcie;
 }
public:
 static constexpr uint32_t LiveState=1,FailureState=5; // pinned tt_pioneer_mmio_result ABI
 PioneerQ38TokenPciLease()=default;
 PioneerQ38TokenPciLease(const PioneerQ38TokenPciLease&)=delete;
 PioneerQ38TokenPciLease& operator=(const PioneerQ38TokenPciLease&)=delete;
 bool Pristine()const{return !captured && !attempted && !writesStarted && !closed && !unsafe && !record && !transaction && !transport && !writes && !reads;}
 bool Prepared()const{return captured && !attempted && !writesStarted && !closed && !unsafe;}
 bool Complete()const{return captured && attempted && writesStarted && closed && !unsafe && writes==11;}
 bool Unsafe()const{return unsafe;}
 unsigned Writes()const{return writes;}
 unsigned Reads()const{return reads;}
 void Fault(){unsafe=true;} // CPU ONLY,all remaining resources retained
 template<class IO,class R,class T> bool Matches(IO& io,const R& r,const T& t)const{
  if(!captured || unsafe || !io.TokenPciLeaseIdentity(*this) || record!=&r || transaction!=&t
   || !transport || io.TokenPciTransport()!=transport || io.TokenPciEndpoint()!=endpoint
   || r.version!=1 || r.profile_status!=0 || r.profile_stage!=6 || r.commands_before[0] || r.commands_before[1])return false;
  for(unsigned i=0;i<6;i++)if(r.bars_before[i]!=bars[i] || r.bars_trial[i]!=trial[i])return false;
  for(unsigned i=0;i<3;i++)if(r.bridge_before[i]!=bridge[i] || r.node_id[i]!=node[i])return false;
  if(closed){
   if(!Complete() || t.config_attempted || t.map_attempted || r.state!=LiveState || r.stage!=5 || r.restored!=1
    || r.commands_after[0] || r.commands_after[1])return false;
   for(unsigned i=0;i<6;i++)if(r.bars_after[i]!=bars[i])return false;
   for(unsigned i=0;i<3;i++)if(r.bridge_after[i]!=bridge[i])return false;
   return true;
  }
  return r.state==LiveState && r.stage==4 && !r.restored && t.config_attempted==!writesStarted && t.map_attempted==!writesStarted;
 }
 template<class IO,class R,class T> __attribute__((noinline)) bool Capture(IO& io,const R& r,const T& t){
  if(!Pristine() || !io.TokenPciLeaseIdentity(*this) || !io.TokenPciCaptureScope() || !io.TokenPciTransport()
   || r.version!=1 || r.state!=LiveState || r.stage!=4 || r.restored || r.profile_status!=0 || r.profile_stage!=6
   || r.commands_before[0] || r.commands_before[1] || !t.config_attempted || !t.map_attempted)return false;
  const uint32_t expected[]={12,0x49,0x2000000c,0x49,12,0xfffffff8};
  for(unsigned i=0;i<6;i++)if(r.bars_before[i]!=(i%2?0U:12U) || r.bars_trial[i]!=expected[i])return false;
  if(r.bridge_before[0]!=0x0001fff1 || r.bridge_before[1] || r.bridge_before[2]
   || ((r.node_id[0]&0xfff)!=2 && (r.node_id[0]&0xfff)!=11) || r.node_id[0]!=r.node_id[1] || r.node_id[0]!=r.node_id[2])return false;
  const void* originalTransport=io.TokenPciTransport();const uint64_t originalEndpoint=io.TokenPciEndpoint();
  for(unsigned i=0;i<6;i++){bars[i]=r.bars_before[i];trial[i]=r.bars_trial[i];}
  for(unsigned i=0;i<3;i++){bridge[i]=r.bridge_before[i];node[i]=r.node_id[i];}
  if(!io.TokenPciCaptureScope() || io.TokenPciTransport()!=originalTransport || io.TokenPciEndpoint()!=originalEndpoint)return false;
  record=&r;transaction=&t;transport=originalTransport;endpoint=originalEndpoint;captured=true;return Matches(io,r,t);
 } // trusted SAME actual BEGIN record captured BEFORE first native worker launch
 bool PermitsWrite(bool root,unsigned offset,unsigned width,uint32_t value)const{
  if(!captured || !attempted || !writesStarted || closed || unsafe || writes>=11)return false;
  if(writes<2)return root==(writes==1) && offset==4 && width==2 && !value;
  const unsigned order[]={5,4,1,0,3,2},bridgeOrder[]={1,2,0};
  if(writes<8){const unsigned i=order[writes-2];return !root && width==4 && offset==0x10+i*4 && value==bars[i];}
  const unsigned i=bridgeOrder[writes-8];return root && width==4 && offset==0x24+i*4 && value==bridge[i];
 } // exact next original only;NEVER BME or arbitrary config writes
 template<class IO,class R,class T> __attribute__((noinline)) bool Close(IO& io,R& r,T& t){
  if(!Prepared() || !Matches(io,r,t) || !io.TokenPciScope())return false;
  attempted=true;uint32_t value=0;
  if(!Expect(io,r,t,false,0,4,0xb1401e52) || !Expect(io,r,t,false,0x2c,4,0x00431e52)
   || !Expect(io,r,t,true,0,4,0x20421e30) || !Read(io,r,t,true,8,4,value) || (value>>8)!=0x060400
   || !Expect(io,r,t,true,0x18,4,0x003f0100) || !Expect(io,r,t,true,0x20,4,0x0000fff0)
   || !Expect(io,r,t,true,0x10,4,0) || !Expect(io,r,t,true,0x14,4,0)
   || !Read(io,r,t,true,0x38,4,value) || (value&1) || !Read(io,r,t,false,0x30,4,value) || (value&1)
   || !Read(io,r,t,false,4,4,value) || (value&0xffff)!=0x402 || !(value&0x00100000)
   || !Quiet(io,r,t,false) || !Quiet(io,r,t,true))return Fail(r);
  for(unsigned i=0;i<6;i++)if(!Expect(io,r,t,false,0x10+i*4,4,trial[i]))return Fail(r);
  const uint32_t currentBridge[]={0x20010001,0x49,0x49};
  for(unsigned i=0;i<3;i++)if(!Expect(io,r,t,true,0x24+i*4,4,currentBridge[i]))return Fail(r);
  if(!Expect(io,r,t,false,4,2,0x402) || !Expect(io,r,t,true,4,2,0x402)
   || !Quiet(io,r,t,false) || !Quiet(io,r,t,true) || !io.TokenPciScope() || !Matches(io,r,t))return Fail(r);
  // Aliases are genuinely gone. Consume the old transaction before the
  // FIRST write so no legacy retry can issue a second restoration attempt.
  writesStarted=true;t.config_attempted=t.map_attempted=false;
  if(!Write(io,r,t,false,4,2,0) || !Write(io,r,t,true,4,2,0))return Fail(r);
  const unsigned order[]={5,4,1,0,3,2},bridgeOrder[]={1,2,0};
  for(unsigned i:order)if(!Write(io,r,t,false,0x10+i*4,4,bars[i]))return Fail(r);
  for(unsigned i:bridgeOrder)if(!Write(io,r,t,true,0x24+i*4,4,bridge[i]))return Fail(r);
  for(unsigned i=0;i<6;i++)if(!Read(io,r,t,false,0x10+i*4,4,r.bars_after[i]) || r.bars_after[i]!=bars[i])return Fail(r);
  for(unsigned i=0;i<3;i++)if(!Read(io,r,t,true,0x24+i*4,4,r.bridge_after[i]) || r.bridge_after[i]!=bridge[i])return Fail(r);
  if(!Read(io,r,t,false,4,2,r.commands_after[0]) || r.commands_after[0]
   || !Read(io,r,t,true,4,2,r.commands_after[1]) || r.commands_after[1]
   || !io.TokenPciScope() || !Matches(io,r,t) || writes!=11)return Fail(r);
  closed=true;r.restored=1;r.stage=5;return Matches(io,r,t);
 } // original PCI values only;host pages/model reservations remain owned
};
#endif
