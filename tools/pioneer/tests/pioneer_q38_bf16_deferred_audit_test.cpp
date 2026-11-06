/* SPDX-License-Identifier: MIT */
// Coverage DATA only. Mock callback outcomes below are NOT native receipts.
#include "../PioneerQ38Bf16DeferredAudit.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <cstdint>
using A=PioneerQ38Bf16DeferredAudit;
static A Done(unsigned width,unsigned used){
 A a;assert(a.Begin(width,used));
 for(unsigned w=0;w<used;w++){assert(!a.Closed() && a.MatrixClosed(w));}
 for(unsigned w=used;w<119;w++){assert(!a.Closed() && a.IdleClosed(w));}
 assert(a.Closed());return a;
}
int main(){
 unsigned positives=0,refused=0,corruptions=0;
 for(unsigned width:{5120U,17408U})for(unsigned used=1;used<=119;used++){
  auto good=Done(width,used);positives++;
  for(unsigned missing=0;missing<119;missing++){
   A a;assert(a.Begin(width,used));
   for(unsigned w=0;w<119;w++)if(w!=missing)assert(w<used?a.MatrixClosed(w):a.IdleClosed(w));
   assert(!a.Closed());refused++;
  }
  for(unsigned w=0;w<119;w++){
   auto a=good;assert(!(w<used?a.MatrixClosed(w):a.IdleClosed(w)) && !a.Closed());refused++;
   A b;assert(b.Begin(width,used));assert(!(w<used?b.IdleClosed(w):b.MatrixClosed(w)) && !b.Closed());refused++;
  }
 }
 for(unsigned width:{0U,5119U,5121U,17407U,17409U,UINT32_MAX}){
  A a;assert(!a.Begin(width,119) && !a.Closed() && !a.Begin(5120,119));refused++;
 }
 for(unsigned used:{0U,120U,UINT32_MAX}){A a;assert(!a.Begin(5120,used) && !a.Closed());refused++;}
 {A a;assert(!a.Closed() && !a.MatrixClosed(0) && !a.Begin(5120,119));refused++;}
 {A a;assert(!a.IdleClosed(118) && !a.Begin(5120,1));refused++;}
 {auto a=Done(5120,1);assert(!a.Begin(17408,119) && !a.Closed());refused++;}
 for(unsigned w:{119U,120U,UINT32_MAX}){
  auto a=Done(5120,119);assert(!a.MatrixClosed(w) && !a.Closed());refused++;
  auto b=Done(17408,1);assert(!b.IdleClosed(w) && !b.Closed());refused++;
 }
 const auto good=Done(17408,68);
 for(unsigned bit=0;bit<sizeof(A)*8;bit++){
  auto a=good;auto* bytes=reinterpret_cast<unsigned char*>(&a);bytes[bit/8]^=1U<<(bit%8);
  assert(!a.Closed());corruptions++;
 }
 std::printf("PASS deferred-audit DATA positive%u missing/duplicate/wrong-partition/shape/order refusals%u single-bit faults%u;all119 closures required,sticky poison,52B stack;MOCK closures NOT actual receipts/native/hardware/TPS\n",positives,refused,corruptions);
}
