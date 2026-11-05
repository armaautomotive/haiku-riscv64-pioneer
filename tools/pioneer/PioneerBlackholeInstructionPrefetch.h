/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BLACKHOLE_INSTRUCTION_PREFETCH_H
#define PIONEER_BLACKHOLE_INSTRUCTION_PREFETCH_H
// Fixed-field policy only. Caller owns the entire held/reset tile context,
// immutable executable L1 and normal/fault lifetime. No ownership grant.
// Vendor cfg_defines81989dcdb:limit203,control208;Blackhole count uses4bits.
class PioneerBlackholeInstructionPrefetch {
 static_assert(sizeof(unsigned)==4,"fixed Blackhole register width");
public:
 enum {Limit=203,Control=208,EnableT0=1,RequestMask=15<<5,Requests=4,
       ProgramLastByte=0xc0c67};
private:
 enum {Fresh,Active,Restored,Unsafe};unsigned state=Fresh;
 unsigned beforeLimit=0,beforeControl=0,selectedControl=0;
 __attribute__((always_inline)) inline bool Fail(){state=Unsafe;return false;}
 template<class IO> __attribute__((always_inline)) inline bool Write(IO& io,unsigned index,unsigned value){
  unsigned observed=0;
  return io.WriteConfig(index,value) && io.ReadConfig(index,observed) && observed==value;
 }
public:
 __attribute__((always_inline)) inline bool IsUnsafe()const{return state==Unsafe;}
 template<class IO> __attribute__((always_inline)) inline bool Begin(IO& io){
  if(state!=Fresh)return false;
  // Latch failure before the first possible operation;no retry/rollback.
  state=Unsafe;unsigned limit=0,control=0;
  if(!io.ReadConfig(Limit,beforeLimit) || !io.ReadConfig(Control,beforeControl)
   || !io.ReadConfig(Limit,limit) || limit!=beforeLimit
   || !io.ReadConfig(Control,control) || control!=beforeControl)return false;
  selectedControl=(beforeControl&~unsigned(RequestMask|EnableT0))|(Requests<<5)|EnableT0;
  // Disable T0's prefetcher before changing its bound. Preserve all other
  // enable/reserved bits and restore the shared request count at STOP.
  if(!Write(io,Control,beforeControl&~unsigned(EnableT0))
   || !Write(io,Limit,ProgramLastByte) || !Write(io,Control,selectedControl))return false;
  state=Active;return true;
 }
 template<class IO> __attribute__((always_inline)) inline bool Restore(IO& io){
  if(state!=Active)return false;
  unsigned limit=0,control=0;
  if(!io.ReadConfig(Limit,limit) || limit!=ProgramLastByte
   || !io.ReadConfig(Control,control) || control!=selectedControl)return Fail();
  state=Unsafe;
  // Restore only while the exact selected values still belong to us.
  // Every write is read back and consumed before the next operation.
  if(!Write(io,Control,selectedControl&~unsigned(EnableT0))
   || !Write(io,Limit,beforeLimit) || !Write(io,Control,beforeControl))return false;
  state=Restored;return true;
 }
};
#endif
