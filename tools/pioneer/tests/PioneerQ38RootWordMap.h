/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TEST_FAST_ROOT_MEMORY) || PIONEER_Q38_TEST_FAST_ROOT_MEMORY != 1
#error Exact optional HOST test storage role required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_ATTENTION_FREESTANDING)
#error Host test word storage is not image/native/Work memory or a device capability
#endif
#ifndef PIONEER_Q38_TEST_ROOT_WORD_MAP_H
#define PIONEER_Q38_TEST_ROOT_WORD_MAP_H
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
// Exact sparse word semantics, with direct indexing ONLY for aligned host
// keys below0x180000. Unaligned/high keys retain std::map behavior. Absent
// words stay absent; no implicit zero bytes or model products are supplied.
// This does not skip instructions,cache results or change any device ROM.
class PioneerQ38RootWordMap {
 static constexpr unsigned Limit=0x180000,Words=Limit/4;
 std::vector<uint32_t> values=std::vector<uint32_t>(Words);
 std::vector<uint8_t> present=std::vector<uint8_t>(Words);
 std::map<uint32_t,uint32_t> other;size_t lowCount=0;
 static bool Direct(uint32_t address){return address<Limit && !(address&3);}
public:
 // Only the read-only find/end diagnostic surface is needed by the ROOT
 // harness. Deliberately no writable iterator or iteration shortcut.
 class const_iterator {
  friend class PioneerQ38RootWordMap;
  const PioneerQ38RootWordMap* owner=nullptr;bool valid=false;
  std::pair<uint32_t,uint32_t> word={};
  const_iterator(const PioneerQ38RootWordMap* p,bool found,uint32_t address=0,uint32_t value=0):owner(p),valid(found),word(address,value){}
 public:
  const std::pair<uint32_t,uint32_t>* operator->()const{return &word;}
  bool operator==(const const_iterator& b)const{return owner==b.owner && valid==b.valid && (!valid || word.first==b.word.first);}
  bool operator!=(const const_iterator& b)const{return !(*this==b);}
 };
 size_t count(uint32_t address)const{return Direct(address)?present[address/4]:other.count(address);}
 uint32_t& operator[](uint32_t address){
  if(!Direct(address))return other[address];unsigned at=address/4;
  if(!present[at]){present[at]=1;values[at]=0;lowCount++;}return values[at];
 }
 const uint32_t& at(uint32_t address)const{
  if(!Direct(address))return other.at(address);
  if(!present[address/4])throw std::out_of_range("unmapped ROOT word");return values[address/4];
 }
 uint32_t& at(uint32_t address){return const_cast<uint32_t&>(static_cast<const PioneerQ38RootWordMap&>(*this).at(address));}
 size_t erase(uint32_t address){
  if(!Direct(address))return other.erase(address);unsigned at=address/4;
  if(!present[at])return 0;present[at]=0;lowCount--;return 1;
 }
 size_t size()const{return lowCount+other.size();}bool empty()const{return !size();}
 void clear(){std::fill(present.begin(),present.end(),0);lowCount=0;other.clear();}
 const_iterator end()const{return const_iterator(this,false);}
 const_iterator find(uint32_t address)const{return count(address)?const_iterator(this,true,address,at(address)):end();}
};
#endif
