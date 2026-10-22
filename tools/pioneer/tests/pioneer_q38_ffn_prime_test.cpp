/* SPDX-License-Identifier: MIT */
// Actual source-bound session/selection/span calls. Synthetic SOURCE BYTES
// and simulated native endpoints,NOT full SHA or native compute authority.
#define PIONEER_Q38_MODE_SESSION_MOCK_ONLY
#include "pioneer_q38_mode_session_test.cpp"
#include <set>
struct FfnPrimeSource:FakeSource {
 FILE* original;unsigned reads=0,failRead=0;
 explicit FfnPrimeSource(FILE* f):FakeSource(f),original(f){}
 static uint32_t Pattern(unsigned part,unsigned page,unsigned word){return (part+1)*0x9e3779b9U+page*1031+word;}
 bool ReadWeightPage(const PioneerQwen38PackedPlan& p,unsigned part,unsigned page,uint8_t* out,unsigned bytes) {
  if(!Ready() || !p.MatchesSource(model) || ++reads==failRead)return false;
  assert(bytes==4096);for(unsigned i=0;i<1024;i++){uint32_t value=Pattern(part,page,i);memcpy(out+i*4,&value,4);}
  return Ready();
 }
};
struct FfnPrimeIO:WorkSessionIO {
 FfnPrimeSource& actual;unsigned selected,spans=0,spanEnds=0,failure=0,failAt=1,progress=0;
 uint64_t uploadedBytes=0;std::set<unsigned> touched;
 std::vector<std::pair<unsigned,uint64_t>> expected;
 static unsigned Seed(FfnPrimeSource& s){const auto* t=s.Model().Find("blk.0.ffn_gate.weight");assert(t);return unsigned(t-s.Model().Tensors().data());}
 explicit FfnPrimeIO(FfnPrimeSource& s,unsigned layer):WorkSessionIO(s,Seed(s)),actual(s),selected(layer) {
  assert(layout.Build(s.Model(),8192,true,true));
  const auto& parts=layout.Parts();for(unsigned f=0;f<parts.size();f++) {
   const auto& p=parts[f];if(p.kind!=PIONEER_Q38_WEIGHT || p.layer>=64 || p.role<2 || p.role>4
    || (selected!=UINT32_MAX && p.layer!=selected))continue;
   for(unsigned at=0;at<p.allocated;at+=1048576)expected.emplace_back(f,at);
  }
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  assert(fd==73 && !stopped);
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT){*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};return true;}
  if(op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) {
   assert(begins==1 && !runs && !spanEnds && bytes==sizeof(tt_q38_span_upload) && spans<expected.size());
   auto& q=*static_cast<tt_q38_span_upload*>(data);auto [fragment,relative]=expected[spans];const auto& p=layout.Parts()[fragment];
   unsigned length=unsigned(std::min(UINT64_C(1048576),uint64_t(p.allocated)-relative));
   assert(q.version==1 && q.bank==p.bank && q.offset==p.proposedAddress-layout.Base+relative
    && q.bytes==length && !q.reserved && !q.result.state && !q.result.started);
   for(unsigned page=0;page<q.bytes/4096;page++)for(unsigned word=0;word<1024;word++)
    assert(q.data[page*1024+word]==FfnPrimeSource::Pattern(fragment,unsigned(relative/4096)+page,word));
   touched.insert(fragment);spans++;uploadedBytes+=q.bytes;
   q.result={1,TT_RUNTIME_PASSED,q.bytes,q.bytes/4096,spans,1,1,1,0,0,0,0};
   if(spans==failAt && failure) {
    if(failure==1){stopped=true;return false;}if(failure==2)q.result.nonce--;
    if(failure==3)q.result.pages--;if(failure==4)q.result.host_exact=1;
    if(failure==5)q.result.recovery_required=1;if(failure==6)actual.valid=false;
    if(failure==7){assert(actual.model.Open(actual.original));actual.epoch=actual.model.Epoch();}
    if(failure<=7)stopped=true;
   }
   return true;
  }
  if(op==TT_PCI_PROBE_Q38_SPAN_END){assert(spans==expected.size() && !runs);spanEnds++;
   *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
   if(failure==8){stopped=true;return false;}if(failure==9){actual.valid=false;stopped=true;}return true;}
  assert(op!=TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD && op!=TT_PCI_PROBE_Q38_BANK_UPLOAD && op!=TT_PCI_PROBE_Q38_WORK_RUN);
  return WorkSessionIO::Call(fd,op,data,bytes);
 }
 static bool Progress(void* opaque,uint64_t bytes,uint64_t planned){auto& io=*static_cast<FfnPrimeIO*>(opaque);
  assert(bytes==io.uploadedBytes && bytes<=planned && !io.runs && !io.spanEnds);io.progress++;return io.failure!=10;}
};
#ifndef PIONEER_Q38_FFN_PRIME_FIXTURE_ONLY
int main() {
 (void)&Check;(void)&CohortHostTestMain;Fixture fixture;unsigned positive=0,faults=0;
 for(unsigned layer:{0U,31U,63U}){FfnPrimeSource source(fixture.file);FfnPrimeIO io(source,layer);
  PioneerQ38WorkSession<FfnPrimeIO,FfnPrimeSource> session(io,source);session.Observe(FfnPrimeIO::Progress,&io);
  assert(session.SelectFfnPrime(layer) && !io.opens && session.Open() && session.PrimeSpan()
   && session.FfnPrimeComplete(layer) && !session.FfnPrimeComplete(layer^1) && !io.runs && !io.uploads
   && io.spans==io.expected.size() && io.touched.size()==21 && io.progress==io.spans && io.spanEnds==1
   && session.pagesUploaded*4096==io.uploadedBytes && session.bytesRetained==io.uploadedBytes);
  assert(session.Finish() && io.ends==1 && io.closes==1 && !session.FfnPrimeComplete(layer));positive++;
 }
 {FfnPrimeSource source(fixture.file);FfnPrimeIO io(source,UINT32_MAX);
  PioneerQ38WorkSession<FfnPrimeIO,FfnPrimeSource> session(io,source);
  assert(session.SelectFfnPrime(UINT32_MAX) && !io.opens && !io.begins && io.expected.size()>1000);positive++;}
 for(unsigned failure=1;failure<=10;failure++){FfnPrimeSource source(fixture.file);FfnPrimeIO io(source,0);io.failure=failure;
  PioneerQ38WorkSession<FfnPrimeIO,FfnPrimeSource> session(io,source);session.Observe(FfnPrimeIO::Progress,&io);
  assert(session.SelectFfnPrime(0) && session.Open() && !session.PrimeSpan() && session.Failed() && io.closes==1 && !io.ends && !io.runs);
  unsigned calls=io.spans+io.spanEnds;assert(!session.PrimeSpan() && !session.Finish() && io.spans+io.spanEnds==calls);faults++;
 }
 for(unsigned failure=0;failure<4;failure++){FfnPrimeSource source(fixture.file);FfnPrimeIO io(source,0);
  PioneerQ38WorkSession<FfnPrimeIO,FfnPrimeSource> session(io,source);
  if(!failure)assert(!session.Open());else {
   if(failure==1)source.valid=false;
   assert(!session.SelectFfnPrime(failure==2?64:failure==3?UINT32_MAX-1:0));
  }
  assert(session.Failed() && !io.opens && !io.begins && !io.spans);faults++;
 }
 for(unsigned failure=0;failure<6;failure++){FfnPrimeSource source(fixture.file);FfnPrimeIO io(source,0);
  PioneerQ38WorkSession<FfnPrimeIO,FfnPrimeSource> session(io,source);assert(session.SelectFfnPrime(0));
  if(!failure){assert(source.model.Open(fixture.file));source.epoch=source.model.Epoch();assert(!session.Open() && !io.opens);}
  else if(failure==1){assert(!session.SelectFfnPrime(1) && !io.opens);}
  else {
   assert(session.Open());
   if(failure==2){source.failRead=1;assert(!session.PrimeSpan() && !io.spans && !io.spanEnds);}
   if(failure==3){assert(!session.PrimeSpan(io.tensor) && !io.spans && !io.spanEnds);}
   if(failure==4){assert(session.PrimeSpan());source.valid=false;assert(!session.Finish());}
   if(failure==5){float x=1,y=2;assert(!session.Multiply(io.tensor,&x,1,&y,1) && y==2 && !io.runs && !io.spans);}
  }
  unsigned calls=io.spans+io.spanEnds+io.ends;assert(session.Failed() && !session.Finish() && !io.ends
   && !io.runs && io.spans+io.spanEnds+io.ends==calls);faults++;
 }
 printf("PASS FFN source-bound real session:positive%u/faults%u,layer0/31/63 ALL3x7stripes+ALL64selection,original TC bytes/zero matrix and CPU learned products,full two-slot canonical layout,one spanCLOSE before BANK_END,first mismatch/source loss CLOSE ONLY no END/retry/fallback;SYNTHETIC_SOURCE_ENDPOINTS_NOT_NATIVE_AUTH_OR_FFN_COMPUTE\n",positive,faults);
}
#endif
