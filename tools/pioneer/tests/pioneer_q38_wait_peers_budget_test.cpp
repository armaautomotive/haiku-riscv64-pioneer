/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include "../PioneerQ38PollSentinel.h"
struct AuditFault {};
struct PollTest {
 unsigned calls=0,backoffs=0,maximumBackoff=0,firstWorker=0,faultAt=0;
 unsigned workers=0,ready[119]={},seen[119]={},audits[119]={};
 bool stopped=false;
}* current=nullptr;
static bool Peer(unsigned worker,unsigned sequence,bool stopped,bool previous) {
 auto& t=*current;assert(worker && worker<=t.workers && sequence==worker+17 && stopped==t.stopped && !previous);
 t.calls++;if(!t.firstWorker)t.firstWorker=worker;if(t.calls==t.faultAt)throw AuditFault{};
 unsigned w=worker-1;assert(!t.audits[w]);t.seen[w]++;
 if(t.seen[w]>=t.ready[w]){t.audits[w]++;return true;}return false;
}
static void PeerPollBackoff(unsigned count) {
 auto& t=*current;assert(count>=64 && count<=2048 && !(count&(count-1)));
 t.backoffs++;if(count>t.maximumBackoff)t.maximumBackoff=count;
}
namespace PioneerQwen35 { static constexpr unsigned Vocabulary=248320; }
static volatile uint32_t workHeader[8]={};
static volatile uint32_t* Words(unsigned address) {
 assert(address==0x74000);return workHeader;
}
#define PIONEER_Q35_WORK_ROOT_IMAGE 1
#define Q38_COMMAND_WITH(value)
#define Q35_PEER_AUDIT_ARGUMENT(work,stopped)
#include "wait_peers.inc"
int main() {
 uint32_t parts[119][8]={};for(unsigned w=0;w<119;w++){parts[w][2]=1+w%3;parts[w][3]=w+18;}
 for(unsigned workers:{1U,2U,7U,119U})for(bool work:{false,true})for(bool stop:{false,true}) {
  if(work && stop)continue;
  PollTest t;current=&t;t.workers=workers;t.stopped=stop;
  for(unsigned w=0;w<workers;w++)t.ready[w]=1+w%5;
  assert(WaitPeers(parts,workers,work,stop));
  for(unsigned w=0;w<workers;w++)assert(t.audits[w]==1 && t.seen[w]==t.ready[w]);
  assert(t.calls<=workers*65536 && (work || !t.backoffs));
  if(work){unsigned sentinel=0;assert(PioneerQ38SelectPollSentinel(parts,workers,sentinel));assert(t.firstWorker==sentinel+1);}
 }
 // Argmax admits larger vocabulary shards. Its sentinel gets a proportional
 // but bounded window; all other peers still require the original proof.
 workHeader[7]=1;parts[118][2]=130;
 {
  PollTest t;current=&t;t.workers=119;t.ready[118]=70000;
  assert(WaitPeers(parts,119,true,false));
  assert(t.firstWorker==119 && t.calls==70000+118);
  for(unsigned w=0;w<119;w++)assert(t.audits[w]==1);
 }
 {
  PollTest t;current=&t;t.workers=119;t.ready[118]=589825;
  for(unsigned w=0;w<118;w++)t.ready[w]=UINT32_MAX;
  assert(!WaitPeers(parts,119,true,false));
  assert(t.firstWorker==119 && t.calls==589824 && t.audits[118]==0);
  for(unsigned w=0;w<118;w++)assert(t.audits[w]==0);
 }
 workHeader[7]=0;parts[118][2]=3;
 for(bool work:{false,true}) {
  PollTest t;current=&t;t.workers=7;for(unsigned& n:t.ready)n=UINT32_MAX;
  assert(!WaitPeers(parts,7,work,false));
  assert(t.calls==(work?65536U:7*65536U) && (work?t.maximumBackoff==2048:!t.backoffs));
  for(auto a:t.audits)assert(!a);
 }
 // Sentinel consumes the entire per-worker allowance;remaining worker
 // then consumes exactly the remaining GLOBAL budget,never budget+1.
 for(bool finalComplete:{false,true}) {
  PollTest t;current=&t;t.workers=2;t.ready[1]=65536;t.ready[0]=finalComplete?65536:UINT32_MAX;
  assert(WaitPeers(parts,2,true,false)==finalComplete && t.calls==2*65536);
  assert(t.audits[1]==1 && t.audits[0]==unsigned(finalComplete));
 }
 for(unsigned fault:{1U,2U,7U,119U}) {
  PollTest t;current=&t;t.workers=119;t.faultAt=fault;for(unsigned& n:t.ready)n=1;
  bool caught=false;try{(void)WaitPeers(parts,119,true,false);}catch(const AuditFault&){caught=true;}
  assert(caught && t.calls==fault && !t.backoffs);
 }
 puts("PASS ACTUAL extracted WaitPeers:all119 audited once,late non-sentinel cannot publish,checked+STOP no backoff,scaled bounded argmax sentinel window,ORIGINAL shared workers*65536 GLOBAL budget exact,no extra poll on exhaustion,first/mid/last audit fault propagates immediately;NO device/physical-proof substitution");
}
