/* SPDX-License-Identifier: MIT */
// ACTUAL RootProof extraction,MOCK scope/physical endpoints. Not a native
// scope or frozen ROOT sealing grant. Full-owner instruction tests separate.
#define PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
#include "pioneer_q38_ffn_persistent_graph_test.cpp"
struct ProofHarness {
 uint32_t manifest[TT_Q38_FFN_GRAPH_WORDS]={},counters[120][8]={},stats[64]={},command[120][10]={};
 bool rootVerified=false;
 static unsigned Route(unsigned w){return PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);}
 template<class IO> bool Guard(IO& io,unsigned w){assert(!w);return io.Guard();}
#include "actual_root_proof.inc"
};
struct ProofIO {
 std::array<uint32_t,TT_Q38_FFN_GRAPH_WORDS> data;
 uint32_t stats[64]={},last[10]={};
 unsigned kind=0,field=0,touches=0,guards=0,pcs=0,postReads=0;
 bool observed=false,work;
 ProofIO(ProofHarness& h,bool run):work(run){
  FfnGraphFixture f;std::copy(f.manifest.begin(),f.manifest.end(),h.manifest);
  if(!work){h.manifest[5]=2;h.manifest[13]=1;h.manifest[14]=0;}
  std::copy(h.manifest,h.manifest+TT_Q38_FFN_GRAPH_WORDS,data.begin());
  auto& s=stats;s[0]=TT_Q38_FFN_GRAPH_MARKER;s[1]=2;s[2]=work?1:2;
  s[10]=s[11]=work?652:357;s[6]=(work?17209:7378)+4*s[10];
  s[7]=work?982+h.OutputPackets(0)+h.OutputPackets(1)+h.OutputPackets(2):238;
  s[8]=s[6]+(work?5714:0);s[9]=work?4065:238;s[12]=s[54]=data[13];
  for(unsigned i=0;i<8;i++){
   h.counters[0][i]=s[16+i]=10000;
   unsigned delta=i==0 || i==3?s[6]:i==1?s[8]:i==2 || i==6?s[6]+s[7]:i==5?s[9]:s[7];
   s[24+i]=10000+delta;
  }
  for(unsigned i=0;i<16;i++)s[32+i]=data[16+i];
  s[48]=work;s[49]=work?8:1;s[50]=work?3:0;s[51]=work?20:0;s[52]=work?68:0;s[53]=work?20480:0;
  s[55]=data[42];s[56]=1;
  const uint32_t receipt[]={0x32494651,20480,2,2,data[13],data[40],data[41]};
  if(work)for(unsigned i=0;i<7;i++)s[57+i]=receipt[i];
  const uint32_t cmd[]={0xffb20228,0,ProofHarness::Route(119),0x75008,0,129,0,0,4,0};
  std::copy(cmd,cmd+10,last);
 }
 void Touch(){assert(!observed);touches++;}
 uint32_t Value(unsigned which,unsigned at,uint32_t value){
  if(kind==which && field==at){observed=true;return value^0x80000000U;}return value;
 }
 bool Guard(){Touch();return !Value(7,guards++,0);}
 void Pause(){Touch();}
 uint32_t RuntimePcRead(){Touch();return Value(6,pcs++,TT_Q38_DISPATCH_PARK);}
 uint32_t RuntimeStatusRead(unsigned i){Touch();return Value(5,i,i==2?TT_Q38_FFN_GRAPH_MARKER:i==0?0x7200c:work?5120:119);}
 uint32_t Q38PanelRead(uint32_t a){Touch();
  if(a>=TT_Q38_FFN_GRAPH_MANIFEST && a<TT_Q38_FFN_GRAPH_COUNTERS){
   unsigned at=(a-TT_Q38_FFN_GRAPH_MANIFEST)/4;postReads++;
   return Value(8,at,data[at%TT_Q38_FFN_GRAPH_WORDS]);
  }
  if(a>=TT_Q38_FFN_GRAPH_STATS && a<TT_Q38_FFN_GRAPH_STATS+256){unsigned i=(a-TT_Q38_FFN_GRAPH_STATS)/4;return Value(1,i,stats[i]);}
  if(a>=TT_Q38_FFN_GRAPH_SEQUENCE && a<TT_Q38_FFN_GRAPH_SEQUENCE+119*4){unsigned i=(a-TT_Q38_FFN_GRAPH_SEQUENCE)/4;return Value(2,i,PioneerQ38FfnGraphNextPeer(data.data(),i));}
  assert(false);return 0;
 }
 uint32_t ResidentNocRead(uint32_t a){Touch();
  if(a<40 && !(a&3))return Value(3,a/4,last[a/4]);
  for(unsigned i=0;i<8;i++)if(a==PioneerQ38PanelCounter(i)){
   if(kind==4 && a==PioneerQ38PanelCounter(field)){observed=true;return stats[24+i]^1;}return stats[24+i];
  }
  for(unsigned i=0;i<4;i++)if(a==0x40+i*0x800)return Value(9,i,0);
  for(unsigned i=0;i<32;i++)if(a==0x240+i*4)return kind==9 && field==i+4?(observed=true,1U):0U;
  assert(false);return 0;
 }
};
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 unsigned positive=0,negative=0;
 for(bool work:{false,true}){
  {ProofHarness h;ProofIO io(h,work);PioneerQ38FfnPersistentGraphResult r;
   assert(h.RootProof(io,r) && h.rootVerified && io.guards==2 && io.pcs==2);
#ifdef PIONEER_Q38_FFN_SEALED_PROOF
   assert(io.postReads==(work?0U:2U*TT_Q38_FFN_GRAPH_WORDS));
#else
   assert(io.postReads==2U*TT_Q38_FFN_GRAPH_WORDS);
#endif
   positive++;
  }
  const unsigned counts[]={0,64,119,10,8,3,2,2,0,36};
  for(unsigned kind=1;kind<=9;kind++)for(unsigned field=0;field<counts[kind];field++){
   ProofHarness h;ProofIO io(h,work);io.kind=kind;io.field=field;PioneerQ38FfnPersistentGraphResult r;
   assert(!h.RootProof(io,r) && io.observed);negative++;
  }
 }
 for(unsigned word:{0U,TT_Q38_FFN_GRAPH_WORDS-1U,unsigned(TT_Q38_FFN_GRAPH_WORDS),2U*TT_Q38_FFN_GRAPH_WORDS-1U}){
  ProofHarness h;ProofIO io(h,false);io.kind=8;io.field=word;PioneerQ38FfnPersistentGraphResult r;
  assert(!h.RootProof(io,r) && io.observed);negative++;
 }
 printf("PASS ACTUAL extracted ROOT proof %u positive/%u FIRST-fault inert receipt/sequence/command/counter/status/PC/guard/idle/STOP-manifest checks;only duplicate work post-read differs;MOCK scope/endpoints NOT native authority or speed\n",positive,negative);
}
