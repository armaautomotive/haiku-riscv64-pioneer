/* SPDX-License-Identifier: MIT */
// Traversal for the exact routed-token held L1 snapshot. This describes
// original-data reads only; it cannot authorize a mapping, write or restore.
#ifndef PIONEER_Q38_HELD_SNAPSHOT_BATCH_H
#define PIONEER_Q38_HELD_SNAPSHOT_BATCH_H

#include "PioneerQ38TokenRedistributedFootprint.h"

namespace PioneerQ38HeldSnapshotBatch {
namespace L1 = PioneerQ38TokenRedistributedL1;
constexpr unsigned MaxWords = 128;

enum class Window { Scalar, Low, Text, Aux, Panel, High };
struct Batch {
 unsigned ordinal, words;
 uint32_t address;
 Window window;
};

inline uint32_t EndOfWindow(uint32_t address, Window& window)
{
 if(address==0x6000 || (address>=0x17ffc0 && address<0x180000)){
  window=Window::Scalar;return address+4;
 }
 if(address>=0x20000 && address<0x40000){window=Window::Low;return 0x40000;}
 if(address>=0x40000 && address<0x60000){window=Window::Text;return 0x60000;}
 if(address>=0x60000 && address<0x6e000){window=Window::Aux;return 0x6e000;}
 if(address>=0x6e000 && address<0xac000){window=Window::Panel;return 0xac000;}
 if(address>=0xac000 && address<0x17ffc0){window=Window::High;return 0x17ffc0;}
 return 0;
}

inline bool Next(unsigned worker,unsigned ordinal,Batch& out)
{
 if(ordinal>=L1::SnapshotWords(worker))return false;
 unsigned offset=ordinal;
 for(unsigned index=0;index<L1::SnapshotSpans(worker);index++){
  const auto span=L1::SnapshotSpan(worker,index);
  if(offset>=span.words){offset-=span.words;continue;}
  const uint32_t address=span.address+offset*4;
  Window window=Window::Scalar;
  const uint32_t end=EndOfWindow(address,window);
  if(end<=address)return false;
  unsigned words=span.words-offset, windowWords=(end-address)/4;
  if(words>MaxWords)words=MaxWords;
  if(words>windowWords)words=windowWords;
  if(!words)return false;
  out={ordinal,words,address,window};return true;
 }
 return false;
}

// IO is provided by the original held owner, not by untrusted callers.
// Scope proves the same live worker, owner and window before and after the
// bounded operation. Load uses the existing scalar read for the exceptional
// boot/tail words and only already mapped uncached L1 aliases otherwise.
// Nothing is published to destination after a failed read or late check.
template<class IO> bool Read(IO& io,unsigned worker,const Batch& request,uint32_t* destination)
{
 Batch actual{};
 if(!destination || !Next(worker,request.ordinal,actual) || request.words!=actual.words
  || request.address!=actual.address || request.window!=actual.window || !io.Scope(worker))return false;
 uint32_t words[MaxWords];
 if(!io.Load(worker,actual,words) || !io.Scope(worker))return false;
 for(unsigned index=0;index<actual.words;index++)destination[index]=words[index];
 return true;
}
}
#endif
