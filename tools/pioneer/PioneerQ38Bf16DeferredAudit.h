/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_DEFERRED_AUDIT_H
#define PIONEER_Q38_BF16_DEFERRED_AUDIT_H
// Internal ROOT coverage DATA,NOT a native/source/cleanup capability. Actual
// producer,worker and idle checks remain at their callers. Every119 final
// closure must be observed before projected results can leave Run().
class PioneerQ38Bf16DeferredAudit {
 using Word=__UINT32_TYPE__;
 volatile Word bits[4],inverse[4];
 volatile Word width=0,widthInverse=(~Word(0)),used=0,usedInverse=(~Word(0)),state=0;
 enum : Word {Armed=0x31414442,Poisoned=0x46414442};
 bool Fail(){state=Poisoned;return false;}
 bool Integrity()const{
  if(state!=Armed || (width!=5120 && width!=17408) || (width^widthInverse)!=(~Word(0))
   || !used || used>119 || (used^usedInverse)!=(~Word(0)) || (bits[3]&0xff800000U))return false;
  for(unsigned i=0;i<4;i++)if((bits[i]^inverse[i])!=(~Word(0)))return false;
  return true;
 }
 bool Mark(unsigned worker,bool matrix){
  if(!Integrity() || worker>=119 || (worker<used)!=matrix)return Fail();
  const unsigned word=worker/32;const Word mask=1U<<(worker%32);
  if(bits[word]&mask)return Fail();
  const Word next=bits[word]|mask;bits[word]=next;inverse[word]=~next;return true;
 }
public:
 PioneerQ38Bf16DeferredAudit(){for(unsigned i=0;i<4;i++){bits[i]=0;inverse[i]=(~Word(0));}}
 bool Begin(unsigned k,unsigned matrixWorkers){
  if(state || width || widthInverse!=(~Word(0)) || used || usedInverse!=(~Word(0)))return Fail();
  for(unsigned i=0;i<4;i++)if(bits[i] || inverse[i]!=(~Word(0)))return Fail();
  if((k!=5120 && k!=17408) || !matrixWorkers || matrixWorkers>119)return Fail();
  width=k;widthInverse=~Word(k);used=matrixWorkers;usedInverse=~Word(matrixWorkers);state=Armed;return true;
 } // Called ONLY AFTER actual all119 initial admission;does not perform it.
 bool MatrixClosed(unsigned worker){return Mark(worker,true);}
 bool IdleClosed(unsigned worker){return Mark(worker,false);}
 bool Closed()const{
  return Integrity() && bits[0]==(~Word(0)) && bits[1]==(~Word(0)) && bits[2]==(~Word(0)) && bits[3]==0x007fffff;
 }
};
static_assert(sizeof(PioneerQ38Bf16DeferredAudit)==52,"bounded ROOT stack ledger,no heap or globals");
#endif
