/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38GdnState.h"
#include <cassert>
#include <cstdio>
#include <type_traits>
#include <initializer_list>
static const PioneerQ38GdnIdentity Owner={11,19,7,2};
static unsigned LayerAt(unsigned ordinal) { return ordinal+ordinal/3; }
static void Complete(PioneerQ38GdnState& s,const PioneerQ38GdnTicket& t,unsigned except=96) {
 for (unsigned w=0;w<96;w++) if (w!=except) assert(s.CompletedHalf(t,w));
}
int main()
{
 static_assert(!std::is_copy_constructible<PioneerQ38GdnState>::value,"never duplicate an owner");
 PioneerQ38GdnState s; PioneerQ38GdnIdentity id={}; uint64_t at=99; unsigned slot=99;
 assert(!s.Snapshot(id,at,slot) && at==99 && slot==99);
 for (unsigned field=0;field<4;field++) {
  auto bad=Owner; if (!field) bad.session=0; if (field==1) bad.sequence=0; if (field==2) bad.modelEpoch=0; if (field==3) bad.driverGeneration=0;
  assert(!s.Bind(bad));
 }
 assert(s.Bind(Owner) && !s.Bind(Owner));
 PioneerQ38GdnTicket stale={};
 for (unsigned token=0;token<16;token++) {
  assert(s.Snapshot(id,at,slot) && id==Owner && at==token && slot==(token%2));
  auto foreign=Owner; foreign.sequence++;
  assert(!s.BeginToken(foreign,token) && !s.BeginToken(Owner,token+1) && s.BeginToken(Owner,token));
  assert(s.Unsafe() && !s.Snapshot(id,at,slot) && !s.Close() && !s.CommitToken(Owner,token));
  for (unsigned ordinal=0;ordinal<48;ordinal++) {
   PioneerQ38GdnTicket t={}; assert(!s.BeginLayer(63,t) && s.BeginLayer(LayerAt(ordinal),t));
   assert(!s.BeginLayer(LayerAt(ordinal),stale) && !s.CompletedHalf(stale,0));
   for (unsigned worker=0;worker<96;worker++) {
    PioneerQ38GdnStateSlice a={},b={}; assert(s.Slice(t,worker,false,a) && s.Slice(t,worker,true,b));
    assert(a.owner==Owner && a.position==token && a.nonce==token+1 && a.layer==LayerAt(ordinal)
     && a.slot==token%2 && b.slot==((token%2)^1U) && a.firstWord==worker*8192 && b.firstWord==a.firstWord
     && a.words==8192 && a.head==worker/2 && a.half==worker%2 && a.keyHead==(worker/2)%16);
   }
   auto bad=t; bad.owner.driverGeneration++; assert(!s.CompletedHalf(bad,0));
   bad=t; bad.position++; assert(!s.CompletedHalf(bad,0));
   bad=t; bad.nonce++; assert(!s.CompletedHalf(bad,0));
   bad=t; bad.layer=LayerAt((ordinal+1)%48); assert(!s.CompletedHalf(bad,0));
   bad=t; bad.readSlot^=1; assert(!s.CompletedHalf(bad,0));
   bad=t; bad.writeSlot^=1; assert(!s.CompletedHalf(bad,0));
   assert(!s.CompletedHalf(t,96));
   unsigned last=(token*17+ordinal*7)%96;
   Complete(s,t,last); PioneerQ38GdnStateSlice view={};
   assert(!s.CompleteLayer(t) && !s.CommitToken(Owner,token) && !s.CompletedStateView(t,last,view));
   assert(s.CompletedHalf(t,last) && !s.CompletedHalf(t,last) && s.CompleteLayer(t) && !s.CompleteLayer(t));
   assert(s.CompletedStateView(t,last,view) && view.slot==t.writeSlot && view.layer==t.layer);
   stale=t;
  }
  assert(!s.CommitToken(Owner,token+1) && s.CommitToken(Owner,token) && !s.CommitToken(Owner,token));
  PioneerQ38GdnStateSlice oldView={}; assert(!s.CompletedStateView(stale,0,oldView));
 }
 assert(s.Close() && !s.Close() && !s.Bind(Owner) && !s.BeginToken(Owner,16));
 for (unsigned layer:{0U,24U,47U}) for (unsigned missing:{0U,47U,95U}) {
  PioneerQ38GdnState broken; assert(broken.Bind(Owner) && broken.BeginToken(Owner,0));
  for (unsigned ordinal=0;ordinal<=layer;ordinal++) {
   PioneerQ38GdnTicket t={}; assert(broken.BeginLayer(LayerAt(ordinal),t));
   Complete(broken,t,ordinal==layer?missing:96);
   if (ordinal<layer) assert(broken.CompleteLayer(t));
   else {
    assert(!broken.CompleteLayer(t)); broken.Fault();
    assert(broken.Unsafe() && !broken.CompletedHalf(t,missing) && !broken.CompleteLayer(t)
     && !broken.CommitToken(Owner,0) && !broken.Snapshot(id,at,slot) && !broken.Close() && !broken.Bind(Owner));
   }
  }
 }
 puts("PASS OFFLINE GDN state policy:16 tokens x48 layers x96 halves,both slots/key-head mapping,fresh tickets/owners,one flip only after ALL layers,first/middle/last missing completion and sticky quarantine;NO allocation/transport/native access authority");
}
