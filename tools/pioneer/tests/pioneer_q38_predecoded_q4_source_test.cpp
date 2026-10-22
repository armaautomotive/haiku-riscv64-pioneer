/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#define PIONEER_Q38_PREDECODED_Q4_PAGE_STUDY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38PredecodedQ4Reader.h"
#include "../PioneerQwen38Panel.h"
using Reader=PioneerQ38PredecodedQ4Reader<PioneerQwen38Gguf>;
static std::vector<uint8_t> OriginalTile(PioneerQwen38Gguf& source,PioneerQwen38TiledReader& reader,
 unsigned tensor,unsigned first,unsigned rows,unsigned tile) {
 std::vector<uint8_t> result(2304);uint8_t page[4096];
 uint64_t offset=uint64_t(tile)*2304;unsigned copied=0;
 while(copied<2304) {
  assert(reader.ReadPage(source,tensor,first,rows,unsigned(offset/4096),page,sizeof(page)));
  unsigned within=unsigned(offset%4096),take=4096-within;if(take>2304-copied)take=2304-copied;
  memcpy(result.data()+copied,page+within,take);copied+=take;offset+=take;
 }
 return result;
}
static void CheckTile(PioneerQwen38Gguf& source,PioneerQwen38TiledReader& original,Reader& reader,
 unsigned tensor,unsigned first,unsigned rows,unsigned tile) {
 uint8_t encoded[4352],page[4098],recovered[2304];uint64_t offset=uint64_t(tile)*4352;unsigned copied=0;
 while(copied<4352) {
  memset(page,0xdd,sizeof(page));assert(reader.ReadPage(source,tensor,first,rows,offset/4096,page+1,4096));
  assert(page[0]==0xdd && page[4097]==0xdd);
  unsigned within=unsigned(offset%4096),take=4096-within;if(take>4352-copied)take=4352-copied;
  memcpy(encoded+copied,page+1+within,take);offset+=take;copied+=take;
 }
 auto packed=OriginalTile(source,original,tensor,first,rows,tile);
 assert(PioneerQ38PredecodedQ4::Recover(encoded,sizeof(encoded),recovered,sizeof(recovered)));
 assert(!memcmp(recovered,packed.data(),2304) && !memcmp(encoded,packed.data(),256));
 for(unsigned p=0;p<16;p++) {
  PioneerQ38Panel panel;assert(PioneerQ38UnpackPanel(12,packed.data(),2304,p,&panel));
  for(unsigned lane=0;lane<256;lane++) {
   unsigned value=(panel.operand[lane/2]>>((lane%2)*16))&65535;
   assert(!(value&0x8000) && (value&1023)<=15 && encoded[256+p*256+lane]==(value&1023));
  }
 }
}
static void Synthetic() {
 Fixture fixture;unsigned tensor=0;
 while(fixture.expected[tensor].name!="blk.0.ffn_down.weight")tensor++;
 const auto& t=fixture.expected[tensor];unsigned blocks=unsigned(t.inputs/256),rowBytes=blocks*144;
 std::vector<uint8_t> packed(rowBytes*48);
 for(unsigned row=0;row<48;row++)for(unsigned block=0;block<blocks;block++) {
  auto* at=packed.data()+row*rowBytes+block*144;
  for(unsigned b=0;b<144;b++)at[b]=uint8_t(row*71+block*11+b*37);
  // Finite bounded positive binary16 metadata for independent panel decoder.
  at[0]=0;at[1]=0x3c;at[2]=0;at[3]=0x38;
 }
 fixture.Write(t.offset,packed);PioneerQwen38Gguf source;assert(source.Open(fixture.file));
 Reader reader;PioneerQwen38TiledReader original;assert(reader.Bind(source) && original.Bind(source));
 for(unsigned first:{0U,16U})for(unsigned tile:{0U,1U,67U,68U,135U})
  CheckTile(source,original,reader,tensor,first,32,tile);
 uint8_t page[4096];memset(page,0xdd,sizeof(page));
 assert(source.Open(fixture.file));
 assert(!reader.ReadPage(source,tensor,0,16,0,page,sizeof(page)));
 for(auto b:page)assert(b==0xdd);
 Reader truncated;assert(truncated.Bind(source));
 assert(!ftruncate(fileno(fixture.file),off_t(PioneerQwen38Gguf::FullBytes)-1));
 assert(!truncated.ReadPage(source,tensor,0,16,0,page,sizeof(page)));
 for(auto b:page)assert(b==0xdd);
 puts("PASS OFFLINE actual GGUF parser/tiled-reader/Q4-panel integration:10boundarytiles all16panels/losslessinverse/canaries/sourceepoch/truncation refusal;NO_AUTH_OR_DEVICE");
}
static void Public(const char* path) {
 FILE* file=fopen(path,"rb");assert(file);PioneerQwen38Gguf source;assert(source.Open(file));
 PioneerQwen38PackedPlan plan;assert(plan.Build(source,8192,true,true));
 Reader reader;PioneerQwen38TiledReader original;assert(reader.Bind(source) && original.Bind(source));
 unsigned fragments=0,tiles=0;uint64_t encodedBytes=0;
 for(const auto& part:plan.Parts()) {
  if(part.kind!=PIONEER_Q38_WEIGHT)continue;
  const auto& t=source.Tensors()[part.tensor];
  if(t.type!=12 || t.name.find("ffn_")==std::string::npos)continue;
  uint32_t bytes=0;assert(reader.FragmentBytes(source,part.tensor,part.firstRow,part.rows,bytes));
  assert(uint64_t(bytes)==uint64_t(part.bytes/2304)*4352 && part.bytes%2304==0);
  unsigned count=bytes/4352;
  for(unsigned tile:{0U,1U,count-1}) {
   CheckTile(source,original,reader,part.tensor,part.firstRow,part.rows,tile);tiles++;
  }
  // Last-page padding is zero even when the final source tile crosses pages.
  uint8_t page[4098];memset(page,0xdd,sizeof(page));
  uint64_t last=(uint64_t(bytes)-1)/4096;
  assert(reader.ReadPage(source,part.tensor,part.firstRow,part.rows,last,page+1,4096));
  assert(page[0]==0xdd && page[4097]==0xdd);
  for(unsigned b=unsigned(bytes-last*4096);b<4096;b++)assert(page[b+1]==0);
  fragments++;encodedBytes+=bytes;
 }
 assert(fragments==1344 && tiles==4032 && source.Stable());
 printf("PASS OFFLINE public GGUF DATA fragments%u sampled_tiles%u panels%u represented_expanded_FFN_bytes%llu;first-second-last ALLfragments/inverse exact versus oldreader/independentpanel values/metadata/lastpadding;NOT_FULL_SHA_NATIVE_OR_MODEL_QUALITY\n",fragments,tiles,tiles*16,(unsigned long long)encodedBytes);
 assert(!fclose(file));
}
int main(int argc,char** argv) {
 assert(argc==1 || argc==2);setvbuf(stdout,nullptr,_IOLBF,0);Synthetic();if(argc==2)Public(argv[1]);
}
