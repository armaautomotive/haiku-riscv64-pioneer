/* SPDX-License-Identifier: MIT */
// Independent offline integer reference for a model-derived BFP8 projection.
// Never used by Work mode and never opens a device.
#define main independent_band_fixture_main
#include "pioneer_dense_band_independent_check.cpp"
#undef main
#include <array>
#include <cstring>

int main(int argc,char** argv){
 if(argc!=3 || (std::strcmp(argv[2],"--input") && std::strcmp(argv[2],"--synthetic")))return 2;
 const auto bytes=Read(argv[1]);
 const unsigned width=6144,rows=5120,tiles=width/32,bands=rows/32;
 const uint32_t head[]={0x31525051,1,16,width,rows,bands,tiles,1088};
 assert(bytes.size()==32+size_t(bands)*(size_t(tiles)*1088+128));
 assert(!std::memcmp(bytes.data(),head,sizeof(head)));
 std::array<uint16_t,6144> input{};
 if(!std::strcmp(argv[2],"--input")){
  std::array<uint32_t,6144> values{};
  assert(fread(values.data(),4,values.size(),stdin)==values.size() && fgetc(stdin)==EOF);
  for(unsigned i=0;i<width;i++){
   unsigned bits=values[i],exp=bits&0x7f800000U;
   assert(exp!=0x7f800000U && (exp || !(bits&0x7fffffU)));
   unsigned rounded=(bits+0x7fffU+((bits>>16)&1U))>>16;
   assert((rounded&0x7f80U)!=0x7f80U && ((rounded&0x7f80U) || !(rounded&127U)));
   input[i]=uint16_t(rounded);
  }
 }else for(unsigned i=0;i<width;i++)input[i]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
 for(unsigned band=0;band<bands;band++){
  std::vector<uint32_t> weights(size_t(tiles)*1104/4,0);
  for(unsigned t=0;t<tiles;t++)std::memcpy(reinterpret_cast<uint8_t*>(weights.data())+t*1104+16,
    bytes.data()+32+size_t(band)*(size_t(tiles)*1088+128)+t*1088,1088);
  for(unsigned row=0;row<32;row++){
   uint32_t result=0;
   for(unsigned tile=0;tile<tiles;tile++)for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){
     a[c]=Weight(weights,tile,row,half*16+c);b[c]=input[tile*32+half*16+c];
    }
    for(unsigned phase=0;phase<4;phase++)result=Instruction(a,b,phase,result);
   }
   if(!std::strcmp(argv[2],"--synthetic")){
    uint32_t expected;std::memcpy(&expected,bytes.data()+32+size_t(band)*(size_t(tiles)*1088+128)+size_t(tiles)*1088+row*4,4);
    assert(result==expected);
   }
   assert(fwrite(&result,4,1,stdout)==1);
  }
 }
 assert(!fflush(stdout));
}
