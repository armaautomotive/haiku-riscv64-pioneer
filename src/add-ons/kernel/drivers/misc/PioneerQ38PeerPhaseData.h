/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PEER_PHASE_DATA_H
#define PIONEER_Q38_PEER_PHASE_DATA_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// DATA envelope ONLY,NEVER a source/address/NoC/ownership/END capability.
// Caller must independently complete/prove ONE private64B read from the
// already-completed selected peer,then bind ALL expected private identity.
// Invalid clocks are explicitly representable,NOT a failed transaction.
enum {PioneerQ38PeerPhaseValid=0x31545051,PioneerQ38PeerPhaseInvalid=0x30545051};
static inline bool PioneerQ38PeerPhaseReadback(const uint32_t* data,unsigned worker,unsigned nonce,
 unsigned type,unsigned inputs,unsigned groups,uint32_t (&out)[7]){
 if(!data||!worker||worker>=120||!nonce||nonce>=0x7fffffff||!groups||groups>16
  ||(type!=8&&type!=12&&type!=14)||(inputs!=5120&&inputs!=6144&&inputs!=17408))return false;
 if(data[0]!=PioneerQ38PeerPhaseValid||data[1]!=1||data[2]!=nonce||data[3]!=worker
  ||data[4]!=type||data[5]!=inputs||data[6]!=groups||data[7]>1||data[14]||data[15])return false;
 if(!data[7])for(unsigned word=8;word<14;word++)if(data[word])return false;
 // No numerical/clock-range check contributes to graph/cleanup authority.
 // Commit only AFTER the entire immutable DATA envelope was checked.
 out[0]=data[7]?PioneerQ38PeerPhaseValid:PioneerQ38PeerPhaseInvalid;
 for(unsigned word=1;word<7;word++)out[word]=data[7+word];
 return true;
}
static inline bool PioneerQ38PeerPhaseEnvelopeValid(const uint32_t* stats){
 if(!stats)return false;
 if(stats[57]==PioneerQ38PeerPhaseValid)return true; // opaque unsigned DATA,not execution proof
 if(stats[57]!=PioneerQ38PeerPhaseInvalid)return false;
 for(unsigned word=58;word<64;word++)if(stats[word])return false;
 return true;
}
#endif
