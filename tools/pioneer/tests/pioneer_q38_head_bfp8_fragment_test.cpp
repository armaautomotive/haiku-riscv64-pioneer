/* SPDX-License-Identifier: MIT */
// Q6 -> BFP8 head stripe as page data; synthetic model, no disk or device.
#define PIONEER_HEAD_FIXTURE_ONLY
#include "pioneer_q38_head_block_float_test.cpp"
#include "../PioneerQ38HeadBfp8FragmentSource.h"

static PioneerQ38PackedPart Fragment(){return {PIONEER_Q38_WEIGHT,64,1,0,0,0,64,4240,0x04000000,271360,274432};}
int main(){
 HeadSource source,reference;auto part=Fragment();PioneerQ38HeadBfp8FragmentSource<HeadSource> reader;
 assert(reader.Bind(source,part) && reader.Ready() && reader.Bytes()==348160 && reader.Pages()==85 && !reader.Bind(source,part));
 std::vector<uint8_t> bands[2];for(unsigned b=0;b<2;b++)
  assert(PioneerQ38HeadBlockFloatDiagnostic::PackBand(reference,0,b*32,PioneerBlackholeBlockFloat::Format::Bfp8B,bands[b]));
 for(unsigned page=0;page<85;page++){
  std::array<uint8_t,4096> actual={},expected={};
  assert(reader.ReadPage(page,actual.data(),actual.size()));
  for(unsigned i=0;i<4096;i++){
   const unsigned at=page*4096+i,band=at/(160*1088),offset=at%(160*1088);assert(band<2);
   expected[i]=bands[band][offset];
  }
  assert(actual==expected);
 }
 assert(source.reads==64); // one decode of each complete32-row band
 std::array<uint8_t,4096> untouched;untouched.fill(0xa5);const auto before=untouched;
 assert(!reader.ReadPage(85,untouched.data(),untouched.size()) && !reader.ReadPage(0,nullptr,4096)
  && !reader.ReadPage(0,untouched.data(),4095) && untouched==before && source.reads==64);
 for(unsigned bad=0;bad<13;bad++){
  HeadSource s;auto p=part;
  if(bad==0)p.kind=PIONEER_Q38_WORKSPACE;if(bad==1)p.layer=63;if(bad==2)p.role=0;if(bad==3)p.tensor=1;
  if(bad==4)p.bank=7;if(bad==5)p.firstRow=1;if(bad==6)p.rows=32;if(bad==7)p.rowBytes++;
  if(bad==8)p.bytes--;if(bad==9)p.allocated+=4096;if(bad==10)p.proposedAddress=0;
  if(bad==11)s.tensors[0].name="token_embd.weight";if(bad==12)s.tensors[0].outputs--;
  PioneerQ38HeadBfp8FragmentSource<HeadSource> broken;assert(!broken.Bind(s,p) && !broken.Ready() && !s.reads);
 }
 for(unsigned bad=0;bad<3;bad++){
  HeadSource s;PioneerQ38HeadBfp8FragmentSource<HeadSource> broken;assert(broken.Bind(s,part));
  if(bad==0)s.fail=5;if(bad==1)s.change=5;if(bad==2)s.tensors[0].name="changed";
  untouched=before;assert(!broken.ReadPage(0,untouched.data(),untouched.size()) && untouched==before && !broken.Ready());
  s.ready=true;s.fail=s.change=0;s.epoch=1;s.tensors[0].name="output.weight";
  unsigned reads=s.reads;assert(!broken.ReadPage(0,untouched.data(),untouched.size()) && s.reads==reads);
 }
 puts("PASS Q6 head -> BFP8 fragment: all85 pages incl cross-band, exact synthetic converted data, source/geometry refusals and poison; NO model, disk, device or TPS");
}
