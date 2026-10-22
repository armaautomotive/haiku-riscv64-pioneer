/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PREDECODED_Q4_PAGE_STUDY
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "../PioneerQ38PredecodedQ4Page.h"
static uint8_t Packed(unsigned tile,unsigned byte,unsigned epoch) {
 uint32_t v=(tile+1)*0x9e3779b9U+byte*0x45d9f3bU+epoch*0x384732U;
 v^=v>>16;v*=0x85ebca6bU;v^=v>>13;return uint8_t(v);
}
// Independent mapping,not Prepare/Recover or the page reader under test.
static uint8_t Expanded(unsigned tile,unsigned byte,unsigned epoch) {
 if(byte<256)return Packed(tile,byte,epoch);
 unsigned panel=(byte-256)/256,lane=(byte-256)%256;
 unsigned packed=256+(panel/4)*512+(panel%2)*256+lane;
 return uint8_t((Packed(tile,packed,epoch)/(panel%4<2?1U:16U))%16);
}
struct Source {
 unsigned tiles,epoch,calls=0,readyCalls=0,failRead=0,failReady=0;
 unsigned previous=UINT32_MAX;
 bool Ready(){return ++readyCalls!=failReady;}
 bool ReadTile(unsigned tile,uint8_t* out,unsigned capacity) {
  assert(tile<tiles && out && capacity==2304);
  assert(!calls || tile==previous+1);previous=tile;
  if(++calls==failRead)return false;
  for(unsigned b=0;b<capacity;b++)out[b]=Packed(tile,b,epoch);
  return true;
 }
};
int main() {
 using Page=PioneerQ38PredecodedQ4Page;
 std::array<uint8_t,4098> out;
 unsigned pages=0,twoTilePages=0,partials=0;bool alignments[17]={};
 for(unsigned epoch:{0U,1U})for(unsigned tiles=1;tiles<=128;tiles++) {
  uint64_t bytes=uint64_t(tiles)*4352,count=(bytes+4095)/4096;
  for(uint64_t page=0;page<count;page++) {
   Source source={tiles,epoch};out.fill(0xa5);
   assert(Page::Read(source,tiles,page,out.data()+1,4096));
   assert(out.front()==0xa5 && out.back()==0xa5);
   unsigned valid=unsigned(bytes-page*4096<4096?bytes-page*4096:4096);
   alignments[unsigned((page*4096)%4352)/256]=true;
   for(unsigned b=0;b<4096;b++) {
    uint64_t at=page*4096+b;
    uint8_t expected=b<valid?Expanded(unsigned(at/4352),unsigned(at%4352),epoch):0;
    assert(out[b+1]==expected);
   }
   assert(source.calls==1 || source.calls==2);
   twoTilePages+=source.calls==2;partials+=valid<4096;pages++;
  }
 }
 assert(twoTilePages && partials);
 for(bool seen:alignments)assert(seen);
 // A crossing page needs TWO source tiles. Every read/Ready failure,including
 // the final Ready check,refuses before publishing even the first byte.
 unsigned failures=0;
 for(unsigned read=1;read<=2;read++) {
  Source source={16,1};source.failRead=read;out.fill(0xdd);
  assert(!Page::Read(source,16,1,out.data()+1,4096));
  for(auto b:out)assert(b==0xdd);failures++;
 }
 for(unsigned ready=1;ready<=6;ready++) {
  Source source={16,1};source.failReady=ready;out.fill(0xdd);
  assert(!Page::Read(source,16,1,out.data()+1,4096));
  for(auto b:out)assert(b==0xdd);failures++;
 }
 // Invalid bounds must not even ask the source about coherence or data.
 for(unsigned bad=0;bad<7;bad++) {
  Source source={16,1};out.fill(0xdd);unsigned tiles=16,capacity=4096;
  uint64_t page=0;uint8_t* pointer=out.data()+1;
  if(bad==0)pointer=nullptr;if(bad==1)capacity=4095;if(bad==2)tiles=0;
  if(bad==3)tiles=Page::MaxTiles+1;if(bad==4)page=17;
  if(bad==5)page=UINT64_MAX;if(bad==6)page=UINT64_MAX/4096+1;
  assert(!Page::Read(source,tiles,page,pointer,capacity));
  assert(!source.calls && !source.readyCalls);
  for(auto b:out)assert(b==0xdd);failures++;
 }
 // Highest admitted fragment and its final page: arithmetic stays bounded;
 // this is DATA size testing,NOT permission to map that range on any device.
 Source source={Page::MaxTiles,1};
 uint64_t bytes=uint64_t(Page::MaxTiles)*4352;
 assert(bytes<=UINT32_MAX && Page::Read(source,Page::MaxTiles,(bytes-1)/4096,out.data()+1,4096));
 uint64_t start=((bytes-1)/4096)*4096;
 for(unsigned b=0;b<4096;b++) {
  uint64_t at=start+b;
  assert(out[b+1]==(at<bytes?Expanded(unsigned(at/4352),unsigned(at%4352),1):0));
 }
 printf("PASS OFFLINE Q4 page DATA pages%u crossing%u partial%u refusals%u;two changed patterns/all17alignments/zero padding/late atomic failure/32bitfragment and64bitpage bounds;NO_FILE_AUTH_LAYOUT_ALLOCATION_DEVICE_NATIVE_OR_SPEED_AUTHORITY\n",pages,twoTilePages,partials,failures);
}
