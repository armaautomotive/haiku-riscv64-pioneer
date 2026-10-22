/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREDECODED_Q4_PAGE_H
#define PIONEER_Q38_PREDECODED_Q4_PAGE_H
#if !defined(PIONEER_Q38_PREDECODED_Q4_PAGE_STUDY) || defined(PIONEER_ATTENTION_FREESTANDING)
#error Separate CPU-memory page-encoding study ONLY;not a native/source/ABI selector
#endif
#include "PioneerQ38PredecodedQ4.h"
// OFFLINE DATA adapter: one original tile -> the independently tested lossless
// byte encoding. No file,model authentication,address,allocation,device or IOCTL
// authority. The caller owns the source and output. Ready is coherence ONLY,
// not authentication. No installed uploader/layout consumes this helper.
struct PioneerQ38PredecodedQ4Page {
 static constexpr unsigned PageBytes=4096;
 // A future fragment must still fit the existing 32-bit byte-count domain.
 static constexpr unsigned MaxTiles=UINT32_MAX/PioneerQ38PredecodedQ4::Bytes;
 template<class Source> static bool Read(Source& source,unsigned tiles,uint64_t page,
  uint8_t* output,unsigned capacity) {
  if(!output || capacity<PageBytes || !tiles || tiles>MaxTiles)return false;
  const uint64_t bytes=uint64_t(tiles)*PioneerQ38PredecodedQ4::Bytes;
  // Division before multiplication also refuses arbitrary overflowing pages.
  if(page>(bytes-1)/PageBytes || !source.Ready())return false;
  const uint64_t start=page*PageBytes;
  unsigned valid=unsigned(bytes-start<PageBytes?bytes-start:PageBytes),copied=0;
  uint8_t stage[PageBytes]={},original[PioneerQ38PredecodedQ4::OriginalBytes];
  uint8_t expanded[PioneerQ38PredecodedQ4::Bytes];
  while(copied<valid) {
   uint64_t at=start+copied;
   unsigned tile=unsigned(at/PioneerQ38PredecodedQ4::Bytes);
   unsigned first=unsigned(at%PioneerQ38PredecodedQ4::Bytes);
   unsigned count=PioneerQ38PredecodedQ4::Bytes-first;
   if(count>valid-copied)count=valid-copied;
   if(tile>=tiles || !source.Ready()
    || !source.ReadTile(tile,original,sizeof(original)) || !source.Ready()
    || !PioneerQ38PredecodedQ4::Prepare(original,sizeof(original),expanded,sizeof(expanded)))return false;
   for(unsigned i=0;i<count;i++)stage[copied+i]=expanded[first+i];
   copied+=count;
  }
  // Publish ALL4096 bytes,including zero padding,only after ALL required source
  // tiles and coherence checks pass. Late failure leaves output untouched.
  if(!source.Ready())return false;
  for(unsigned i=0;i<PageBytes;i++)output[i]=stage[i];
  return true;
 }
};
#endif
